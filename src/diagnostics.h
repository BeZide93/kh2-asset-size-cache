/* Optional call-site telemetry. Apache-2.0. No load results are cached here.
 * Call-site relays leave function prologues, unwind metadata and the original
 * game's hooks alone. The deliberately strict diagnostic profile is separate
 * from the size cache's address-independent recognizer.
 */
IMP int QueryPerformanceCounter(int64_t *);
IMP int QueryPerformanceFrequency(int64_t *);
#define DIAG_ROWS 2048
#define DIAG_NAME 384
enum {D_ATTR,D_PATH,D_INFO,D_OPEN,D_READ,D_SEEK,D_TELL,D_CLOSE,D_REMASTER,D_REGISTER,D_SIZE_TOTAL,D_PHASES};
typedef struct {u32 used,scope,phase;char name[DIAG_NAME];u64 count,ticks,max,unsuccessful,bytes;} DiagRow;
typedef struct {void *file;char name[DIAG_NAME];} DiagStream;
static DiagRow diag_rows[DIAG_ROWS];
static DiagStream diag_streams[32];
static u32 diag_lock,diag_stream_lock,diag_active;
static u64 diag_freq,diag_origin,diag_dropped,diag_untracked,diag_clock_errors,diag_write_errors;
static uint16_t diag_path[520];
static void *volatile diag_direct[8];
static void **diag_attrs_slot,**diag_register_slot,**diag_info_slot;
#include "negative_paths.h"
static u8 *diag_page;
static int diag_try(u32 *lock){return !__atomic_exchange_n(lock,1,__ATOMIC_ACQUIRE);}
static void diag_unlock(u32 *lock){__atomic_store_n(lock,0,__ATOMIC_RELEASE);}
static u64 diag_now(void){int64_t t=0;if(!QueryPerformanceCounter(&t)||t<0){INC(diag_clock_errors);return 0;}return (u64)t;}
/* Names are copied, bounded and escaped; borrowed game/CRT pointers are never
 * retained. Oversize names receive an explicit marker, not a misleading key. */
static void diag_name(char *out,const void *src,int wide){
    if(!src){copy(out,"<null>",7);return;}
    unsigned n=0,i=0;for(;i<1024;i++){
        u32 c=wide?((const uint16_t *)src)[i]:((const u8 *)src)[i];
        if(!c){out[n]=0;return;}
        if(n+8>=DIAG_NAME)break;
        if(c<32||c==127||c=='"'||c=='\\'){static const char hex[]="0123456789abcdef";out[n++]='\\';out[n++]='u';out[n++]='0';out[n++]='0';out[n++]=hex[(c>>4)&15];out[n++]=hex[c&15];}
        else if(!wide||c<128)out[n++]=(char)c;
        else if(c<2048){out[n++]=(char)(0xc0|(c>>6));out[n++]=(char)(0x80|(c&63));}
        else {if(c>=0xd800&&c<=0xdfff)c=0xfffd;out[n++]=(char)(0xe0|(c>>12));out[n++]=(char)(0x80|((c>>6)&63));out[n++]=(char)(0x80|(c&63));}
    }
    copy(out,"<name-too-long>",16);
}
static void diag_record(unsigned scope,unsigned phase,const char *name,u64 start,u64 end,int unsuccessful,u64 bytes){
    if(!diag_active)return;
    if(!start||end<start){INC(diag_clock_errors);return;}
    u64 elapsed=end-start;u32 hash=2166136261U;
    unsigned len=0;while(name[len]){hash=(hash^(u8)name[len])*16777619U;len++;}
    hash^=scope*31+phase;
    if(!diag_try(&diag_lock)){INC(diag_dropped);return;}
    DiagRow *r=0;
    for(unsigned p=0;p<16;p++){
        DiagRow *e=&diag_rows[(hash+p)&(DIAG_ROWS-1)];
        if(!e->used){r=e;r->scope=scope;r->phase=phase;copy(r->name,name,len+1);r->used=1;break;}
        if(e->scope==scope&&e->phase==phase&&equal(e->name,name,len+1)){r=e;break;}
    }
    if(r){r->count++;r->ticks+=elapsed;if(elapsed>r->max)r->max=elapsed;r->unsuccessful+=!!unsuccessful;r->bytes+=bytes;}
    else INC(diag_dropped);
    diag_unlock(&diag_lock);
}
static void diag_stream_add(void *file,const char *name){
    if(!file)return;
    if(!diag_try(&diag_stream_lock)){INC(diag_untracked);return;}
    DiagStream *empty=0;
    for(unsigned i=0;i<32;i++){if(diag_streams[i].file==file){empty=&diag_streams[i];break;}if(!empty&&!diag_streams[i].file)empty=&diag_streams[i];}
    if(empty){empty->file=file;copy(empty->name,name,DIAG_NAME);}else INC(diag_untracked);
    diag_unlock(&diag_stream_lock);
}
static void diag_stream_name(void *file,char *name,int remove){
    copy(name,"<untracked-stream>",19);
    if(diag_try(&diag_stream_lock)){
        for(unsigned i=0;i<32;i++)if(file&&diag_streams[i].file==file){copy(name,diag_streams[i].name,DIAG_NAME);if(remove)diag_streams[i].file=0;diag_unlock(&diag_stream_lock);return;}
        diag_unlock(&diag_stream_lock);
    }
    INC(diag_untracked);
}
#define DIAG_BEGIN u32 incoming=GetLastError();char key[DIAG_NAME]={0};u64 begin,end;u32 after
#define DIAG_START begin=diag_now();SetLastError(incoming)
#define DIAG_END after=GetLastError();end=diag_now()
#define DIAG_SAVE(s,p,fail,bytes) diag_record(s,p,key,begin,end,fail,bytes);SetLastError(after)
/* Scope 0 is Panacea LoadFile, scope 1 is an original GetFileSize execution.
 * The latter is a cache miss OR an uncached fallback, never a cache hit. */
