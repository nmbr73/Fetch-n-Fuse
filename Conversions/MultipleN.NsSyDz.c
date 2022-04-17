// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


//  ----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//  ----    ----    ----    ----    ----    ----    ----    ----

#define txdata (iChannel0)
#define PI 3.14159265359f
#define LN 2.71828182846f

//const uint MAX_RADIUS = 12u;
#define MAX_RADIUS 12

__DEVICE__ uint ub[64] =  {  4086443839u, 803560891u, 3521573439u, 155586747u, 
                  2355529581u, 3804082561u, 1278181521u, 1198599219u, 
                  790900093u, 2043079403u, 72510135u, 329989440u, 
                  1205735441u, 1165390975u, 1863025477u, 1552315409u, 
                  1460749058u, 1961704519u, 1063988442u, 304586942u, 
                  1969173229u, 623707175u, 2719649363u, 533620173u, 
                  734380903u, 1866742626u, 69847740u, 779938642u, 
                  3928012151u, 1597352029u, 1939485308u, 1599391651u, 
                  824038858u, 498247516u, 609160531u, 634145519u, 
                  2022645937u, 4285315796u, 87513080u, 246410766u, 
                  1160189374u, 688725303u, 1266836767u, 670912482u, 
                  2162941226u, 1742659144u, 481786434u, 3618106514u, 
                  0u, 0u, 0u, 0u, 
                  0u, 0u, 0u, 0u, 
                  0u, 0u, 0u, 0u, 
                  0u, 1067687164u, 1122344419u, 0u };

__DEVICE__ uint u32_upk(uint u32, uint bts, uint off) { return (u32 >> off) & ((1u << bts)-1u); }

__DEVICE__ float lmap(float2 fragCoord, float2 R) { return (fragCoord.x / R.x); }
__DEVICE__ float vmap(float2 fragCoord, float2 R) { return (fragCoord.y / R.y); }
__DEVICE__ float cmap(float2 fragCoord, float2 R) { return _sqrtf  ( ((fragCoord.x - R.x*0.5f) / R.x*0.5f)
                                                                   * ((fragCoord.x - R.x*0.5f) / R.x*0.5f)
                                                                   + ((fragCoord.y - R.y*0.5f) / R.y*0.5f)
                                                                   * ((fragCoord.y - R.y*0.5f) / R.y*0.5f) ); }
                                                                                            
                                         
union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };                                         
                                         
__DEVICE__ float vwm(float2 fragCoord, float2 R) 
{
  
  Zahl z;
  
  //float   scale_raw = uintBitsToFloat(ub[62]);
  z._Uint = ub[62];
  float   scale_raw = z._Float;
  
  //float   zoom      = uintBitsToFloat(ub[61]);
  z._Uint = ub[61];
  float   zoom = z._Float;

  float  scale_new  = scale_raw;
  uint   mode       = u32_upk(ub[59], 2u, 0u);
  if( mode == 1u ) { //  Linear Parameter Map
    scale_new = ((lmap(fragCoord,R) + zoom) * (scale_raw / (1.0f + zoom * 2.0f))) * 2.0f; }
  if( mode == 2u ) { //  Circular Parameter Map
    scale_new = ((_sqrtf(cmap(fragCoord,R)) + zoom) * (scale_raw / (1.0f + zoom * 2.0f))) * 2.0f; }
  return scale_new; 
}
    
__DEVICE__ float  tp(uint n, float s)       { return ((float)(n+1u)/256.0f) * ((s*0.5f)/128.0f); }
__DEVICE__ float utp(uint v, uint  w, uint o, float2 fragCoord, float2 R)   { return tp(u32_upk(v,w,w*o), vwm(fragCoord,R)); }
__DEVICE__ float bsn(uint v, uint  o)       { return (float)(u32_upk(v,1u,o)*2u)-1.0f; }
    
__DEVICE__ float4 sigm(float4  x, float w) { return 1.0f / ( 1.0f + exp_f4( (-w*2.0f * x * (PI/2.0f)) + w * (PI/2.0f) ) ); }
__DEVICE__ float hmp2(float x, float w) { return 3.0f*((x-0.5f)*(x-0.5f))+0.25f; }

__DEVICE__ float4  gdv( int2 of, __TEXTURE2D__ tx, float2 fragCoord, float2 R ) 
{
  of     = to_int2_cfloat(fragCoord) + of;
  of.x   = (of.x + (int)R.x) % (int)(R.x);
  of.y   = (of.y + (int)R.y) % (int)(R.y);
  return   texture( tx, (make_float2(of)+0.5)/R); 
}
    
