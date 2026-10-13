/* Use the same actual-PE loader and MS-x64 WinAPI substitutes as the cache tests. */
#define main cache_harness_main
#include "harness.c"
#undef main
#include "recognition_fixture.h"
typedef struct {u8 *size_fn;InfoFn volatile *info;} Discovery;
static u8 *synthetic_image(void){
    u8 *b=mmap(NULL,0xc000,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(b!=MAP_FAILED);
    *(uint16_t *)b=0x5a4d;*(u32 *)(b+60)=0x80;*(u32 *)(b+0x80)=0x4550;
    *(uint16_t *)(b+0x84)=0x8664;*(uint16_t *)(b+0x86)=3;*(uint16_t *)(b+0x94)=240;
    *(uint16_t *)(b+0x98)=0x20b;*(u32 *)(b+0x98+56)=0xc000;*(u32 *)(b+0x98+60)=0x400;*(u32 *)(b+0x98+108)=16;
    const u32 starts[]={0x1000,0x7000,0xa000},sizes[]={0x5000,0x3000,0x2000},flags[]={0x60000020,0x40000040,0xc0000040};
    for(unsigned i=0;i<3;i++){u8 *s=b+0x188+40*i;*(u32 *)(s+8)=sizes[i];*(u32 *)(s+12)=starts[i];*(u32 *)(s+36)=flags[i];}
    return b;
}
static void place_function(u8 *b,u32 rva,u32 data_shift,u32 code_shift,u32 read_shift){
    memcpy(b+rva,fixture_code,sizeof(fixture_code));
    for(unsigned i=0;i<sizeof(fixture_refs)/sizeof(*fixture_refs);i++){
        u32 dest=fixture_refs[i].target;
        if(dest>=0xa000)dest+=data_shift;else if(dest>=0x9000)dest+=read_shift;else if(dest>=0x8a00&&dest<=0x8a10)dest+=read_shift;else if(dest>=0x5000&&dest<0x6000)dest+=code_shift;
        *(int32_t *)(b+rva+fixture_refs[i].at)=(int32_t)dest-(int32_t)(rva+fixture_refs[i].end);
    }
}
static void prepare(u32 rva,u32 shift){
    panacea=synthetic_image();fakegame=synthetic_image();panacea_size=0xc000;
    *(u32 *)(panacea+0x88)=0x12345678+shift; /* arbitrary build timestamp */
    *(u32 *)(panacea+0x98+120)=0x7800;*(u32 *)(panacea+0x98+124)=40;
    *(u32 *)(panacea+0x98+136)=0x7000;*(u32 *)(panacea+0x98+140)=12;
    *(RF *)(panacea+0x7000)=(RF){rva,rva+sizeof(fixture_code),0x7100};
    memcpy(panacea+0x7100,fixture_unwind,sizeof(fixture_unwind));
    *(u32 *)(panacea+0x7800)=0x7a00;*(u32 *)(panacea+0x780c)=0x7900;*(u32 *)(panacea+0x7810)=0x8a00+shift;
    strcpy((char *)panacea+0x7900,"KERNEL32.dll");
    const char *names[]={"GetFileAttributesW","FindFirstFileA","CloseHandle"};
    for(unsigned i=0;i<3;i++){*(u64 *)(panacea+0x7a00+8*i)=0x7b00+64*i;strcpy((char *)panacea+0x7b02+64*i,names[i]);}
    for(unsigned i=0;i<sizeof(fixture_strings)/sizeof(*fixture_strings);i++)memcpy(panacea+fixture_strings[i].rva+shift,fixture_strings[i].bytes,100);
    place_function(panacea,rva,shift,shift,shift);
    *(void **)(panacea+0xa000+shift)=fakegame+0x1000;
    lookup_base=panacea;lookup_target=panacea+rva;lookup_rf=(RF *)(panacea+0x7000);
}
static void cleanup(void){assert(!munmap(panacea,0xc000));assert(!munmap(fakegame,0xc000));panacea=fakegame=NULL;}
static int discover(Discovery *d){return ((int(MS *)(u8 *,u8 *,Discovery *))(im+RVA_discover_panacea))(panacea,fakegame,d);}
static void test_recognition(const char *dll){
    fresh(dll);Discovery d;
    for(unsigned i=0;i<3;i++){
        u32 rva=0x1800+i*0x700,shift=i*0x100;prepare(rva,shift);
        u8 *before=malloc(0xc000);assert(before);memcpy(before,panacea,0xc000);
        assert(discover(&d)==1&&d.size_fn==panacea+rva&&(u8 *)d.info==panacea+0xa000+shift);
        assert(!memcmp(before,panacea,0xc000));free(before);
        /* Install through real OnInit using a non-original RVA, no export table,
         * shifted globals/strings/callees and a different timestamp. */
        fresh(dll);vp_calls=flush_calls=addcalls=delcalls=0;page=NULL;
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        assert(strstr(logtext,"INSTALLED:")&&strstr(logtext,"path_cache=DISABLED")&&panacea[rva]==0xff&&last_error==321);
        assert(*(void **)(im+RVA_info_slot)==panacea+0xa000+shift);
        assert(creates==1&&!threadcalls&&strchr(logtext,'\n')==logtext+loglen-1);
        unsigned opens=creates;size_t bytes=loglen;
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        assert(creates==opens&&loglen==bytes);
        assert(!munmap(page,4096));page=NULL;cleanup();
    }
    puts("PASS actual PE discovery/OnInit: moved function, data slot, callees and strings; arbitrary timestamps; no export RVAs; unchanged scanning input and correct installed target.");
    for(unsigned bad=0;bad<12;bad++){
        fresh(dll);prepare(0x2000,0);u32 rva=0x2000;
        switch(bad){
        case 0: panacea[rva+0x15]^=1;break; /* opcode/operand mutation */
        case 1: *(u32 *)(panacea+rva+0x161)=0x130;break; /* package layout operand */
        case 2: *(void **)(panacea+0xa000)=fakegame+0xa100;break; /* game data, not code */
        case 3: *(void **)(panacea+0xa000)=NULL;break;
        case 4: panacea[0x7b02]='X';break; /* wrong attribute import */
        case 5: panacea[0x7104]^=1;break; /* incompatible unwind */
        case 6: *(u32 *)(panacea+0x98+136)=0xbfff;break; /* bad exception table */
        case 7: *(u32 *)(panacea+0x188+36)|=0x80000000U;break; /* writable code */
        case 8: *(u32 *)(panacea+0x98+124)=0xffffffffU;break; /* bad import range */
        case 9: *(u32 *)(panacea+rva+fixture_refs[1].at)=0x7fffffff;break; /* out-of-image reference */
        case 10: for(unsigned j=0;j<sizeof(fixture_strings)/sizeof(*fixture_strings);j++)memset(panacea+fixture_strings[j].rva,0,100);break;
        case 11: *(u32 *)(panacea+0x98+140)=24;*(RF *)(panacea+0x700c)=(RF){0x3000,0x3000+sizeof(fixture_code),0x7100};place_function(panacea,0x3000,0,0,0);break;
        }
        u8 *before=malloc(0xc000);assert(before);memcpy(before,panacea,0xc000);
        int ok=discover(&d);assert(ok==(bad==11?-1:0)&&!d.size_fn&&!d.info);
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        assert(strstr(logtext,"DISABLED:")&&!pins&&!threadcalls&&!memcmp(before,panacea,0xc000));
        assert(creates==1&&strchr(logtext,'\n')==logtext+loglen-1);free(before);cleanup();
    }
    puts("PASS actual PE: opcode/layout/import/string/unwind mutations, invalid pointers/tables, writable code and ambiguous matches fail closed without any patch.");
}
static void test_status(const char *dll){
    /* An old multi-line file must be replaced on each independent load,
     * including refusal; a second OnInit in the same load is a no-op. */
    for(unsigned mode=0;mode<7;mode++){
        fresh(dll);prepare(0x2000,0);vp_calls=flush_calls=addcalls=delcalls=0;page=NULL;
        strcpy(logtext,"OLD RUN\nOLD STATS\n");loglen=strlen(logtext);
        u8 saved[20];memcpy(saved,panacea+0x2000,20);
        if(mode==1)panacea[0x2015]^=1;
        if(mode==2)fail_mode=8;
        if(mode==3)fail_mode=10; /* file cannot be opened */
        if(mode==4)fail_mode=11; /* write fails */
        if(mode==5)fail_mode=12; /* short writes */
        if(mode==6)fail_mode=5; /* hook transaction fails */
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        assert(creates==1&&!threadcalls&&last_error==321);
        if(mode==3)assert(strstr(logtext,"OLD STATS"));
        else if(mode==4)assert(loglen==0);
        else assert(!strstr(logtext,"OLD")&&strchr(logtext,'\n')==logtext+loglen-1&&strstr(logtext,(mode==1||mode==2||mode==6)?"DISABLED:":"INSTALLED:"));
        if(mode==1||mode==2||mode==6)assert(!memcmp(saved,panacea+0x2000,20));
        else assert(panacea[0x2000]==0xff); /* log failures cannot disable cache */
        size_t bytes=loglen;((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        assert(creates==1&&loglen==bytes);
        if(page){assert(!munmap(page,4096));page=NULL;}cleanup();
    }
    fresh(dll);panacea=NULL;fakegame=NULL;
    ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
    assert(creates==1&&strstr(logtext,"none of version.dll, dbghelp.dll or dinput8.dll is loaded")&&last_error==321);
    puts("PASS: one startup line replaces old logs; duplicate init is silent; missing/incompatible Panacea and initialization failures report status; open/write failures preserve cache installation and LastError; short writes complete.");
}

static void test_proxy_names(const char *dll){
    const char *names[]={"version.dll","VERSION.DLL","dbghelp.dll","DBGHELP.dll","dinput8.dll","DINPUT8.DLL"};
    for(unsigned i=0;i<6;i++){
        fresh(dll);prepare(0x2000,0);vp_calls=flush_calls=addcalls=delcalls=0;page=NULL;
        u8 *ordinary=synthetic_image();u8 before[0xc000];memcpy(before,ordinary,sizeof(before));
        proxy_map=1;proxy_names[0]=names[i];proxy_modules[0]=panacea;
        /* A non-Panacea version module may coexist with either other proxy. */
        proxy_names[1]=i>=2?"version.dll":"dbghelp.dll";proxy_modules[1]=ordinary;
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
        const char *expected=i<2?"panacea=version.dll":i<4?"panacea=dbghelp.dll":"panacea=dinput8.dll";
        assert(strstr(logtext,"INSTALLED:")&&strstr(logtext,expected)&&last_error==321);
        assert(panacea[0x2000]==0xff&&!memcmp(before,ordinary,sizeof(before))&&creates==1&&pins==2);
        assert(strchr(logtext,'\n')==logtext+loglen-1);unsigned opens=creates;
        ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);assert(creates==opens);
        munmap(page,4096);page=NULL;munmap(ordinary,0xc000);cleanup();
    }
    /* The same handle under aliases is a single candidate, not ambiguity. */
    fresh(dll);prepare(0x2000,0);vp_calls=flush_calls=0;page=NULL;proxy_map=1;
    proxy_names[0]="version.dll";proxy_names[1]="DBGHELP.dll";proxy_names[2]="dinput8.dll";
    for(unsigned i=0;i<3;i++)proxy_modules[i]=panacea;
    ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
    assert(strstr(logtext,"INSTALLED:")&&pins==2);munmap(page,4096);page=NULL;cleanup();
    /* Two distinct compatible modules must be refused before either changes. */
    fresh(dll);prepare(0x2000,0);u8 *other=synthetic_image();memcpy(other,panacea,0xc000);
    proxy_map=1;proxy_names[0]="version.dll";proxy_names[1]="dbghelp.dll";proxy_modules[0]=panacea;proxy_modules[1]=other;
    u8 before[0xc000];memcpy(before,panacea,sizeof(before));
    ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
    assert(strstr(logtext,"multiple compatible Panacea")&&!pins&&creates==1&&last_error==321);
    assert(!memcmp(before,panacea,sizeof(before))&&!memcmp(before,other,sizeof(before)));
    munmap(other,0xc000);cleanup();
    /* Unsupported modules under every name are not patched. */
    fresh(dll);prepare(0x2000,0);panacea[0x2015]^=1;proxy_map=1;
    proxy_names[0]="DBGHELP.dll";proxy_modules[0]=panacea;memcpy(before,panacea,sizeof(before));
    ((void(MS *)(const uint16_t *))(im+RVA_OnInit))(NULL);
    assert(strstr(logtext,"DISABLED: unsupported")&&!pins&&!memcmp(before,panacea,sizeof(before)));
    cleanup();
    puts("PASS compiled OnInit: three proxy names and mixed/uppercase loader matching; coexisting system module unchanged; repeated handle deduplication; multiple compatible modules refused before writes; unsupported renamed modules refused; one startup line and LastError preserved.");
}

int main(int argc,char **argv){assert(argc==2);test_recognition(argv[1]);test_status(argv[1]);test_proxy_names(argv[1]);munmap(im,IMAGE_SIZE);puts("LIMIT: synthetic layout variants are not independently released Panacea builds or a Proton/game test.");return 0;}
