
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//--------------------------------------
// CONFIG
//--------------------------------------

#define ANIMATE_WATER 1

//Bloom
#define BLOOM_SIZE (0.5f)
#define BLOOM_THRESHOLD (1.01f)
#define BLOOM_RANGE (0.3f)
#define BLOOM_FRAME_BLEND (0.2f)

//--------------------------------------
// DEFINES & CONSTANTS
//--------------------------------------

//utility defines
#define ZERO   (_fminf(1,0))
#define X_AXIS to_float3(1,0,0)
#define Y_AXIS to_float3(0,1,0)
#define Z_AXIS to_float3(0,0,1)

#define PI 3.14159265f
#define TAU (2*PI)
#define PHI (_sqrtf(5)*0.5f + 0.5f)

// Materials
#define MAT_WOOD  101
#define MAT_LEAFS 102
#define MAT_GROUND 201
#define MAT_GRASS 202
#define MAT_UNDERWATER 301
#define MAT_BONE 401
#define MAT_FRUIT 501
#define MAT_PLANT 701
#define MAT_STONE 801

//--------------------------------------
// TYPES
//--------------------------------------

//--------------------------------------
// UTILITY FUNCTIONS
//--------------------------------------
  __DEVICE__ inline mat3 multi( float B, mat3 A)  
  {  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  }

  __DEVICE__ inline mat3 inverse( mat3 A)  
  {  
   mat3 R;  
   float result[3][3];  
   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
                    {A.r1.x, A.r1.y, A.r1.z},  
                    {A.r2.x, A.r2.y, A.r2.z}};  
     
   float det = a[0][0] * a[1][1] * a[2][2]  
             + a[0][1] * a[1][2] * a[2][0]  
             + a[0][2] * a[1][0] * a[2][1]  
             - a[2][0] * a[1][1] * a[0][2]  
             - a[2][1] * a[1][2] * a[0][0]  
             - a[2][2] * a[1][0] * a[0][1];  
   if( det != 0.0 )  
   {  
	   result[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];  
	   result[0][1] = a[2][1] * a[0][2] - a[2][2] * a[0][1];  
	   result[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];  
	   result[1][0] = a[2][0] * a[1][2] - a[1][0] * a[2][2];  
	   result[1][1] = a[0][0] * a[2][2] - a[2][0] * a[0][2];  
	   result[1][2] = a[1][0] * a[0][2] - a[0][0] * a[1][2];  
	   result[2][0] = a[1][0] * a[2][1] - a[2][0] * a[1][1];  
	   result[2][1] = a[2][0] * a[0][1] - a[0][0] * a[2][1];  
	   result[2][2] = a[0][0] * a[1][1] - a[1][0] * a[0][1];  
		 
	   R = to_mat3_f3(make_float3(result[0][0], result[0][1], result[0][2]),   
                    make_float3(result[1][0], result[1][1], result[1][2]), 
                    make_float3(result[2][0], result[2][1], result[2][2]));  
	   return multi( 1.0f / det, R);  
   }  
   R = to_mat3_f3(make_float3(1.0f, 0.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), make_float3(0.0f, 0.0f, 1.0f));  
   return R;  
  } 



__DEVICE__ mat3 rotation(float3 axis, float angle)
{
    //axis = normalize(axis);
    float s = _sinf(angle);
    float c = _cosf(angle);
    float oc = 1.0f - c;
    
    return inverse(to_mat3(oc * axis.x * axis.x + c, 
                          oc * axis.x * axis.y - axis.z * s,  
                          oc * axis.z * axis.x + axis.y * s, 
                          oc * axis.x * axis.y + axis.z * s,  
                          oc * axis.y * axis.y + c,           
                          oc * axis.y * axis.z - axis.x * s,  
                          oc * axis.z * axis.x - axis.y * s,  
                          oc * axis.y * axis.z + axis.x * s,  
                          oc * axis.z * axis.z + c));
}

//--------------------------------------
// BASIC SDFs SHAPES
// https://iquilezles.org/articles/distfunctions
//--------------------------------------
__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
    float3 d = abs_f3(p) - b;
    return _fminf(_fmaxf(d.x,_fmaxf(d.y,d.z)),0.0f) + length(_fmaxf(d,to_float3_s(0.0f)));
}

__DEVICE__ float sdCone(float3 p, float3 a, float3 b, float ra, float rb)
{
    float rba  = rb-ra;
    float baba = dot(b-a,b-a);
    float papa = dot(p-a,p-a);
    float paba = dot(p-a,b-a)/baba;
    float x = _sqrtf( papa - paba*paba*baba );
    float cax = _fmaxf(0.0f,x-((paba<0.5f)?ra:rb));
    float cay = _fabs(paba-0.5f)-0.5f;
    float k = rba*rba + baba;
    float f = clamp( (rba*(x-ra)+paba*baba)/k, 0.0f, 1.0f );
    float cbx = x-ra - f*rba;
    float cby = paba - f;
    float s = (cbx < 0.0f && cay < 0.0f) ? -1.0f : 1.0f;
    return s*_sqrtf( _fminf(cax*cax + cay*cay*baba,
                            cbx*cbx + cby*cby*baba) );
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r ) // approximated
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0f)/k1;
}