#define DIAG_WRAPPERS(s) \
static u32 diag_attrs_##s(const uint16_t *p){DIAG_BEGIN;diag_name(key,p,1);DIAG_START;u32 r=negative_attrs(p);DIAG_END;DIAG_SAVE(s,D_ATTR,r==0xffffffffU,0);return r;} \
static u8 diag_transform_##s(uint16_t *out,int n,const char *p){DIAG_BEGIN;diag_name(key,p,0);DIAG_START;u8 r=((u8 (*)(uint16_t *,int,const char *))diag_direct[0])(out,n,p);DIAG_END;DIAG_SAVE(s,D_PATH,!r,0);return r;} \
static void *diag_info_##s(const char *p,const char *alt){DIAG_BEGIN;diag_name(key,p,0);DIAG_START;void *r=((InfoFn)*diag_info_slot)(p,alt);DIAG_END;DIAG_SAVE(s,D_INFO,!r,0);return r;} \
static void *diag_open_##s(const uint16_t *p,const uint16_t *mode){DIAG_BEGIN;diag_name(key,p,1);DIAG_START;void *r=((void *(*)(const uint16_t *,const uint16_t *))diag_direct[1])(p,mode);DIAG_END;diag_stream_add(r,key);DIAG_SAVE(s,D_OPEN,!r,0);return r;} \
static size_t diag_read_##s(void *p,size_t sz,size_t n,void *file){DIAG_BEGIN;diag_stream_name(file,key,0);DIAG_START;size_t r=((size_t (*)(void *,size_t,size_t,void *))diag_direct[2])(p,sz,n,file);DIAG_END;DIAG_SAVE(s,D_READ,r<n,(sz&&r>UINT64_MAX/sz)?UINT64_MAX:r*sz);return r;} \
static int diag_seek_##s(void *file,int offset,int whence){DIAG_BEGIN;diag_stream_name(file,key,0);DIAG_START;int r=((int (*)(void *,int,int))diag_direct[3])(file,offset,whence);DIAG_END;DIAG_SAVE(s,D_SEEK,r!=0,0);return r;} \
static int diag_tell_##s(void *file){DIAG_BEGIN;diag_stream_name(file,key,0);DIAG_START;int r=((int (*)(void *))diag_direct[4])(file);DIAG_END;DIAG_SAVE(s,D_TELL,r<0,0);return r;} \
static int diag_close_##s(void *file){DIAG_BEGIN;diag_stream_name(file,key,1);DIAG_START;int r=((int (*)(void *))diag_direct[5])(file);DIAG_END;DIAG_SAVE(s,D_CLOSE,r!=0,0);return r;}
DIAG_WRAPPERS(0)
DIAG_WRAPPERS(1)
static void diag_remaster(void *pkg,const uint16_t *path,void *data){DIAG_BEGIN;diag_name(key,path,1);DIAG_START;((void (*)(void *,const uint16_t *,void *))diag_direct[6])(pkg,path,data);DIAG_END;DIAG_SAVE(0,D_REMASTER,0,0);}
static int diag_register(const char *name,int size,void *data){DIAG_BEGIN;diag_name(key,name,0);DIAG_START;int r=((int (*)(const char *,int,void *))*diag_register_slot)(name,size,data);DIAG_END;DIAG_SAVE(0,D_REGISTER,r==-1,0);return r;}
/* Called only for an original size execution; leaves cache semantics intact. */
static __attribute__((noinline)) int diag_size(SizeFn original,void *self,const char *name){
    if(!diag_active)return original(self,name);
    DIAG_BEGIN;diag_name(key,name,0);DIAG_START;int r=original(self,name);DIAG_END;DIAG_SAVE(1,D_SIZE_TOTAL,r<=0,0);return r;
}
#undef DIAG_BEGIN
#undef DIAG_START
#undef DIAG_END
#undef DIAG_SAVE
#undef DIAG_WRAPPERS
/* Generated from the original, user-supplied Panacea. No game/DLL bytes. */
#include "diagnostic_profile.h"
typedef struct {u8 *disp;int32_t saved,replacement;u32 old;int writable;} DiagPatch;
static DiagPatch diag_patches[64];
static volatile unsigned diag_patch_count;
static u64 diag_hash(const u8 *b,u32 n){u64 h=14695981039346656037ULL;while(n--)h=(h^*b++)*1099511628211ULL;return h;}
static __attribute__((noinline)) int diag_profile_ok(u8 *base,u8 *size_fn){
    Image image;if(!image_view(base,&image)||size_fn!=base+0x10e60)return 0;
    for(unsigned i=0;i<sizeof(diag_profiles)/sizeof(*diag_profiles);i++){
        const DiagProfile *p=&diag_profiles[i];
        if(!section_has(&image,p->rva,p->length,READABLE|EXECUTABLE,WRITABLE)||diag_hash(base+p->rva,p->length)!=p->hash)return 0;
    }
    if(import_slot(&image,"GetFileAttributesW")!=0x460f0||!section_has(&image,0x5fcc0,8,READABLE|WRITABLE,EXECUTABLE)||!section_has(&image,0x5fd80,8,READABLE|WRITABLE,EXECUTABLE))return 0;
    return 1;
}
static void *diag_wrapper(unsigned id){
    switch(id){
    case 0:return diag_attrs_0;case 1:return diag_transform_0;case 2:return diag_info_0;case 3:return diag_open_0;case 4:return diag_read_0;case 5:return diag_seek_0;case 6:return diag_tell_0;case 7:return diag_close_0;case 8:return diag_remaster;case 9:return diag_register;
    case 10:return diag_attrs_1;case 11:return diag_transform_1;case 12:return diag_info_1;case 13:return diag_open_1;case 14:return diag_read_1;case 15:return diag_seek_1;case 16:return diag_tell_1;case 17:return diag_close_1;case 18:return negative_attrs;
    default:return 0;}
}
/* Prepare without touching Panacea. Called BEFORE installing the size detour,
 * because the diagnostic profile also validates the original size prologue. */
