
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by sebastien durand - 2015
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------

// Shading, essentially based on one of incredible TekF shaders:
// https://www.shadertoy.com/view/lslXRj

// Pupils effect came from lexicobol shader: [famous iq tutorial]
// https://www.shadertoy.com/view/XsjXz1

// Smooth max from cabbibo shader:
// https://www.shadertoy.com/view/Ml2XDw

//-----------------------------------------------------

// Display distance field in a plane perpendicular to camera crossing pt(0,0,0)
//#define DRAW_DISTANCE


#ifndef DRAW_DISTANCE
// To enable mouse rotation (enable to explore modeling)
  #define MOUSE

// Change this to improve quality (3 is good)
  #define ANTIALIASING 3

#else

// To enable mouse rotation (enable to explore modeling)
   #define MOUSE

// Change this to improve quality (3 is good)
  #define ANTIALIASING 1

#endif

#define ZERO 0 //_fminf(0,iFrame)

// consts
#define tau  6.2831853f
#define phi  1.61803398875f

// Isosurface Renderer
__DEVICE__ const int g_traceLimit=48;
__DEVICE__ const float g_traceSize=0.005f;

// globals
__DEVICE__ const float3 g_nozePos = {0,-0.28f+0.04f,0.47f+0.08f};
__DEVICE__ const float3 g_eyePos = {0.14f,-0.14f,0.29f};
__DEVICE__ const float g_eyeSize = 0.09f;

__DEVICE__ float3 g_envBrightness = {0.5f,0.6f,0.9f}; // Global ambiant color
__DEVICE__ float3 g_lightPos;
__DEVICE__ mat2 ma, mb, mc, g_eyeRot, g_headRotH, g_headRot;
__DEVICE__ float animNoze;
    
__DEVICE__ bool g_bHead = true, g_bBody = true;

// -----------------------------------------------------------------


__DEVICE__ float hash( float n ) { return fract(_sinf(n)*43758.5453123f); }

// Smooth HSV to RGB conversion 
// [iq: https://www.shadertoy.com/view/MsS3Wc]
__DEVICE__ float3 hsv2rgb_smooth(float x, float y, float z) {
  float3 rgb = clamp( abs_f3(mod_f3(x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f);
  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  
  return z * _mix( to_float3_s(1), rgb, y);
}

// Distance from ray to point
__DEVICE__ float dist(float3 ro, float3 rd, float3 p) {
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

__DEVICE__ float udRoundBox(in float3 p,in float3 b, in float r) {
  return length(_fmaxf(abs_f3(p)-b, to_float3_s(0.0f)))-r ;
}

__DEVICE__ float sdCapsule(in float3 p, in float3 a, in float3 b, in float r0, in float r1 ) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f);
    return length( pa - ba*h ) - _mix(r0,r1,h);
}

// capsule with bump in the middle -> use for neck
__DEVICE__ float2 sdCapsule2(in float3 p,in float3 a,in float3 b, in float r0,in float r1,in float bump) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    float dd = bump*_sinf(3.14f*h);  // Little adaptation
    return to_float2(length(pa - ba*h) - _mix(r0,r1,h)*(1.0f+dd), 1.0f); 
}