__DEVICE__ float dot2( in float2 v ) { return dot(v,v); }
__DEVICE__ float dot2( in float3 v ) { return dot(v,v); }
__DEVICE__ float ndot( in float2 a, in float2 b ) { return a.x*b.x - a.y*b.y; }
__DEVICE__ float udTriangle( float3 p, float3 a, float3 b, float3 c )
{
  float3 ba = b - a; float3 pa = p - a;
  float3 cb = c - b; float3 pb = p - b;
  float3 ac = a - c; float3 pc = p - c;
  float3 nor = cross( ba, ac );

  return _sqrtf(
                (sign_f(dot(cross(ba,nor),pa)) +
                 sign_f(dot(cross(cb,nor),pb)) +
                 sign_f(dot(cross(ac,nor),pc))<2.0f)
                 ?
                 _fminf( _fminf(
                 dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0f,1.0f)-pa),
                 dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0f,1.0f)-pb) ),
                 dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0f,1.0f)-pc) )
                 :
                 dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

//--------------------------------------
// SDF BLEND & DOMAIN REPETITION
// https://mercury.sexy/hg_sdf/
//--------------------------------------

//HG
__DEVICE__ float fOpUnionRound(float a, float b, float r) 
{
  
  float2 u = _fmaxf(to_float2(r - a,r - b), to_float2_s(0));
  return _fmaxf(r, _fminf (a, b)) - length(u);
}

__DEVICE__ float2 pModPolar(in float2 p, float repetitions) 
{
  float angle = 2.0f*PI/repetitions;
  float a = _atan2f((p).y, (p).x) + angle/2.0f;
  float r = length(p);
  float c = _floor(a/angle);
  a = mod_f(a,angle) - angle/2.0f;
  p = to_float2(_cosf(a), _sinf(a))*r;
  // For an odd number of repetitions, fix cell index of the cell in -x direction
  // (cell index would be e.g. -5 and 5 in the two halves of the cell):
//  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
//  return c;

  return p;
}

// Repeat in two dimensions
__DEVICE__ float2 pMod2(inout float2 *p, float2 size) {
  float2 c = _floor((*p + size*0.5f)/size);
  *p = mod_f2f2(*p + size*0.5f,size) - size*0.5f;
  return c;
}

// Same, but mirror every second cell so all boundaries match
__DEVICE__ float2 pModMirror2(inout float2 *p, float2 size) {
  float2 halfsize = size*0.5f;
  float2 c = _floor((*p + halfsize)/size);
  *p = mod_f2f2(*p + halfsize, size) - halfsize;
  *p *= mod_f2f2(c,to_float2_s(2))*2.0f - to_float2_s(1.0f);
  return c;
}

__DEVICE__ float fOpIntersectionRound(float a, float b, float r)
{
  float2 u = _fmaxf(to_float2(r + a,r + b), to_float2_s(0));
  return _fminf(-r, _fmaxf (a, b)) + length(u);
}

__DEVICE__ float fOpDifferenceRound (float a, float b, float r)
{
  return fOpIntersectionRound(a, -b, r);
}

//IQ
__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) 
{
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); 
}


//--------------------------------------
// SDF FIGURES
//--------------------------------------
__DEVICE__ float treeTrunk(float3 pos)
{
    float r = 1e10;
    
    //trunk stem
    r =  sdCone(pos, to_float3_s(0.0f), to_float3(0.0f,0.35f,0.0f), 0.12f, 0.1f );

    // repeat ellpisoids for roots
    float3 rootsDomain = pos;
    
    swi2S(rootsDomain,x,z, pModPolar(swi2(rootsDomain,x,z), 5.0f));
    r = fOpUnionRound(r, sdEllipsoid(rootsDomain - to_float3(0.1f, 0.01f, 0.0f), to_float3(0.09f, 0.04f, 0.04f)), 0.04f);
   
    //displacement
    r += _sinf(30.0f*pos.x)*_sinf(30.0f*pos.y)*_sinf(30.0f*pos.z) * 0.01f;   
   
    return r;
}

__DEVICE__ float treeLeafs(float3 pos)
{
    float r = 1e10;
    
    //pos += (noise(pos * 200.0f) - 0.5f) * 2.0f * 0.001f;
    
    //sphere piramid
    float3 leafsDomain = pos - to_float3(0.0f, 0.42f, 0.0f);

    float blend = 0.12f;
    swi2S(leafsDomain,x,z, pModPolar(swi2(leafsDomain,x,z), 3.0f));
    r = _fminf(r, sdSphere(leafsDomain - to_float3(0.16f, 0.0f, 0.0f), 0.22f));
    //r = opSmoothUnion(r, sdSphere(leafsDomain - to_float3(-0.08f, 0.00f, 0.14f), 0.22f), blend);
    //r = opSmoothUnion(r, sdSphere(leafsDomain - to_float3(-0.09f, 0.00f, -0.14f), 0.22f), blend);
    
    r = _fminf(r, sdSphere(pos - to_float3(0.0f, 0.62f, 0.0f), 0.22f));
    //r = opSmoothUnion(r, sdSphere(pos - to_float3(0.0f, 0.50f, 0.0f), 0.36f), blend);
    
    //leaf displacement
    r += _sinf(20.0f*pos.x)*_sinf(20.0f*pos.y)*_sinf(20.0f*pos.z) * 0.01f;
        
    return r + 0.02f;
}

