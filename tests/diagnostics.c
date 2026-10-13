/* Run the real Windows PE wrappers with MS-x64 calls and controlled clocks.
 * An optional locally supplied Panacea is used for full call-site validation;
 * that copyrighted DLL is neither required by CI nor distributed here. */
#define main cache_harness_main
#include "harness.c"
#undef main
static void writable_imports(void){uintptr_t a=(uintptr_t)(im+IAT_GetLastError)&~4095ULL;assert(!mprotect((void *)a,4096,PROT_READ|PROT_WRITE));}
static u64 clock_ticks=1000;
static unsigned phase_calls;
static int MS clock_(int64_t *out){*out=++clock_ticks;last_error=987;return 1;}
static int MS frequency_(int64_t *out){*out=1000;return 1;}
static const uint16_t wide_path[]={'m','o','d','/','o','b','j','/','e','n','e','m','y',0};
static const uint16_t mode[]={'r','b',0};
static void *stream=(void *)0x98765432;
static void called(void){assert(last_error==321);phase_calls++;clock_ticks+=70;last_error=777;}
static void *MS open_(const uint16_t *p,const uint16_t *m){assert(p==wide_path&&m==mode);called();return stream;}
static size_t MS read_(void *p,size_t sz,size_t n,void *f){assert(p==self&&sz==4&&n==8&&f==stream);called();return 7;}
static int MS seek_(void *f,int off,int whence){assert(f==stream&&off==-17&&whence==2);called();return -1;}
static int MS tell_(void *f){assert(f==stream);called();return 12345;}
static int MS fclose_(void *f){assert(f==stream);called();return -1;}
static u32 MS attrs_(const uint16_t *p){assert(p==wide_path);called();return 0xffffffffU;}
static u8 MS transform_(uint16_t *p,int n,const char *s){assert(p==(void *)self&&n==520&&s==target);called();return 1;}
static void *MS info_(const char *s,const char *alt){assert(s==target&&!alt);called();return package;}
static int MS register_(const char *s,int size,void *data){assert(s==target&&size==12345&&data==self);called();return -1;}
static void MS remaster_(void *pkg,const uint16_t *p,void *data){assert(pkg==package&&p==wide_path&&data==self);called();}
static void diagnostic_setup(const char *dll){
    fresh(dll);writable_imports();clock_ticks=1000;phase_calls=0;
    BIND(QueryPerformanceCounter,clock_);BIND(QueryPerformanceFrequency,frequency_);
    void **d=(void **)(im+RVA_diag_direct);
    d[0]=transform_;d[1]=open_;d[2]=read_;d[3]=seek_;d[4]=tell_;d[5]=fclose_;d[6]=remaster_;
    static void *attrs_ptr=attrs_,*info_ptr=info_,*reg_ptr=register_;
    *(void ***)(im+RVA_diag_attrs_slot)=&attrs_ptr;
    *(void ***)(im+RVA_diag_info_slot)=&info_ptr;
    *(void ***)(im+RVA_diag_register_slot)=&reg_ptr;
    *(u32 *)(im+RVA_diag_active)=1;*(u64 *)(im+RVA_diag_freq)=1000;*(u64 *)(im+RVA_diag_origin)=1000;
}
#define FN(name,type) ((type)(im+RVA_##name))
#define CHECK(expr) do{last_error=321;assert(expr);assert(last_error==777);}while(0)
static void wrappers(const char *dll){
    diagnostic_setup(dll);
    CHECK(FN(diag_open_0,void *(MS *)(const uint16_t *,const uint16_t *))(wide_path,mode)==stream);
    CHECK(FN(diag_read_0,size_t(MS *)(void *,size_t,size_t,void *))(self,4,8,stream)==7);
    CHECK(FN(diag_seek_0,int(MS *)(void *,int,int))(stream,-17,2)==-1);
    CHECK(FN(diag_tell_0,int(MS *)(void *))(stream)==12345);
    CHECK(FN(diag_close_0,int(MS *)(void *))(stream)==-1);
    CHECK(FN(diag_attrs_0,u32(MS *)(const uint16_t *))(wide_path)==0xffffffffU);
    CHECK(FN(diag_transform_0,u8(MS *)(uint16_t *,int,const char *))(self,520,target)==1);
    CHECK(FN(diag_info_0,void *(MS *)(const char *,const char *))(target,0)==package);
    CHECK(FN(diag_register,int(MS *)(const char *,int,void *))(target,12345,self)==-1);
    last_error=321;FN(diag_remaster,void(MS *)(void *,const uint16_t *,void *))(package,wide_path,self);assert(last_error==777);
    CHECK(FN(diag_size,int(MS *)(SizeFn,void *,const char *))(original,self,target)==100);
    assert(phase_calls==10&&creates==0); /* NO per-call file logging */
    FN(diag_flush,void(MS *)(void))();assert(creates==1); /* one handle per flush */
    assert(strstr(logtext,"phase=read count=1 total_ticks=71 max_ticks=71 unsuccessful=1 bytes=28 file=\"mod/obj/enemy\""));
    assert(strstr(logtext,"phase=seek count=1 total_ticks=71 max_ticks=71 unsuccessful=1"));
    assert(strstr(logtext,"scope=size phase=size_total count=1"));
    assert(strstr(logtext,"phase=remastered count=1")&&strstr(logtext,"phase=register count=1"));
    loglen=0;logtext[0]=0;FN(diag_flush,void(MS *)(void))();assert(!strstr(logtext,"PHASE"));
    /* A closed stream must not borrow its previous path; contention loses only
     * telemetry and must still execute the real call with correct error state. */
    CHECK(FN(diag_tell_0,int(MS *)(void *))(stream)==12345);
    assert(*(u64 *)(im+RVA_diag_untracked)==1);
    *(u32 *)(im+RVA_diag_lock)=1;
    CHECK(FN(diag_attrs_0,u32(MS *)(const uint16_t *))(wide_path)==0xffffffffU);
    *(u32 *)(im+RVA_diag_lock)=0;assert(*(u64 *)(im+RVA_diag_dropped)==1);
    FN(diag_flush,void(MS *)(void))();assert(strstr(logtext,"<untracked-stream>"));
    for(unsigned i=0;i<8192;i++){char name[64];snprintf(name,sizeof(name),"many/%u.asset",i);CHECK(FN(diag_size,int(MS *)(SizeFn,void *,const char *))(original,self,name)==100);}
    assert(*(u64 *)(im+RVA_diag_dropped)>1);
    puts("PASS actual PE diagnostics: all typed phase wrappers preserve arguments, return values and LastError; measured durations/read bytes/failures; cache miss total; no per-call log I/O; one handle per flush; closed stream removal and nonblocking drops.");
}
typedef struct {u8 *disp;int32_t saved,replacement;u32 old;int writable;} Patch;
static unsigned diag_vp_count,diag_fail_at,diag_flush_count,diag_fail_flush;
static int MS diag_protect(void *p,size_t n,u32 mode_,u32 *old){
    if(++diag_vp_count==diag_fail_at)return 0;
    *old=0x20;(void)p;(void)n;(void)mode_;return 1;
}
static int MS diag_flush_ic(void *process,const void *p,size_t n){assert(process==(void *)(intptr_t)-1&&p&&n);return ++diag_flush_count!=diag_fail_flush;}
static void transactions(const char *dll){
    for(unsigned failure=0;failure<4;failure++){
        fresh(dll);writable_imports();BIND(VirtualProtect,diag_protect);BIND(FlushInstructionCache,diag_flush_ic);
        int32_t operands[]={11,22,33};Patch *p=(Patch *)(im+RVA_diag_patches);
        for(unsigned i=0;i<3;i++)p[i]=(Patch){(u8 *)&operands[i],operands[i],(int32_t)(44+i),0,0};
        *(unsigned *)(im+RVA_diag_patch_count)=3;*(void **)(im+RVA_diag_page)=im;
        diag_vp_count=diag_flush_count=0;diag_fail_at=failure==1?2:(failure==3?5:0);diag_fail_flush=failure==2?2:0;
        int r=FN(diag_install,int(MS *)(void))();assert(r==(failure==0));
        for(unsigned i=0;i<3;i++)assert(operands[i]==(failure?(int32_t)(11*(i+1)):(int32_t)(44+i)));
        assert(*(u32 *)(im+RVA_diag_active)==(failure==0));
    }
    puts("PASS actual PE diagnostic transaction: success; protection-before-write failure, instruction-cache flush failure and protection-restore failure roll back all call displacements.");
}
static void profile(const char *dll,const char *fixture){
    fresh(dll);writable_imports();panacea=loadpe(fixture,&panacea_size);
    assert(FN(diag_profile_ok,int(MS *)(u8 *,u8 *))(panacea,panacea+0x10e60));
    /* Callee as well as caller mutations are refused. */
    const u32 mutations[]={0xfeb0,0x10e60,0xd660,0xfa50,0x268e0,0x26814,0x2a934,0x280f8,0x27ddc,0x26fa0};
    for(unsigned i=0;i<sizeof(mutations)/sizeof(*mutations);i++){panacea[mutations[i]]^=1;assert(!FN(diag_profile_ok,int(MS *)(u8 *,u8 *))(panacea,panacea+0x10e60));panacea[mutations[i]]^=1;}
    BIND(QueryPerformanceCounter,clock_);BIND(QueryPerformanceFrequency,frequency_);
    assert(FN(diag_prepare,int(MS *)(u8 *,u8 *))(panacea,panacea+0x10e60));
    u8 *relay_page=page;
    lookup_base=panacea;lookup_target=panacea+0x10e60;
    u32 nt=*(u32 *)(panacea+60),pd=*(u32 *)(panacea+nt+24+136),pn=*(u32 *)(panacea+nt+24+140);lookup_rf=NULL;
    for(unsigned i=0;i<pn/12;i++){RF *r=(RF *)(panacea+pd+12*i);if(r->begin==0x10e60)lookup_rf=r;}assert(lookup_rf);
    assert(FN(install_hook,int(MS *)(u8 *))(lookup_target));
    u8 *cache_page=page;
    u8 *before=malloc(panacea_size);assert(before);memcpy(before,panacea,panacea_size);
    assert(FN(diag_install,int(MS *)(void))());
    unsigned count=*(unsigned *)(im+RVA_diag_patch_count);assert(count==34);assert(!mprotect(panacea,panacea_size,7));
    Patch *patches=(Patch *)(im+RVA_diag_patches);
    for(unsigned i=0;i<count;i++){
        Patch *p=&patches[i];assert(*(int32_t *)p->disp==p->replacement);
        u8 *dest=p->disp+4+p->replacement;assert(dest>=relay_page&&dest<relay_page+19*16);
        memcpy(p->disp,&p->saved,4);
    }
    assert(!memcmp(before,panacea,panacea_size));free(before);
    assert(!munmap(cache_page,4096));assert(!munmap(relay_page,4096));page=NULL;assert(!munmap(panacea,panacea_size));panacea=NULL;
    puts("PASS supplied Panacea: complete caller/callee profiles; all 34 relay operands installed; installed together with the cache detour; every other byte unchanged; patched and diagnostic version.dll variants supported without replacing them.");
}
int main(int argc,char **argv){assert(argc>=2);wrappers(argv[1]);transactions(argv[1]);for(int i=2;i<argc;i++)profile(argv[1],argv[i]);puts("LIMIT: Linux MS-x64 harness, not a game/Proton ARM64 run. Full Panacea profile tests require a local DLL and are not part of CI.");return 0;}
