/* Relocation/unwind helpers from the tested stable Panacea hook retrofit. */
static int fits32(int64_t v) {return v>=INT32_MIN && v<=INT32_MAX;}
static unsigned imm_size(hde64s *h) {
    if(h->flags&F_IMM64)return 8;if(h->flags&F_IMM32)return 4;
    if(h->flags&F_IMM16)return 2;if(h->flags&F_IMM8)return 1;return 0;
}
static void absjump(u8 *p,const void *to) {p[0]=0xff;p[1]=0x25;memset(p+2,0,4);*(u64 *)(p+6)=(u64)to;}

/* Each relocated instruction retains its length except short Jcc, which is
 * expanded to 0F 8x rel32. Internal branch targets are remapped in pass 2. */
typedef struct {hde64s h;u8 old,newpos,nlen;} Insn;
static unsigned map_offset(Insn *a,unsigned count,unsigned old,unsigned copied,unsigned emitted) {
    if(old==copied)return emitted;
    for(unsigned i=0;i<count;i++)if(a[i].old==old)return a[i].newpos;
    return 0xFFFFFFFF;
}
/* Clone only completed original unwind operations. SAVE offsets refer to the
 * final RSP; removing later PUSH/ALLOC operations requires subtracting their
 * total stack delta. This also handles spills before the stack allocation. */
static int make_unwind(u8 *out,u8 *target,Insn *a,unsigned count,unsigned copied,unsigned emitted) {
    u64 base=0;RF *r=RtlLookupFunctionEntry((u64)target,&base,0);
    memset(out,0,512);out[0]=1;out[1]=(u8)emitted;
    if(!r) {
        for(unsigned i=0;i<count;i++) {
            hde64s *h=&a[i].h;
            if((h->opcode>=0x50&&h->opcode<=0x57)||
               ((h->opcode==0x81||h->opcode==0x83)&&h->modrm==0xec))return 0;
        }
        return 1;
    }
    if(base+r->begin!=(u64)target)return 0;
    const u8 *u=(const u8 *)(base+r->unwind);
    if((u[0]&7)!=1 || (u[0]>>3)&4 || u[3])return 0; /* no chained/frame-register functions */
    unsigned cnt=u[2],omitted=0,kept=0;
    for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<cnt;) {
        const u8 *c=u+4+2*i;unsigned off=c[0],op=c[1]&15,info=c[1]>>4,slots=1,delta=0;
        if(op==0)delta=8;
        else if(op==1) {if(info>1)return 0;slots=info?3:2;}
        else if(op==2)delta=info*8+8;
        else if(op==4||op==8)slots=2;
        else if(op==5||op==9)slots=3;
        else return 0;
        if(i+slots>cnt)return 0;
        if(op==1)delta=info?*(const u32 *)(c+2):*(const uint16_t *)(c+2)*8;
        if(!pass) {if(off>copied)omitted+=delta;}
        else if(off<=copied) {
            unsigned neo=map_offset(a,count,off,copied,emitted);
            if(neo==0xFFFFFFFF || kept+slots>250)return 0;
            u8 *d=out+4+2*kept;copy(d,c,2*slots);d[0]=(u8)neo;
            if(op==4||op==5||op==8||op==9) {
                unsigned scale=(op==8)?16:8;
                u64 save=(slots==2)?(u64)*(const uint16_t *)(c+2)*scale:*(const u32 *)(c+2);
                if(save<omitted)return 0;save-=omitted;
                if(slots==2) {if(save%scale || save/scale>65535)return 0;*(uint16_t *)(d+2)=(uint16_t)(save/scale);}
                else *(u32 *)(d+2)=(u32)save;
            }
            kept+=slots;
        }
        i+=slots;
    }
    out[2]=(u8)kept;return 1;
}