__DEVICE__ float4 nbhd( float2 r, __TEXTURE2D__ tx, float2 fragCoord, float2 R ) 
{
//  Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
  uint  chk = 2147483648u /
      (  (   uint( r.x*r.x*PI + r.x*PI + PI  )
           - uint( r.y*r.y*PI + r.y*PI    ) ) * 128u );
float vvvvvvvvvvvvvvvvvvvv;             
  float  psn = (chk >= 65536u) ? 65536.0f : (float)(chk);
  float4  a = to_float4(0.0f,0.0f,0.0f,0.0f);
  float   w = 1.0f;  // Weighting, unused
  if(r.x == 0.0f) { return ( gdv( to_int2(0,0), tx, fragCoord, R )*w*psn ); }
  else       {
    for(float i = 0.0f; i <= r.x; i+=1.0f) {
      for(float j = 1.0f; j <= r.x; j+=1.0f) {
        float  d = round(_sqrtf(i*i+j*j));
        w = 1.0f;  //  Per-Neighbor Weighting, unused
        if( d <= r.x && d > r.y ) {
          float4 t0  = gdv( to_int2( i, j), tx, fragCoord, R ) * w * psn; a += t0 - fract_f4(t0);
          float4 t1  = gdv( to_int2( j,-i), tx, fragCoord, R ) * w * psn; a += t1 - fract_f4(t1);
          float4 t2  = gdv( to_int2(-i,-j), tx, fragCoord, R ) * w * psn; a += t2 - fract_f4(t2);
          float4 t3  = gdv( to_int2(-j, i), tx, fragCoord, R ) * w * psn; a += t3 - fract_f4(t3); } } }
    return a; } 
}

__DEVICE__ float4 totl( float2 r, __TEXTURE2D__ tx ) 
{
//  Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
  uint  chk = 2147483648u /
      (  (   uint( r.x*r.x*PI + r.x*PI + PI  )
           - uint( r.y*r.y*PI + r.y*PI    ) ) * 128u );
  float  psn = (chk >= 65536u) ? 65536.0f : (float)(chk);
  float4  b = to_float4(0.0f,0.0f,0.0f,0.0f);
  float  w = 1.0f;  // Weighting, unused
  if(r.x == 0.0f) { return to_float4( psn * w, psn * w, psn * w, psn * w ); }
  else       {
    for(float i = 0.0f; i <= r.x; i+=1.0f) {
      for(float j = 1.0f; j <= r.x; j+=1.0f) {
        float  d = round(_sqrtf(i*i+j*j));
            w = 1.0f;  //  Per-Neighbor Weighting, unused
        if( d <= r.x && d > r.y ) { b += psn * w * 4.0f; } } }
    return b; } 
}

__DEVICE__ float4 bitring( float4 rings_a[MAX_RADIUS], float4 rings_b[MAX_RADIUS], uint bits, uint of) {
  float4 sum = to_float4(0.0f,0.0f,0.0f,0.0f);
  float4 tot = to_float4(0.0f,0.0f,0.0f,0.0f);
  for(uint i = 0u; i < MAX_RADIUS; i++) {
    if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings_a[i]; tot += rings_b[i]; } }
  return sigm( (sum / tot), LN ); } // TODO
    
__DEVICE__ float4 conv1(float2 r, __TEXTURE2D__ tx, float2 fragCoord, float2 R) {
  float4 nha = nbhd(r, tx, fragCoord, R);
  float4 nhb = totl(r, tx);
  return   nha / nhb; }
    
//  Used to reseed the surface with lumpy noise
__DEVICE__ float get_xc(float x, float y, float xmod) {
  float sq = _sqrtf(mod_f(x*y+y, xmod)) / _sqrtf(xmod);
  float xc = mod_f((x*x)+(y*y), xmod) / xmod;
  return clamp((sq+xc)*0.5f, 0.0f, 1.0f); }
  
__DEVICE__ float shuffle(float x, float y, float xmod, float val) {
  val = val * mod_f( x*y + x, xmod );
  return (val-_floor(val)); }
  
__DEVICE__ float get_xcn(float x, float y, float xm0, float xm1, float ox, float oy) {
  float  xc = get_xc(x+ox, y+oy, xm0);
  return shuffle(x+ox, y+oy, xm1, xc); }

__DEVICE__ float get_lump(float x, float y, float nhsz, float xm0, float xm1) {
  float   nhsz_c   = 0.0f;
  float   xcn   = 0.0f;
  float   nh_val   = 0.0f;
  for(float i = -nhsz; i <= nhsz; i += 1.0f) {
    for(float j = -nhsz; j <= nhsz; j += 1.0f) {
      nh_val = round(_sqrtf(i*i+j*j));
      if(nh_val <= nhsz) {
        xcn = xcn + get_xcn(x, y, xm0, xm1, i, j);
        nhsz_c = nhsz_c + 1.0f; } } }
  float   xcnf   = ( xcn / nhsz_c );
  float   xcaf  = xcnf;
  for(float i = 0.0f; i <= nhsz; i += 1.0f) {
      xcaf   = clamp((xcnf*xcaf + xcnf*xcaf) * (xcnf+xcnf), 0.0f, 1.0f); }
  return xcaf; }

__DEVICE__ float reseed(uint seed, float scl, float amp, float2 fragCoord) {
  float   fx = fragCoord.x;
  float   fy = fragCoord.y;
  float   r0 = get_lump(fx, fy, round( 6.0f  * scl), 19.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),17.0f), 23.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),43.0f));
  float   r1 = get_lump(fx, fy, round( 22.0f * scl), 13.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),29.0f), 17.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),31.0f));
  float   r2 = get_lump(fx, fy, round( 14.0f * scl), 13.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),11.0f), 51.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),37.0f));
  float   r3 = get_lump(fx, fy, round( 18.0f * scl), 29.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed), 7.0f), 61.0f + mod_f((float)(u32_upk(ub[63], 24u, 0u)+seed),28.0f));
  return clamp( _sqrtf((r0+r1)*r3*(amp+1.2f))-r2*(amp*1.8f+0.2f) , 0.0f, 1.0f); }