__DEVICE__ float grassStems(float3 pos, float grassMask)
{
    float r = 1e10;
    //noise pattern approach, (DISCARDED)
    //float noisePattern = noise(to_float3(swi2(pos,x,z), 0.0f) * 300.0f);
    //r = sdBox( pos - to_float3( 0.0f, 0.02f, 0.0f), to_float3(0.6f,0.05f,0.6f) );
    //r += step(0.5f, noisePattern) * 0.001f;
    
    float distortion = (_sinf(20.0f*pos.x)*_sinf(30.0f*pos.y)*_sinf(20.0f*pos.z)+_cosf(30.0f*pos.x)*_cosf(35.0f*pos.z)) * 0.012f;
    grassMask += distortion * 0.8f;
    float area = fOpDifferenceRound(sdBox( pos - to_float3( 0.0f, -0.03f, 0.0f), to_float3(0.58f,0.02f,0.58f) ), grassMask - 0.03f, 0.03f) -0.04f;
    
    //pos.y += 0.01f;
    
    //grass stem CONE (DISCARDED)
    //vec2 id = pMod2(swi2(pos,x,z), to_float2(0.02f, 0.02f));
    //r =  sdCone(pos, to_float3(_sinf(id.y * 12.34f)*_cosf(id.x*23.0f)*0.007f, 0.0f, _sinf(id.x * 12.34f)*0.008f), 
    //                 to_float3_aw(_sinf(id.y * 30.54f)*0.008f, 0.04f + 0.02f*((_sinf(id.x* 1.23f)*_cosf(id.y*20.0f) + 0.5f) * 2.0f),0.0f), 0.003f, 0.001f );
    
    //triangle stem alternative (DISCARDED)
    //r = udTriangle( pos - to_float3(_sinf(id.y * 12.34f)*0.015f, 0.0f, _sinf(id.x * 12.34f)*0.012f),
    //                      to_float3(-0.005f, 0.0f, _sinf(id.x * 2.34f)*0.005f), 
    //                      to_float3(0.005f, 0.0f,  _sinf(id.y * 1.34f)*0.005f), 
    //                      to_float3(_sinf(id.y * 52.34f)*0.002f, 0.05f, _sinf(id.x * 32.34f)*0.002f) );
    //r = opSmoothUnion(r, sdBox( pos - to_float3( 0.0f, -0.05f, 0.0f), to_float3(0.6f,0.035f,0.6f)) - 0.02f, 0.01f);
    
    
    r = _fminf(r, sdBox( pos - to_float3( 0.0f, -0.07f, 0.0f), to_float3(0.6f,0.04f,0.6f) - 0.04f) - 0.02f) ;
    r = _fmaxf(r, (area - distortion))- 0.04f;
    
    float remover = sdBox( pos - to_float3( 0.0f, -0.12f, 0.0f), to_float3(0.7f,0.1f,0.7f) - 0.02f);
    //remover = _fmaxf(remover, grassMask - distortion);
    remover -= distortion;
    
    r = fOpDifferenceRound(area, remover, 0.02f);
    
    return r;
    //return grassMask + 0.03f;
}

__DEVICE__ float treeFruits(float3 pos)
{
    float r = 1e10;
    //vec3 fruitPos = pos - to_float3(-0.32f, 0.45f, -0.31f);
    //fruitPos = rotation(Y_AXIS, PI*0.25f) * fruitPos;
    //pModPolar(swi2(fruitPos,x,y), 3.0f);
    //fruitPos -= to_float3(0.15f, 0.0f, 0.01f);
    //pModPolar(swi2(fruitPos,z,y), 2.0f);
    //fruitPos -= to_float3(0.1f, 0.0f, 0.15f);
    
    //pModPolar(swi2(fruitPos,x,y), 2.0f);
    //fruitPos -= to_float3(0.02f, 0.0f, -0.1f);
    //r = sdSphere(fruitPos , 0.04f);
    
    
    //fruit that... works...
    //r =        sdSphere(pos - to_float3(-0.25f, 0.45f, -0.0f),  0.04f);
    //r = _fminf(r, sdSphere(pos - to_float3(-0.46f, 0.4f,   0.02f), 0.04f));
    //r = _fminf(r, sdSphere(pos - to_float3(-0.46f, 0.8f,  -0.28f), 0.04f));
    //r = _fminf(r, sdSphere(pos - to_float3(-0.3f,  0.4f,  -0.62f), 0.04f));
    //r = _fminf(r, sdSphere(pos - to_float3(-0.55f, 0.4f,  -0.55f), 0.04f));
    //r = _fminf(r, sdSphere(pos - to_float3(0.04f,  0.34f, -0.28f), 0.04f));
    
    //new fruit ?
    //r =        sdSphere(pos - to_float3(-0.26f, 0.45f, -0.03f),  0.08f);
    
    return r;
}