__DEVICE__ float smin(in float a, in float b, in float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

// Smooth max from cabbibo shader:
// https://www.shadertoy.com/view/Ml2XDw
__DEVICE__ float smax(in float a, in float b, in float k) {
    return _logf(exp(a/k)+_expf(b/k))*k;
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r) {
    return (length(p/r ) - 1.0f) * _fminf(min(r.x,r.y),r.z);
}



// -- Modeling Head ---------------------------------------------------------

__DEVICE__ float dEar(in float3 p, in float scale_ear) {
    float3 p_ear = scale_ear*p;
    swi2S(p_ear,x,y, mul_f2_mat2(swi2(p_ear,x,y) , ma));
    swi2S(p_ear,x,z, mul_f2_mat2(swi2(p_ear,x,z) , ma)); 
    float d = _fmaxf(-sdEllipsoid(p_ear-to_float3(0.005f,0.025f,0.02f), to_float3(0.07f,0.11f,0.07f)), 
                      sdEllipsoid(p_ear, to_float3(0.08f,0.12f,0.09f)));
    swi2S(p_ear,y,z, mul_f2_mat2(swi2(p_ear,y,z) , mb)); 
    d = _fmaxf(p_ear.z, d); 
    d = smin(d, sdEllipsoid(p_ear+to_float3(0.035f,0.045f,0.01f), to_float3(0.04f,0.04f,0.018f)), 0.01f);
    return d/scale_ear;
}

__DEVICE__ float dSkinPart(in float3 pgeneral, in float3 p) {
#ifndef DRAW_DISTANCE    
    if (!g_bHead) return 100.0f;
#endif
    
// Neck    
    float dNeck = sdCapsule2(pgeneral, to_float3(0,-0.24f,-0.11f), to_float3(0,-0.7f,-0.12f), 0.22f, 0.12f, -0.45f).x;
    
    float d = 1000.0f;
   
// Skull modeling -------------------------
    d = sdEllipsoid(p-to_float3(0,0.05f,0.0f), to_float3(0.39f,0.48f,0.46f));          
    d = smin(d, sdEllipsoid(p-to_float3(0.0f,0.1f,-0.15f), to_float3(0.42f,0.4f,0.4f)),0.1f);     
    d = smin(d, udRoundBox(p-to_float3(0,-0.28f,0.2f), to_float3(0.07f,0.05f,0.05f),0.05f),0.4f); // Basic jaw 
  // small forehead correction with a rotated plane
    float3 p_plane = p; 
    swi2S(p_plane,y,z, mul_f2_mat2(swi2(p_plane,y,z) , ma));
    d = smax(d, p_plane.z-0.68f, 0.11f);  

// Neck -----------------------------------
    d = smin(d, dNeck, 0.05f);

// Symetrie -------------------------------
    p.x = _fabs(p.x);

// Eye hole 
    d = smax(d, -sdEllipsoid(p-to_float3(0.12f,-0.16f,0.48f), to_float3(0.09f,0.06f,0.09f)), 0.07f);

// Noze ------------------------------------
    d = smin(d, _fmaxf(-(length(p-to_float3(0.032f,-0.325f,0.45f))-0.028f),   // Noze hole
                    smin(length(p-to_float3(0.043f,-0.29f+0.015f*animNoze,0.434f))-0.01f,  // Nostrils
                    sdCapsule(p, to_float3(0,-0.13f,0.39f), to_float3(0,-0.28f+0.004f*animNoze,0.47f), 0.01f,0.04f), 0.05f)) // Bridge of the nose
            ,0.065f); 
   
// Mouth -----------------------------------    
    d = smin(d, length(p- to_float3(0.22f,-0.34f,0.08f)), 0.17f); // Jaw
    d = smin(d, sdCapsule(p, to_float3(0.16f,-0.35f,0.2f), to_float3(-0.16f,-0.35f,0.2f), 0.06f,0.06f), 0.15f); // Cheeks
   
    d = smin(d, _fmaxf(-length(swi2(p,x,z)-to_float2(0,0.427f))+0.015f,    // Line under the noze
                _fmaxf(-p.y-0.41f+0.008f*animNoze,               // Upper lip
                sdEllipsoid(p- to_float3(0,-0.34f,0.37f), to_float3(0.08f,0.15f,0.05f)))), // Mouth bump
             0.032f);

// Chin -----------------------------------  
    d = smin(d, length(p- to_float3(0,-0.5f,0.26f)), 0.2f);   // Chin
    d = smin(d, length(p- to_float3(0,-0.44f,0.15f)), 0.25f); // Under chin 
  
    //d = smin(d, sdCapsule(p, to_float3(0.24f,-0.1f,0.33f), to_float3(0.08f,-0.05f,0.46f), 0.0f,0.01f), 0.11f); // Eyebrow 
    
// Eyelid ---------------------------------
    float3 p_eye1 = p - g_eyePos;
    swi2S(p_eye1,x,z, mul_f2_mat2(swi2(p_eye1,x,z) , mb));
    
    float3 p_eye2 = p_eye1;
    float d_eye = length(p_eye1) - g_eyeSize;
          
    swi2S(p_eye1,y,z, mul_f2_mat2(swi2(p_eye1,y,z) , g_eyeRot));
    swi2S(p_eye2,z,y, mul_f2_mat2(swi2(p_eye2,z,y) , mc));
    
    float d1 = _fminf(_fmaxf(-p_eye1.y,d_eye - 0.01f),
                      _fmaxf(p_eye2.y,d_eye - 0.005f));
    d = smin(d,d1,0.01f);

// Ear ------------------------------------
    d = smin(d, dEar(to_float3(p.x-0.4f,p.y+0.22f,p.z), 0.9f), 0.01f);    

//  d = _fmaxf(p.y+_cosf(iTime),d); // Cut head  :)
  return d; 
}

__DEVICE__ float dEye(float3 p_eye) {
    swi2S(p_eye,x,z, mul_f2_mat2(swi2(p_eye,x,z) , ma));     
    return length(p_eye) - g_eyeSize;
}

__DEVICE__ float2 min2(in float2 dc1, in float2 dc2) {
  return dc1.x < dc2.x ? dc1 : dc2; 
}

__DEVICE__ float2 dToga(float3 p) {
#ifndef DRAW_DISTANCE        
    if (!g_bBody) return to_float2(100.0f,-1.0f);
#endif
    
    p -= to_float3(0.0f,0.0f,-0.02f);
    
    float d_skin = udRoundBox(p- to_float3(0,-1.22f,-0.12f), to_float3(0.25f,0.5f,0.0f), 0.13f); // Shoulder

    // Scarf
    float d1 = udRoundBox(p - to_float3(-0.05f, -1.02f,-0.1f), to_float3(0.15f, 0.25f, 0.0f), 0.22f);
    float r = length(p-to_float3(1.0f,0,-0.1f))-1.25f;
    d1 = _fmaxf(d1, -r);
    d1 = _fmaxf(d1+0.007f*_sinf(r*42.0f+0.6f), (length(p-to_float3(1.0f,0.1f,-0.1f))-1.62f)); 
    
    // Toga
    float d = 0.004f*smoothstep(0.0f,0.45f, -p.x)*_cosf(r*150.0f)+udRoundBox(p - to_float3(-0.05f, -1.0f,-0.1f), to_float3(0.15f, 0.23f, 0.0f), 0.2f);
    return min2(to_float2(d_skin,2.0f), min2(to_float2(d,0.0f), to_float2(d1, 1.0f)));
}


__DEVICE__ float3 headRotCenter = {0,-0.2f,-0.07f};

__DEVICE__ float map( float3 p) {
    float d = dToga(p).x;
    
    float3 p0 = p;
    p -= headRotCenter;
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , g_headRotH));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , g_headRot));
    p += headRotCenter;
    
    d = _fminf(d, dSkinPart(p0,p));
    p.x = _fabs(p.x);
    d = _fminf(d, dEye(p- g_eyePos));
    return d;
}


