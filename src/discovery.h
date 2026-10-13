/* Panacea GetFileSize discovery. Apache-2.0.
 * Profile: MSVC implementation with the known Axa package layout/prologue.
 * Addresses are decoded, not pinned. RIP displacements and external CALL
 * displacements are normalized; instructions, constants and internal branches
 * must still match. Compiler/ABI rewrites deliberately require a new profile.
 */
#define SIZE_PROFILE_LEN 0x254U
#define SIZE_PROFILE_HASH 0xe980bdd266470970ULL
typedef struct {u8 *base,*sections;u32 size,count;u32 imports,import_size,pdata,pdata_size;} Image;
typedef struct {u8 *size_fn;InfoFn volatile *info;} Discovery;
static int span(u32 off,u32 n,u32 size){return off<size&&n<=size-off;}
static int image_view(u8 *base,Image *v){
    if(!base||*(uint16_t *)base!=0x5a4d)return 0;
    u32 nt=*(u32 *)(base+0x3c);if(nt<64||nt>0x800)return 0;
    u8 *h=base+nt;u32 opt=*(uint16_t *)(h+20),count=*(uint16_t *)(h+6);
    if(*(u32 *)h!=0x4550||*(uint16_t *)(h+4)!=0x8664||opt<240||opt>512||!count||count>96||*(uint16_t *)(h+24)!=0x20b)return 0;
    u32 size=*(u32 *)(h+24+56),headers=*(u32 *)(h+24+60);
    if(size<4096||size>0x10000000||headers>size||nt+24+opt+count*40>headers||*(u32 *)(h+24+108)<4)return 0;
    *v=(Image){base,h+24+opt,size,count,*(u32 *)(h+24+120),*(u32 *)(h+24+124),*(u32 *)(h+24+136),*(u32 *)(h+24+140)};
    for(u32 i=0;i<count;i++){
        u8 *s=v->sections+40*i;u32 start=*(u32 *)(s+12),len=*(u32 *)(s+8);
        if(!len||!span(start,len,size))return 0;
        for(u32 j=0;j<i;j++){u8 *t=v->sections+40*j;u32 a=*(u32 *)(t+12),n=*(u32 *)(t+8);if(start<a+n&&a<start+len)return 0;}
    }
    return 1;
}
static int section_has(const Image *v,u32 rva,u32 n,u32 required,u32 forbidden){
    if(!span(rva,n,v->size))return 0;
    for(u32 i=0;i<v->count;i++){
        const u8 *s=v->sections+40*i;u32 start=*(const u32 *)(s+12),len=*(const u32 *)(s+8),flags=*(const u32 *)(s+36);
        if(rva>=start&&span(rva-start,n,len))return (flags&required)==required&&!(flags&forbidden);
    }return 0;
}
#define READABLE 0x40000000U
#define EXECUTABLE 0x20000000U
#define WRITABLE 0x80000000U
static int string_at(const Image *v,u32 rva,const char *s,int wide,int fold){
    u32 n=0;while(s[n])n++;if(!section_has(v,rva,(n+1)*(wide?2:1),READABLE,EXECUTABLE))return 0;
    for(u32 i=0;i<=n;i++){unsigned c=wide?((uint16_t *)(v->base+rva))[i]:v->base[rva+i],w=(u8)s[i];if(fold){if(c>='A'&&c<='Z')c+=32;if(w>='A'&&w<='Z')w+=32;}if(c!=w)return 0;}return 1;
}
static u32 import_slot(const Image *v,const char *name){
    if(!v->imports||v->import_size<20||!section_has(v,v->imports,v->import_size,READABLE,EXECUTABLE))return 0;
    u32 found=0;
    for(u32 d=0;d+20<=v->import_size;d+=20){
        u32 *q=(u32 *)(v->base+v->imports+d);if(!q[0]&&!q[3]&&!q[4])break;
        if(!string_at(v,q[3],"KERNEL32.dll",0,1)||!q[0]||!q[4])continue;
        for(u32 j=0;j<v->size/8;j++){
            u64 a=(u64)q[0]+8*j,b=(u64)q[4]+8*j;
            if(a>=v->size||b>=v->size||!section_has(v,(u32)a,8,READABLE,EXECUTABLE)||!section_has(v,(u32)b,8,READABLE,EXECUTABLE))return 0;
            u64 thunk=*(u64 *)(v->base+a);if(!thunk)break;if(thunk>>63)continue;
            if(thunk+2>=v->size)return 0;
            if(string_at(v,(u32)thunk+2,name,0,0)){if(found)return 0;found=(u32)b;}
        }
    }return found;
}
static int relative_rva(u32 pc,unsigned len,int32_t displacement,u32 size,u32 *out){
    int64_t r=(int64_t)pc+len+displacement;if(r<0||r>=size)return 0;*out=(u32)r;return 1;
}
static int candidate(const Image *v,const Image *game,const RF *rf,Discovery *out){
    if(rf->end<=rf->begin||rf->end-rf->begin!=SIZE_PROFILE_LEN||!section_has(v,rf->begin,SIZE_PROFILE_LEN,READABLE|EXECUTABLE,WRITABLE))return 0;
    u64 hash=14695981039346656037ULL;u32 pos=0,slot=0,attrs=0,find=0,close=0;unsigned calls=0,info_count=0,formats=0;
    while(pos<SIZE_PROFILE_LEN){
        /* HDE may inspect up to 15 bytes, including beyond the instruction. */
        u8 bytes[32]={0};u32 left=SIZE_PROFILE_LEN-pos;copy(bytes,v->base+rf->begin+pos,left<16?left:16);
        hde64s h;hde64_disasm(bytes,&h);if(!h.len||h.len>left||h.flags&F_ERROR||h.p_67)return 0;
        if((h.flags&F_MODRM)&&h.modrm_mod==0&&h.modrm_rm==5){
            if(h.len<imm_size(&h)+4)return 0;unsigned off=h.len-imm_size(&h)-4;u32 dest;
            if(!relative_rva(rf->begin+pos,h.len,(int32_t)h.disp.disp32,v->size,&dest)||!section_has(v,dest,8,READABLE,EXECUTABLE))return 0;
            /* Identify the package lookup by its complete call setup, not RVA. */
            const u8 setup[]={0x48,0x8b,0xfa,0x48,0x8b,0xcf,0x33,0xd2,0xff,0xd0};
            if(h.len==7&&bytes[0]==0x48&&bytes[1]==0x8b&&bytes[2]==0x05&&left>=7+sizeof(setup)&&equal(v->base+rf->begin+pos+7,setup,sizeof(setup))){slot=dest;info_count++;}
            if(bytes[0]==0xff&&bytes[1]==0x15){if(calls==0)attrs=dest;else if(calls==1)find=dest;else if(calls==2)close=dest;calls++;}
            if(h.opcode==0x8d){
                if(string_at(v,dest,"%ls\\raw\\%hs",1,0))formats|=1;
                if(string_at(v,dest,"GetFileSize(\"%ls\") = %d\n",1,0))formats|=2;
            }
            memset(bytes+off,0,4);
        }
        if(h.opcode==0xe8){
            u32 dest;if(h.len!=5||!relative_rva(rf->begin+pos,5,(int32_t)h.imm.imm32,v->size,&dest)||!section_has(v,dest,1,READABLE|EXECUTABLE,WRITABLE))return 0;
            memset(bytes+1,0,4);
        }
        for(unsigned j=0;j<h.len;j++)hash=(hash^bytes[j])*1099511628211ULL;
        pos+=h.len;
    }
    if(hash!=SIZE_PROFILE_HASH||info_count!=1||calls!=3||formats!=3||!slot||(slot&7)||!section_has(v,slot,8,READABLE|WRITABLE,EXECUTABLE))return 0;
    if(attrs!=import_slot(v,"GetFileAttributesW")||find!=import_slot(v,"FindFirstFileA")||close!=import_slot(v,"CloseHandle"))return 0;
    u64 info=*(u64 *)(v->base+slot);
    if(info<(u64)game->base||info-(u64)game->base>=game->size||!section_has(game,(u32)(info-(u64)game->base),1,READABLE|EXECUTABLE,WRITABLE))return 0;
    /* Validate the unwind layout used by the existing trampoline before any write. */
    if((rf->unwind&3)||!section_has(v,rf->unwind,4,READABLE,EXECUTABLE))return 0;
    const u8 *u=v->base+rf->unwind;u32 n=4+2*((u[2]+1U)&~1U);
    if((u[0]&7)!=1||(u[0]>>3)&4||u[3]||u[1]!=0x56||!section_has(v,rf->unwind,n,READABLE,EXECUTABLE))return 0;
    /* Expected completed prologue: save RSI, push RDI, allocate 0x3a0.
     * Later RBX save is retained in the original function, not the trampoline. */
    const u8 codes[]={0x56,0x34,0x76,0,0x0d,0x64,0x78,0,0x0d,0x01,0x74,0,0x06,0x70};
    if(u[2]!=7||!equal(u+4,codes,sizeof(codes)))return 0;
    out->size_fn=v->base+rf->begin;out->info=(InfoFn volatile *)(v->base+slot);return 1;
}
/* 1 unique compatible match; 0 unsupported/invalid image; -1 ambiguous. */
static __attribute__((noinline)) int discover_panacea(u8 *base,u8 *game_base,Discovery *out){
    Image v,game;out->size_fn=0;out->info=0;
    if(!image_view(base,&v)||!image_view(game_base,&game)||!v.pdata||!v.pdata_size||v.pdata_size%12||(v.pdata&3)||!section_has(&v,v.pdata,v.pdata_size,READABLE,0))return 0;
    unsigned matches=0;Discovery found={0};
    for(u32 i=0;i<v.pdata_size/12;i++){
        RF *rf=(RF *)(base+v.pdata+12*i);Discovery item;
        if(candidate(&v,&game,rf,&item)){found=item;if(++matches>1)return -1;}
    }
    if(matches==1){*out=found;return 1;}return 0;
}