//--------------------------------------
// MAIN SDF 
//--------------------------------------
__DEVICE__ float2 map(in float3 pos)
{
    float2 res = to_float2( 1e10, 0.0f );
#   define opMin(_v, _m)    res = (_v < res.x) ? to_float2(_v, _m) : res

    float substrate = sdBox( pos - to_float3( 0.0f, -0.15f, 0.0f), to_float3(0.6f,0.15f,0.6f) );
    float waterCavity = sdEllipsoid(pos - to_float3(0.65f, -0.02f, 0.35f), to_float3(0.5f, 0.2f, 0.6f));
          waterCavity = opSmoothUnion(waterCavity, sdEllipsoid(pos - to_float3(0.28f, -0.02f, 0.65f), to_float3(0.4f, 0.2f, 0.4f)), 0.08f);
          waterCavity -= _sinf(20.0f*pos.x)*_sinf(30.0f*pos.y)*_sinf(20.0f*pos.z) * 0.012f;
    substrate = fOpDifferenceRound(substrate, waterCavity, 0.07f);
    //substrate = _fmaxf(substrate, -waterCavity);
    
    opMin(substrate, MAT_GROUND);
    
    
    float underWater = _fmaxf(res.x , waterCavity - 0.03f );
    //res.x += 0.001f;
    res = (underWater < (res.x + 0.0001f)) ? to_float2(underWater, MAT_UNDERWATER) : res;
    //opMin(underWater, MAT_UNDERWATER);
    
    opMin(grassStems(pos, waterCavity), MAT_GRASS);
    
    float3 treePos = pos - to_float3( -0.3f, 0.028f, -0.3f);
    float treeTrunkDist = treeTrunk( treePos );
    opMin(treeTrunkDist, MAT_WOOD);
    float fruitsDist = treeFruits(pos);
    float leafsDist = treeLeafs( treePos );
    opMin(leafsDist, MAT_LEAFS);
    opMin(fruitsDist, MAT_FRUIT);
    
    //Rocks
    float rock = sdEllipsoid(pos - to_float3(0.3f, 0.06f, -0.3f), to_float3(0.09f, 0.09f, 0.07f));
          rock = _fminf(rock, sdEllipsoid(pos - to_float3(0.26f, 0.06f, -0.38f), to_float3(0.04f, 0.055f, 0.04f)));
          rock = _fminf(rock, sdEllipsoid(pos - to_float3(-0.28f, 0.04f, 0.38f), to_float3(0.06f, 0.055f, 0.055f)));
    rock += _sinf(30.0f*pos.x)*_sinf(30.0f*pos.y + 5.5f)*_sinf(28.0f*pos.z) * 0.02f;
    opMin(rock, MAT_STONE);
    
    //Bone
    //float bone = 0;
    
    //water test
    //opMin(sdBox( pos - to_float3( 0.25f, -0.16f, 0.15f), to_float3(0.35f,0.13f,0.45f) ), 0);
    
    //res.x -= 0.005f;
    return res;
}

//float gTime = 0.0f;
__DEVICE__ float mapWaterVolume(float3 pos, float gTime)
{   
    float baseBox = sdBox( pos - to_float3( 0.18f, -0.17f, 0.15f), to_float3(0.4f,0.11f,0.43f) );
    float wv = baseBox;
    //animate water
#if defined(ANIMATE_WATER) && ANIMATE_WATER
    float3 offset = to_float3(-gTime * 0.04f - 0.02f, 0.0f, gTime * 0.06f + 0.1f);
#else
    float3 offset = to_float3_s(0.1f);
#endif
    wv += (1.0f - clamp((pos.y / -0.15f), 0.0f, 1.0f)) // affect mostly on top of the water surface
        *( 1.0
         * _sinf(-22.0f*(pos.x+offset.x))
         * _sinf(23.0f*(pos.z + offset.z))
         + _sinf(20.0f*(pos.z + offset.z + 12.5f)) * 0.3
         + _cosf(15.0f*(pos.z + 2.1f +offset.x)) * 0.2
         + _cosf(-21.0f*(pos.x+offset.z)) *0.5
         )* 0.015f; //*_sinf(30.0f*pos.y)
    wv -= 0.004f;
    wv = _fmaxf(wv, sdBox( pos - to_float3( 0.18f, -0.17f, 0.15f), to_float3(0.4f,0.14f,0.43f) ));
    wv -= 0.004f;
    return wv;
}