static __attribute__((noinline)) int diag_prepare(u8 *base,u8 *size_fn){
    if(!diag_profile_ok(base,size_fn))return 0;
    int64_t f=0;if(!QueryPerformanceFrequency(&f)||f<=0)return 0;
    diag_freq=(u64)f;diag_origin=diag_now();if(!diag_origin)return 0;
    u64 origin=(u64)base&~65535ULL;
    for(u64 d=65536;d<0x40000000&&!diag_page;d+=65536){
        if(origin>d+65536)diag_page=VirtualAlloc((void *)(origin-d),4096,0x3000,4);
        if(!diag_page&&origin+d<0x00007fff00000000ULL)diag_page=VirtualAlloc((void *)(origin+d),4096,0x3000,4);
    }
    if(!diag_page)return 0;
    for(unsigned i=0;i<19;i++)absjump(diag_page+i*16,diag_wrapper(i));
    diag_patch_count=sizeof(diag_sites)/sizeof(*diag_sites);
    for(unsigned i=0;i<diag_patch_count;i++){
        const DiagSite *s=&diag_sites[i];u8 *pc=base+s->rva;
        /* Full profile already checks bytes; explicit instruction checks also
         * keep relay construction independent of assumptions in the table. */
        unsigned off=s->kind==0?1:(s->kind==1?2:3),len=off+4;
        if((s->kind==0&&pc[0]!=0xe8)||(s->kind==1&&(pc[0]!=0xff||pc[1]!=0x15))||(s->kind==2&&(pc[0]!=0x48||pc[1]!=0x8b||pc[2]!=0x05)))goto failed;
        u8 *relay=diag_page+s->wrapper*16+(s->kind?6:0);
        int64_t rel=relay-(pc+len);if(!fits32(rel))goto failed;
        diag_patches[i]=(DiagPatch){pc+off,*(int32_t *)(pc+off),(int32_t)rel,0,0};
    }
    for(unsigned i=0;i<7;i++)diag_direct[i]=base+diag_callees[i];
    diag_attrs_slot=(void **)(base+0x460f0);diag_info_slot=(void **)(base+0x5fcc0);diag_register_slot=(void **)(base+0x5fd80);
    u32 old;if(!VirtualProtect(diag_page,4096,0x20,&old)||!FlushInstructionCache((void *)(intptr_t)-1,diag_page,19*16))goto failed;
    return 1;
failed:VirtualFree(diag_page,0,0x8000);diag_page=0;diag_patch_count=0;return 0;
}
/* At OnInit, before gameplay. All sites become writable before any change.
 * Restore protections in reverse order: several sites share the same page. */
