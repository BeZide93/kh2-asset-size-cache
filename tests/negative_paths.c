/* Executes the compiled Windows x64 negative cache, not a C reimplementation. */
#define main diagnostic_harness_main
#include "harness.c"
#undef main
#define FN(name,type) ((type)(im+RVA_##name))
static u32 attr_result=0xffffffffU,attr_error=2;
static unsigned attr_calls,attr_hold,attr_entered,attr_release;
static const uint16_t absolute_path[]={'Z',':','/','m','o','d','/','m','i','s','s','i','n','g',0};
static u32 MS attributes(const uint16_t *p){
    (void)p;assert(last_error==321);__atomic_fetch_add(&attr_calls,1,__ATOMIC_RELAXED);
    if(attr_hold&&tid==2){__atomic_store_n(&attr_entered,1,__ATOMIC_RELEASE);while(!__atomic_load_n(&attr_release,__ATOMIC_ACQUIRE))sched_yield();last_error=888;return 0x20;}
    last_error=attr_error;return attr_result;
}
static u32 MS changed_attributes(const uint16_t *p){return attributes(p);}
static void *attr_slot=attributes;
static void setup_negative(const char *dll){
    fresh(dll);attr_slot=attributes;*(void ***)(im+RVA_diag_attrs_slot)=&attr_slot;*(u32 *)(im+RVA_negative_active)=1;
    attr_calls=attr_hold=attr_entered=attr_release=0;attr_result=0xffffffffU;attr_error=2;
}
static u32 query(const uint16_t *p,u32 error){last_error=321;u32 r=FN(negative_attrs,u32(MS *)(const uint16_t *))(p);assert(last_error==error);return r;}
static void basic(const char *dll){
    setup_negative(dll);assert(query(absolute_path,2)==0xffffffffU);
    for(unsigned i=0;i<1000;i++){ticks=101+i;assert(query(absolute_path,2)==0xffffffffU);}assert(attr_calls==1&&*(u64 *)(im+RVA_neg_hits)==1000);
    ticks=5099;assert(query(absolute_path,2)==0xffffffffU&&attr_calls==1);
    ticks=5100;attr_result=0x20;attr_error=999;assert(query(absolute_path,999)==0x20&&attr_calls==2);
    assert(query(absolute_path,999)==0x20&&attr_calls==3); /* success never cached */
    attr_result=0x10;assert(query(absolute_path,999)==0x10&&attr_calls==4); /* directory */
    attr_result=0xffffffffU;attr_error=5;assert(query(absolute_path,5)==0xffffffffU);assert(query(absolute_path,5)==0xffffffffU&&attr_calls==6);
    attr_error=3;assert(query(absolute_path,3)==0xffffffffU);assert(query(absolute_path,3)==0xffffffffU&&attr_calls==7);
    ticks=1;assert(query(absolute_path,3)==0xffffffffU&&attr_calls==8); /* backward clock */
    attr_slot=changed_attributes;assert(query(absolute_path,3)==0xffffffffU&&attr_calls==9);
    uint16_t other[260];memcpy(other,absolute_path,sizeof(absolute_path));other[7]='M';assert(query(other,3)==0xffffffffU&&attr_calls==10); /* no case folding */
    memcpy(other,absolute_path,sizeof(absolute_path));other[0]='Y';assert(query(other,3)==0xffffffffU&&attr_calls==11);
    memcpy(other,absolute_path,sizeof(absolute_path));other[8]=0x3042;assert(query(other,3)==0xffffffffU&&attr_calls==12);assert(query(other,3)==0xffffffffU&&attr_calls==12);
    const uint16_t relative[]={'m','o','d','/','x',0},drive_relative[]={'Z',':','x',0},root_relative[]={'/','x',0},unc[]={'\\','\\','s','e','r','v','e','r',0};
    const uint16_t *skip[]={NULL,relative,drive_relative,root_relative,unc};
    for(unsigned i=0;i<5;i++){unsigned before=attr_calls;query(skip[i],3);query(skip[i],3);assert(attr_calls==before+2);}
    uint16_t longpath[300];for(unsigned i=0;i<299;i++)longpath[i]='x';longpath[0]='Z';longpath[1]=':';longpath[2]='/';longpath[299]=0;
    unsigned before=attr_calls;query(longpath,3);query(longpath,3);assert(attr_calls==before+2);
    assert(creates==0);
    puts("PASS actual PE negative cache: exact UTF-16 absolute keys, 1000 hits, fixed 5-second expiry/no sliding TTL, file creation visible after expiry, success/directory/access-denied never cached, both missing errors preserved, backward clock, changed callback, unsafe/long path bypass; no request logging.");
}
static void *overlap(void *unused){(void)unused;tid=2;assert(query(absolute_path,888)==0x20);return NULL;}
static void concurrency(const char *dll){
    setup_negative(dll);query(absolute_path,2);assert(attr_calls==1);
    *(u32 *)(im+RVA_negative)=1;attr_hold=1;pthread_t t;assert(!pthread_create(&t,NULL,overlap,NULL));while(!__atomic_load_n(&attr_entered,__ATOMIC_ACQUIRE))sched_yield();
    *(u32 *)(im+RVA_negative)=0;
    query(absolute_path,2);query(absolute_path,2);assert(attr_calls==4); /* neither hit nor publication during fallback */
    __atomic_store_n(&attr_release,1,__ATOMIC_RELEASE);assert(!pthread_join(t,NULL));
    query(absolute_path,2);assert(attr_calls==5);query(absolute_path,2);assert(attr_calls==5);
    assert(!*(u32 *)(im+RVA_negative_fallbacks));
    puts("PASS actual PE negative cache: overlapping positive fallback invalidates old misses, prevents hit/publication while active, and permits caching again after completion; no spin/wait in cache.");
}
int main(int argc,char **argv){assert(argc==2);basic(argv[1]);concurrency(argv[1]);return 0;}