//--------------------------------------
// RAYMARCHING 
// https://iquilezles.org/articles/rmshadows
//--------------------------------------
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float tmin, in float tmax )
{
    // bounding volume
    //float tp = (maxHei-ro.y)/rd.y; if( tp>0.0f ) tmax = _fminf( tmax, tp );

    float res = 1.0f;
    float t = tmin;
    for( int i=ZERO; i<22; i++ )
    {
    float h = map( ro + rd*t ).x;
        float s = clamp(8.0f*h/t,0.0f,1.0f);
        res = _fminf( res, s*s*(3.0f-2.0f*s) );
        t += clamp( h, 0.02f, 0.10f );
        if( res<0.005f || t>tmax ) break;
    }
    return clamp( res, 0.0f, 1.0f );
}

// https://iquilezles.org/articles/normalsSDF
__DEVICE__ float3 calcNormal( in float3 pos )
{
#if 0
    float2 e = to_float2(1.0f,-1.0f)*0.5773f*0.0005f;
    return normalize( swi3(e,x,y,y)*map( pos + swi3(e,x,y,y) ).x + 
                      swi3(e,y,y,x)*map( pos + swi3(e,y,y,x) ).x + 
                      swi3(e,y,x,y)*map( pos + swi3(e,y,x,y) ).x + 
                      swi3(e,x,x,x)*map( pos + swi3(e,x,x,x) ).x );
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.0005f*e).x;
    }
    return normalize(n);
#endif    
}

__DEVICE__ float3 calcNormalWater( in float3 pos, float gTime )
{
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*mapWaterVolume(pos+0.0005f*e, gTime);
    }
    return normalize(n); 
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor )
{
  float occ = 0.0f;
    float sca = 1.00f;
    for( int i=ZERO; i<5; i++ )
    {
        float hr = 0.01f + 0.12f*(float)(i)/4.0f;
        float3 aopos =  nor * hr + pos;
        float dd = map( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95f;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f ) * (0.5f+0.5f*nor.y);
}

__DEVICE__ float3 calcSkyColor(float3 aDirection)
{
    float t = smoothstep(0.1f, 0.6f, 0.5f*(aDirection.y + 1.0f));
    return _mix(to_float3(0.4f, 0.4f, 0.2f), to_float3(0.4f, 0.6f, 1.0f), t);
}

__DEVICE__ float4 calcColor(int matId, float3 pos, float3 normal, float diffuse, float fresnel)
{
    float4 FinalColor = to_float4(0.1f, 0.1f, 0.1f, 1) * diffuse;
    if(matId == MAT_WOOD)
    {
        float3 WoodBrown = to_float3(0.287f, 0.11882f, 0.04f) * 1.5f;
        float3 WoodBrownShadow = to_float3(0.1847f, 0.0482f, 0.016f) * 1.2f;
        swi3S(FinalColor,x,y,z, _mix(WoodBrownShadow, WoodBrown, diffuse));
        swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + WoodBrown * fresnel * 2.0f);
    }
    else if(matId == MAT_LEAFS)
    {
        float3 Leafs = to_float3(0.0882f, 0.447f, 0.04f);
        float3 LeafsShadow = to_float3(0.00582f, 0.247f, 0.02f);
        swi3S(FinalColor,x,y,z, _mix(LeafsShadow, Leafs, diffuse) * 0.7f);
        swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + Leafs * fresnel * 2.5f);
    }
    else if(matId == MAT_GRASS)
    {                
        float3 Grass = to_float3(0.0882f, 0.247f, 0.04f);
        float3 GrassShadow = to_float3(0.00582f, 0.147f, 0.02f);
        swi3S(FinalColor,x,y,z, _mix(GrassShadow, Grass, diffuse));
        swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + Grass * fresnel * 1.5f);
    }
    else if(matId == MAT_STONE)
    {
        float3 Stone = to_float3(0.4f, 0.4f, 0.4f);
        float3 StoneShadow = to_float3(0.2f, 0.2f, 0.3f);
        swi3S(FinalColor,x,y,z, _mix(StoneShadow, Stone, diffuse));
        swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + Stone * fresnel * 0.5f);
    }
    else if(matId == MAT_FRUIT)
    {
        float3 Fruit = to_float3(0.8f, 0.1f, 0.01f);
        float3 FruitShadow = to_float3(0.75f, 0.1f, 0.01f);
        swi3S(FinalColor,x,y,z, _mix(FruitShadow, Fruit, diffuse));
        swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + to_float3(0.8f, 0.5f, 0.5f) * fresnel * 2.5f);
    }
    else if(matId == MAT_GROUND)
    {
        if(_fabs(dot(normal, Y_AXIS)) > 0.5f)
        {
            float3 Sand = to_float3(0.447f, 0.447f, 0.04f);
            float3 SandShadow = to_float3(0.347f, 0.247f, 0.02f);
            swi3S(FinalColor,x,y,z, _mix(SandShadow, Sand, diffuse));
        }
        else 
        {
            float axis = dot(normal, X_AXIS);
            float2 basePos = (_fabs(axis) > 0.5f) ? swi2(pos,z,y) : swi2(pos,x,y);
            
            //0-1 UVs based on ground block size
            float2 uv = swi2(basePos,x,y) / to_float2(0.6f, 0.15f); 
            
            //Adjust position
            uv += to_float2(0.0f, 0.6f);
            uv += (axis > 0.5f) ? to_float2(-0.12f, 0.0f) : to_float2(0.32f, 0.0f);
          
            //Sine
            float2 p = uv*2.0f - 1.0f;
            p *= 15.0f;
            float sfunc = p.y + 5.0f*_sinf(uv.x*5.0f ) + 4.0f*_cosf(uv.x*3.0f );
            sfunc *= 0.01f;
            sfunc = _fabs(sfunc);
            sfunc = smoothstep(0.25f, 0.251f, sfunc);
            
            //Brown mix
            float3 GroundBrown = _mix(to_float3(0.4f, 0.25f, 0.08f), to_float3(0.28f, 0.15f, 0.05f), sfunc); // to_float3(0.35f, 0.18f, 0.1f)
            float3 GroundBrownShadow = GroundBrown * 0.5f; // _mix(to_float3(0.20f, 0.08f, 0.04f), to_float3(0.12f, 0.06f, 0.025f), sfunc); // to_float3(0.35f, 0.18f, 0.1f)
            swi3S(FinalColor,x,y,z, _mix(GroundBrownShadow, GroundBrown, _mix(0.3f, 1.0f, diffuse)));
            swi3S(FinalColor,x,y,z, swi3(FinalColor,x,y,z) + GroundBrown * fresnel * 0.8f);
        }
    }
    else if(matId == MAT_UNDERWATER)
    {
        float3 Sand = to_float3(0.447f, 0.447f, 0.04f);
        float3 SandShadow = to_float3(0.347f, 0.247f, 0.02f);
        swi3S(FinalColor,x,y,z, _mix(SandShadow, Sand, diffuse));
    }
    
    return FinalColor;
}

