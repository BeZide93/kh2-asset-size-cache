/* d4: bounded import-table instrumentation. No game code addresses.
 * Only the game and Panacea normal named imports are covered. The add-on's
 * own imports are untouched, preventing logger/API recursion.
 */
#define IO_APIS 13
#define IO_SLOTS 64
#define IO_EVENTS 1024
typedef struct {void **slot;void *original,*replacement;u32 module,api,rva,old;} IoPatch;
typedef struct {u64 count,ticks,max,bytes;} IoSum;
typedef struct {u64 start,end,handle,requested,bytes;u32 module,api,thread,result,error,timeout;} IoEvent;
static IoPatch io_patches[IO_SLOTS];
static void *volatile io_original[2][IO_APIS];
static u32 io_count,io_enabled,io_windows,io_lock,io_read,io_write,io_queued;
static u64 io_dropped,io_clock_errors;
static IoSum io_sums[2][IO_APIS];
static IoEvent io_events[IO_EVENTS];
static void io_window_begin(void){INC(io_windows);}
static void io_window_end(void){__atomic_fetch_sub(&io_windows,1,__ATOMIC_ACQ_REL);}
static void io_record(u32 module,u32 api,u64 start,u64 end,const void *handle,u64 requested,u64 bytes,u32 result,u32 error,u32 timeout){
    if(!start||end<start){INC(io_clock_errors);return;}
    u32 thread=GetCurrentThreadId();
    if(!diag_try(&io_lock)){INC(io_dropped);return;}
    IoSum *s=&io_sums[module][api];s->count++;s->ticks+=end-start;s->bytes+=bytes;if(end-start>s->max)s->max=end-start;
    /* Record every read/completion query, and other calls lasting >=1ms. */
    if(api==0||api==2||api==3||end-start>=diag_freq/1000){
        if(io_queued==IO_EVENTS)INC(io_dropped);
        else {io_events[io_write]=(IoEvent){start,end,(u64)handle,requested,bytes,module,api,thread,result,error,timeout};io_write=(io_write+1)%IO_EVENTS;io_queued++;}
    }
    diag_unlock(&io_lock);
}
static int io_0_ReadFile(void *h,void *buf,u32 requested,u32 *done,void *ov){
    int (*fn)(void *h,void *buf,u32 requested,u32 *done,void *ov)=(void *)io_original[0][0];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,buf,requested,done,ov);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,buf,requested,done,ov);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,0,start,end,h,requested,r&&!ov&&done?*done:0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_0_SetFilePointerEx(void *h,int64_t distance,int64_t *pos,u32 method){
    int (*fn)(void *h,int64_t distance,int64_t *pos,u32 method)=(void *)io_original[0][1];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,distance,pos,method);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,distance,pos,method);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,1,start,end,h,method,0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_0_GetOverlappedResult(void *h,void *ov,u32 *done,int wait){
    int (*fn)(void *h,void *ov,u32 *done,int wait)=(void *)io_original[0][2];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,ov,done,wait);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,ov,done,wait);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,2,start,end,h,wait,r&&done?*done:0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_0_GetOverlappedResultEx(void *h,void *ov,u32 *done,u32 timeout,int alertable){
    int (*fn)(void *h,void *ov,u32 *done,u32 timeout,int alertable)=(void *)io_original[0][3];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,ov,done,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,ov,done,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,3,start,end,h,alertable,r&&done?*done:0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_0_WaitForSingleObject(void *h,u32 timeout){
    u32 (*fn)(void *h,u32 timeout)=(void *)io_original[0][4];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,timeout);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(h,timeout);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,4,start,end,h,0,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_0_WaitForSingleObjectEx(void *h,u32 timeout,int alertable){
    u32 (*fn)(void *h,u32 timeout,int alertable)=(void *)io_original[0][5];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(h,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,5,start,end,h,alertable,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_0_WaitForMultipleObjects(u32 count,void *const *handles,int all,u32 timeout){
    u32 (*fn)(u32 count,void *const *handles,int all,u32 timeout)=(void *)io_original[0][6];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(count,handles,all,timeout);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(count,handles,all,timeout);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,6,start,end,handles,count,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_0_WaitForMultipleObjectsEx(u32 count,void *const *handles,int all,u32 timeout,int alertable){
    u32 (*fn)(u32 count,void *const *handles,int all,u32 timeout,int alertable)=(void *)io_original[0][7];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(count,handles,all,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(count,handles,all,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,7,start,end,handles,count,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static void io_0_Sleep(u32 timeout){
    void (*fn)(u32 timeout)=(void *)io_original[0][8];
    if(!READ(io_enabled)||!READ(io_windows)){fn(timeout);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(timeout);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(0,8,start,end,0,0,0,(u32)r,error,timeout);
    SetLastError(error);
}
static u32 io_0_SleepEx(u32 timeout,int alertable){
    u32 (*fn)(u32 timeout,int alertable)=(void *)io_original[0][9];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(0,9,start,end,0,alertable,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static void io_0_EnterCriticalSection(void *lock){
    void (*fn)(void *lock)=(void *)io_original[0][10];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(0,10,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static void io_0_AcquireSRWLockExclusive(void *lock){
    void (*fn)(void *lock)=(void *)io_original[0][11];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(0,11,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static void io_0_AcquireSRWLockShared(void *lock){
    void (*fn)(void *lock)=(void *)io_original[0][12];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(0,12,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static int io_1_ReadFile(void *h,void *buf,u32 requested,u32 *done,void *ov){
    int (*fn)(void *h,void *buf,u32 requested,u32 *done,void *ov)=(void *)io_original[1][0];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,buf,requested,done,ov);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,buf,requested,done,ov);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,0,start,end,h,requested,r&&!ov&&done?*done:0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_1_SetFilePointerEx(void *h,int64_t distance,int64_t *pos,u32 method){
    int (*fn)(void *h,int64_t distance,int64_t *pos,u32 method)=(void *)io_original[1][1];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,distance,pos,method);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,distance,pos,method);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,1,start,end,h,method,0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_1_GetOverlappedResult(void *h,void *ov,u32 *done,int wait){
    int (*fn)(void *h,void *ov,u32 *done,int wait)=(void *)io_original[1][2];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,ov,done,wait);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,ov,done,wait);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,2,start,end,h,wait,r&&done?*done:0,(u32)r,error,0);
    SetLastError(error);return r;
}
static int io_1_GetOverlappedResultEx(void *h,void *ov,u32 *done,u32 timeout,int alertable){
    int (*fn)(void *h,void *ov,u32 *done,u32 timeout,int alertable)=(void *)io_original[1][3];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,ov,done,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    int r=fn(h,ov,done,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,3,start,end,h,alertable,r&&done?*done:0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_1_WaitForSingleObject(void *h,u32 timeout){
    u32 (*fn)(void *h,u32 timeout)=(void *)io_original[1][4];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,timeout);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(h,timeout);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,4,start,end,h,0,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_1_WaitForSingleObjectEx(void *h,u32 timeout,int alertable){
    u32 (*fn)(void *h,u32 timeout,int alertable)=(void *)io_original[1][5];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(h,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(h,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,5,start,end,h,alertable,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_1_WaitForMultipleObjects(u32 count,void *const *handles,int all,u32 timeout){
    u32 (*fn)(u32 count,void *const *handles,int all,u32 timeout)=(void *)io_original[1][6];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(count,handles,all,timeout);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(count,handles,all,timeout);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,6,start,end,handles,count,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static u32 io_1_WaitForMultipleObjectsEx(u32 count,void *const *handles,int all,u32 timeout,int alertable){
    u32 (*fn)(u32 count,void *const *handles,int all,u32 timeout,int alertable)=(void *)io_original[1][7];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(count,handles,all,timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(count,handles,all,timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,7,start,end,handles,count,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static void io_1_Sleep(u32 timeout){
    void (*fn)(u32 timeout)=(void *)io_original[1][8];
    if(!READ(io_enabled)||!READ(io_windows)){fn(timeout);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(timeout);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(1,8,start,end,0,0,0,(u32)r,error,timeout);
    SetLastError(error);
}
static u32 io_1_SleepEx(u32 timeout,int alertable){
    u32 (*fn)(u32 timeout,int alertable)=(void *)io_original[1][9];
    if(!READ(io_enabled)||!READ(io_windows)){return fn(timeout,alertable);}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    u32 r=fn(timeout,alertable);u32 error=GetLastError();u64 end=diag_now();
    io_record(1,9,start,end,0,alertable,0,(u32)r,error,timeout);
    SetLastError(error);return r;
}
static void io_1_EnterCriticalSection(void *lock){
    void (*fn)(void *lock)=(void *)io_original[1][10];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(1,10,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static void io_1_AcquireSRWLockExclusive(void *lock){
    void (*fn)(void *lock)=(void *)io_original[1][11];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(1,11,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static void io_1_AcquireSRWLockShared(void *lock){
    void (*fn)(void *lock)=(void *)io_original[1][12];
    if(!READ(io_enabled)||!READ(io_windows)){fn(lock);return;}
    u32 incoming=GetLastError();u64 start=diag_now();SetLastError(incoming);
    fn(lock);u32 r=0;u32 error=GetLastError();u64 end=diag_now();
    io_record(1,12,start,end,lock,0,0,(u32)r,error,0);
    SetLastError(error);
}
static const char *io_name(u32 api){switch(api){
case 0:return "ReadFile";
case 1:return "SetFilePointerEx";
case 2:return "GetOverlappedResult";
case 3:return "GetOverlappedResultEx";
case 4:return "WaitForSingleObject";
case 5:return "WaitForSingleObjectEx";
case 6:return "WaitForMultipleObjects";
case 7:return "WaitForMultipleObjectsEx";
case 8:return "Sleep";
case 9:return "SleepEx";
case 10:return "EnterCriticalSection";
case 11:return "AcquireSRWLockExclusive";
case 12:return "AcquireSRWLockShared";
default:return "unknown";}}
static void *io_wrapper(u32 module,u32 api){switch(module*IO_APIS+api){
case 0:return io_0_ReadFile;
case 1:return io_0_SetFilePointerEx;
case 2:return io_0_GetOverlappedResult;
case 3:return io_0_GetOverlappedResultEx;
case 4:return io_0_WaitForSingleObject;
case 5:return io_0_WaitForSingleObjectEx;
case 6:return io_0_WaitForMultipleObjects;
case 7:return io_0_WaitForMultipleObjectsEx;
case 8:return io_0_Sleep;
case 9:return io_0_SleepEx;
case 10:return io_0_EnterCriticalSection;
case 11:return io_0_AcquireSRWLockExclusive;
case 12:return io_0_AcquireSRWLockShared;
case 13:return io_1_ReadFile;
case 14:return io_1_SetFilePointerEx;
case 15:return io_1_GetOverlappedResult;
case 16:return io_1_GetOverlappedResultEx;
case 17:return io_1_WaitForSingleObject;
case 18:return io_1_WaitForSingleObjectEx;
case 19:return io_1_WaitForMultipleObjects;
case 20:return io_1_WaitForMultipleObjectsEx;
case 21:return io_1_Sleep;
case 22:return io_1_SleepEx;
case 23:return io_1_EnterCriticalSection;
case 24:return io_1_AcquireSRWLockExclusive;
case 25:return io_1_AcquireSRWLockShared;
default:return 0;}}
static int io_module_name(const Image *v,u32 rva){
    if(string_at(v,rva,"KERNEL32.dll",0,1)||string_at(v,rva,"KERNELBASE.dll",0,1))return 1;
    const char *prefix="api-ms-win-core-";
    for(u32 i=0;prefix[i];i++){if(!section_has(v,rva+i,1,READABLE,EXECUTABLE))return 0;u8 c=v->base[rva+i];if(c>='A'&&c<='Z')c+=32;if(c!=prefix[i])return 0;}
    for(u32 i=16;i<128;i++){if(!section_has(v,rva+i,1,READABLE,EXECUTABLE))return 0;if(!v->base[rva+i])return i>=4&&string_at(v,rva+i-4,".dll",0,1);}
    return 0;
}
/* Collect without writing. Reject malformed, overflowing or ambiguous tables.
 * No attempt is made to infer names from resolved addresses or delay imports. */
static __attribute__((noinline)) int io_collect(u8 *base,u32 module){
    Image v;if(module>1||!image_view(base,&v))return 0;
    if(!v.imports&&!v.import_size)return 1;
    if(v.import_size<20||!section_has(&v,v.imports,v.import_size,READABLE,EXECUTABLE))return 0;
    for(u32 d=0;d+20<=v.import_size;d+=20){
        u32 *q=(u32 *)(base+v.imports+d);
        if(!q[0]&&!q[3]&&!q[4])return 1;
        if(!q[0]||!q[4]||!io_module_name(&v,q[3]))continue;
        int terminated=0;
        for(u32 j=0;j<8192;j++){
            u64 a=(u64)q[0]+8ULL*j,b=(u64)q[4]+8ULL*j;
            if(a>=v.size||b>=v.size||(b&7)||!section_has(&v,(u32)a,8,READABLE,EXECUTABLE)||!section_has(&v,(u32)b,8,READABLE,EXECUTABLE))return 0;
            u64 name=*(u64 *)(base+a);if(!name){terminated=1;break;}if(name>>63)continue;
            if(name>=v.size||name+2>=v.size)return 0;
            for(u32 api=0;api<IO_APIS;api++)if(string_at(&v,(u32)name+2,io_name(api),0,0)){
                if(io_count==IO_SLOTS)return 0;
                void **slot=(void **)(base+b);void *original=__atomic_load_n(slot,__ATOMIC_ACQUIRE);
                if(!original||original==io_wrapper(module,api))return 0;
                for(u32 k=0;k<io_count;k++)if(io_patches[k].slot==slot)return 0;
                if(io_original[module][api]&&io_original[module][api]!=original)return 0;
                io_original[module][api]=original;
                io_patches[io_count++]=(IoPatch){slot,original,io_wrapper(module,api),module,api,(u32)b,0};
                break;
            }
        }
        if(!terminated)return 0;
    }
    return 0;
}
static __attribute__((noinline)) int io_commit(void){
    u32 ready=0,ignored;
    for(;ready<io_count;ready++)if(!VirtualProtect(io_patches[ready].slot,8,4,&io_patches[ready].old))goto rollback;
    for(u32 i=0;i<io_count;i++){
        IoPatch *p=&io_patches[i];void *expected=p->original;
        if(!__atomic_compare_exchange_n(p->slot,&expected,p->replacement,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))goto rollback;
    }
    for(u32 i=ready;i;i--){IoPatch *p=&io_patches[i-1];if(!VirtualProtect(p->slot,8,p->old,&ignored))goto rollback;}
    __atomic_store_n(&io_enabled,1,__ATOMIC_RELEASE);return 1;
rollback:
    for(u32 i=0;i<ready;i++){IoPatch *p=&io_patches[i];u32 old;
        if(VirtualProtect(p->slot,8,4,&old)){void *expected=p->replacement;__atomic_compare_exchange_n(p->slot,&expected,p->original,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);}
    }
    for(u32 i=ready;i;i--){IoPatch *p=&io_patches[i-1];VirtualProtect(p->slot,8,p->old,&ignored);}
    /* A surviving wrapper remains a valid pass-through; never free originals. */
    return 0;
}
static void io_install(u8 *game,u8 *panacea){
    if(!io_collect(game,0)||!io_collect(panacea,1)||!io_count||!io_commit()){
        log_line("IO DISABLED: unsupported/ambiguous imports or IAT transaction failure; d3 diagnostics and caches retained");return;
    }
    for(u32 i=0;i<io_count;i++){
        IoPatch *p=&io_patches[i];char b[200];u32 n=text(b,0,"IOHOOK module=");n=text(b,n,p->module?"panacea":"game");
        n=text(b,n," api=");n=text(b,n,io_name(p->api));n=text(b,n," slot_rva=");n=number(b,n,p->rva);b[n]=0;log_line(b);
    }
    log_line("IO INSTALLED: named game/Panacea imports; capture during original_load/register windows; read/completion events and waits >=1 ms");
}
