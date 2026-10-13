/* KH2AssetSizeCache. General Panacea-side positive size cache, automatic discovery.
 * Apache-2.0. Windows AMD64. No asset replacement, VERSION proxy or CRT.
 * Install only from Panacea OnInit, before gameplay; module pinned thereafter.
 */
#include <stdint.h>
#include <stddef.h>
#include "hde/hde64.h"
typedef uint8_t u8;typedef uint32_t u32;typedef uint64_t u64;
typedef struct {u32 begin,end,unwind;} RF;
#define IMP __declspec(dllimport)
#define EXPORT __declspec(dllexport)
IMP void *GetModuleHandleW(const uint16_t *);
IMP int GetModuleHandleExW(u32,const uint16_t *,void **);
IMP u32 GetModuleFileNameW(void *,uint16_t *,u32);
IMP int VirtualProtect(void *,size_t,u32,u32 *);
IMP void *VirtualAlloc(void *,size_t,u32,u32);
IMP int VirtualFree(void *,size_t,u32);
IMP int FlushInstructionCache(void *,const void *,size_t);
IMP RF *RtlLookupFunctionEntry(u64,u64 *,void *);
IMP u8 RtlAddFunctionTable(RF *,u32,u64);
IMP u8 RtlDeleteFunctionTable(RF *);
IMP void *CreateFileW(const uint16_t *,u32,u32,void *,u32,u32,void *);
IMP int WriteFile(void *,const void *,u32,u32 *,void *);
IMP int CloseHandle(void *);
IMP u32 GetLastError(void);
IMP void SetLastError(u32);
IMP u64 GetTickCount64(void);
IMP u32 GetCurrentThreadId(void);
IMP int DisableThreadLibraryCalls(void *);
void *memset(void *v,int c,size_t n){u8 *p=v;for(size_t i=0;i<n;i++)p[i]=(u8)c;return v;}
void *memcpy(void *d,const void *s,size_t n){u8 *a=d;const u8 *b=s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
static void copy(void *d,const void *s,size_t n){memcpy(d,s,n);}
static int equal(const void *a,const void *b,size_t n){const u8 *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return 0;return 1;}
#include "relocate.h"
typedef int (*SizeFn)(void *,const char *);
typedef void *(*InfoFn)(const char *,const char *);
#include "discovery.h"
static SizeFn previous_size;
static InfoFn volatile *info_slot;
static u64 game_begin,game_end;
static u32 init_once,installed,hook_cleanup_failed;
static uint16_t log_path[520]={'K','H','2','A','s','s','e','t','S','i','z','e','C','a','c','h','e','.','l','o','g',0};
static unsigned text(char *b,unsigned n,const char *s){while(*s)b[n++]=*s++;return n;}
/* Exactly one status write per OnInit; CREATE_ALWAYS replaces the prior run.
 * Logging failure never changes whether the cache can initialize. */
static void log_status(const char *status){
    char b[1024];unsigned n=text(b,0,"KH2AssetSizeCache v1-startup; ");
    n=text(b,n,status);b[n++]='\r';b[n++]='\n';
    void *f=CreateFileW(log_path,0x40000000U,3,0,2,0x80,0);
    if(f==(void *)(intptr_t)-1)return;
    const char *p=b;
    while(n){u32 w=0;if(!WriteFile(f,p,n,&w,0)||!w||w>n)break;n-=w;p+=w;}
    CloseHandle(f);
}
static void set_log_path(const uint16_t *folder){if(!folder)return;unsigned n=0;while(folder[n]&&n<460)n++;if(n==460)return;copy(log_path,folder,n*2);static const uint16_t end[]={'\\','d','l','l','\\','K','H','2','A','s','s','e','t','S','i','z','e','C','a','c','h','e','.','l','o','g',0};copy(log_path+n,end,sizeof(end));}
/* Metadata is refreshed by the game's GetFileInfo on EVERY cache candidate.
 * This retains package selection, header/remastered metadata and filename side
 * effects. Keys contain no borrowed string or resource-data buffer pointers.
 */
typedef struct {u64 package,vtable,header,handle_count,offset;u8 entry[16];} PackageKey;
/* Fixed memory: 64 hash sets with four entries each. No per-call allocation.
 * Exact byte paths stay distinct: no filesystem case/alias assumptions. */
#define CACHE_SETS 64
#define CACHE_WAYS 4
#define CACHE_TTL_MS 1000
typedef struct {
    u32 valid,owner,error,hash;u64 stamp,self,info,epoch;int result;
    char path[260];PackageKey package;
} CacheEntry;
static struct {u32 lock,next[CACHE_SETS];CacheEntry entries[CACHE_SETS][CACHE_WAYS];} cache;
static u64 generation,candidates,hits,refreshes,bypasses,contention,no_package,invalid_size,expired,changed,evicted;
static u32 fallback_active;
#define INC(x) __atomic_fetch_add(&(x),1,__ATOMIC_ACQ_REL)
#define READ(x) __atomic_load_n(&(x),__ATOMIC_ACQUIRE)
static int lock_cache(void){return !__atomic_exchange_n(&cache.lock,1,__ATOMIC_ACQUIRE);}
static void unlock_cache(void){__atomic_store_n(&cache.lock,0,__ATOMIC_RELEASE);}
static unsigned lower(unsigned c){if(c>='A'&&c<='Z')c+=32;if(c=='\\')c='/';return c;}
static int cacheable_name(const char *s,unsigned *len,u32 *hash){
    if(!s||!*s)return 0;
    unsigned n=0;u32 h=2166136261U;
    while(n<260&&s[n]){h=(h^(u8)s[n])*16777619U;n++;}
    if(n==260)return 0;
    *len=n;*hash=h;return 1;
}
static CacheEntry *select_entry(const char *name,unsigned len,u32 hash,u64 now){
    unsigned set=hash&(CACHE_SETS-1);CacheEntry *unused=0;
    for(unsigned i=0;i<CACHE_WAYS;i++){
        CacheEntry *e=&cache.entries[set][i];
        if(e->valid&&e->hash==hash&&equal(e->path,name,len+1))return e;
        if(!unused&&(!e->valid||now<e->stamp||now-e->stamp>=CACHE_TTL_MS))unused=e;
    }
    if(unused)return unused;
    INC(evicted);CacheEntry *e=&cache.entries[set][cache.next[set]];
    cache.next[set]=(cache.next[set]+1)%CACHE_WAYS;e->valid=0;return e;
}
/* Calls outside our lock may alter shared package context. Invalidate epochs
 * at BOTH ends, and refuse reuse/publication while such a call is in flight. */
static int fallback_size(void *self,const char *name,u32 incoming){
    INC(fallback_active);INC(generation);SetLastError(incoming);
    int r=previous_size(self,name);u32 error=GetLastError();
    INC(generation);__atomic_fetch_sub(&fallback_active,1,__ATOMIC_ACQ_REL);
    SetLastError(error);return r;
}
static void package_key(PackageKey *k,void *p){k->package=(u64)p;copy(&k->vtable,p,8);copy(&k->header,(u8 *)p+8,8);copy(&k->handle_count,(u8 *)p+16,8);copy(&k->offset,(u8 *)p+24,8);copy(k->entry,(u8 *)p+0x12c,16);}
static int cached_size(void *self,const char *name){
    u32 incoming=GetLastError(),hash=0;unsigned len=0;
    if(!cacheable_name(name,&len,&hash)){INC(bypasses);return fallback_size(self,name,incoming);}
    INC(candidates);
    if(!lock_cache()){INC(contention);return fallback_size(self,name,incoming);}
    u64 gen=READ(generation),now=GetTickCount64();u32 tid=GetCurrentThreadId();InfoFn info=*info_slot;
    /* A third-party replacement of the info function disables caching. */
    if((u64)info<game_begin||(u64)info>=game_end){
        unlock_cache();INC(bypasses);return fallback_size(self,name,incoming);
    }
    CacheEntry *e=select_entry(name,len,hash,now);
    SetLastError(incoming);void *pkg=info(name,0);PackageKey key;
    if(pkg)package_key(&key,pkg);
    if(pkg&&e->valid&&e->hash==hash&&e->epoch==gen&&e->self==(u64)self&&e->owner==tid&&e->info==(u64)info&&equal(e->path,name,len+1)&&equal(&e->package,&key,sizeof(key))){
        if(now>=e->stamp&&now-e->stamp<CACHE_TTL_MS){
            if(!READ(fallback_active)&&gen==READ(generation)){
                int r=e->result;u32 error=e->error;INC(hits);unlock_cache();SetLastError(error);return r;
            }
        }else INC(expired);
    }else if(e->valid)INC(changed);
    e->valid=0;INC(refreshes);if(!pkg)INC(no_package);
    /* Misses use the original routine, retaining all of its side effects.
     * Never publish zero/negative results, unstable metadata, or a result
     * overlapping an unlocked fallback. Hits never extend the fixed TTL. */
    SetLastError(incoming);int r=previous_size(self,name);u32 after=GetLastError();
    if(r>0&&pkg&&!READ(fallback_active)&&gen==READ(generation)){
        PackageKey after_key;package_key(&after_key,pkg);
        if(equal(&key,&after_key,sizeof(key))){
            e->package=key;copy(e->path,name,len+1);e->hash=hash;
            e->self=(u64)self;e->owner=tid;e->info=(u64)info;e->stamp=now;
            e->epoch=gen;e->result=r;e->error=after;e->valid=1;
        }
    }else if(r<=0)INC(invalid_size);
    unlock_cache();SetLastError(after);return r;
}
/* One-shot initialization, before game worker threads use the target. */
static __attribute__((noinline)) int install_hook(u8 *target){
    Insn a[16];unsigned count=0,copied=0,emitted=0;u8 saved[64],patch[14];
    while(copied<14){
        if(count>=16)return 0;Insn *in=&a[count++];hde64_disasm(target+copied,&in->h);hde64s *h=&in->h;
        if(!h->len||h->flags&F_ERROR||h->p_67||h->flags&F_RELATIVE||h->opcode==0xc2||h->opcode==0xc3)return 0;
        in->old=(u8)copied;in->newpos=(u8)emitted;in->nlen=h->len;copied+=h->len;emitted+=h->len;
    }
    /* The recognized profile has four instructions ending at +20. */
    if(copied!=20||count!=4)return 0;copy(saved,target,14);
    u8 *page=0;u64 origin=(u64)target&~(u64)65535;
    for(u64 distance=65536;distance<0x40000000&&!page;distance+=65536){
        if(origin>distance+65536)page=VirtualAlloc((void *)(origin-distance),4096,0x3000,4);
        if(!page&&origin+distance<0x00007fff00000000ULL)page=VirtualAlloc((void *)(origin+distance),4096,0x3000,4);
    }
    if(!page)return 0;int registered=0;RF *rf=(RF *)(page+1024);u32 old=0,ignored;
    for(unsigned i=0;i<count;i++){
        Insn *in=&a[i];hde64s *h=&in->h;u8 *src=target+in->old,*dst=page+in->newpos;copy(dst,src,h->len);
        if((h->flags&F_MODRM)&&h->modrm_mod==0&&h->modrm_rm==5){unsigned pos=h->len-imm_size(h)-4;int64_t d=(int64_t)((src+h->len+(int32_t)h->disp.disp32)-(dst+h->len));if(!fits32(d))goto failed;*(int32_t *)(dst+pos)=(int32_t)d;}
    }
    absjump(page+emitted,target+copied);if(!make_unwind(page+256,target,a,count,copied,emitted))goto failed;
    *rf=(RF){0,emitted+14,256};
    if(!VirtualProtect(page,4096,0x20,&ignored)||!FlushInstructionCache((void *)(intptr_t)-1,page,emitted+14))goto failed;
    if(!RtlAddFunctionTable(rf,1,(u64)page))goto failed;registered=1;
    previous_size=(SizeFn)page;absjump(patch,(void *)cached_size);
    if(!VirtualProtect(target,14,0x40,&old))goto failed;
    copy(target,patch,14);
    if(!FlushInstructionCache((void *)(intptr_t)-1,target,14))goto restore;
    if(!VirtualProtect(target,14,old,&ignored))goto restore;
    return 1;
restore:
    /* Initialization has not returned to gameplay. Restore bytes before freeing
     * the unpublished trampoline; do not leave a half-installed detour. */
    copy(target,saved,14);FlushInstructionCache((void *)(intptr_t)-1,target,14);VirtualProtect(target,14,old,&ignored);
failed:
    previous_size=0;if(registered&&!RtlDeleteFunctionTable(rf)){hook_cleanup_failed=1;return 0;}VirtualFree(page,0,0x8000);return 0;
}
static int kh2_executable(void){
    uint16_t path[520];u32 n=GetModuleFileNameW(0,path,520);if(!n||n>=520)return 0;u32 pos=n;while(pos&&path[pos-1]!='/'&&path[pos-1]!='\\')pos--;
    const char want[]="kingdom hearts ii final mix.exe";unsigned i=0;while(want[i]&&pos+i<n){if(lower(path[pos+i])!=(u8)want[i])return 0;i++;}return !want[i]&&pos+i==n;
}
EXPORT void OnInit(const uint16_t *folder){
    if(__atomic_exchange_n(&init_once,1,__ATOMIC_ACQ_REL))return;
    u32 error=GetLastError();set_log_path(folder);
    const char *status="DISABLED: initialization did not complete";
    if(!kh2_executable()){status="DISABLED: not the supported KH2 executable";goto done;}
    static const uint16_t version[]={'v','e','r','s','i','o','n','.','d','l','l',0};u8 *base=GetModuleHandleW(version);
    if(!base){status="DISABLED: Panacea version.dll is not loaded";goto done;}
    u8 *game=GetModuleHandleW(0);Discovery detected;int result=discover_panacea(base,game,&detected);
    if(result!=1){status=result<0?"DISABLED: ambiguous Panacea GetFileSize matches; no patch applied":"DISABLED: unsupported Panacea code/layout or uninitialized package lookup; no patch applied";goto done;}
    Image game_image;if(!image_view(game,&game_image)){status="DISABLED: invalid KH2 image layout";goto done;}
    game_begin=(u64)game;game_end=game_begin+game_image.size;info_slot=detected.info;
    void *pin=0;if(!GetModuleHandleExW(5,(const uint16_t *)(uintptr_t)OnInit,&pin)||!GetModuleHandleExW(5,(const uint16_t *)base,&pin)){status="DISABLED: module lifetime pin failed";goto done;}
    if(!install_hook(detected.size_fn)){status=hook_cleanup_failed?"DISABLED: hook installation failed; original entry retained/restored; inactive trampoline retained because unwind removal failed":"DISABLED: trampoline installation failed; original entry retained/restored";goto done;}
    installed=1;status="INSTALLED: size cache initialized; slots=256; TTL=1000 ms; package lookup retained";
 done:log_status(status);SetLastError(error);
}
int DllMain(void *module,u32 reason,void *reserved){(void)reserved;if(reason==1)DisableThreadLibraryCalls(module);return 1;}