__DEVICE__ float3 castRay(float3 ro, float3 rd)
{
    float3 res = to_float3(0.0f, 1e10, 0.0f);
    float tmin = 1.0f;
    float tmax = 20.0f;
    float t = tmin;
    for( int i=0; i<70 && t<tmax; i++ )
    {
        float2 h = map( ro+rd*t );
        if( _fabs(h.x)<(0.0001f*t) )
        { 
            res = to_float3(t, h.x, h.y); 
            break;
        }
        t += h.x;
    }
    
    return res;
}

__DEVICE__ float4 applyWaterVolume(float3 ro, float3 rd, float depth, float4 color, float gTime)
{
    float tmin = 1.0f;
    float tmax = 20.0f;
    float t = tmin;
    float hit = 0.0f;
    float h = 0.0f;
    float distInsideWater = 0.0f;
    for( int i=0; i<70 && t<tmax; i++ )
    {
        h = mapWaterVolume( ro+rd*t, gTime );
        if( _fabs(h)<(0.0001f*t) )
        { 
            distInsideWater += h;
            hit = 1.0f;
            break;
        }
        else if(hit > 0.0f)
        {
            break;
        }
        t += h;
        if(depth > 0.0f && ((t + 0.0011f) > depth))
        {
            break;
        }
    }
    
    depth = (depth > 0.0f) ? depth : t*2.5f;
    
    float4 WaterBlue = to_float4(0.1f, 0.4f, 1.0f, color.w);
    
    float3 pos = ro + rd * t;
    float3 lightDir = normalize( to_float3(-0.5f, 1.1f, -0.6f) );
    float shadow = calcSoftshadow( pos, lightDir, 0.02f, 2.5f );
    float3 normal = calcNormalWater(pos, gTime);
    float NdL = clamp( dot( normal, lightDir ), 0.0f, 1.0f );
    float3  hal = normalize( lightDir-rd );
    float spe = _powf( clamp( dot( normal, hal ), 0.0f, 1.0f),40.0f)
                    //*_mix(0.5f, 1.0f, NdL* shadow)  //shadow
                    //*(0.04f + 2.5f*_powf( clamp(1.0f+dot(hal, rd),0.0f,1.0f), 1.0f ));
                    ;
    spe = smoothstep(0.5f, 0.9f, spe);
    //light affecting water
    WaterBlue = _mix(WaterBlue * 0.5f, WaterBlue, NdL * shadow);
    
    //all inside water is bluiedish
    color = _mix(color, WaterBlue * 0.8f + color * WaterBlue * 0.5f, hit * 0.3f);
    
    //distance to closest point
    float nearest = clamp(map(pos).x, 0.0f, 1.0f);
    color = _mix(color, WaterBlue, clamp(_powf(nearest * hit, 1.3f) * 5.0f, 0.0f, 1.0f));
    
    //distance to center of the diorama hack
    color = _mix(color, WaterBlue , clamp(_powf(length(pos) * hit, 2.0f) * 1.2f, 0.0f, 1.0f));
    //color = _mix(color, WaterBlue * 0.5f, ((t / depth)) * hit * 0.7f);
    //return to_float4(_mix(swi3(color,x,y,z), normal * 0.5f + 0.5f, hit), 1.0f);

#define WATER_OPACITY_INIT 0.3f
#define WATER_OPACITY_COEFF 2.5f

    float fresnel = _powf( clamp(1.0f+dot(normal,rd),0.0f,1.0f), 2.4f );
    swi3S(color,x,y,z, swi3(color,x,y,z) + hit*2.00f*spe*to_float3_s(1.0f));
    //color = _mix(color, color * _expf(-(1.0f-WaterBlue)*(WATER_OPACITY_INIT + WATER_OPACITY_COEFF*distInsideWater)), hit);
    color += _mix(to_float4_s(0.0f), fresnel*color*2.0f, hit * _mix(0.2f, 1.0f, shadow));
    return color;
}

