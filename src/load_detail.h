/* Wall/CPU classification for the two long-call boundaries. No asset caching.
 * CPU units are Windows FILETIME 100ns; they describe this thread only.
 * A low CPU/wall ratio is NOT proof of disk I/O, GPU wait, or idle hardware.
 */
IMP int GetThreadTimes(void *,u64 *,u64 *,u64 *,u64 *);
#define DETAIL_EVENTS 256
typedef struct {
    char name[DIAG_NAME];u64 start,end,kernel,user;u32 phase,thread,cpu_valid;int result;
} DetailEvent;
typedef struct {u64 kernel,user;int valid;} DetailCPU;
static DetailEvent detail_events[DETAIL_EVENTS];
static u32 detail_lock,detail_read,detail_write,detail_count;
static u64 detail_dropped,detail_cpu_errors;
static DetailCPU detail_cpu(void){
    u64 created=0,exited=0;DetailCPU c={0};
    c.valid=GetThreadTimes((void *)(intptr_t)-2,&created,&exited,&c.kernel,&c.user);
    return c;
}
static void detail_record(unsigned phase,const char *name,u64 start,u64 end,DetailCPU before,DetailCPU after,int result){
    if(!diag_active)return;
    int valid=before.valid&&after.valid&&after.kernel>=before.kernel&&after.user>=before.user;
    if(!valid)INC(detail_cpu_errors);
    /* QPC supplies elapsed time; do not treat a failed/backward clock as slow. */
    if(!start||end<start||!diag_freq||end-start<diag_freq/50)return;
    u32 thread=GetCurrentThreadId();
    if(!diag_try(&detail_lock)){INC(detail_dropped);return;}
    if(detail_count==DETAIL_EVENTS){diag_unlock(&detail_lock);INC(detail_dropped);return;}
    DetailEvent *e=&detail_events[detail_write];copy(e->name,name,DIAG_NAME);
    e->start=start;e->end=end;e->phase=phase;e->thread=thread;e->result=result;e->cpu_valid=valid;
    e->kernel=valid?after.kernel-before.kernel:0;e->user=valid?after.user-before.user:0;
    detail_write=(detail_write+1)%DETAIL_EVENTS;detail_count++;diag_unlock(&detail_lock);
}
static int diag_register(const char *name,int size,void *data){
    DIAG_BEGIN;diag_name(key,name,0);DetailCPU cpu_before=detail_cpu();
    DIAG_START;int r=((int (*)(const char *,int,void *))*diag_register_slot)(name,size,data);
    DIAG_END;DetailCPU cpu_after=detail_cpu();
    detail_record(D_REGISTER,key,begin,end,cpu_before,cpu_after,r);
    DIAG_SAVE(0,D_REGISTER,r==-1,0);return r;
}
/* Panacea keeps its Hook object and its original target. Read the target for
 * every call so the normal original function OR the stable trampoline is used.
 * The existing unpatch/call/repatch sequence stays in the caller. This wrapper
 * does not install another game-entry hook or alter the Hook object.
 */
static int diag_original_load(void *self,const char *name,void *data,u8 hd){
    DIAG_BEGIN;
    void **hook=*(void **volatile *)diag_load_hook_slot;
    int (*original)(void *,const char *,void *,u8)=(void *)*hook;
    diag_name(key,name,0);DetailCPU cpu_before=detail_cpu();
    DIAG_START;int r=original(self,name,data,hd);
    DIAG_END;DetailCPU cpu_after=detail_cpu();
    detail_record(D_ORIGINAL_LOAD,key,begin,end,cpu_before,cpu_after,r);
    DIAG_SAVE(0,D_ORIGINAL_LOAD,r<=0,0);return r;
}
