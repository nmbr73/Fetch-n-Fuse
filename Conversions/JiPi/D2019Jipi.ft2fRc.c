
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Font 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//-----------------------------------------------------
// Created by sebastien durand - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------

// Lightening, essentially based on one of incredible TekF shaders:
// https://www.shadertoy.com/view/lslXRj

//-----------------------------------------------------


// Change this to improve quality (3 is good)

#define ANTIALIASING 1

#define _iTime (1.5f*iTime)

// consts
#define tau  6.2831853f
#define phi  1.61803398875f

// Isosurface Renderer
__DEVICE__ const int g_traceLimit=240;
__DEVICE__ const float g_traceSize=0.004f;

#ifdef XXXX
const float3 g_boxSize = to_float3_s(0.4f);

const float3 g_ptOnBody = to_float3(g_boxSize.x*0.5f, g_boxSize.y*0.15f, g_boxSize.z*0.5f); 
const float3 g_ptOnBody2 = to_float3(g_boxSize.x*0.5f, -g_boxSize.y*0.5f, -g_boxSize.z*0.5f); 
#endif

// Data to read in Buf A
__DEVICE__ float3 g_posBox;
__DEVICE__ mat3 g_rotBox;

__DEVICE__ float3 g_envBrightness;// = to_float3(0.5f,0.6f,0.9f); // Global ambiant color
__DEVICE__ float3 g_lightPos1, g_lightPos2;
__DEVICE__ float3 g_vConnexionPos, g_posFix; 
__DEVICE__ float3 g_vConnexionPos2;
//__DEVICE__ const float3 g_posFix2 = to_float3(0.0f,1.0f,0.0f);
__DEVICE__ float g_rSpring, g_rSpring2;
__DEVICE__ bool g_WithSpring2;

// -----------------------------------------------------------------


__DEVICE__ float hash( float n ) { return fract(_sinf(n)*43758.5453123f); }

// ---------------------------------------------

// Distance from ray to point
__DEVICE__ float dista(float3 ro, float3 rd, float3 p) {
  return length(cross(p-ro,rd));
}

// Intersection ray / sphere
__DEVICE__ bool intersectSphere(in float3 ro, in float3 rd, in float3 c, in float r, out float *t0, out float *t1) {
  ro -= c;
  float b = dot(rd,ro), d = b*b - dot(ro,ro) + r*r;
  if (d<0.0f) return false;
  float sd = _sqrtf(d);
  *t0 = _fmaxf(0.0f, -b - sd);
  *t1 = -b + sd;
  return (*t1 > 0.0f);
}


// -- Modeling Primitives ---------------------------------------------------
// [iq] https://www.shadertoy.com/view/lsccR8
__DEVICE__ float sdfStar5( in float2 p )
{
    // using reflections
    const float2 k1 = to_float2(0.809016994375f, -0.587785252292f); // pi/5
    const float2 k2 = to_float2(-k1.x,k1.y);
    p.x = _fabs(p.x);
    p -= 2.0f*_fmaxf(dot(k1,p),0.0f)*k1;
    p -= 2.0f*_fmaxf(dot(k2,p),0.0f)*k2;
    // draw triangle
    const float2 k3 = to_float2(0.951056516295f,  0.309016994375f); // pi/10
    return dot( to_float2(_fabs(p.x)-0.3f,p.y), k3);
}

__DEVICE__ float sdPlane( float3 p ) {
  return p.y;
}

//const int[] txt = int[] (50,48,49,57,0,2,2,2,2,0);


//----------------------------------------------------------
// Adapted from
//  [iq] https://www.shadertoy.com/view/4lyfzw
//       https://iquilezles.org/articles/distfunctions
//----------------------------------------------------------

__DEVICE__ float opExtrusion( in float3 p, in float d )
{
    float2 w = to_float2( d, p.z );
    return _fmaxf(p.z, _fminf(_fmaxf(w.x,w.y),0.0f) + length(_fmaxf(w,to_float2_s(0.0f))));
}

//----------------------------------------------------------
// FONT
//----------------------------------------------------------


//----------------------------------------------------------
// Adapted from
//  [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
//----------------------------------------------------------