__DEVICE__ float4 render( float2 uv, in float3 ro, in float3 rd, in float3 rdx, in float3 rdy, float gTime )
{ 
    //vec4 finalColor = to_float4_s(0.042f); //to_float4_aw(calcSkyColor(rd), 0.0f);
    float4 finalColor = to_float4_aw(calcSkyColor(rd), 0.0f);
    float3 res = castRay(ro,rd);
    
    if(res.y < 0.002f)
    {
        float3 lightDir = normalize( to_float3(-0.5f, 1.1f, -0.6f) );
        float3 pos = ro + rd * res.x;
        float3 normal = calcNormal(pos);
        
        float ao = calcAO(pos, normal);
        float shadow = calcSoftshadow( pos, lightDir, 0.02f, 2.5f );
        float NdL = clamp( dot( normal, lightDir ), 0.0f, 1.0f );
        float fresnel = _powf( clamp(1.0f+dot(normal,rd),0.0f,1.0f), 2.4f );
        
        float diffuse  = shadow * NdL * 12.0f;

        float4 color = calcColor((int)(res.z), pos, normal, diffuse, fresnel) * _mix(0.22f, 1.0f, ao);

        finalColor = to_float4_aw(swi3(color,x,y,z), res.x);
      
        swi3S(finalColor,x,y,z, swi3(finalColor,x,y,z) * 0.4f + 0.6f * swi3(finalColor,x,y,z) * calcSkyColor(normal));
        
        //finalColor = to_float4_aw(normal * 0.5f + 0.5f, res.x);
    }
    
    finalColor = applyWaterVolume(ro, rd, res.x, finalColor, gTime);
    
    return finalColor;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------



//main raymarching

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv =          ( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

__KERNEL__ void GreenfielddioramaJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    fragCoord+=0.5f;
    
    float gTime = (float)(iTime);
    float2 mo = swi2(iMouse,x,y)/iResolution;
    //mo.y += 0.2f;
    float time = 23.5f + iTime*1.5f;
    //float time = 23.5f;
    
    // camera  
    float3 ro = to_float3( 4.6f*_cosf(0.1f*time + 12.0f*mo.x),  1.2f + 3.0f*mo.y, 4.6f*_sinf(0.1f*time + 12.0f*mo.x) );
    float3 ta = to_float3( 0.0f, 0.14f, 0.0f );
    // camera-to-world transformation
    mat3 ca = setCamera( ro, ta, 0.0f );
 
    float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;

    // ray direction
    float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,6.0f) ));

     // ray differentials (NOT USED YET)
    float2 px = (-iResolution+2.0f*(fragCoord+to_float2(1.0f,0.0f)))/iResolution.y;
    float2 py = (-iResolution+2.0f*(fragCoord+to_float2(0.0f,1.0f)))/iResolution.y;
    float3 rdx = mul_mat3_f3(ca , normalize( to_float3_aw(px,2.0f) ));
    float3 rdy = mul_mat3_f3(ca , normalize( to_float3_aw(py,2.0f) ));

    // render
    float4 col = render( p, ro, rd, rdx, rdy, gTime );
    
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//Pyramid Bloom
//https://www.shadertoy.com/view/lsBfRc

__DEVICE__ float3 makeBloom(float lod, float2 offset, float2 bCoord, float2 aPixelSize, __TEXTURE2D__ iChannel1)
{
    offset += aPixelSize;

    float lodFactor = _exp2f(lod);

    float3 bloom = to_float3_s(0.0f);
    float2 scale = lodFactor * aPixelSize;

    float2 coord = (swi2(bCoord,x,y)-offset)*lodFactor;
    float totalWeight = 0.0f;

    //if (any(greaterThanEqual(_fabs(coord - 0.5f), scale + 0.5f)))
      if (_fabs(coord.x - 0.5f) >= scale.x + 0.5f || _fabs(coord.y - 0.5f) >= scale.y + 0.5f)
        return to_float3_s(0.0f);

    for (int i = -3; i < 3; i++) 
    {
        for (int j = -3; j < 3; j++) 
        {
            float wg = _powf(1.0f-length(to_float2(i,j)) * 0.125f, 6.0f); //* 0.125f, 6.0
            float3 lTextureColor = swi3(texture(iChannel1, to_float2(i,j) * scale + lodFactor * aPixelSize + coord),x,y,z); //, lod
            //lTextureColor = (any(greaterThan(lTextureColor, to_float3_aw(BLOOM_THRESHOLD)))) ? lTextureColor * BLOOM_SIZE : to_float3_s(0.0f);
            lTextureColor = (lTextureColor.z > BLOOM_THRESHOLD || lTextureColor.y > BLOOM_THRESHOLD || lTextureColor.z > BLOOM_THRESHOLD ) ? lTextureColor * BLOOM_SIZE : to_float3_s(0.0f);
            lTextureColor = pow_f3(lTextureColor, to_float3_s(2.2f)) * wg;
            bloom = lTextureColor + bloom;

            totalWeight += wg;
            
        }
    }

    bloom /= totalWeight;

    return bloom;
}