struct ConvData {
  float4   value;
  float   total;
};


__DEVICE__ ConvData ring( float r, float2 fragCoord, float2 R, __TEXTURE2D__ txdata ) {

  const float psn = 32768.0f;

  float tot = 0.0f;
  float4  val = to_float4(0.0f,0.0f,0.0f,0.0f);

  float sq2  = _sqrtf(2.0f);

  float o_0 = r + 0.5f;
  float o_1 = sq2 * o_0;
  float o_2 = o_1 / 2.0f;
  float o_3 = _sqrtf( o_0*o_0 - r*r );
  float o_4 = o_2 - ( _floor(o_2) + 0.5f );
  float o_5 = _floor( o_2 ) + _floor( o_4 );

  float i_0 = r - 0.5f;
  float i_1 = sq2 * i_0;
  float i_2 = i_1 / 2.0f;
  float i_3 = _sqrtf( i_0*i_0 - r*r );
  float i_4 = i_2 - ( _floor(i_2) + 1.0f );
  float i_5 = _floor( i_2 ) + _floor( i_4 );

  float d_0 = ( i_5 ) + 1.0f - ( o_5 );

  for(float i = 1.0f; i < _floor( i_2 ) + 1.0f - d_0; i++) {

    float j_0 = _sqrtf( o_0*o_0 - (i+0.0f)*(i+0.0f) );
    float j_1 = _sqrtf( i_0*i_0 - (i+0.0f)*(i+0.0f) );
    float j_2 = ( 1.0f - _fabs( sign_f ( (_floor( i_2 ) + 1.0f) - i ) ) );

    for(float j = _floor( j_1 ) + j_2; j < _floor( j_0 ); j+=1.0f) {
      val += _floor(gdv(to_int2( i, ((int)(j)+1)), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2( i,-((int)(j)+1)), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2(-i,-((int)(j)+1)), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2(-i, ((int)(j)+1)), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2( ((int)(j)+1), i), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2( ((int)(j)+1),-i), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2(-((int)(j)+1),-i), txdata, fragCoord, R) * psn);
      val += _floor(gdv(to_int2(-((int)(j)+1), i), txdata, fragCoord, R) * psn);
      tot += 8.0f * psn; } }

//  Orthagonal
float mmmmmmmmmmmmmmmmmmmmmmmmmmmm;
  val += _floor(gdv(make_int2( r, 0), txdata, fragCoord, R) * psn);
  val += _floor(gdv(to_int2( 0,-r), txdata, fragCoord, R) * psn);
  val += _floor(gdv(to_int2(-r,-0), txdata, fragCoord, R) * psn);
  val += _floor(gdv(to_int2(-0, r), txdata, fragCoord, R) * psn);
  tot += 4.0f * psn;
float aaaaaaaaaaaaaaaaaaaaaaaa;
//  Diagonal
//  TODO This is not quite perfect
  float k_0 = r;
  float k_1 = sq2 * k_0;
  float k_2 = k_1 / 2.0f;
  float k_3 = _sqrtf( k_0*k_0 - r*r );
  float k_4 = k_2 - ( _floor(k_2) + 1.0f );
  float k_5 = _floor( k_2 ) + _floor( k_4 );

  float dist = round(k_2);

  if( sign_f( o_4 ) == -1.0f ) {
  //  val += gdv(to_int2( (_floor(o_5)+1), _floor(o_5)+1), txdata);
    val += _floor(gdv(to_int2( (_floor(o_5)+1.0f), (_floor(o_5)+1.0f)), txdata, fragCoord, R) * psn);
    val += _floor(gdv(to_int2( (_floor(o_5)+1.0f),-(_floor(o_5)+1.0f)), txdata, fragCoord, R) * psn);
    val += _floor(gdv(to_int2(-(_floor(o_5)+1.0f),-(_floor(o_5)+1.0f)), txdata, fragCoord, R) * psn);
    val += _floor(gdv(to_int2(-(_floor(o_5)+1.0f), (_floor(o_5)+1.0f)), txdata, fragCoord, R) * psn);
    tot += 4.0f * psn; }
 
  ConvData ret = { val, tot };
  //return ConvData( val, tot ); }
  return ret;
}

__DEVICE__ float4 conv2( float r, float2 fragCoord, float2 R, __TEXTURE2D__ txdata ) {
  ConvData nh = ring( r , fragCoord, R, txdata);
  return   nh.value / nh.total; }
    
__DEVICE__ float4 bitmake(ConvData rings[MAX_RADIUS], uint bits, uint of) {
  float4  sum = to_float4(0.0f,0.0f,0.0f,0.0f);
  float tot = 0.0f;
  for(uint i = 0u; i < MAX_RADIUS; i++) {
    if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings[i].value; tot += rings[i].total; } }
  return sum / tot; }


union A2F
 {
   float4  F;    // float4
   float  A[4];  // float [4]
 };


//*************************************************************************************************************************
__KERNEL__ void MultipleNFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    fragCoord+=0.5f;

//  ----    ----    ----    ----    ----    ----    ----    ----
//  Rule Initilisation
//  ----    ----    ----    ----    ----    ----    ----    ----

//  NH Rings
  ConvData nh_rings_m[MAX_RADIUS];
  for(uint i = 0u; i < MAX_RADIUS; i++) { nh_rings_m[i] = ring((float)(i+1u), fragCoord, R, txdata); }