__DEVICE__ float sdFont(float2 p, int c, __TEXTURE2D__ iChannel0) {
    float2 uv = (p + to_float2(float(c%16), float(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel0, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage(float2 p, float scale, __TEXTURE2D__ iChannel0) { 
    p /= scale;
    
    //int txt[] = {74,105,80,105,32,32};
    int txt[] = {110,109,98,114,55,51};
    
    float d;
    d = sdFont(p, txt[0],iChannel0);
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[1],iChannel0));
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[2],iChannel0));
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[3],iChannel0));
    
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[4],iChannel0));

    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[5],iChannel0));
    
    return d*scale;
}


__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0) { 

    p.y -=1.2f;
    float z = _iTime + p.z;
    p.x += 0.01f*_cosf(z);
    float k = 0.1f*z+0.5f*_cosf(z*0.2f)*(0.5f+0.5f*_cosf(z));
    float c = _cosf(4.0f*k);
    float s = _sinf(4.0f*k);
    mat2  m = to_mat2(c,-s,s,c);
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),m));
    float sc = 0.6f+0.5f*_cosf(z);
    float d2D = sdMessage(swi2(p,x,y), sc,iChannel0);
    
    float2 p2 = swi2(p,x,y)-to_float2(0.5f,0.75f);
    p2 = mul_f2_mat2(mul_f2_mat2(p2,m),m);
    sc = 4.0f+5.0f/sc;
    float sc2 = (0.9f +0.1f*sc)*0.5f;
    float fstar = sdfStar5((p2+sc2*to_float2(0.1f,0.15f))*sc)/sc;
    fstar = _fminf(fstar, sdfStar5((p2+sc2*to_float2(0.1f,-0.15f))*sc)/sc);
    fstar = _fminf(fstar, sdfStar5((p2+sc2*to_float2(-0.1f,0.0f))*sc)/sc);
    return opExtrusion(p, _fminf(fstar,d2D));
}



//----------------------------------------------------------------------




__DEVICE__ float isGridLine(float2 p, float2 v) {

    float2 k = smoothstep(to_float2_s(0.0f),to_float2_s(1.0f),abs_f2(mod_f2f2(p+v*0.5f, v)-v*0.5f)/0.01f);
    return k.x * k.y;
}


// render for color extraction
__DEVICE__ float3 colorField(float3 p, float iTime, __TEXTURE2D__ iChannel0) {
    p.y -= 1.2f;
    float z = _iTime + p.z;
    
    p.x += 0.01f*_cosf(z);
    float k = 0.1f*z+0.5f*_cosf(z*0.2f)*(0.5f+0.5f*_cosf(z));
    float c = _cosf(4.0f*k);
    float s = _sinf(4.0f*k);
    mat2  m = to_mat2(c,-s,s,c);
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),m));
    float sc = 0.6f+0.5f*_cosf(z);
    
    float d2D = sdMessage(swi2(p,x,y), sc,iChannel0);
    
    float2 p2 = swi2(p,x,y)-to_float2(0.5f,0.75f);
    p2 = mul_f2_mat2(mul_f2_mat2(p2,m),m);
    float sc1 = 4.0f+5.0f/sc;
    float sc2 = (0.9f +0.1f*sc1)*0.5f;
    float fstar = sdfStar5((p2+sc2*to_float2(0.1f,0.15f))*sc1)/sc1;
    fstar = _fminf(fstar, sdfStar5((p2+sc2*to_float2(0.1f,-0.15f))*sc1)/sc1);
    fstar = _fminf(fstar, sdfStar5((p2+sc2*to_float2(-0.1f,0.0f))*sc1)/sc1);
   
    return (d2D < fstar) ? to_float3_aw(swi2(p,x,y)/sc,z) : to_float3(0.05f,2.05f,z);
}


// ---------------------------------------------------------------------------

__DEVICE__ float SmoothMax( float a, float b, float smoothing ) {
  return a-_sqrtf(smoothing*smoothing + _powf(_fmaxf(0.0f,a-b),2.0f));
}

__DEVICE__ float3 Sky( float3 ray) {
  return g_envBrightness * mix_f3( to_float3_s(0.8f), to_float3_s(0), exp2_f3(-(1.0f/_fmaxf(ray.y,0.01f))*to_float3(0.4f,0.6f,1.0f)) );
}

// -------------------------------------------------------------------