// render for color extraction
__DEVICE__ float colorField(float3 p) {
    float2 dc = dToga(p);
    float3 p0 = p;
    p -= headRotCenter;
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , g_headRotH));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , g_headRot));
    p += headRotCenter;

    dc = min2(dc, to_float2(dSkinPart(p0,p), 2.0f));
         
    p.x = _fabs(p.x);
  return min2(dc, to_float2(dEye(p - g_eyePos), 3.0f)).y;
}


// ---------------------------------------------------------------------------

__DEVICE__ float SmoothMax( float a, float b, float smoothing ) {
  return a-_sqrtf(smoothing*smoothing + _powf(_fmaxf(0.0f,a-b),2.0f));
}

__DEVICE__ float3 Sky( float3 ray) {
  return g_envBrightness* mix_f3( to_float3_s(0.8f), to_float3_s(0), exp2_f3(-(1.0f/_fmaxf(ray.y,0.01f))*to_float3(0.4f,0.6f,1.0f)) );
}


// -------------------------------------------------------------------
// pupils effect came from lexicobol shader:
// https://www.shadertoy.com/view/XsjXz1
// -------------------------------------------------------------------

__DEVICE__ float3 hash3( float2 p )
{
  float3 q = to_float3( dot(p,to_float2(127.1f,311.7f)), 
                          dot(p,to_float2(269.5f,183.3f)), 
                          dot(p,to_float2(419.2f,371.9f)) );
  return fract(sin_f3(q)*43758.5453f);
}

__DEVICE__ float noise(float2 x)
{
    float2 p = _floor(x), f = fract_f2(x);
    float va = 0.0f, wt = 0.0f;
    for( int j=-2+ZERO; j<=2; j++ )
    for( int i=-2+ZERO; i<=2; i++ ) {
      float2 g = to_float2(i,j);
      float3 o = hash3( p + g )*to_float3(0,0,1);
      float2 r = g - f + swi2(o,x,y);
      float d = dot(r,r),
      ww = 1.0f-smoothstep(0.0f,1.414f,_sqrtf(d));
      va += o.z*ww;
      wt += ww;
    }
  
    return va/wt;
}





__DEVICE__ float fbm( float2 p)
{
  mat2 m = to_mat2(0.8f,0.6f,-0.6f,0.8f);
  float f;
    f  = 0.5f    * noise(p); p = mul_f2_mat2(p,m)* 2.02f;
    f += 0.25f   * noise(p); p = mul_f2_mat2(p,m)* 2.03f;
    f += 0.125f  * noise(p); p = mul_f2_mat2(p,m)* 2.01f;
    f += 0.0625f * noise(p); p = mul_f2_mat2(p,m)* 2.04f;
    f /= 0.9375f;
    return f;
}