//  Parameters
  const  float   mnp   = 1.0f / 65536.0f;      //  Minimum value of a precise step for 16-bit channel
  const  float   s      = mnp *  80.0f *  128.0f;
  const  float   n      = mnp *  80.0f *   2.0f;

//  Output Values
  //float4 res_c = gdv( to_int2(0, 0), txdata, fragCoord, R );
  A2F res_c;
  res_c.F  = gdv( to_int2(0, 0), txdata, fragCoord, R );

//  Result Values
  //float4 res_v = res_c;
  //float res_v[4] = {res_c.x,res_c.y,res_c.z,res_c.w};
  float res_v[4] = { res_c.A[0],res_c.A[1],res_c.A[2],res_c.A[3]}; // = {res_c.A};
  //res_v = res_c.A; //{res_c.x,res_c.y,res_c.z,res_c.w};

//  ----    ----    ----    ----    ----    ----    ----    ----
//  Update Functions
//  ----    ----    ----    ----    ----    ----    ----    ----

  uint nb[12] = {
    ub[0],  ub[1],  ub[2],  ub[3],
    ub[4],  ub[5],  ub[6],  ub[7],
    ub[8],  ub[9],  ub[10], ub[11] };

  uint ur[24] = {
    ub[12], ub[13], ub[14], ub[15], 
    ub[16], ub[17], ub[18], ub[19],  
    ub[20], ub[21], ub[22], ub[23],
    ub[24], ub[25], ub[26], ub[27],  
    ub[28], ub[29], ub[30], ub[31], 
    ub[32], ub[33], ub[34], ub[35]  };

  uint ch2[ 3] = { 2286157824u, 295261525u, 1713547946u };
  uint ch [ 3] = { ub[38], ub[39], ub[40] };
  uint ch3[ 3] = { ub[41], ub[42], ub[43] };

//  Update Sign
  uint us[ 2] = { ub[36], ub[37] };

float AAAAAAAAAAAAAAAAAAAAA; 
  //float4 smnca_res[12] = {res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c};
  A2F smnca_res[12];
  smnca_res[0].F = smnca_res[1].F = smnca_res[2].F = smnca_res[3].F = smnca_res[4].F = smnca_res[5].F = smnca_res[6].F = smnca_res[7].F = smnca_res[8].F = smnca_res[9].F = smnca_res[10].F = smnca_res[11].F = res_c.F;


  float4 _conv1 = conv2(1.0f, fragCoord, R, txdata);

  for(uint i = 0u; i < 24u; i++) {
    uint    cho = u32_upk( ch[i/8u], 2u, (i*4u+0u) & 31u );
            cho = (cho == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+0u) & 31u ) : cho;
    uint    chi = u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u );
            chi = (chi == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+2u) & 31u ) : chi;
    uint    chm = u32_upk( ch3[i/8u], 2u, (i*4u+2u) & 31u );
            chm = (chm == 3u) ? u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u ) : chm;
                
    float nhv[4] = { bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).x,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).y,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).z,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).w };
float ttttttttttttt;
smnca_res[i/4u].A[chi] = 0.5f;


    if( nhv[cho] >= utp( ur[i], 8u, 0u,fragCoord,R) && nhv[cho] <= utp( ur[i], 8u, 1u,fragCoord,R)) {
      (smnca_res[i/4u]).A[chi] += bsn(us[i/16u], ((i*2u+0u) & 31u)) * s * res_c.A[chm]; }

    if( nhv[cho] >= utp( ur[i], 8u, 2u,fragCoord,R) && nhv[cho] <= utp( ur[i], 8u, 3u,fragCoord,R)) {
      (smnca_res[i/4u]).A[chi] += bsn(us[i/16u], ((i*2u+1u) & 31u)) * s * res_c.A[chm]; } 
  }

  uint dev_idx[4] = {0u,0u,0u,0u};

  float dev[4] = {0.0f,0.0f,0.0f,0.0f};
  for(uint i = 0u; i < 6u; i++) {
    float4 smnca_res_temp = abs_f4(res_c.F - smnca_res[i].F);
    if(smnca_res_temp.x > dev[0]) { dev_idx[0] = i; dev[0] = smnca_res_temp.x; }
    if(smnca_res_temp.y > dev[1]) { dev_idx[1] = i; dev[1] = smnca_res_temp.y; }
    if(smnca_res_temp.z > dev[2]) { dev_idx[2] = i; dev[2] = smnca_res_temp.z; }
    if(smnca_res_temp.w > dev[3]) { dev_idx[3] = i; dev[3] = smnca_res_temp.w; } }

  res_v[0] = smnca_res[dev_idx[0]].A[0];
  res_v[1] = smnca_res[dev_idx[1]].A[1];
  res_v[2] = smnca_res[dev_idx[2]].A[2];
  res_v[3] = smnca_res[dev_idx[3]].A[3];

  res_c.F = ((to_float4(res_v[0],res_v[1],res_v[2],res_v[3]) + (_conv1 * (s*2.13333f))) / (1.0f + (s*2.13333f)))- 0.01f * s;
    
