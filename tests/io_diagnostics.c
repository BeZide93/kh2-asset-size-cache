#define main cache_harness_main
#include "harness.c"
#undef main
#define FN(name,type) ((type)(im+RVA_##name))
typedef struct {void **slot;void *original,*replacement;u32 module,api,rva,old;} IoPatch;
static u64 qclock=1000;
static u32 qcalls,api_calls,read_mode,protection_calls,protection_failure;
static int MS qclock_(int64_t *v){*v=++qclock;qcalls++;last_error=555;return 1;}
static void api_called(void){assert(last_error==321);api_calls++;qclock+=4;last_error=777;}
static int MS read_api(void *h,void *buf,u32 n,u32 *done,void *ov){
    assert(h==self&&buf==package&&n==123);
    api_called();if(read_mode){assert(ov==package&&done==(void *)1);last_error=997;return 0;}
    assert(!ov&&done);*done=37;return 1;
}
static int MS seek_api(void *h,int64_t dist,int64_t *pos,u32 method){assert(h==self&&dist==-0x123456789LL&&method==2);api_called();*pos=0xabcdef0123LL;return 1;}
static int MS over_api(void *h,void *ov,u32 *done,int wait){assert(h==self&&ov==package&&wait==1);api_called();*done=71;return 1;}
static int MS overex_api(void *h,void *ov,u32 *done,u32 ms,int alert){assert(h==self&&ov==package&&ms==456&&alert==1);api_called();*done=72;return 1;}
static u32 MS single_api(void *h,u32 ms){assert(h==self&&ms==456);api_called();return 258;}
static u32 MS singleex_api(void *h,u32 ms,int alert){assert(h==self&&ms==456&&alert==1);api_called();return 192;}
static void *handles[2];
static u32 MS multi_api(u32 n,void *const *h,int all,u32 ms){assert(n==2&&h==handles&&all==1&&ms==456);api_called();return 1;}
static u32 MS multiex_api(u32 n,void *const *h,int all,u32 ms,int alert){assert(n==2&&h==handles&&all==1&&ms==456&&alert==1);api_called();return 192;}
static void MS sleep_api(u32 ms){assert(ms==456);api_called();}
static u32 MS sleepex_api(u32 ms,int alert){assert(ms==456&&alert==1);api_called();return 192;}
static void MS lock_api(void *l){assert(l==self);api_called();}
static void setup(const char *dll){
    fresh(dll);uintptr_t a=(uintptr_t)(im+IAT_GetLastError)&~4095ULL;assert(!mprotect((void *)a,4096,3));BIND(QueryPerformanceCounter,qclock_);
    qclock=1000;qcalls=api_calls=read_mode=0;*(u64 *)(im+RVA_diag_freq)=1000;*(u32 *)(im+RVA_diag_active)=1;
    void *p[]={read_api,seek_api,over_api,overex_api,single_api,singleex_api,multi_api,multiex_api,sleep_api,sleepex_api,lock_api,lock_api,lock_api};
    memcpy(im+RVA_io_original,p,sizeof(p));memcpy(im+RVA_io_original+sizeof(p),p,sizeof(p));
    *(u32 *)(im+RVA_io_enabled)=1;
}
static void *wrap(u32 m,u32 api){return FN(io_wrapper,void *(MS *)(u32,u32))(m,api);}
#define CALL(T,api,args,expected) do{last_error=321;assert(((T)wrap(m,api))args==(expected));assert(last_error==777);}while(0)
#define VOIDCALL(T,api,args) do{last_error=321;((T)wrap(m,api))args;assert(last_error==777);}while(0)
static void abi(const char *dll){
    setup(dll);
    for(u32 enabled=0;enabled<2;enabled++){
        *(u32 *)(im+RVA_io_windows)=enabled;u32 before=qcalls;
        for(u32 m=0;m<2;m++){
            u32 done=0;int64_t pos=0;
            CALL(int(MS *)(void *,void *,u32,u32 *,void *),0,(self,package,123,&done,0),1);assert(done==37);
            CALL(int(MS *)(void *,int64_t,int64_t *,u32),1,(self,-0x123456789LL,&pos,2),1);assert(pos==0xabcdef0123LL);
            CALL(int(MS *)(void *,void *,u32 *,int),2,(self,package,&done,1),1);assert(done==71);
            CALL(int(MS *)(void *,void *,u32 *,u32,int),3,(self,package,&done,456,1),1);assert(done==72);
            CALL(u32(MS *)(void *,u32),4,(self,456),258);
            CALL(u32(MS *)(void *,u32,int),5,(self,456,1),192);
            CALL(u32(MS *)(u32,void *const *,int,u32),6,(2,handles,1,456),1);
            CALL(u32(MS *)(u32,void *const *,int,u32,int),7,(2,handles,1,456,1),192);
            VOIDCALL(void(MS *)(u32),8,(456));
            CALL(u32(MS *)(u32,int),9,(456,1),192);
            for(u32 id=10;id<13;id++)VOIDCALL(void(MS *)(void *),id,(self));
        }
        assert(qcalls-before==(enabled?52:0)); /* no clocks when gate closed */
    }
    assert(api_calls==52&&creates==0);
    u32 m=0;read_mode=1;last_error=321;
    assert(((int(MS *)(void *,void *,u32,u32 *,void *))wrap(m,0))(self,package,123,(void *)1,package)==0);
    assert(last_error==997); /* pending: never dereference the output pointer */
    read_mode=0;FN(diag_flush,void(MS *)(void))();
    assert(strstr(logtext,"IOEVENT module=game api=ReadFile")&&strstr(logtext,"bytes=37 result=1 last_error=777"));
    assert(strstr(logtext,"bytes=0 result=0 last_error=997"));
    assert(strstr(logtext,"api=WaitForMultipleObjectsEx"));
    *(u32 *)(im+RVA_io_lock)=1;u32 done;CALL(int(MS *)(void *,void *,u32,u32 *,void *),0,(self,package,123,&done,0),1);
    *(u32 *)(im+RVA_io_lock)=0;assert(*(u64 *)(im+RVA_io_dropped)==1);
    for(u32 i=0;i<1100;i++)CALL(int(MS *)(void *,void *,u32,u32 *,void *),0,(self,package,123,&done,0),1);
    assert(*(u32 *)(im+RVA_io_queued)==1024&&*(u64 *)(im+RVA_io_dropped)>1);
    puts("PASS actual PE I/O: all 26 wrappers preserve 64-bit/stack arguments, outputs, result and LastError; no clocks outside load windows; async pending output not dereferenced; no hook log I/O; bounded overflow/contention.");
}
static u8 *fixture(void){
    u8 *b=mmap(NULL,0x6000,3,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(b!=MAP_FAILED);
    *(uint16_t *)b=0x5a4d;*(u32 *)(b+60)=0x80;*(u32 *)(b+0x80)=0x4550;
    *(uint16_t *)(b+0x84)=0x8664;*(uint16_t *)(b+0x86)=1;*(uint16_t *)(b+0x94)=240;
    *(uint16_t *)(b+0x98)=0x20b;*(u32 *)(b+0x98+56)=0x6000;*(u32 *)(b+0x98+60)=0x400;*(u32 *)(b+0x98+108)=16;
    *(u32 *)(b+0x190)=0x5000;*(u32 *)(b+0x194)=0x1000;*(u32 *)(b+0x1ac)=0x40000040;
    *(u32 *)(b+0x98+120)=0x1100;*(u32 *)(b+0x98+124)=40;
    *(u32 *)(b+0x1100)=0x1300;*(u32 *)(b+0x110c)=0x1200;*(u32 *)(b+0x1110)=0x2000;
    strcpy((char *)b+0x1200,"KERNEL32.dll");
    *(u64 *)(b+0x1300)=0x1400;strcpy((char *)b+0x1402,"ReadFile");*(void **)(b+0x2000)=read_api;
    *(u64 *)(b+0x1308)=0x1480;strcpy((char *)b+0x1482,"WaitForSingleObject");*(void **)(b+0x2008)=single_api;
    return b;
}
static int MS vp_fail(void *p,size_t n,u32 mode,u32 *old){
    protection_calls++;if(protection_calls==protection_failure)return 0;
    assert(n==8&&(mode==4||mode==2));*old=2;(void)p;return 1;
}
static void imports(const char *dll){
    for(u32 which=0;which<3;which++){
        setup(dll);memset(im+RVA_io_original,0,2*13*8);*(u32 *)(im+RVA_io_enabled)=0;u8 *b=fixture();
        if(which==1)strcpy((char *)b+0x1200,"KERNELBASE.dll");
        if(which==2)strcpy((char *)b+0x1200,"api-ms-win-core-file-l1-1-0.dll");
        assert(FN(io_collect,int(MS *)(u8 *,u32))(b,0)==1&&*(u32 *)(im+RVA_io_count)==2);
        assert(*(void **)(b+0x2000)==read_api); /* collection is read-only */
        assert(FN(io_commit,int(MS *)(void))());
        assert(*(void **)(b+0x2000)==wrap(0,0)&&*(void **)(b+0x2008)==wrap(0,4));
        *(u32 *)(im+RVA_io_windows)=1;u32 done=0;last_error=321;
        assert(((int(MS *)(void *,void *,u32,u32 *,void *))*(void **)(b+0x2000))(self,package,123,&done,0)==1&&done==37&&last_error==777);
        assert(!mprotect(b,0x6000,3));assert(!munmap(b,0x6000));
    }
    for(u32 bad=0;bad<7;bad++){
        setup(dll);memset(im+RVA_io_original,0,2*13*8);u8 *b=fixture();
        if(bad==0)*(u32 *)(b+0x1110)=0x2001;
        if(bad==1)*(u64 *)(b+0x1300)=0x6000;
        if(bad==2)*(u32 *)(b+0x98+124)=20; /* missing descriptor terminator */
        if(bad==3){*(u64 *)(b+0x1308)=0x1400;*(void **)(b+0x2008)=single_api;} /* ambiguous original */
        if(bad==4)*(void **)(b+0x2000)=NULL;
        if(bad==5)*(u32 *)(b+0x1ac)|=0x20000000;
        if(bad==6)*(u32 *)(im+RVA_io_count)=64;
        assert(!FN(io_collect,int(MS *)(u8 *,u32))(b,0));assert(!munmap(b,0x6000));
    }
    for(u32 failure=1;failure<=4;failure++){
        setup(dll);memset(im+RVA_io_original,0,2*13*8);*(u32 *)(im+RVA_io_enabled)=0;u8 *b=fixture();
        assert(FN(io_collect,int(MS *)(u8 *,u32))(b,0));
        BIND(VirtualProtect,vp_fail);protection_calls=0;protection_failure=failure;
        assert(!FN(io_commit,int(MS *)(void))());
        assert(*(void **)(b+0x2000)==read_api&&*(void **)(b+0x2008)==single_api&&!*(u32 *)(im+RVA_io_enabled));
        assert(!munmap(b,0x6000));
    }
    setup(dll);memset(im+RVA_io_original,0,2*13*8);*(u32 *)(im+RVA_io_enabled)=0;u8 *b=fixture();
    assert(FN(io_collect,int(MS *)(u8 *,u32))(b,0));*(void **)(b+0x2008)=read_api;
    assert(!FN(io_commit,int(MS *)(void))());assert(*(void **)(b+0x2000)==read_api&&*(void **)(b+0x2008)==read_api);
    assert(!mprotect(b,0x6000,3));assert(!munmap(b,0x6000));
    puts("PASS actual PE IAT: kernel32/kernelbase/API-set discovery, unchanged-before-commit, alignment/bounds/null/ambiguous/execute/capacity rejection, page-protection rollback and foreign-pointer CAS preservation.");
}
int main(int argc,char **argv){
    assert(argc>=2);abi(argv[1]);imports(argv[1]);
    for(int i=2;i<argc;i++){setup(argv[1]);memset(im+RVA_io_original,0,2*13*8);size_t n;u8 *b=loadpe(argv[i],&n);assert(FN(io_collect,int(MS *)(u8 *,u32))(b,1));assert(*(u32 *)(im+RVA_io_count)>=3);munmap(b,n);}
    puts("LIMIT: compiled AMD64 code with API doubles; actual game IAT coverage is only known from the device startup log.");return 0;
}