__DEVICE__ float3 iris(float2 p, float open)
{
    float
     r = _sqrtf( dot (p,p)),
     r_pupil = 0.15f + 0.15f*smoothstep(0.5f,2.0f,open),
     a = _atan2f(p.y, p.x),
     ss = 0.5f,
     anim = 1.0f + 0.05f*ss* clamp(1.0f-r, 0.0f, 1.0f);
  
     r *= anim;
     float3 col = to_float3_s(1);
        
     if (r< 0.8f) {
        col = _mix(to_float3(0.12f, 0.6f, 0.57f), to_float3(0.12f,0.52f, 0.60f), fbm(5.0f * p)); // iris bluish green mix
        col = _mix(col, to_float3(0.6f,0.44f,0.12f), 1.0f - smoothstep( r_pupil, r_pupil+0.2f, r)); //yellow
        
        a += 0.05f * fbm(20.0f*p);
        col = _mix(col, to_float3_s(1), smoothstep(0.3f, 1.0f, fbm(to_float2(5.0f * r, 20.0f * a))));  // white highlight
        col = _mix(col, to_float3(0.6f,0.44f,0.12f), smoothstep(0.3f, 1.0f, fbm(to_float2(5.0f * r, 5.0f * a)))); // yellow highlight
        col *= 1.0f - smoothstep(0.5f, 1.0f, fbm(to_float2(5.0f * r, 15.0f * a))); // dark highlight
        col *= 1.0f - 0.6f*smoothstep(0.55f, 0.8f, r); //dark at edge
        col *= smoothstep( r_pupil, r_pupil + 0.05f, r); //pupil; 
        col = 0.5f*_mix(col, to_float3_s(1), smoothstep(0.75f, 0.8f, r));
    }
    
  return col;
}

// -------------------------------------------------------------------



__DEVICE__ float3 Shade( float3 pos, float3 ray, float3 normal, float3 lightDir1, float3 lightDir2, float3 lightCol1, float3 lightCol2, float shadowMask1, float shadowMask2, float distance )
{
  float colorId = colorField(pos);
    
  float3 ambient = g_envBrightness*_mix( to_float3(0.2f,0.27f,0.4f), to_float3_s(0.4f), (-normal.y*0.5f+0.5f) ); // ambient
    
  // ambient occlusion, based on my DF Lighting: https://www.shadertoy.com/view/XdBGW3
  float aoRange = distance/20.0f;
  
  float occlusion = _fmaxf( 0.0f, 1.0f - map( pos + normal*aoRange )/aoRange ); // can be > 1.0
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
  float transmission1 = map( pos + lightDir1*transmissionRange )/transmissionRange;
  float transmission2 = map( pos + lightDir2*transmissionRange )/transmissionRange;
   
  float3 sslight = lightCol1 * smoothstep(0.0f,1.0f,transmission1) + lightCol2 * smoothstep(0.0f,1.0f,transmission2);
  float3 subsurface = to_float3(1,0.8f,0.5f) * sslight;

  float specularity = 0.2f; 
  float3 h1 = normalize(lightDir1-ray);
  float3 h2 = normalize(lightDir2-ray);
    
  float specPower;
  specPower = _exp2f(3.0f+5.0f*specularity);

  float3 p = pos;
  p -= headRotCenter;
  swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , g_headRotH));
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , g_headRot));
  p += headRotCenter;

  float3 albedo;
  if (colorId < 0.5f) {  
        // Toge 1
        albedo = to_float3(1.0f,0.6f,0.0f);
        specPower = _sqrtf(specPower);
  } else if (colorId < 1.5f) {  
        // Toge 2
        albedo = to_float3(0.6f,0.3f,0.0f);
        specPower = _sqrtf(specPower);
  } else if (colorId < 2.5f) {
         // Skin color
        albedo = to_float3(0.6f,0.43f,0.3f); 
        float v = 1.0f;
        if (p.z>0.0f) {
          v = smoothstep(0.02f,0.03f, length(swi2(p,x,y)-to_float2(0,-0.03f)));
        }
        albedo = _mix(to_float3(0.5f,0,0), albedo, v);
         
  } else {
        // Eye
        if (p.z>0.0f) {
            float3 g_eyePosloc = g_eyePos;
            g_eyePosloc.x *= sign_f(p.x);
            float3 pe = p - g_eyePosloc;
 
            // Light point in face coordinates
            float3 g_lightPos2 = g_lightPos - headRotCenter;
            swi2S(g_lightPos2,y,z, mul_f2_mat2(swi2(g_lightPos2,y,z) , g_headRotH));
            swi2S(g_lightPos2,x,z, mul_f2_mat2(swi2(g_lightPos2,x,z) , g_headRot));
            g_lightPos2 += headRotCenter;

            float3 dir = normalize(g_lightPos2-g_eyePosloc);
            
            float a = clamp(_atan2f(-dir.x, dir.z), -0.6f,0.6f), 
            ca = _cosf(a), sa = _sinf(a);
            swi2S(pe,x,z, mul_f2_mat2(swi2(pe,x,z) , to_mat2(ca, sa, -sa, ca)));

            float b = clamp(_atan2f(-dir.y, dir.z), -0.3f,0.3f), 
            cb = _cosf(b), sb = _sinf(b);
            swi2S(pe,y,z, mul_f2_mat2(swi2(pe,y,z) , to_mat2(cb, sb, -sb, cb)));
            
            albedo = (pe.z>0.0f) ? iris(17.0f*(swi2(pe,x,y)), length(g_lightPos2-g_eyePosloc)) : to_float3_s(1);
        }
        specPower *= specPower;
     }
