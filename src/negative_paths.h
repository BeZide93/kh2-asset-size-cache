/* Bounded negative GetFileAttributesW cache for verified Panacea call sites.
 * Only error 2/3, exact absolute DOS paths, fixed TTL. Apache-2.0. */
#define NEG_SETS 256
#define NEG_WAYS 4
#define NEG_TTL_MS 5000
typedef u32 (*AttrFn)(const uint16_t *);
typedef struct {u32 valid,hash,error;u64 stamp,epoch,original;uint16_t path[260];} NegativeEntry;
static struct {u32 lock,next[NEG_SETS];NegativeEntry entry[NEG_SETS][NEG_WAYS];} negative;
static u32 negative_active,negative_fallbacks;
static u64 negative_generation,neg_calls,neg_hits,neg_queries,neg_stored,neg_expired,neg_evicted,neg_bypass,neg_contention,neg_file_missing,neg_path_missing,neg_other_errors,neg_success;
static int negative_name(const uint16_t *p,unsigned *len,u32 *hash){
    if(!p||!((p[0]>='A'&&p[0]<='Z')||(p[0]>='a'&&p[0]<='z'))||p[1]!=':'||(p[2]!='/'&&p[2]!='\\'))return 0;
    unsigned n=0;u32 h=2166136261U;while(n<260&&p[n]){h=(h^p[n])*16777619U;n++;}
    if(n==260)return 0;*len=n;*hash=h;return 1;
}
static u32 negative_query(AttrFn original,const uint16_t *path,u32 incoming){
    INC(neg_queries);SetLastError(incoming);u32 r=original(path),error=GetLastError();
    if(r!=0xffffffffU)INC(neg_success);else if(error==2)INC(neg_file_missing);else if(error==3)INC(neg_path_missing);else INC(neg_other_errors);
    SetLastError(error);return r;
}
static __attribute__((noinline)) u32 negative_attrs(const uint16_t *path){
    AttrFn original=(AttrFn)*path_attrs_slot;
    if(!negative_active)return original(path);
    u32 incoming=GetLastError(),hash;unsigned len;INC(neg_calls);
    if(!negative_name(path,&len,&hash)){INC(neg_bypass);return negative_query(original,path,incoming);}
    if(__atomic_exchange_n(&negative.lock,1,__ATOMIC_ACQUIRE)){
        INC(neg_contention);INC(negative_fallbacks);INC(negative_generation);
        u32 r=negative_query(original,path,incoming),error=GetLastError();
        INC(negative_generation);__atomic_fetch_sub(&negative_fallbacks,1,__ATOMIC_ACQ_REL);SetLastError(error);return r;
    }
    u64 gen=READ(negative_generation),now=GetTickCount64();unsigned set=hash&(NEG_SETS-1);NegativeEntry *e=0,*unused=0;
    for(unsigned i=0;i<NEG_WAYS;i++){
        NegativeEntry *p=&negative.entry[set][i];
        if(p->valid&&p->hash==hash&&equal(p->path,path,(len+1)*2)){e=p;break;}
        if(!unused&&(!p->valid||now<p->stamp||now-p->stamp>=NEG_TTL_MS))unused=p;
    }
    if(e&&e->epoch==gen&&e->original==(u64)original&&now>=e->stamp&&now-e->stamp<NEG_TTL_MS&&!READ(negative_fallbacks)&&gen==READ(negative_generation)){
        u32 error=e->error;INC(neg_hits);__atomic_store_n(&negative.lock,0,__ATOMIC_RELEASE);SetLastError(error);return 0xffffffffU;
    }
    if(e&&(now<e->stamp||now-e->stamp>=NEG_TTL_MS))INC(neg_expired);
    if(!e)e=unused;
    if(!e){e=&negative.entry[set][negative.next[set]];negative.next[set]=(negative.next[set]+1)%NEG_WAYS;INC(neg_evicted);}
    e->valid=0;
    u32 r=negative_query(original,path,incoming),error=GetLastError();
    if(r==0xffffffffU&&(error==2||error==3)&&!READ(negative_fallbacks)&&gen==READ(negative_generation)){
        e->hash=hash;e->error=error;e->stamp=now;e->epoch=gen;e->original=(u64)original;copy(e->path,path,(len+1)*2);e->valid=1;INC(neg_stored);
    }
    __atomic_store_n(&negative.lock,0,__ATOMIC_RELEASE);SetLastError(error);return r;
}