__KERNEL__ void GreenfielddioramaJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 pixelSize = 1.0f / iResolution;
    float4 lInputColor0 = _tex2DVecN(iChannel0,uv.x,uv.y,15);

    float3 lBlur  = makeBloom(2.0f, to_float2(0.0f, 0.0f), uv, pixelSize, iChannel1);
           lBlur += makeBloom(3.0f, to_float2(0.3f, 0.0f), uv, pixelSize, iChannel1);
           lBlur += makeBloom(4.0f, to_float2(0.0f, 0.3f), uv, pixelSize, iChannel1);
           lBlur += makeBloom(5.0f, to_float2(0.1f, 0.3f), uv, pixelSize, iChannel1);
           lBlur += makeBloom(6.0f, to_float2(0.2f, 0.3f), uv, pixelSize, iChannel1);

        float4 lOutputColor = to_float4_aw(clamp(pow_f3(lBlur, to_float3_s(1.0f / 2.2f)), to_float3_s(0), to_float3_s(100)), 1.0f);
        fragColor = _mix(lInputColor0, lOutputColor, BLOOM_FRAME_BLEND); 

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1



__DEVICE__ float3 aces_tonemap(float3 color){  
  
  mat3 m1 = to_mat3(
                         0.59719f, 0.07600f, 0.02840f,
                         0.35458f, 0.90834f, 0.13383f,
                         0.04823f, 0.01566f, 0.83777
                   );
  mat3 m2 = to_mat3(
                         1.60475f, -0.10208f, -0.00327f,
                         -0.53108f,  1.10813f, -0.07276f,
                         -0.07367f, -0.00605f,  1.07602
                   );
  float3 v = mul_mat3_f3(m1 , color);    
  float3 a = v * (v + 0.0245786f) - 0.000090537f;
  float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
  return pow_f3(clamp(mul_mat3_f3(m2 , (a / b)), 0.0f, 1.0f), to_float3_s(1.0f / 2.2f));  
}

__DEVICE__ float3 bloomTile(float lod, float2 offset, float2 uv, __TEXTURE2D__ iChannel1)
{
    return swi3(texture(iChannel1, uv * _exp2f(-lod) + offset),x,y,z);
}

__DEVICE__ float3 getBloom(float2 uv, __TEXTURE2D__ iChannel1)
{
    float3 blur = to_float3_s(0.0f);
    float2 lOffsetFix = to_float2(0.00025f, 0.0005f);
    blur = pow_f3(bloomTile(2.0f, to_float2(0.0f, 0.0f) + lOffsetFix, uv, iChannel1),to_float3_s(2.2f))               + blur;
    blur = pow_f3(bloomTile(3.0f, to_float2(0.3f, 0.0f) + lOffsetFix, uv, iChannel1),to_float3_s(2.2f)) * 1.3f        + blur;
    blur = pow_f3(bloomTile(4.0f, to_float2(0.0f, 0.3f) + lOffsetFix, uv, iChannel1),to_float3_s(2.2f)) * 1.6f        + blur;
    blur = pow_f3(bloomTile(5.0f, to_float2(0.1f, 0.3f) + lOffsetFix, uv, iChannel1),to_float3_s(2.2f)) * 1.9f        + blur;
    blur = pow_f3(bloomTile(6.0f, to_float2(0.2f, 0.3f) + lOffsetFix, uv, iChannel1),to_float3_s(2.2f)) * 2.2f        + blur;

    return blur * BLOOM_RANGE;
}

__KERNEL__ void GreenfielddioramaJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(AlphaOn, 0);

    mat3 dummy;

    float2 uv = fragCoord/iResolution;
  
    float4 colBuff = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    //swi3(col,x,y,z) += _powf(getBloom(uv, iChannel1), to_float3_s(2.2f));
    float3 col = swi3(colBuff,x,y,z) + getBloom(uv, iChannel1);
    col = aces_tonemap(col);
    
    fragColor = to_float4_aw(col, 1.0f);
    //fragColor = to_float4_aw(pow_f3(col.aaa  * 0.1f, to_float3_s(2.0f)), 1.0f);

    if(AlphaOn) fragColor *= clamp(colBuff.w, 0.0f, 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}