//  ----    ----    ----    ----    ----    ----    ----    ----
//  Shader Output
//  ----    ----    ----    ----    ----    ----    ----    ----


  if (iMouse.z > 0.0f && length(swi2(iMouse,x,y) - fragCoord) < 14.0f) {
        res_c.A[0] = round(mod_f((float)(iFrame),2.0f));
        res_c.A[1] = round(mod_f((float)(iFrame),3.0f));
        res_c.A[2] = round(mod_f((float)(iFrame),5.0f)); }
  if (iFrame == 0) { res_c.A[0] = reseed(0u, 1.0f, 0.4f, fragCoord); res_c.A[1] = reseed(1u, 1.0f, 0.4f, fragCoord); res_c.A[2] = reseed(2u, 1.0f, 0.4f, fragCoord); }
float sssssssssssssssss;    
  //  Force alpha to 1.0
  res_c.A[3]   = 1.0f;
  fragColor=clamp(res_c.F,0.0f,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//  ----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//  ----    ----    ----    ----    ----    ----    ----    ----

#ifdef XXX

#define txdata (iChannel0)
#define PI 3.14159265359
#define LN 2.71828182846

const uint MAX_RADIUS = 12u;

uint ub[64] =  {
    4086443839u, 803560891u, 3521573439u, 155586747u, 
    2355529581u, 3804082561u, 1278181521u, 1198599219u, 
    790900093u, 2043079403u, 72510135u, 329989440u, 
    1205735441u, 1165390975u, 1863025477u, 1552315409u, 
    1460749058u, 1961704519u, 1063988442u, 304586942u, 
    1969173229u, 623707175u, 2719649363u, 533620173u, 
    734380903u, 1866742626u, 69847740u, 779938642u, 
    3928012151u, 1597352029u, 1939485308u, 1599391651u, 
    824038858u, 498247516u, 609160531u, 634145519u, 
    2022645937u, 4285315796u, 87513080u, 246410766u, 
    1160189374u, 688725303u, 1266836767u, 670912482u, 
    2162941226u, 1742659144u, 481786434u, 3618106514u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 1067687164u, 1122344419u, 0u };

__DEVICE__ uint u32_upk(uint u32, uint bts, uint off) { return (u32 >> off) & ((1u << bts)-1u); }

__DEVICE__ float lmap() { return (gl_FragCoord[0] / float(textureSize(txdata,0)[0])); }
__DEVICE__ float vmap() { return (gl_FragCoord[1] / float(textureSize(txdata,0)[1])); }
__DEVICE__ float cmap() { return _sqrtf  ( ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5f) / float(textureSize(txdata,0)[0])*0.5f)
              * ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5f) / float(textureSize(txdata,0)[0])*0.5f)
              + ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5f) / float(textureSize(txdata,0)[1])*0.5f)
              * ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5f) / float(textureSize(txdata,0)[1])*0.5f) ); }
__DEVICE__ float vwm() {
  float   scale_raw   = uintBitsToFloat(ub[62]);
  float   zoom     = uintBitsToFloat(ub[61]);
  float  scale_new  = scale_raw;
  uint   mode     = u32_upk(ub[59], 2u, 0u);
  if( mode == 1u ) { //  Linear Parameter Map
    scale_new = ((lmap() + zoom) * (scale_raw / (1.0f + zoom * 2.0f))) * 2.0f; }
  if( mode == 2u ) { //  Circular Parameter Map
    scale_new = ((_sqrtf(cmap()) + zoom) * (scale_raw / (1.0f + zoom * 2.0f))) * 2.0f; }
  return scale_new; }
    
__DEVICE__ float  tp(uint n, float s)       { return (float(n+1u)/256.0f) * ((s*0.5f)/128.0f); }
__DEVICE__ float utp(uint v, uint  w, uint o)   { return tp(u32_upk(v,w,w*o), vwm()); }
__DEVICE__ float bsn(uint v, uint  o)       { return float(u32_upk(v,1u,o)*2u)-1.0f; }
    
__DEVICE__ float4  sigm(float4  x, float w) { return 1.0f / ( 1.0f + _expf( (-w*2.0f * x * (PI/2.0f)) + w * (PI/2.0f) ) ); }
__DEVICE__ float hmp2(float x, float w) { return 3.0f*((x-0.5f)*(x-0.5f))+0.25f; }

__DEVICE__ float4  gdv( int2 of, __TEXTURE2D__ tx ) {
  of     = to_int2(gl_FragCoord) + of;
  of[0]   = (of[0] + textureSize(tx,0)[0]) % (textureSize(tx,0)[0]);
  of[1]   = (of[1] + textureSize(tx,0)[1]) % (textureSize(tx,0)[1]);
  return   texelFetch( tx, of, 0); }
    
__DEVICE__ float4 nbhd( float2 r, __TEXTURE2D__ tx ) {
//  Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
  uint  chk = 2147483648u /
      (  (   uint( r[0]*r[0]*PI + r[0]*PI + PI  )
        -   uint( r[1]*r[1]*PI + r[1]*PI    ) ) * 128u );
  float  psn = (chk >= 65536u) ? 65536.0f : float(chk);
  float4  a = to_float4(0.0f,0.0f,0.0f,0.0f);
  float  w = 1.0f;  // Weighting, unused
  if(r[0] == 0.0f) { return to_float4_aw( gdv( to_int2(0,0), tx )*w*psn ); }
  else       {
    for(float i = 0.0f; i <= r[0]; i++) {
      for(float j = 1.0f; j <= r[0]; j++) {
        float  d = round(_sqrtf(i*i+j*j));
            w = 1.0f;  //  Per-Neighbor Weighting, unused
        if( d <= r[0] && d > r[1] ) {
          float4 t0  = gdv( to_int2( i, j), tx ) * w * psn; a += t0 - fract(t0);
          float4 t1  = gdv( to_int2( j,-i), tx ) * w * psn; a += t1 - fract(t1);
          float4 t2  = gdv( to_int2(-i,-j), tx ) * w * psn; a += t2 - fract(t2);
          float4 t3  = gdv( to_int2(-j, i), tx ) * w * psn; a += t3 - fract(t3); } } }
    return a; } }