static __attribute__((noinline)) int diag_install(void){
    if(!diag_page||!diag_patch_count)return 0;
    unsigned ready=0;u32 ignored;
    for(;ready<diag_patch_count;ready++){
        DiagPatch *p=&diag_patches[ready];
        if(*(int32_t *)p->disp!=p->saved||!VirtualProtect(p->disp,4,0x40,&p->old))goto rollback;
        p->writable=1;
    }
    for(unsigned i=0;i<diag_patch_count;i++)copy(diag_patches[i].disp,&diag_patches[i].replacement,4);
    for(unsigned i=0;i<diag_patch_count;i++)if(!FlushInstructionCache((void *)(intptr_t)-1,diag_patches[i].disp,4))goto rollback;
    for(unsigned i=diag_patch_count;i;i--){DiagPatch *p=&diag_patches[i-1];if(!VirtualProtect(p->disp,4,p->old,&ignored))goto rollback;p->writable=0;}
    negative_active=1;diag_active=1;return 1;
rollback:
    /* Keep relay memory pinned if restoring a page fails: never free a target
     * that a surviving call site might still reference. Wrappers still forward
     * correctly with telemetry disabled. Report this exceptional state. */
    for(unsigned i=0;i<ready;i++){DiagPatch *p=&diag_patches[i];u32 temp;if(VirtualProtect(p->disp,4,0x40,&temp)){copy(p->disp,&p->saved,4);FlushInstructionCache((void *)(intptr_t)-1,p->disp,4);}}
    for(unsigned i=ready;i;i--){DiagPatch *p=&diag_patches[i-1];VirtualProtect(p->disp,4,p->old,&ignored);}
    log_line("DIAG DISABLED: call-site transaction failed; original displacements restored where writable; relay retained for safe forwarding");return 0;
}
static void diag_set_path(void){
    unsigned n=0,last=0;while(log_path[n]&&n<490){if(log_path[n]=='/'||log_path[n]=='\\')last=n+1;n++;}
    copy(diag_path,log_path,last*2);const char *name="KH2AssetLoadDiag.log";unsigned i=0;do{diag_path[last+i]=(u8)name[i];}while(name[i++]);
}
static void *diag_output; /* Worker-owned persistent handle for one flush. */
static void diag_write(const char *b,u32 n){
    void *file=diag_output?diag_output:CreateFileW(diag_path,4,3,0,4,0x80,0);if(file==(void *)(intptr_t)-1){INC(diag_write_errors);return;}
    while(n){u32 wrote=0;if(!WriteFile(file,b,n,&wrote,0)||!wrote||wrote>n){INC(diag_write_errors);break;}b+=wrote;n-=wrote;}if(!diag_output)CloseHandle(file);
}
static const char *diag_phase(unsigned p){switch(p){case D_ATTR:return "raw_attributes";case D_PATH:return "path_lookup";case D_INFO:return "package_lookup";case D_OPEN:return "open";case D_READ:return "read";case D_SEEK:return "seek";case D_TELL:return "tell";case D_CLOSE:return "close";case D_REMASTER:return "remastered";case D_REGISTER:return "register";default:return "size_total";}}
static void diag_header(void){
    diag_set_path();char b[600];unsigned n=text(b,0,"KH2AssetLoadDiag p2; call-site profile=panacea-msvc-1; ticks are QPC wall time; frequency=");n=number(b,n,diag_freq);n=text(b,n," origin=");n=number(b,n,diag_origin);n=text(b,n,"\r\nScopes: load=LoadFile only; size=original GetFileSize (miss/fallback). remastered is inclusive; size_total includes size phases. Do not sum nested phases.\r\n");diag_write(b,n);
}
static __attribute__((noinline)) void diag_flush(void){
    if(!diag_active)return;
    diag_output=CreateFileW(diag_path,4,3,0,4,0x80,0);
    if(diag_output==(void *)(intptr_t)-1){diag_output=0;INC(diag_write_errors);return;}
    u64 now=diag_now();char b[1100];unsigned n=text(b,0,"BUCKET elapsed_ticks=");n=number(b,n,now>=diag_origin?now-diag_origin:0);
    n=text(b,n," dropped=");n=number(b,n,READ(diag_dropped));n=text(b,n," untracked=");n=number(b,n,READ(diag_untracked));n=text(b,n," clock_errors=");n=number(b,n,READ(diag_clock_errors));n=text(b,n," write_errors=");n=number(b,n,READ(diag_write_errors));n=text(b,n,"\r\n");diag_write(b,n);
    for(unsigned i=0;i<DIAG_ROWS;i++){
        DiagRow row;if(!diag_try(&diag_lock))continue;
        row=diag_rows[i];memset(&diag_rows[i],0,sizeof(row));diag_unlock(&diag_lock);if(!row.used)continue;
        n=text(b,0,row.scope?"PHASE scope=size phase=":"PHASE scope=load phase=");n=text(b,n,diag_phase(row.phase));n=text(b,n," count=");n=number(b,n,row.count);n=text(b,n," total_ticks=");n=number(b,n,row.ticks);n=text(b,n," max_ticks=");n=number(b,n,row.max);n=text(b,n," unsuccessful=");n=number(b,n,row.unsuccessful);n=text(b,n," bytes=");n=number(b,n,row.bytes);n=text(b,n," file=\"");n=text(b,n,row.name);n=text(b,n,"\"\r\n");diag_write(b,n);
    }
    CloseHandle(diag_output);diag_output=0;
}