__DEVICE__ float3 Shade( float3 pos, float3 ray, float3 normal, float3 lightDir1, float3 lightDir2, float3 lightCol1, float3 lightCol2, float shadowMask1, float shadowMask2, float distance, float iTime, __TEXTURE2D__ iChannel0 )
{

  float3 ambient = g_envBrightness*_mix( to_float3(0.2f,0.27f,0.4f), to_float3_s(0.4f), (-normal.y*0.5f+0.5f) ); // ambient
    
  // ambient occlusion, based on my DF Lighting: https://www.shadertoy.com/view/XdBGW3
  float aoRange = distance/20.0f;

  float occlusion = _fmaxf( 0.0f, 1.0f - map( pos + normal*aoRange, iTime,iChannel0 )/aoRange ); // can be > 1.0
  occlusion = _exp2f( -2.0f*_powf(occlusion,2.0f) ); // tweak the curve
    
  ambient *= occlusion*0.8f+0.2f; // reduce occlusion to imply indirect sub surface scattering

  float ndotl1 = _fmaxf(0.0f,dot(normal,lightDir1));
  float ndotl2 = _fmaxf(0.0f,dot(normal,lightDir2));
    
  float lightCut1 = smoothstep(0.0f,0.1f,ndotl1);
  float lightCut2 = smoothstep(0.0f,0.1f,ndotl2);

  float3 light = to_float3_s(0);
   

  light += lightCol1*shadowMask1*ndotl1;
  light += lightCol2*shadowMask2*ndotl2;

    
  // And sub surface scattering too! Because, why not?
    float transmissionRange = distance/10.0f; // this really should be constant... right?
    float transmission1 = map( pos + lightDir1*transmissionRange,iTime,iChannel0 )/transmissionRange;
    float transmission2 = map( pos + lightDir2*transmissionRange,iTime,iChannel0 )/transmissionRange;
    
    float3 sslight = lightCol1 * smoothstep(0.0f,1.0f,transmission1) + 
                   lightCol2 * smoothstep(0.0f,1.0f,transmission2);
    float3 subsurface = to_float3(1,0.8f,0.5f) * sslight;

    float specularity = 0.012f; 
    float3 h1 = normalize(lightDir1-ray);
    float3 h2 = normalize(lightDir2-ray);
    
    float specPower;
    specPower = _exp2f(3.0f+5.0f*specularity);

    float3 albedo;

    if (pos.y<-0.48f) {  
        pos.z+=_iTime;
         float f = mod_f( _floor(2.0f*pos.z) + _floor(2.0f*pos.x), 2.0f);
        albedo = (0.4f + 0.1f*f)*to_float3(0.7f,0.6f,0.8f);
        albedo *= 0.2f*(0.3f+0.5f*isGridLine(swi2(pos,x,z), to_float2_s(0.5f)));
        specPower *= 5.0f;

    } else {
        float3 colorId = colorField(pos,iTime,iChannel0);
        float3 col = colorId.y > 0.5f ? to_float3(0.96f,0.96f,0) : 
                     colorId.x < 0.2f ? to_float3(0.6f,0.3f,0.0f) : 
                     colorId.x < 0.7f ? to_float3(0.3f,0.6f,0.0f) : 
                     colorId.x < 1.2f ? to_float3(0.0f,0.6f,0.3f) : 
                     colorId.x < 1.7f ? to_float3(0.0f,0.3f,0.6f) : 
                     colorId.x < 2.2f ? to_float3(1.0f,0.0f,0.3f) : to_float3(0.7f,0.9f,0.0f);
                     
        float grid = 0.7f+0.3f*isGridLine(to_float2_s(colorId.z),to_float2_s(0.1f))*isGridLine(swi2(colorId,x,y),to_float2_s(0.1f));
        albedo = grid * 2.0f*col; 
    }       
    
  float3 specular1 = lightCol1*shadowMask1*_powf(_fmaxf(0.0f,dot(normal,h1))*lightCut1, specPower)*specPower/32.0f;
  float3 specular2 = lightCol2*shadowMask2*_powf(_fmaxf(0.0f,dot(normal,h2))*lightCut2, specPower)*specPower/32.0f;
    
  float3 rray = reflect(ray,normal);
  float3 reflection = Sky( rray );
  
  // specular occlusion, adjust the divisor for the gradient we expect
  float specOcclusion = _fmaxf( 0.0f, 1.0f - map( pos + rray*aoRange, iTime,iChannel0 )/(aoRange*_fmaxf(0.01f,dot(rray,normal))) ); // can be > 1.0
  specOcclusion = _exp2f( -2.0f*_powf(specOcclusion,2.0f) ); // tweak the curve
  
  // prevent sparkles in heavily occluded areas
  specOcclusion *= occlusion;

  reflection *= specOcclusion; // could fire an additional ray for more accurate results
    
  float fresnel = _powf( 1.0f+dot(normal,ray), 5.0f );
  fresnel = _mix( _mix( 0.0f, 0.01f, specularity ), _mix( 0.4f, 1.0f, specularity ), fresnel );

  light += ambient;
  light += subsurface;

  float3 result = light*albedo;
  result = _mix( result, reflection, fresnel );
  result += specular1;
  result += specular2;

  return result;
}