__DEVICE__ float4 totl( float2 r, __TEXTURE2D__ tx ) {
//  Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
  uint  chk = 2147483648u /
      (  (   uint( r[0]*r[0]*PI + r[0]*PI + PI  )
        -   uint( r[1]*r[1]*PI + r[1]*PI    ) ) * 128u );
  float  psn = (chk >= 65536u) ? 65536.0f : float(chk);
  float4  b = to_float4(0.0f,0.0f,0.0f,0.0f);
  float  w = 1.0f;  // Weighting, unused
  if(r[0] == 0.0f) { return to_float4( psn * w, psn * w, psn * w, psn * w ); }
  else       {
    for(float i = 0.0f; i <= r[0]; i++) {
      for(float j = 1.0f; j <= r[0]; j++) {
        float  d = round(_sqrtf(i*i+j*j));
            w = 1.0f;  //  Per-Neighbor Weighting, unused
        if( d <= r[0] && d > r[1] ) { b += psn * w * 4.0f; } } }
    return b; } }

__DEVICE__ float4 bitring(vec4[MAX_RADIUS] rings_a, vec4[MAX_RADIUS] rings_b, uint bits, uint of) {
  float4 sum = to_float4(0.0f,0.0f,0.0f,0.0f);
  float4 tot = to_float4(0.0f,0.0f,0.0f,0.0f);
  for(uint i = 0u; i < MAX_RADIUS; i++) {
    if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings_a[i]; tot += rings_b[i]; } }
  return sigm( (sum / tot), LN ); } // TODO
    
__DEVICE__ float4 conv(float2 r, __TEXTURE2D__ tx) {
  float4 nha = nbhd(r, tx);
  float4 nhb = totl(r, tx);
  return   nha / nhb; }
    
//  Used to reseed the surface with lumpy noise
__DEVICE__ float get_xc(float x, float y, float xmod) {
  float sq = _sqrtf(mod_f(x*y+y, xmod)) / _sqrtf(xmod);
  float xc = mod_f((x*x)+(y*y), xmod) / xmod;
  return clamp((sq+xc)*0.5f, 0.0f, 1.0f); }
__DEVICE__ float shuffle(float x, float y, float xmod, float val) {
  val = val * mod_f( x*y + x, xmod );
  return (val-_floor(val)); }
__DEVICE__ float get_xcn(float x, float y, float xm0, float xm1, float ox, float oy) {
  float  xc = get_xc(x+ox, y+oy, xm0);
  return shuffle(x+ox, y+oy, xm1, xc); }
__DEVICE__ float get_lump(float x, float y, float nhsz, float xm0, float xm1) {
  float   nhsz_c   = 0.0f;
  float   xcn   = 0.0f;
  float   nh_val   = 0.0f;
  for(float i = -nhsz; i <= nhsz; i += 1.0f) {
    for(float j = -nhsz; j <= nhsz; j += 1.0f) {
      nh_val = round(_sqrtf(i*i+j*j));
      if(nh_val <= nhsz) {
        xcn = xcn + get_xcn(x, y, xm0, xm1, i, j);
        nhsz_c = nhsz_c + 1.0f; } } }
  float   xcnf   = ( xcn / nhsz_c );
  float   xcaf  = xcnf;
  for(float i = 0.0f; i <= nhsz; i += 1.0f) {
      xcaf   = clamp((xcnf*xcaf + xcnf*xcaf) * (xcnf+xcnf), 0.0f, 1.0f); }
  return xcaf; }
__DEVICE__ float reseed(uint seed, float scl, float amp) {
  float   fx = gl_FragCoord[0];
  float   fy = gl_FragCoord[1];
  float   r0 = get_lump(fx, fy, round( 6.0f  * scl), 19.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),17.0f), 23.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),43.0f));
  float   r1 = get_lump(fx, fy, round( 22.0f * scl), 13.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),29.0f), 17.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),31.0f));
  float   r2 = get_lump(fx, fy, round( 14.0f * scl), 13.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),11.0f), 51.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),37.0f));
  float   r3 = get_lump(fx, fy, round( 18.0f * scl), 29.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed), 7.0f), 61.0f + mod_f(float(u32_upk(ub[63], 24u, 0u)+seed),28.0f));
  return clamp( _sqrtf((r0+r1)*r3*(amp+1.2f))-r2*(amp*1.8f+0.2f) , 0.0f, 1.0f); }

struct ConvData {
  float4   value;
  float   total;
};