float zzzzzzzzzzzzzzzzzzzzzzz;    
  float3 specular1 = lightCol1*shadowMask1*_powf(_fmaxf(0.0f,dot(normal,h1))*lightCut1, specPower)*specPower/32.0f;
  float3 specular2 = lightCol2*shadowMask2*_powf(_fmaxf(0.0f,dot(normal,h2))*lightCut2, specPower)*specPower/32.0f;
    
  float3 rray = reflect(ray,normal);
  float3 reflection = Sky( rray );
  
  // specular occlusion, adjust the divisor for the gradient we expect
  float specOcclusion = _fmaxf( 0.0f, 1.0f - map( pos + rray*aoRange )/(aoRange*_fmaxf(0.01f,dot(rray,normal))) ); // can be > 1.0
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


__DEVICE__ float Trace( float3 pos, float3 ray, float traceStart, float traceEnd )
{
    float t0=0.0f,t1=100.0f;
    float t2=0.0f,t3=100.0f;
    // trace only if intersect bounding spheres
#ifndef DRAW_DISTANCE       
    g_bHead = intersectSphere(pos, ray, to_float3(0,-0.017f,0.02f), 0.65f, &t0, &t1);
    g_bBody = intersectSphere(pos, ray, to_float3(0,-0.7f,0.02f), 0.5f, &t2, &t3);
    if (g_bHead || g_bBody) 
#endif        
    {   
            float t = _fmaxf(traceStart, _fminf(t2,t0));
            traceEnd = _fminf(traceEnd, _fmaxf(t3,t1));
            float h;
            for( int i=ZERO; i < g_traceLimit; i++) {
                h = map( pos+t*ray );
                if (h < g_traceSize || t > traceEnd)
                    return t>traceEnd?100.0f:t;
                t = t+h;
            }
      }
    
  return 100.0f;
}



__DEVICE__ float3 Normal( float3 pos, float3 ray, float t, float2 iResolution) {

  float pitch = 0.2f * t / iResolution.x;
    
//#ifdef FAST
//  // don't sample smaller than the interpolation errors in Noise()
  pitch = _fmaxf( pitch, 0.005f );
//#endif
  
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x); // tetrahedral offsets
  float3 p1 = pos+swi3(d,x,y,y);
  float3 p2 = pos+swi3(d,y,x,y);
  float3 p3 = pos+swi3(d,y,y,x);
  
  float f0 = map(p0);
  float f1 = map(p1);
  float f2 = map(p2);
  float f3 = map(p3);
  
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  //return normalize(grad);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}


// Camera
__DEVICE__ float3 Ray( float zoom, in float2 fragCoord, float2 iResolution) {
  return to_float3_aw( fragCoord-iResolution*0.5f, iResolution.x*zoom );
}