__DEVICE__ float Trace( float3 pos, float3 ray, float traceStart, float traceEnd, float iTime, __TEXTURE2D__ iChannel0 ) {
  float t0=0.0f,t1=100.0f;
  float t2=0.0f,t3=100.0f;
  // trace only if intersect bounding spheres
  
  float t = _fmaxf(traceStart, _fminf(t2,t0));
  traceEnd = _fminf(traceEnd, _fmaxf(t3,t1));
  float h;
  for( int i=0; i < g_traceLimit; i++) {
    h = map( pos+t*ray, iTime,iChannel0 );
    if (h < g_traceSize || t > traceEnd)
      return t>traceEnd?100.0f:t;
    t = t+h*0.45f;
  }
        
  return 100.0f;
}



__DEVICE__ float3 Normal( float3 pos, float3 ray, float t, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0) {

  float pitch = 0.2f * t / iResolution.x;   
  pitch = _fmaxf( pitch, 0.005f );
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x); // tetrahedral offsets
  float3 p1 = pos+swi3(d,x,y,y);
  float3 p2 = pos+swi3(d,y,x,y);
  float3 p3 = pos+swi3(d,y,y,x);

  float f0 = map(p0,iTime,iChannel0), f1 = map(p1,iTime,iChannel0), f2 = map(p2,iTime,iChannel0),  f3 = map(p3, iTime,iChannel0);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}

// Camera
__DEVICE__ float3 Ray( float zoom, in float2 fragCoord, float2 iResolution) {
  return to_float3_aw( fragCoord-iResolution*0.5f, iResolution.x*zoom );
}


// Camera Effects

__DEVICE__ void BarrelDistortion( inout float3 *ray, float degree ){
  // would love to get some disperson on this, but that means more rays
  (*ray).z /= degree;
  (*ray).z = ( (*ray).z*(*ray).z - dot(swi2(*ray,x,y),swi2(*ray,x,y)) ); // fisheye
  (*ray).z = degree*_sqrtf((*ray).z);
}


__DEVICE__ mat2 matRot(in float a) {
    float ca = _cosf(a), sa = _sinf(a);
    return to_mat2(ca,sa,-sa,ca);
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr) {
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}



__KERNEL__ void D2019JipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    
  float2 m = swi2(iMouse,x,y)/iResolution.y - 0.5f;

  float time = 15.0f + _iTime;

  

  // Positon du point lumineux
  float distLightRot =  0.7f;
                              
  float lt = 3.0f*(time-1.0f);
    
   
  g_lightPos1 = g_posBox + distLightRot*to_float3(_cosf(lt*0.5f), 0.4f+0.15f*_sinf(2.0f*lt), _sinf(lt*0.5f));
  g_lightPos2 = g_posBox + distLightRot*to_float3(_cosf(-lt*0.5f), 0.4f+0.15f*_sinf(-2.0f*lt), _sinf(-lt*0.5f));
  
  // Ambiant color
  g_envBrightness = to_float3(0.6f,0.65f,0.9f);
    
  // intensitee et couleur du point
  float3 lightCol1 = to_float3(1.05f,0.95f,0.95f)*0.5f;//*0.2f*g_envBrightness;
  float3 lightCol2 = to_float3(0.95f,1.0f,1.05f)*0.5f;//*0.2f*g_envBrightness;
  
    
  float lightRange1 = 0.4f, 
        lightRange2 = 0.4f; 
  float traceStart = 0.2f;

  float t, s1, s2;
    
  float3 col, colorSum = to_float3_s(0.0f);
  float3 pos;
  float3 ro, rd;
  
#if (ANTIALIASING == 1)  
  int i=0;
  float2 q = (fragCoord)/iResolution;