ConvData ring( float r ) {

  const float psn = 32768.0f;
float rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrring;
  float tot = 0.0f;
  float4  val = to_float4(0.0f,0.0f,0.0f,0.0f);

  float sq2  = _sqrtf(2.0f);

  float o_0 = r + 0.5f;
  float o_1 = sq2 * o_0;
  float o_2 = o_1 / 2.0f;
  float o_3 = _sqrtf( o_0*o_0 - r*r );
  float o_4 = o_2 - ( _floor(o_2) + 0.5f );
  float o_5 = _floor( o_2 ) + _floor( o_4 );

  float i_0 = r - 0.5f;
  float i_1 = sq2 * i_0;
  float i_2 = i_1 / 2.0f;
  float i_3 = _sqrtf( i_0*i_0 - r*r );
  float i_4 = i_2 - ( _floor(i_2) + 1.0f );
  float i_5 = _floor( i_2 ) + _floor( i_4 );

  float d_0 = ( i_5 ) + 1.0f - ( o_5 );

  for(float i = 1.0f; i < _floor( i_2 ) + 1.0f - d_0; i++) {

    float j_0 = _sqrtf( o_0*o_0 - (i+0.0f)*(i+0.0f) );
    float j_1 = _sqrtf( i_0*i_0 - (i+0.0f)*(i+0.0f) );
    float j_2 = ( 1.0f - _fabs( sign_f ( (_floor( i_2 ) + 1.0f) - i ) ) );
float xxxxxxxxxxxxxxxx;
    for(float j = _floor( j_1 ) + j_2; j < _floor( j_0 ); j++) {
      val += _floor(gdv(to_int2( i, (int(j)+1)), txdata) * psn);
      val += _floor(gdv(to_int2( i,-(int(j)+1)), txdata) * psn);
      val += _floor(gdv(to_int2(-i,-(int(j)+1)), txdata) * psn);
      val += _floor(gdv(to_int2(-i, (int(j)+1)), txdata) * psn);
      val += _floor(gdv(to_int2( (int(j)+1), i), txdata) * psn);
      val += _floor(gdv(to_int2( (int(j)+1),-i), txdata) * psn);
      val += _floor(gdv(to_int2(-(int(j)+1),-i), txdata) * psn);
      val += _floor(gdv(to_int2(-(int(j)+1), i), txdata) * psn);
      tot += 8.0f * psn; } }

//  Orthagonal
  val += _floor(gdv(to_int2( r, 0), txdata) * psn);
  val += _floor(gdv(to_int2( 0,-r), txdata) * psn);
  val += _floor(gdv(to_int2(-r,-0), txdata) * psn);
  val += _floor(gdv(to_int2(-0, r), txdata) * psn);
  tot += 4.0f * psn;

//  Diagonal
//  TODO This is not quite perfect
  float k_0 = r;
  float k_1 = sq2 * k_0;
  float k_2 = k_1 / 2.0f;
  float k_3 = _sqrtf( k_0*k_0 - r*r );
  float k_4 = k_2 - ( _floor(k_2) + 1.0f );
  float k_5 = _floor( k_2 ) + _floor( k_4 );

  float dist = round(k_2);

  if( sign( o_4 ) == -1.0f ) {
  //  val += gdv(to_int2( (_floor(o_5)+1), _floor(o_5)+1), txdata);
    val += _floor(gdv(to_int2( (_floor(o_5)+1.0f), (_floor(o_5)+1.0f)), txdata) * psn);
    val += _floor(gdv(to_int2( (_floor(o_5)+1.0f),-(_floor(o_5)+1.0f)), txdata) * psn);
    val += _floor(gdv(to_int2(-(_floor(o_5)+1.0f),-(_floor(o_5)+1.0f)), txdata) * psn);
    val += _floor(gdv(to_int2(-(_floor(o_5)+1.0f), (_floor(o_5)+1.0f)), txdata) * psn);
    tot += 4.0f * psn; }

  return ConvData( val, tot ); }


__DEVICE__ float4 conv( float r ) {
  ConvData nh = ring( r );
  return   nh.value / nh.total; }
    
__DEVICE__ float4 bitmake(ConvData[MAX_RADIUS] rings, uint bits, uint of) {
  float4  sum = to_float4(0.0f,0.0f,0.0f,0.0f);
  float tot = 0.0f;
  for(uint i = 0u; i < MAX_RADIUS; i++) {
    if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings[i].value; tot += rings[i].total; } }
  return sum / tot; }

#endif


