/* Optional missing-path cache only. No timing, resource-load or I/O diagnosis.
 * Validate the original caller bodies before installing the size detour.
 * Patch only five GetFileAttributesW call operands at startup. Apache-2.0. */
static void **path_attrs_slot;
#include "negative_paths.h"
typedef struct {u32 rva,length;u64 hash;} PathProfile;
static const PathProfile path_profiles[]={
    {0xfeb0,0x505,0x055aaad3ff206c1aULL},
    {0x10e60,0x254,0xd9769dc77bfce38cULL},
    {0xd660,0x121,0x0ee8c074121ff9e0ULL},
};
static const u32 path_sites[]={0xd6ca,0xd70c,0xd765,0xff44,0x10ef6};
typedef struct {u8 *disp;int32_t saved,replacement;u32 old;int writable,written;} PathPatch;
static PathPatch path_patches[5];
static u8 *path_page;
static volatile unsigned path_patch_count;
static u64 path_hash_bytes(const u8 *b,u32 n){u64 h=14695981039346656037ULL;while(n--)h=(h^*b++)*1099511628211ULL;return h;}
static __attribute__((noinline)) int path_profile_ok(u8 *base,u8 *size_fn){
    Image image;if(!image_view(base,&image)||size_fn!=base+0x10e60)return 0;
    for(unsigned i=0;i<3;i++){
        const PathProfile *p=&path_profiles[i];
        if(!section_has(&image,p->rva,p->length,READABLE|EXECUTABLE,WRITABLE)||path_hash_bytes(base+p->rva,p->length)!=p->hash)return 0;
    }
    u32 slot=import_slot(&image,"GetFileAttributesW");
    if(slot!=0x460f0||!section_has(&image,slot,8,READABLE,EXECUTABLE)||!*(void **)(base+slot))return 0;
    return 1;
}
/* At OnInit, before gameplay. Acquire all page protections before any write;
 * restore them in reverse order because call sites can share a page. */
static __attribute__((noinline)) int path_commit(void){
    if(!path_page||path_patch_count!=5)return 0;
    unsigned ready=0;u32 ignored;
    for(;ready<path_patch_count;ready++){
        PathPatch *p=&path_patches[ready];
        if(*(int32_t *)p->disp!=p->saved||!VirtualProtect(p->disp,4,0x40,&p->old))goto rollback;
        p->writable=1;
    }
    for(unsigned i=0;i<path_patch_count;i++){PathPatch *p=&path_patches[i];copy(p->disp,&p->replacement,4);p->written=1;}
    for(unsigned i=0;i<path_patch_count;i++)if(!FlushInstructionCache((void *)(intptr_t)-1,path_patches[i].disp,4))goto rollback;
    for(unsigned i=path_patch_count;i;i--){PathPatch *p=&path_patches[i-1];if(!VirtualProtect(p->disp,4,p->old,&ignored))goto rollback;p->writable=0;}
    negative_active=1;return 1;
rollback:
    /* Retain the relay even if a restore fails. Any surviving call forwards
     * through the live original import with negative_active still false. */
    for(unsigned i=0;i<ready;i++){
        PathPatch *p=&path_patches[i];u32 temp;
        if(p->written&&VirtualProtect(p->disp,4,0x40,&temp)){
            if(*(int32_t *)p->disp==p->replacement){copy(p->disp,&p->saved,4);FlushInstructionCache((void *)(intptr_t)-1,p->disp,4);}
        }
    }
    for(unsigned i=ready;i;i--){PathPatch *p=&path_patches[i-1];VirtualProtect(p->disp,4,p->old,&ignored);}
    return 0;
}
/* Called only after path_profile_ok succeeded and the size hook installed.
 * No allocation is left behind if the independent size hook fails. */
static __attribute__((noinline)) int path_install(u8 *base){
    for(unsigned i=0;i<5;i++){
        u8 *pc=base+path_sites[i];
        if(pc[0]!=0xff||pc[1]!=0x15||(int64_t)path_sites[i]+6+*(int32_t *)(pc+2)!=0x460f0)return 0;
    }
    path_attrs_slot=(void **)(base+0x460f0);
    u64 origin=(u64)base&~65535ULL;
    for(u64 d=65536;d<0x40000000&&!path_page;d+=65536){
        if(origin>d+65536)path_page=VirtualAlloc((void *)(origin-d),4096,0x3000,4);
        if(!path_page&&origin+d<0x00007fff00000000ULL)path_page=VirtualAlloc((void *)(origin+d),4096,0x3000,4);
    }
    if(!path_page)return 0;
    absjump(path_page,negative_attrs);
    path_patch_count=5;
    for(unsigned i=0;i<5;i++){
        u8 *pc=base+path_sites[i];int64_t rel=(int64_t)(uintptr_t)(path_page+6)-(int64_t)(uintptr_t)(pc+6);
        if(!fits32(rel))goto failed;
        path_patches[i]=(PathPatch){pc+2,*(int32_t *)(pc+2),(int32_t)rel,0,0,0};
    }
    u32 old;if(!VirtualProtect(path_page,4096,0x20,&old)||!FlushInstructionCache((void *)(intptr_t)-1,path_page,14))goto failed;
    return path_commit()?1:-1;
failed:VirtualFree(path_page,0,0x8000);path_page=0;path_patch_count=0;return 0;
}