#else
  for (int i=0;i<ANTIALIASING;i++) {
        float randPix = hash(_iTime);
        float2 subPix = 0.4f*to_float2(_cosf(randPix+6.28f*(float)(i)/(float)(ANTIALIASING)),
                                       _sinf(randPix+6.28f*(float)(i)/(float)(ANTIALIASING)));        
      // camera  
        float2 q = (fragCoord+subPix)/iResolution;
#endif
        float2 p = -1.0f+2.0f*q;
        p.x *= iResolution.x/iResolution.y;

        float dis = 7.0f*(1.2f+0.6f*_cosf(0.41f*_iTime)); 
        ro = to_float3( dis*_cosf(0.2f*time),6.5f, dis*_sinf(0.2f*time) );
        float3 ta = to_float3( -1.0f, 1.0f, 0.0f );

        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta, 0.0f);

        // ray direction
         rd = mul_mat3_f3(ca , normalize( to_float3_aw(swi2(p,x,y),4.5f) ));

        float tGround = -(ro.y+0.5f) / rd.y;
        float traceEnd = _fminf(tGround+1.0f,100.0f); 
        col = to_float3_s(0);
        float3 n;
        t = Trace(ro, rd, traceStart, traceEnd, iTime,iChannel0);
        if ( t > tGround ) {
            pos = ro + rd*tGround;   
            n = to_float3(0,1.0f,0);
            t = tGround;
        } else {
            pos = ro + rd*t;
            n = Normal(pos, rd, t, iTime, iResolution, iChannel0);
        }

        // Shadows
        float3 lightDir1 = g_lightPos1-pos;
        float lightIntensity1 = length(lightDir1);
        lightDir1 /= lightIntensity1;
        
        float3 lightDir2 = g_lightPos2-pos;
        float lightIntensity2 = length(lightDir2);
        lightDir2 /= lightIntensity2;

        s1 = Trace(pos, lightDir1, 0.04f, lightIntensity1, iTime,iChannel0 );
        s2 = Trace(pos, lightDir2, 0.01f, lightIntensity2, iTime,iChannel0 );

        lightIntensity1 = lightRange1/(0.1f+lightIntensity1*lightIntensity1);
        lightIntensity2 = lightRange2/(0.1f+lightIntensity2*lightIntensity2);

        col = Shade(pos, rd, n, lightDir1, lightDir2, lightCol1*lightIntensity1, lightCol2*lightIntensity2,
                    (s1<40.0f)?0.0:1.0, (s2<40.0f)?0.0:1.0, t, iTime,iChannel0 );

#if (ANTIALIASING > 1)  
        colorSum += col;
  }
    
    col = colorSum/(float)(ANTIALIASING);
#endif
    
    // fog
    float f = 100.0f;
    col = mix_f3( to_float3_s(0.8f), col, exp2_f3(-t*to_float3(0.4f,0.6f,1.0f)/f) );
    
    // Draw light
    s1 = 0.5f*_fmaxf(dista(ro, rd, g_lightPos1)+0.05f,0.0f);
    float dist = 0.5f*length(g_lightPos1-ro);
    if (dist < t*0.5f) {
        float3 col1 = 2.0f*lightCol1*_expf( -0.03f*dist*dist );
        float BloomFalloff = 50000.0f;
        col += col1*col1/(1.0f+s1*s1*s1*BloomFalloff);
    }

    s2 = 0.5f*_fmaxf(dista(ro, rd, g_lightPos2)+0.05f,0.0f);
    dist = 0.5f*length(g_lightPos2-ro);
    if (dist < t*0.5f) {
        float3 col2 = 2.0f*lightCol2*_expf( -0.03f*dist*dist );
        float BloomFalloff = 50000.0f;
        col += col2*col2/(1.0f+s2*s2*s2*BloomFalloff);
    }
        
    // Compress bright colours, (because bloom vanishes in vignette)
    float3 c = (col-1.0f);
    c = sqrt_f3(c*c+0.05f); // soft abs
    col = _mix(col,1.0f-c,0.48f); // 0.5f = never saturate, 0.0f = linear
  
  // compress bright colours
  float l = _fmaxf(col.x,_fmaxf(col.y,col.z));//dot(col,normalize(to_float3(2,4,1)));
  l = _fmaxf(l,0.01f); // prevent div by zero, darker colours will have no curve
  float l2 = SmoothMax(l,1.0f,0.01f);
  col *= l2/l;
    
  fragColor =  to_float4_aw(pow_f3(col,to_float3_s(1.0f/2.0f)),1);

  SetFragmentShaderComputedColor(fragColor);
}