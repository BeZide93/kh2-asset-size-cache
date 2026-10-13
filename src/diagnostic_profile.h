/* Diagnostic-only profile: known Panacea implementation. Hashes include all
 * operands, so relocated/recompiled layouts disable diagnostics, not the cache. */
typedef struct {u32 rva,length;u64 hash;} DiagProfile;
static const DiagProfile diag_profiles[]={
    {0xfeb0,0x505,0x055aaad3ff206c1aULL},
    {0x10e60,0x254,0xd9769dc77bfce38cULL},
    {0xd660,0x121,0x0ee8c074121ff9e0ULL},
    {0xfa50,0x3c4,0x6ffbfddcd8f94ba2ULL},
    {0x268e0,0xb,0x885780c8c16f0e25ULL},
    {0x26814,0xc9,0xaefd244d68fca037ULL},
    {0x2a934,0x1d,0x16c93ef2ec80e19cULL},
    {0x280f8,0x9b,0xe3a8eeac22f71112ULL},
    {0x27ddc,0x98,0x2966a0a1fe302af6ULL},
    {0x26fa0,0x98,0xbd4211926232dec5ULL},
};
static const u32 diag_callees[]={0xd660,0x268e0,0x2a934,0x280f8,0x27ddc,0x26fa0,0xfa50};
typedef struct {u32 rva;u8 kind,wrapper;} DiagSite;
/* kind: E8 call=0, FF15 indirect call=1, MOV RAX,[RIP+slot]=2. */
static const DiagSite diag_sites[]={
    {0xff44,1,0},
    {0x1023f,0,1},
    {0xff84,2,2},
    {0x102e1,2,2},
    {0xffa5,0,3},
    {0x102fd,0,3},
    {0xffc0,0,4},
    {0x10032,0,4},
    {0x10163,0,4},
    {0x101a4,0,4},
    {0x101f6,0,4},
    {0x1033c,0,4},
    {0x10138,0,5},
    {0x10310,0,5},
    {0x10328,0,5},
    {0x10318,0,6},
    {0x101fe,0,7},
    {0x10344,0,7},
    {0x1035c,0,8},
    {0x10210,2,9},
    {0x10361,2,9},
    {0x10ef6,1,10},
    {0x10f7f,0,11},
    {0x10e7f,2,12},
    {0x10f14,0,13},
    {0x11029,0,13},
    {0x10f2f,0,14},
    {0x11041,0,15},
    {0x11049,0,16},
    {0x10f37,0,17},
    {0x11053,0,17},
};
