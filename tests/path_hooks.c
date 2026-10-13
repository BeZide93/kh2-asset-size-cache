/* Execute the actual PE path-only patch transaction and five call sites.
 * Optional local Panacea fixtures are never packaged/distributed. */
#define main cache_harness_main
#include "harness.c"
#undef main
#define FN(name,type) ((type)(im+RVA_##name))
typedef struct {u8 *disp;int32_t saved,replacement;u32 old;int writable,written;} PathPatch;
static unsigned protect_calls,protect_fail,ic_calls,ic_fail;
static int MS patch_protect(void *ptr,size_t n,u32 mode,u32 *old){(void)ptr;assert(n==4);(void)mode;*old=0x20;return ++protect_calls!=protect_fail;}
static int MS patch_flush(void *process,const void *ptr,size_t n){assert(process==(void *)(intptr_t)-1&&ptr&&n==4);return ++ic_calls!=ic_fail;}
static void writable_imports(void){assert(!mprotect((void *)((uintptr_t)(im+IAT_GetLastError)&~4095ULL),4096,3));}
static void transactions(const char *dll){
    /* All five make-writable calls, all five restore calls, all flushes,
     * plus a changed operand observed before the first write. */
    for(unsigned failure=0;failure<=16;failure++){
        fresh(dll);writable_imports();BIND(VirtualProtect,patch_protect);BIND(FlushInstructionCache,patch_flush);
        int32_t operands[5],before[5];PathPatch *p=(PathPatch *)(im+RVA_path_patches);
        for(unsigned i=0;i<5;i++){operands[i]=before[i]=100+i;p[i]=(PathPatch){&((u8 *)operands)[4*i],operands[i],200+(int)i,0,0,0};}
        *(u8 **)(im+RVA_path_page)=im;*(unsigned *)(im+RVA_path_patch_count)=5;
        if(failure==16)operands[4]=before[4]=999;
        protect_calls=ic_calls=0;protect_fail=failure<=10?failure:0;ic_fail=failure>=11&&failure<=15?failure-10:0;
        int ok=FN(path_commit,int(MS *)(void))();assert(ok==(failure==0));
        for(unsigned i=0;i<5;i++){if(operands[i]!=(failure?before[i]:200+(int)i))fprintf(stderr,"failure=%u operand=%u value=%d expected=%d protects=%u flush=%u\n",failure,i,operands[i],failure?before[i]:200+(int)i,protect_calls,ic_calls);assert(operands[i]==(failure?before[i]:200+(int)i));}
        assert(*(u32 *)(im+RVA_negative_active)==(failure==0)&&creates==0);
        assert(*(u8 **)(im+RVA_path_page)==im); /* keep surviving targets valid */
    }
    puts("PASS compiled path-only transaction: exactly five operands; every protect/flush/restore failure rolls back; changed operand is not overwritten; no log writes; cache stays disabled on failure.");
}
static const u32 sites[]={0xd6ca,0xd70c,0xd765,0xff44,0x10ef6};
static const uint16_t name[]={'Z',':','/','m','o','d','/','x',0};
static unsigned calls;
static u32 MS attrs(const uint16_t *p){assert(p==name&&last_error==321);calls++;last_error=2;return 0xffffffffU;}
static void synthetic_calls(const char *dll){
    fresh(dll);panacea=mmap(NULL,0x60000,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(panacea!=MAP_FAILED);
    *(void **)(panacea+0x460f0)=attrs;
    for(unsigned i=0;i<5;i++){
        u8 *pc=panacea+sites[i];pc[0]=0xff;pc[1]=0x15;*(int32_t *)(pc+2)=0x460f0-sites[i]-6;
        /* MS-x64 callable thunk: shadow space + stack alignment. */
        memcpy(pc-4,"\x48\x83\xec\x28",4);memcpy(pc+6,"\x48\x83\xc4\x28\xc3",5);
    }
    vp_calls=flush_calls=0;page=NULL;
    assert(FN(path_install,int(MS *)(u8 *))(panacea)==1);calls=0;
    for(unsigned i=0;i<5;i++){last_error=321;assert(((u32(MS *)(const uint16_t *))(panacea+sites[i]-4))(name)==0xffffffffU&&last_error==2);}
    assert(calls==1&&creates==0);ticks=5100;last_error=321;
    assert(((u32(MS *)(const uint16_t *))(panacea+sites[0]-4))(name)==0xffffffffU&&calls==2&&last_error==2);
    *(u32 *)(im+RVA_negative_active)=0;
    for(unsigned i=0;i<5;i++){last_error=321;assert(((u32(MS *)(const uint16_t *))(panacea+sites[i]-4))(name)==0xffffffffU&&last_error==2);}
    assert(calls==7);munmap(page,4096);page=NULL;munmap(panacea,0x60000);panacea=NULL;
    puts("PASS all five emitted indirect calls: MS-x64 arguments/return/LastError; shared negative cache; five-second expiry; disabled relay forwards without caching; no request logging.");
}
static void profile(const char *dll,const char *fixture){
    fresh(dll);panacea=loadpe(fixture,&panacea_size);
    assert(FN(path_profile_ok,int(MS *)(u8 *,u8 *))(panacea,panacea+0x10e60));
    const u32 mutations[]={0xfeb0,0x10e60,0xd660,0xd6ca,0xd70c,0xd765,0xff44,0x10ef6};
    for(unsigned i=0;i<sizeof(mutations)/sizeof(*mutations);i++){panacea[mutations[i]]^=1;assert(!FN(path_profile_ok,int(MS *)(u8 *,u8 *))(panacea,panacea+0x10e60));panacea[mutations[i]]^=1;}
    setup_lookup();vp_calls=flush_calls=addcalls=delcalls=0;page=NULL;
    assert(FN(install_hook,int(MS *)(u8 *))(lookup_target));u8 *size_page=page;
    u8 *before=malloc(panacea_size);assert(before);memcpy(before,panacea,panacea_size);
    assert(FN(path_install,int(MS *)(u8 *))(panacea)==1);u8 *relay=page;
    assert(*(unsigned *)(im+RVA_path_patch_count)==5);assert(!mprotect(panacea,panacea_size,7));
    PathPatch *p=(PathPatch *)(im+RVA_path_patches);
    for(unsigned i=0;i<5;i++){assert(p[i].disp==panacea+sites[i]+2);assert(*(int32_t *)p[i].disp==p[i].replacement);assert(p[i].disp+4+p[i].replacement==relay+6);memcpy(p[i].disp,&p[i].saved,4);}
    assert(!memcmp(before,panacea,panacea_size));free(before);
    munmap(size_page,4096);munmap(relay,4096);page=NULL;munmap(panacea,panacea_size);panacea=NULL;
    puts("PASS supplied Panacea: caller profiles reject mutations; independent size detour plus exactly five path-cache operands; all other bytes unchanged.");
}
int main(int argc,char **argv){assert(argc>=2);transactions(argv[1]);synthetic_calls(argv[1]);for(int i=2;i<argc;i++)profile(argv[1],argv[i]);puts("LIMIT: compiled PE tests with WinAPI substitutes, not a KH2/Proton ARM64 integration test.");return 0;}