__DEVICE__ float3 Rotate( inout float3 *v, float2 a ) {
  float4 cs = to_float4( _cosf(a.x), _sinf(a.x), _cosf(a.y), _sinf(a.y) );
  
  swi2S(*v,y,z, swi2(*v,y,z)*cs.x+swi2(*v,z,y)*cs.y*to_float2(-1,1));
  swi2S(*v,x,z, swi2(*v,x,z)*cs.z+swi2(*v,z,x)*cs.w*to_float2(1,-1));
  
  float3 p;
  swi2S(p,x,z, to_float2( -cs.w, -cs.z )*cs.x);
  p.y = cs.y;
  
  return p;
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

#ifdef DRAW_DISTANCE

// ---------------------------------------------
__DEVICE__ const float3 ep2 = {0.001f,0.0f,0.0f}; 

__DEVICE__ float3 gradAt(in float3 p) {
  return to_float3(
        map(p+swi3(ep2,x,y,y)) - map(p-swi3(ep2,x,y,y)),
        map(p+swi3(ep2,y,x,y)) - map(p-swi3(ep2,y,x,y)),
        map(p+swi3(ep2,y,y,x)) - map(p-swi3(ep2,y,y,x)));
}

__DEVICE__ float isoline(float3 p, float3 n, float pas, float tickness, float2 iResolution) {
    float dist = map(p);
    float3 grad = (dist - to_float3(map(p-swi3(ep2,x,y,y)), map(p-swi3(ep2,y,x,y)), map(p-swi3(ep2,y,y,x))));
    grad -= n*dot(grad,n);
    float k = length(grad);
    if (k != 0.0f) {
        k = (iResolution.x*ep2.x)/(k*tickness);
      float v1 = _fabs(mod_f(dist+pas*0.5f, pas)-pas*0.5f)*k/3.0f;
      float v2 = _fabs(mod_f(dist+pas*2.0f, pas*4.0f)-pas*2.0f)*k/4.0f;
      float v3 = _fabs(dist)*k/8.0f;
      return smoothstep(0.01f,0.99f, v3) * (0.5f+0.5f*smoothstep(0.01f,0.99f, v1)) * smoothstep(0.01f,0.99f, v2);
    } 
    return 1.0f;
}

__DEVICE__ float3 heatmapGradient(in float t) {
    return clamp((_powf(t, 1.5f) * 0.8f + 0.2f) * to_float3(smoothstep(0.0f, 0.35f, t) + t * 0.5f, smoothstep(0.5f, 1.0f, t), _fmaxf(1.0f - t * 1.7f, t * 7.0f - 6.0f)), 0.0f, 1.0f);
}

__DEVICE__ bool intersectPlane(in float3 ro, in float3 rd, in float3 pt, in float3 n, out float *t) {
  float k = dot(rd, n);
  if (k == 0.0f) return false;
  *t = (dot(pt, n)-dot(ro,n))/k;
  return *t>0.0f;
}
#endif



// adapted from  BigWIngs : https://www.shadertoy.com/view/XlcSzM
__DEVICE__ float3 background(float3 upCol, float3 r, float2 uv, float iTime) {
    float u = dot(r, to_float3(0,1,0))*0.5f+0.5f;
    float3 col = upCol*u*2.0f;
    float t = iTime*0.1f,     
       x = _atan2f(r.x, r.z),          
       y = 3.1415f*0.5f-_acosf(r.y),
       a = _sinf(r.x),
       beam = clamp(_sinf(10.0f*x+a*y*5.0f+t),0.0f,1.0f) * clamp(_sinf(7.0f*x+a*y*3.5f-t),0.0f,1.0f)
            + clamp(_sinf(42.0f*x+a*y*21.0f-t),0.0f,1.0f) * clamp(_sinf(34.0f*x+a*y*17.0f+t),0.0f,1.0f);
    col *= 1.0f+beam*0.05f;
    return col;
}

// -------------------------------------------




__KERNEL__ void MeditationForDummiesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
  CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
  
  const float
    a_eyeClose = 0.55f, 
    a_eyeOpen = -0.3f;


  const float 
    t_apear = 5.0f,
    t_noze = t_apear+8.0f, 
    t_openEye = t_noze + 1.0f,
    t_g_headRot = t_openEye + 4.5f,
    t_rotDown = t_g_headRot + 3.5f,
    t_outNoze = t_rotDown + 3.0f,
    t_night = t_outNoze + 4.0f,
    t_colorfull = t_night + 5.0f,
    t_disapear = t_colorfull + 2.0f,
    t_closeEye = t_disapear + 3.0f;

    float st = 1.2f; // speed coeff
    float time = mod_f(iTime*st+55.0f, 62.831f);
    
// constantes
    ma = matRot(-0.5f);
    mb = matRot(-0.15f);
    mc = matRot(-0.6f);

// Eye blink
    float a_PaupieresCligne = _mix(a_eyeOpen,a_eyeClose, hash(_floor(time*10.0f))>.98?2.*_fabs(fract(20.0f*time)-0.5f):0.);    
    float a_Paupieres = _mix(a_eyeClose, 0.2f, smoothstep(t_openEye, t_openEye+2.0f, time));    
    a_Paupieres = _mix(a_Paupieres, a_PaupieresCligne, smoothstep(t_rotDown, t_rotDown+1.0f, time));
    a_Paupieres = _mix(a_Paupieres, a_eyeClose, smoothstep(t_closeEye, t_closeEye+3.0f, time));

    g_eyeRot = matRot(a_Paupieres);

// rotation de la tete 
    float a_headRot = 0.1f, a_headRotH = 0.1f;
  
    a_headRot = _mix(0.0f, 0.2f*_cosf(20.0f*(time-t_g_headRot)), smoothstep(t_g_headRot, t_g_headRot+0.5f, time)-smoothstep(t_g_headRot+1.0f, t_g_headRot+1.5f, time));
    a_headRotH = _mix(-0.1f, 0.2f*_sinf(20.0f*(time-t_g_headRot)), smoothstep(t_g_headRot+1.5f, t_g_headRot+2.0f, time)-smoothstep(t_g_headRot+2.0f, t_g_headRot+2.5f, time));
    a_headRotH = _mix(a_headRotH, 0.3f, smoothstep(t_g_headRot+2.6f, t_rotDown, time));
    a_headRotH = _mix(a_headRotH, -0.2f, smoothstep(t_outNoze, t_outNoze+2.0f, time));
    a_headRotH = _mix(a_headRotH, -0.1f, smoothstep(t_closeEye, t_closeEye+3.0f, time));
    
    g_headRot = matRot(a_headRot); 
    g_headRotH = matRot(a_headRotH); 
    mat2 g_headRot2 = matRot(-a_headRot); 
    mat2 g_headRotH2 = matRot(-a_headRotH); 
float IIIIIIIIIIIIIIIIIIII;
// Position du nez
    animNoze = smoothstep(t_openEye+2.0f, t_openEye+2.1f, time) - smoothstep(t_openEye+2.1f, t_openEye+2.3f, time)
             + smoothstep(t_openEye+2.5f, t_openEye+2.6f, time) - smoothstep(t_openEye+2.6f, t_openEye+2.8f, time);
    
    float3 p_noze = g_nozePos - headRotCenter;
    swi2S(p_noze,x,z, mul_f2_mat2(swi2(p_noze,x,z) , g_headRot2));
    swi2S(p_noze,y,z, mul_f2_mat2(swi2(p_noze,y,z) , g_headRotH2));
    p_noze += headRotCenter;

// Positon du point lumineux
    float distLightRot = _mix(1.0f, 0.4f, smoothstep(3.0f,t_noze-2.0f, time));
    float3 centerLightRot = to_float3(0,0.2f,1.7f);
                              
    float lt = 3.0f*(time-1.0f);
    float3 lightRot = centerLightRot + distLightRot*to_float3(_cosf(lt*0.5f), 0.025f*_sinf(2.0f*lt), _sinf(lt*0.5f));
  
    g_lightPos = _mix(lightRot, p_noze+0.004f*animNoze, smoothstep(t_noze, t_noze + 1.0f, time));
    g_lightPos = _mix(g_lightPos, lightRot, smoothstep(t_outNoze,t_outNoze+2.0f, time));

// intensitee et couleur du point
    float lightAppear = smoothstep(t_apear, t_apear+2.0f, time)-smoothstep(t_disapear, t_disapear+3.0f, time);
    float3 lightCol2 = hsv2rgb_smooth(0.6f*(_floor(st*iTime/62.831f))+0.04f,1.0f,0.5f);
    
  // Ambiant color
    g_envBrightness = _mix(to_float3(0.6f,0.65f,0.9f), to_float3(0.02f,0.03f,0.05f), smoothstep(t_night, t_night+3.0f, time));
    g_envBrightness = _mix(g_envBrightness, lightCol2, smoothstep(t_colorfull, t_colorfull+1.0f, time));
    g_envBrightness = _mix(g_envBrightness, to_float3(0.6f,0.65f,0.9f), smoothstep(t_disapear+5.0f, t_disapear+9.0f, time));
  

    float3 lightDir1 = normalize(to_float3(0.5f,1.5f,1.5f));
    float3 lightCol1 = to_float3(1.1f,1.0f,0.9f)*0.7f*g_envBrightness;

    float lightRange2 = 0.4f; 
    float traceStart = 0.0f;
    float traceEnd = 40.0f;

    float3 col, colorSum = to_float3_s(0.0f);

#if (ANTIALIASING == 1)  
  int i=0;
#else
  for (int i=ZERO;i<ANTIALIASING;i++) {
#endif
    col = to_float3_s(0);

        // Camera    

#if (ANTIALIASING == 1)          
        float randPix = 0.0f;
#else 
        float randPix = hash(iTime); // Use frame rate to improve antialiasing ... not sure of result
#endif        
    float2 subPix = 0.4f*to_float2(_cosf(randPix+6.28f*(float)(i)/(float)(ANTIALIASING)),
                                   _sinf(randPix+6.28f*(float)(i)/(float)(ANTIALIASING)));
    float3 ray = Ray(2.0f,fragCoord+subPix,iResolution);
    
    BarrelDistortion(&ray, 0.5f );
    
    ray = normalize(ray);
    float3 localRay = ray;
    float2 mouse = to_float2_s(0);
  #ifdef MOUSE
    if ( iMouse.z > 0.0f )
      mouse = 0.5f-swi2(iMouse,y,x)/swi2(iResolution,y,x);
      float3 pos = 5.0f*Rotate(&ray, to_float2(-0.1f,1.0f+time*0.1f)+to_float2(-1.0f,-3.3f)*mouse ) + to_float3_aw(ViewXY,ViewZ);        
  #else    
    float3 pos = to_float3(0,0,0.6f) + 5.5f*Rotate(&ray, to_float2(-0.1f,1.0f+time*0.1f)) + to_float3_aw(ViewXY,ViewZ);        
  #endif

    
#ifdef DRAW_DISTANCE    
        float tPlane;
        if (intersectPlane(pos, ray, to_float3_s(0.0f), -ray, &tPlane)) {
            float3 p = pos+tPlane*ray;
            float dist = map(p);
            if (dist > 0.0f) {
              col = 0.1f+0.8f*heatmapGradient(clamp(1.2f*dist,0.0f,10.0f));   
            }
            else {
              //col.brg = 0.1f+0.8f*heatmapGradient(clamp(-1.2f*dist,0.0f,10.0f));     
              swi3S(col,z,x,y, 0.1f+0.8f*heatmapGradient(clamp(-1.2f*dist,0.0f,10.0f)));
            }
            col *= isoline(p, -ray, 0.05f, 1.0f, iResolution); 
          
        } 
        else {
            col = to_float3_s(0);
        }
#else            
    float t = Trace(pos, ray, traceStart, traceEnd );
    if ( t < 10.0f )
    {           
      float3 p = pos + ray*t;
      
      // Shadows
      float3 lightDir2 = g_lightPos-p;
      float lightIntensity2 = length(lightDir2);
      lightDir2 /= lightIntensity2;
      lightIntensity2 = lightAppear*lightRange2/(0.1f+lightIntensity2*lightIntensity2);
      
      float s1 = 0.0f;
      s1 = Trace(p, lightDir1, 0.05f, 4.0f );
      float s2 = 0.0f;
      s2 = Trace(p, lightDir2, 0.05f, 4.0f );
      
      float3 n = Normal(p, ray, t, iResolution);
      col = Shade(p, ray, n, lightDir1, lightDir2,
            lightCol1, lightCol2*lightIntensity2,
            (s1<20.0f)?0.0f:1.0f, (s2<20.0f)?0.0f:1.0f, t );
      
      // fog
      float f = 200.0f;
      col = mix_f3( to_float3_s(0.8f), col, exp2_f3(-t*to_float3(0.4f,0.6f,1.0f)/f) );
    }
    else
    {
      col = Sky( ray );
            col = background(col, ray, subPix, iTime);
        //    col *= 0.8f+0.2f*b;
    }
        // Draw light
    float s1 = _fmaxf(dist(pos, ray, g_lightPos)+0.03f,0.0f);
    float dist = length(g_lightPos-pos);
    if (dist < t) {
      float3 col2 = lightCol2*2.5f*_expf( -0.01f*dist*dist );
      float BloomFalloff = 15000.0f; //_mix(1000.0f,5000.0f, Anim);
      col = col *(1.0f-lightAppear) + lightAppear*_mix(col2, col, smoothstep(0.037f,0.047f, s1));
      col += lightAppear*col2*col2/(1.0f+s1*s1*s1*BloomFalloff);
    }
        
#endif      
    

  // Post traitments -----------------------------------------------------    
    // Vignetting:
    col *= smoothstep(0.5f, 0.0f, dot(swi2(localRay,x,y),swi2(localRay,x,y)) );
      
    colorSum += col;
        
#if (ANTIALIASING > 1)  
  }
    
    col = colorSum/(float)(ANTIALIASING);
#else
  col = colorSum;
#endif
    

    // Compress bright colours, (because bloom vanishes in vignette)
    float3 c = (col-1.0f);
    c = sqrt_f3(c*c+0.05f); // soft abs
    col = _mix(col,1.0f-c,0.48f); // 0.5f = never saturate, 0.0f = linear
  
  // compress bright colours
  float l = _fmaxf(col.x,_fmaxf(col.y,col.z));//dot(col,normalize(to_float3(2,4,1)));
  l = _fmaxf(l,0.01f); // prevent div by zero, darker colours will have no curve
  float l2 = SmoothMax(l,1.0f,0.01f);
  col *= l2/l;
    
  fragColor =  to_float4_aw(pow_f3(col,to_float3_s(1.0f/1.6f)),1);

  SetFragmentShaderComputedColor(fragColor);
}