__KERNEL__ void MultipleNFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution,  float4 iMouse, int iFrame, sampler2D iChannel0)
{
    fragCoord+=0.5f;

//  ----    ----    ----    ----    ----    ----    ----    ----
//  Rule Initilisation
//  ----    ----    ----    ----    ----    ----    ----    ----

//  NH Rings
  ConvData nh_rings_m[MAX_RADIUS];
  for(uint i = 0u; i < MAX_RADIUS; i++) { nh_rings_m[i] = ring((float)(i+1u), fragCoord, R, txdata); }

//  Parameters
  const  float   mnp   = 1.0f / 65536.0f;      //  Minimum value of a precise step for 16-bit channel
  const  float   s     = mnp *  80.0f *  128.0f;
  const  float   n     = mnp *  80.0f *   2.0f;

//  Output Values
  //float4 res_c = gdv( to_int2(0, 0), txdata, fragCoord, R );
  A2F res_c;
  res_c.F  = gdv( to_int2(0, 0), txdata, fragCoord, R );

//  Result Values
  //float res_v[4] = {res_c.x,res_c.y,res_c.z,res_c.w};
  float res_v[4] = { res_c.A[0],res_c.A[1],res_c.A[2],res_c.A[3]}; // = {res_c.A};

//  ----    ----    ----    ----    ----    ----    ----    ----
//  Update Functions
//  ----    ----    ----    ----    ----    ----    ----    ----

  uint nb[12] = {
    ub[0],  ub[1],  ub[2],  ub[3],
    ub[4],  ub[5],  ub[6],  ub[7],
    ub[8],  ub[9],  ub[10], ub[11] };

  uint ur[24] = {
    ub[12], ub[13], ub[14], ub[15], 
    ub[16], ub[17], ub[18], ub[19],  
    ub[20], ub[21], ub[22], ub[23],
    ub[24], ub[25], ub[26], ub[27],  
    ub[28], ub[29], ub[30], ub[31], 
    ub[32], ub[33], ub[34], ub[35]  };

  uint ch2[ 3] = { ( 2286157824u, 295261525u, 1713547946u )};
  uint ch [ 3] = {  ub[38], ub[39], ub[40] };
  uint ch3[ 3] = {  ub[41], ub[42], ub[43] };

//  Update Sign
  uint us[ 2] = { ub[36], ub[37] };
float BBBBBBBBBBBBBBBBBBBBBBBBBBBBB;
  //float4 smnca_res[12] = {res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c};
  A2F smnca_res[12];
  smnca_res[0].F = smnca_res[1].F = smnca_res[2].F = smnca_res[3].F = smnca_res[4].F = smnca_res[5].F = smnca_res[6].F = smnca_res[7].F = smnca_res[8].F = smnca_res[9].F = smnca_res[10].F = smnca_res[11].F = res_c.F;


  float4 _conv1 = conv2(1.0f, fragCoord, R, txdata);

  for(uint i = 0u; i < 24u; i++) {
    uint    cho = u32_upk( ch[i/8u], 2u, (i*4u+0u) & 31u );
        cho = (cho == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+0u) & 31u ) : cho;
    uint    chi = u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u );
        chi = (chi == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+2u) & 31u ) : chi;
    uint    chm = u32_upk( ch3[i/8u], 2u, (i*4u+2u) & 31u );
        chm = (chm == 3u) ? u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u ) : chm;
                
    //float4 nhv = bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u );
    
    float nhv[4] = { bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).x,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).y,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).z,
                     bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u ).w };

    if( nhv[cho] >= utp( ur[i], 8u, 0u,fragCoord,R) && nhv[cho] <= utp( ur[i], 8u, 1u,fragCoord,R)) {
      smnca_res[i/4u].A[chi] += bsn(us[i/16u], ((i*2u+0u) & 31u)) * s * res_c.A[chm]; }

    if( nhv[cho] >= utp( ur[i], 8u, 2u,fragCoord,R) && nhv[cho] <= utp( ur[i], 8u, 3u,fragCoord,R)) {
      smnca_res[i/4u].A[chi] += bsn(us[i/16u], ((i*2u+1u) & 31u)) * s * res_c.A[chm]; } }

  uint dev_idx[4] = {0u,0u,0u,0u};
    
  float dev[4] = {0.0f,0.0f,0.0f,0.0f};
  for(uint i = 0u; i < 6u; i++) {
    float4 smnca_res_temp = abs_f4(res_c.F - smnca_res[i].F);
    if(smnca_res_temp.x > dev[0]) { dev_idx[0] = i; dev[0] = smnca_res_temp.x; }
    if(smnca_res_temp.y > dev[1]) { dev_idx[1] = i; dev[1] = smnca_res_temp.y; }
    if(smnca_res_temp.z > dev[2]) { dev_idx[2] = i; dev[2] = smnca_res_temp.z; }
    if(smnca_res_temp.w > dev[3]) { dev_idx[3] = i; dev[3] = smnca_res_temp.w; } }

  res_v[0] = smnca_res[dev_idx[0]].A[0];
  res_v[1] = smnca_res[dev_idx[1]].A[1];
  res_v[2] = smnca_res[dev_idx[2]].A[2];
  res_v[3] = smnca_res[dev_idx[3]].A[3];

    res_c.F = ((to_float4(res_v[0],res_v[1],res_v[2],res_v[3]) + (_conv1 * (s*2.13333f))) / (1.0f + (s*2.13333f)))- 0.01f * s;
    
//  ----    ----    ----    ----    ----    ----    ----    ----
//  Shader Output
//  ----    ----    ----    ----    ----    ----    ----    ----


    if (iMouse.z > 0.0f && length(swi2(iMouse,x,y) - fragCoord) < 14.0f) {
        res_c.A[0] = round(mod_f(float(iFrame),2.0f));
        res_c.A[1] = round(mod_f(float(iFrame),3.0f));
        res_c.A[2] = round(mod_f(float(iFrame),5.0f)); }
    if (iFrame == 0) { res_c.A[0] = reseed(0u, 1.0f, 0.4f,fragCoord); res_c.A[1] = reseed(1u, 1.0f, 0.4f,fragCoord); res_c.A[2] = reseed(2u, 1.0f, 0.4f,fragCoord); }

//  Force alpha to 1.0
    res_c.A[3]   = 1.0f;
    fragColor=clamp(res_c.F,0.0f,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


//  ----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//  ----    ----    ----    ----    ----    ----    ----    ----

__KERNEL__ void MultipleNFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    fragColor = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R); 

  SetFragmentShaderComputedColor(fragColor);
}