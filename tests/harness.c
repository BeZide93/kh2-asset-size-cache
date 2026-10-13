/* Executes compiled Windows x64 addon code, including OnInit and the detour.
 * WinAPI services are controlled doubles; no game/Proton runtime is simulated.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include "generated.h"
#define MS __attribute__((ms_abi))
typedef uint8_t u8;typedef uint32_t u32;typedef uint64_t u64;
typedef struct {u32 begin,end,unwind;} RF;
typedef int (MS *SizeFn)(void *,const char *);
typedef void *(MS *InfoFn)(const char *,const char *);
static u8 *im,*panacea,*fakegame,*page,*lookup_base,*lookup_target;
static size_t panacea_size;static RF *lookup_rf;
static unsigned originals,infos,creates,addcalls,delcalls,threadcalls,pins;
static int result=100,package_missing,mutate_metadata,fail_mode,bad_game,path_metadata;
static unsigned vp_calls,flush_calls;
static _Thread_local unsigned last_error=321,tid=1;
static u64 ticks=100;
static unsigned hold_fallback,fallback_entered,release_fallback;
static char blocked_name[300];
static u8 package[1024];
static const char target[]="C:/hd28/STEAM/juefigs/KH2ReSource/obj/P_EH010.a.us";
static void *self=(void *)0x1234;
static char logtext[20000];static size_t loglen;
static unsigned MS geterr(void){return last_error;}
static void MS seterr(unsigned v){last_error=v;}
static u64 MS tick(void){return ticks;}
static u32 MS threadid(void){return tid;}
static void *MS info(const char *n,const char *other){assert(n&&!other);__atomic_fetch_add(&infos,1,__ATOMIC_RELAXED);last_error=555;if(path_metadata){u32 h=2166136261u;for(const unsigned char *p=(const unsigned char *)n;*p;p++)h=(h^*p)*16777619u;memcpy(package+0x12c,&h,4);}return package_missing?NULL:package;}
static InfoFn slot=info;
static int MS original(void *s,const char *n){assert(s==self&&n&&last_error==321);__atomic_fetch_add(&originals,1,__ATOMIC_RELAXED);info(n,0);if(mutate_metadata)package[0x12c]++;if(hold_fallback&&n==blocked_name){__atomic_store_n(&fallback_entered,1,__ATOMIC_RELEASE);while(!__atomic_load_n(&release_fallback,__ATOMIC_ACQUIRE))sched_yield();}last_error=777;return result;}
static void jump(u8 *p,void *fn){p[0]=0xff;p[1]=0x25;memset(p+2,0,4);memcpy(p+6,&fn,8);}
static void *MS module(const uint16_t *s){if(!s)return fakegame;assert(s[0]=='v');return panacea;}
static int MS pin(u32 flags,const uint16_t *addr,void **out){assert(flags==5&&addr);pins++;*out=im;return fail_mode!=8;}
static u32 MS modulename(void *m,uint16_t *b,u32 cap){assert(!m&&cap==520);const char *s=bad_game?"other.exe":"Z:\\KH\\KINGDOM HEARTS II FINAL MIX.exe";unsigned i=0;do{b[i]=(u8)s[i];}while(s[i++]);return i-1;}
static int MS disable(void *m){assert(m==im);return 1;}
static void *MS createfile(const uint16_t *n,u32 a,u32 sh,void *sec,u32 d,u32 attrs,void *templ){assert(n&&a==4&&sh==3&&!sec&&d==4&&attrs==0x80&&!templ);creates++;return (void *)2;}
static int MS write_(void *h,const void *b,u32 n,u32 *out,void *ov){assert(h==(void *)2&&!ov&&loglen+n<sizeof(logtext));memcpy(logtext+loglen,b,n);loglen+=n;logtext[loglen]=0;*out=n;return 1;}
static int MS close_(void *h){assert(h==(void *)2||h==(void *)3);return 1;}
static void MS sleep_(u32 t){(void)t;abort();}
static void *MS thread_(void *sec,size_t stack,void *fn,void *arg,u32 flags,u32 *id){assert(!sec&&!stack&&fn&&!arg&&!flags&&!id);threadcalls++;return NULL;}
static void *MS alloc_(void *want,size_t n,u32 mode,u32 prot){assert(want&&n==4096&&mode==0x3000&&prot==4);if(fail_mode==1)return NULL;void *p=mmap(want,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(p==MAP_FAILED)return NULL;page=p;return p;}
static int MS free_(void *p,size_t n,u32 mode){assert(p==page&&!n&&mode==0x8000);assert(!munmap(page,4096));page=NULL;return 1;}
static int MS protect_(void *ptr,size_t n,u32 prot,u32 *old){vp_calls++;if((fail_mode==2&&vp_calls==1)||((fail_mode==5||fail_mode==9)&&vp_calls==2)||(fail_mode==7&&vp_calls==3))return 0;*old=ptr==page?4:0x20;uintptr_t start=(uintptr_t)ptr&~4095ULL;size_t size=(((uintptr_t)ptr+n+4095)&~4095ULL)-start;int pr=PROT_READ;if(prot==4||prot==0x40||prot==0x80)pr|=PROT_WRITE;if(prot==0x20||prot==0x40||prot==0x80)pr|=PROT_EXEC;return !mprotect((void *)start,size,pr);}
static int MS flush_(void *process,const void *p,size_t n){assert(process==(void *)(intptr_t)-1&&p&&n);flush_calls++;return !((fail_mode==3&&flush_calls==1)||(fail_mode==6&&flush_calls==2));}
static RF *MS lookup(u64 target_,u64 *base,void *hist){assert((void *)target_==lookup_target&&!hist);*base=(u64)lookup_base;return lookup_rf;}
static u8 MS add_(RF *rf,u32 count,u64 base){assert(base==(u64)page&&count==1&&rf->begin==0&&rf->end==34&&rf->unwind==256);assert(page[256]==1&&page[257]==20);addcalls++;return fail_mode!=4;}
static u8 MS delete_(RF *rf){assert(rf==(RF *)(page+1024));delcalls++;return fail_mode!=9;}
static u8 *loadpe(const char *path,size_t *sz){FILE *f=fopen(path,"rb");assert(f);u8 hdr[4096];assert(fread(hdr,1,4096,f)==4096);u32 nt=*(u32 *)(hdr+0x3c);unsigned ns=*(uint16_t *)(hdr+nt+6),opt=*(uint16_t *)(hdr+nt+20);*sz=*(u32 *)(hdr+nt+24+56);u8 *mem=mmap(NULL,*sz,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(mem!=MAP_FAILED);memcpy(mem,hdr,*(u32 *)(hdr+nt+24+60));for(unsigned i=0;i<ns;i++){u8 *s=hdr+nt+24+opt+40*i;u32 rva=*(u32 *)(s+12),rawsz=*(u32 *)(s+16),raw=*(u32 *)(s+20);if(rawsz){assert(!fseek(f,raw,SEEK_SET));assert(fread(mem+rva,1,rawsz,f)==rawsz);}}fclose(f);return mem;}
static void bind(void){
#define BIND(n,fn) *(void **)(im+IAT_##n)=(void *)(fn)
 BIND(GetLastError,geterr);BIND(SetLastError,seterr);BIND(GetTickCount64,tick);BIND(GetCurrentThreadId,threadid);BIND(GetModuleHandleW,module);BIND(GetModuleFileNameW,modulename);BIND(GetModuleHandleExW,pin);BIND(CreateFileW,createfile);BIND(WriteFile,write_);BIND(CloseHandle,close_);BIND(CreateThread,thread_);BIND(Sleep,sleep_);BIND(DisableThreadLibraryCalls,disable);BIND(VirtualAlloc,alloc_);BIND(VirtualFree,free_);BIND(VirtualProtect,protect_);BIND(FlushInstructionCache,flush_);BIND(RtlLookupFunctionEntry,lookup);BIND(RtlAddFunctionTable,add_);BIND(RtlDeleteFunctionTable,delete_);
 for(unsigned i=0;i<sizeof(chunks)/sizeof(*chunks);i++){unsigned flags=chunks[i].flags;int p=PROT_READ;if(flags&0x80000000U)p|=PROT_WRITE;if(flags&0x20000000U)p|=PROT_EXEC;assert(!mprotect(im+chunks[i].rva,(chunks[i].size+4095)&~4095U,p));}
}
static void fresh(const char *dll){size_t n;if(im)assert(!munmap(im,IMAGE_SIZE));im=loadpe(dll,&n);assert(n==IMAGE_SIZE);bind();*(SizeFn *)(im+RVA_previous_size)=original;*(InfoFn **)(im+RVA_info_slot)=&slot;*(u64 *)(im+RVA_game_begin)=0;*(u64 *)(im+RVA_game_end)=UINT64_MAX;originals=infos=creates=threadcalls=pins=0;fail_mode=0;package_missing=mutate_metadata=path_metadata=0;memset(package,0,sizeof(package));*(int *)(package+0x12c)=100;ticks=100;tid=1;last_error=321;result=100;hold_fallback=fallback_entered=release_fallback=0;loglen=0;logtext[0]=0;}
static int call(const char *n){last_error=321;int r=((SizeFn)(im+RVA_cached_size))(self,n);assert(last_error==777);return r;}
static void test_cache(const char *dll){
 fresh(dll);assert(call(target)==100&&originals==1&&infos==2);for(unsigned i=0;i<1000;i++){ticks=101+i/2;assert(call(target)==100);}assert(originals==1&&infos==1002&&creates==0);assert(*(u64 *)(im+RVA_hits)==1000);
 ticks=1099;assert(call(target)==100&&originals==1);ticks=1100;assert(call(target)==100&&originals==2);ticks=1101;
 package[24]++;assert(call(target)==100&&originals==3);package[8]++;assert(call(target)==100&&originals==4);package[16]++;assert(call(target)==100&&originals==5);package[0x12c]++;result=101;assert(call(target)==101&&originals==6);
 tid=2;assert(call(target)==101&&originals==7);tid=1;assert(call(target)==101&&originals==8);
 assert(call("obj/P_EH010.a.fm")==101&&originals==9);assert(call("obj/P_EH010.a.fm")==101&&originals==9);assert(call("obj/P_EH000.a.us")==101&&originals==10);assert(call("obj/P_EH010.a.fm")==101&&originals==10);
 ticks=1;assert(call("obj/P_EH010.a.fm")==101&&originals==11);
 result=0;ticks+=1000;assert(call(target)==0);unsigned before=originals;assert(call(target)==0&&originals==before+1);result=-6;assert(call(target)==-6);before=originals;assert(call(target)==-6&&originals==before+1);
 result=100;package_missing=1;assert(call(target)==100);before=originals;assert(call(target)==100&&originals==before+1);package_missing=0;mutate_metadata=1;assert(call(target)==100);before=originals;assert(call(target)==100&&originals==before+1);mutate_metadata=0;
 assert(call(target)==100);before=originals;*(u32 *)(im+RVA_cache)=1;assert(call(target)==100&&originals==before+1);*(u32 *)(im+RVA_cache)=0;assert(call(target)==100&&originals==before+2);assert(*(u64 *)(im+RVA_contention)==1);
 const char *others[]={"obj/P_EH010.a.jp","obj/P_EH010.a.us.bak","obj/XP_EH010.a.us","xobj/P_EH010.a.us","obj/P_EX020.a.us"};for(unsigned i=0;i<sizeof(others)/sizeof(*others);i++){before=originals;assert(call(others[i])==100&&originals==before+1);}
 assert(call("C:\\foo\\OBJ\\p_eh010.A.US")==100);before=originals;assert(call("C:\\foo\\OBJ\\p_eh010.A.US")==100&&originals==before);
 char longpath[300];memset(longpath,'x',299);longpath[299]=0;before=originals;assert(call(longpath)==100&&originals==before+1);
 puts("PASS: actual PE cache; 1000 repeated calls use one original size lookup but retain 1000 package lookups; no per-request log I/O.");
 puts("PASS: fixed 1000 ms TTL, backward clock, package metadata/handle changes, thread/path/language changes, unrelated-file retention; zero/negative/null/unstable data never cached; lock contention falls back without waiting; LastError preserved.");
}
/* Exact byte-path hashing used only to generate colliding test inputs. */
static u32 path_hash(const char *n){u32 h=2166136261U;while(*n)h=(h^(u8)*n++)*16777619U;return h;}
static void test_general(const char *dll){
 fresh(dll);path_metadata=1;
 const char *names[]={"obj/P_EH010.mdlx","obj/P_EH010.a.us","obj/P_EH010.mset","limit/us/riku.bar","obj/P_EH010.a.fm","limit/fm/riku.bar"};
 for(unsigned cycle=0;cycle<100;cycle++){
  ticks=100+cycle*5;
  for(unsigned i=0;i<6;i++){result=200+i;assert(call(names[i])==200+(int)i);}
  result=201;assert(call(names[1])==201);assert(call(names[1])==201);
  result=999;assert(call("unrelated_file")==999);
 }
 assert(originals==7&&infos==907&&*(u64 *)(im+RVA_hits)==893&&creates==0);
 ticks=1100;for(unsigned i=0;i<6;i++){result=300+i;assert(call(names[i])==300+(int)i);}
 assert(originals==13);
 package[8]++;result=700;assert(call(names[0])==700&&originals==14);
 result=701;assert(call(names[1])==701&&originals==15);
 package_missing=1;result=0;assert(call(names[0])==0);package_missing=0;
 unsigned before=originals;result=702;assert(call(names[0])==702&&originals==before+1);
 before=originals;tid=2;result=703;assert(call(names[0])==703&&originals==before+1);tid=1;
 // No suffix whitelist: arbitrary directories, extensions and language paths.
 fresh(dll);path_metadata=1;
 const char *generic[]={"obj/M_EX760.a.us","obj/M_EX770.mset","obj/W_EX020_a0.mdlx", "obj_2nd/P_EH010.a.de", "obj_3rd/OTHER.a.jp", "limit_3rd/es/donald.bar", "map/hb02.map", "se/se005.win32.scd", "menu/test.2ld", "rootfile", "custom/mod/resource.xyz", "C:/A/obj/model.mdlx", "C:/B/obj/model.mdlx", "obj/model.mdlx", "OBJ/model.mdlx", "obj/model.MDLX", "obj\\model.mdlx"};
 unsigned count=sizeof(generic)/sizeof(*generic);
 for(unsigned i=0;i<count;i++){result=1000+i;assert(call(generic[i])==1000+(int)i);}
 before=originals;
 for(unsigned cycle=0;cycle<20;cycle++)for(unsigned i=0;i<count;i++){result=9999;assert(call(generic[i])==1000+(int)i);}
 assert(originals==before&&before==count);
 // Fill all 256 slots, then force a hash-set collision and bounded eviction.
 fresh(dll);path_metadata=1;char paths[64][5][100];unsigned counts[64]={0};
 for(unsigned i=0;;i++){
  char n[100];snprintf(n,sizeof(n),"C:/cache/root%u/arbitrary.asset",i);unsigned set=path_hash(n)&63;
  if(counts[set]<5)strcpy(paths[set][counts[set]++],n);
  unsigned done=0;for(unsigned k=0;k<64;k++)done+=counts[k]==5;if(done==64)break;
 }
 for(unsigned set=0;set<64;set++)for(unsigned way=0;way<4;way++){result=1000+set*4+way;assert(call(paths[set][way])==result);}
 assert(originals==256);
 for(unsigned set=0;set<64;set++)for(unsigned way=0;way<4;way++){result=9999;assert(call(paths[set][way])==1000+(int)set*4+(int)way);}
 assert(originals==256&&*(u64 *)(im+RVA_evicted)==0);
 result=888;assert(call(paths[0][4])==888&&originals==257&&*(u64 *)(im+RVA_evicted)==1);
 result=889;assert(call(paths[0][0])==889&&originals==258);
 result=9999;assert(call(paths[1][0])==1004&&originals==258);
 // Expired space is reused before a live entry, without capacity eviction.
 ticks=1100;result=700;assert(call(paths[0][4])==700);
 // A foreign info callback disables reuse without calling that callback.
 fresh(dll);assert(call(target)==100);*(u64 *)(im+RVA_game_begin)=UINT64_MAX;
 result=901;assert(call(target)==901&&originals==2);*(u64 *)(im+RVA_game_begin)=0;
 result=902;assert(call(target)==902&&originals==3);
 // Empty and overly long names always use the original routine.
 before=originals;assert(call("")==902);assert(call("")==902&&originals==before+2);
 puts("PASS: 900 interleaved Re:Fined-style calls use seven original size queries; arbitrary paths/extensions/languages; exact path separation; all 256 slots; collision eviction; per-file TTL/metadata and foreign callback refusal.");
}
static void *fallback_runner(void *unused){(void)unused;tid=2;assert(call(blocked_name)==100);return NULL;}
static void test_inflight_fallback(const char *dll){
 fresh(dll);assert(call(target)==100&&originals==1);
 memset(blocked_name,'x',sizeof(blocked_name)-1);blocked_name[sizeof(blocked_name)-1]=0;
 hold_fallback=1;pthread_t th;assert(!pthread_create(&th,0,fallback_runner,0));
 while(!__atomic_load_n(&fallback_entered,__ATOMIC_ACQUIRE))sched_yield();
 unsigned before=originals;assert(*(u32 *)(im+RVA_fallback_active)==1);
 // While an unlocked original call is running, neither reuse nor publication.
 assert(call(target)==100&&originals==before+1);assert(call(target)==100&&originals==before+2);
 __atomic_store_n(&release_fallback,1,__ATOMIC_RELEASE);assert(!pthread_join(th,0));
 assert(*(u32 *)(im+RVA_fallback_active)==0);before=originals;
 assert(call(target)==100&&originals==before+1);assert(call(target)==100&&originals==before+1);
 puts("PASS: real overlapping fallback suppresses cache hits and publication until completion; subsequent lookup refreshes before reuse.");
}
static void *cache_runner(void *arg){tid=(unsigned)(uintptr_t)arg;for(unsigned i=0;i<10000;i++)assert(call(target)==100);return NULL;}
static void test_threads(const char *dll){fresh(dll);pthread_t threads[4];for(unsigned i=0;i<4;i++)assert(!pthread_create(&threads[i],0,cache_runner,(void *)(uintptr_t)(i+1)));for(unsigned i=0;i<4;i++)assert(!pthread_join(threads[i],0));u64 h=*(u64 *)(im+RVA_hits),r=*(u64 *)(im+RVA_refreshes),c=*(u64 *)(im+RVA_contention);assert(h+originals==40000&&originals==r+c&&infos==40000+r&&creates==0);puts("PASS: 40,000 actual PE cache calls on four threads; exact hit/refresh/fallback conservation and unchanged results/LastError.");}
static void test_hook(const char *dll){
 for(int pass=0;pass<=8;pass++){int failure=pass==8?9:pass;
  fresh(dll);fail_mode=failure;vp_calls=flush_calls=addcalls=delcalls=0;page=NULL;
  u8 *g=mmap(NULL,0x60000,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(g!=MAP_FAILED);lookup_base=lookup_target=g;
  memcpy(g,panacea+0x10e60,20);const u8 tail[]={0x48,0x8b,0xb4,0x24,0xc0,3,0,0,0x48,0x81,0xc4,0xa0,3,0,0,0x5f,0x48,0x89,0xc8,0xc3};memcpy(g+20,tail,sizeof(tail));
  RF rf={0,64,1024};lookup_rf=&rf;memcpy(g+1024,panacea+0x589ac,32);u8 saved[20];memcpy(saved,g,20);assert(!mprotect(g,0x60000,PROT_READ|PROT_EXEC));
  int ok=((int(MS *)(u8 *))(im+RVA_install_hook))(g);
  if(failure){assert(!ok&&!memcmp(saved,g,20));if(failure==9){assert(page&&strstr(logtext,"unwind removal failed"));assert(!munmap(page,4096));page=NULL;}else assert(page==NULL);}
  else{assert(ok&&page&&g[0]==0xff&&g[1]==0x25&&addcalls==1);assert(((SizeFn)g)(self,"ordinary_file")==0x1234);assert(((SizeFn)(*(void **)(im+RVA_previous_size)))(self,"ordinary_file")==0x1234);assert(!munmap(page,4096));page=NULL;}
  assert(!munmap(g,0x60000));
 }
 puts("PASS: actual relocated prologue and RIP operand execute; unwind registration; allocation/protection/flush/registration failures leave or restore original bytes.");
}
static void setup_lookup(void){u32 nt=*(u32 *)(panacea+0x3c);u32 pd=*(u32 *)(panacea+nt+24+112+3*8),sz=*(u32 *)(panacea+nt+24+112+3*8+4);lookup_base=panacea;lookup_target=panacea+0x10e60;lookup_rf=NULL;for(unsigned i=0;i<sz/12;i++){RF *rf=(RF *)(panacea+pd+i*12);if(rf->begin==0x10e60)lookup_rf=rf;}assert(lookup_rf);}
static void test_init(const char *dll){
 fakegame=mmap(NULL,0x3000,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(fakegame!=MAP_FAILED);*(uint16_t *)fakegame=0x5a4d;*(u32 *)(fakegame+0x3c)=0x80;*(u32 *)(fakegame+0x80)=0x4550;*(uint16_t *)(fakegame+0x84)=0x8664;*(uint16_t *)(fakegame+0x98)=0x20b;*(uint16_t *)(fakegame+0x86)=1;*(uint16_t *)(fakegame+0x94)=240;*(u32 *)(fakegame+0x98+60)=0x400;*(u32 *)(fakegame+0x98+108)=16;u8 *section=fakegame+0x188;*(u32 *)(section+8)=0x2000;*(u32 *)(section+12)=0x1000;*(u32 *)(section+36)=0x60000020;*(u32 *)(fakegame+0x98+56)=0x3000;jump(fakegame+0x1000,info);
 *(void **)(panacea+0x5fcc0)=fakegame+0x1000;setup_lookup();u8 saved[20];memcpy(saved,panacea+0x10e60,20);
 uint16_t folder[]={'Z',':','\\','m','o','d',0};
 fresh(dll);bad_game=1;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(folder);assert(strstr(logtext,"DISABLED: not")&&!memcmp(saved,panacea+0x10e60,20));bad_game=0;
 fresh(dll);panacea[0x10e60]^=1;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(folder);assert(strstr(logtext,"DISABLED: unsupported")&&!threadcalls);panacea[0x10e60]^=1;
 fresh(dll);fail_mode=8;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(folder);assert(strstr(logtext,"pin failed")&&!memcmp(saved,panacea+0x10e60,20));
 fresh(dll);vp_calls=flush_calls=addcalls=delcalls=0;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(folder);assert(last_error==321&&pins==2&&threadcalls==1&&strstr(logtext,"INSTALLED:")&&panacea[0x10e60]==0xff);unsigned logs=creates;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(folder);assert(creates==logs&&threadcalls==1);
 assert(!mprotect((void *)((uintptr_t)(panacea+0x10e60)&~4095ULL),4096,7));memcpy(panacea+0x10e60,saved,20);assert(!munmap(page,4096));page=NULL;assert(!munmap(fakegame,0x3000));fakegame=NULL;
 puts("PASS: real OnInit against supplied Panacea PE; automatic function/layout validation, wrong-game/modified-module refusal, lifetime pinning, duplicate initialization guard; worker failure does not disable installed fix.");
}
int main(int argc,char **argv){assert(argc==2||argc==3);if(argc==2){test_cache(argv[1]);test_general(argv[1]);test_inflight_fallback(argv[1]);test_threads(argv[1]);munmap(im,IMAGE_SIZE);puts("LIMIT: cache/ABI tests only; Panacea fixture and in-game tests not run.");return 0;}panacea=loadpe(argv[2],&panacea_size);test_cache(argv[1]);test_general(argv[1]);test_inflight_fallback(argv[1]);test_threads(argv[1]);test_hook(argv[1]);test_init(argv[1]);munmap(im,IMAGE_SIZE);munmap(panacea,panacea_size);puts("LIMIT: no KH2, Wine, Proton or ARM64/FEX integration run.");return 0;}
