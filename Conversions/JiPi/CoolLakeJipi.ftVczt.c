
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}



// https://www.shadertoy.com/view/7tKyz3 Cool Lake [FORK] with Caustics
// Combined Xor's Cool Lake https://www.shadertoy.com/view/MlXGWf
// with Dave_Hoskins' Tileable Water Caustic https://www.shadertoy.com/view/MdlXz8

// (hastily jumbled together by jt)


// Caustics from https://www.shadertoy.com/view/MdlXz8 Tileable Water Caustic by Dave_Hoskins 

// Redefine below to see the tiling...
//#define SHOW_TILING

#define TAU 6.28318530718f
#define MAX_ITER 5

__DEVICE__ float3 caustics(float2 fragCoord, float iTime, float2 iResolution ) 
{
  float time = iTime;// * 0.5f+23.0f;
    // uv should be the 0-1 uv of texture...
  float2 uv = fragCoord / iResolution;
    
#ifdef SHOW_TILING
  float2 p = mod_f2(uv*TAU*2.0f, TAU)-250.0f;
#else
  float2 p = mod_f2(uv*TAU, TAU)-250.0f;
#endif
  float2 i = (p);
  float c = 1.0f;
  float inten = 0.005f;
float zzzzzzzzzzzzzzzzzz;
  for (int n = 0; n < MAX_ITER; n++) 
  {
    float t = time * (1.0f - (3.5f / (float)(n+1)));
    i = p + to_float2(_cosf(t - i.x) + _sinf(t + i.y), _sinf(t - i.y) + _cosf(t + i.x));
    c += 1.0f/length(to_float2(p.x / (_sinf(i.x+t)/inten),p.y / (_cosf(i.y+t)/inten)));
  }
  c /= (float)(MAX_ITER);
  c = 1.17f-_powf(c, 1.4f);
  float3 colour = to_float3_s(_powf(_fabs(c), 8.0f));
  colour = clamp(colour + to_float3(0.0f, 0.35f, 0.5f), 0.0f, 1.0f);
    

  #ifdef SHOW_TILING
  // Flash tile borders...
  float2 pixel = 2.0f / iResolution;
  uv *= 2.0f;

  float f = _floor(mod_f(iTime*0.5f, 2.0f));   // Flash value.
  float2 first = step(pixel, uv) * f;         // Rule out first screen pixels and flash.
  uv  = step(fract_f2(uv), pixel);        // Add one line of pixels per tile.
  colour = _mix(colour, to_float3(1.0f, 1.0f, 0.0f), (uv.x + uv.y) * first.x * first.y); // Yellow line
  
  #endif
  return colour;
}


// Cool Lake from https://www.shadertoy.com/view/MlXGWf Xor - Cool Lake by Xor

#define time iTime
__DEVICE__ float sfract(float n)
{
    return smoothstep(0.0f,1.0f,fract(n));
}
__DEVICE__ float rand(float2 n)
{
   return fract(_fabs(_sinf(dot(n,to_float2(5.3357f,-5.8464f))))*256.75f+0.325f);   
}

__DEVICE__ float noise(float2 n)
{
    float h1 = _mix(rand(to_float2(_floor(n.x),_floor(n.y))),rand(to_float2(_ceil(n.x),_floor(n.y))),sfract(n.x));
    float h2 = _mix(rand(to_float2(_floor(n.x),_ceil(n.y))),rand(to_float2(_ceil(n.x),_ceil(n.y))),sfract(n.x));
    float s1 = _mix(h1,h2,sfract(n.y));
    return s1;
}
__DEVICE__ void doCamera( out float3 *camPos, out float3 *camTar, in float time, in float2 mouse )
{
    float2 dir = (mouse*to_float2(1.0f,-0.5f)+to_float2(0.0f,0.75f))*6.28f;
    float3 pos = to_float3(0.0f,0.5f,0.0f);//to_float3_aw(noise((time/32.0f)*to_float2(1.0f,0.0f)),0.05f,noise((time/32.0f)*to_float2(0.0f,1.0f)))*20.0f;
    *camPos = pos;//to_float3(_cosf(time/4.0f)*8.0f,1.0f,_sinf(time/4.0f)*8.0f);
    *camTar = pos+to_float3(_cosf(dir.x)*_cosf(dir.y),_sinf(dir.y),_sinf(dir.x)*_cosf(dir.y));
}
__DEVICE__ float3 doBackground( in float3 dir, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, float ratio)
{
    float sky = dot(dir,to_float3(0.0f,-1.0f,0.0f))*0.5f+0.5f;
    float sun = _powf(dot(dir,normalize(to_float3(1.0f,0.7f,0.9f)))*0.5f+0.5f,32.0f);
    float2 p = to_float2(dir.x+dir.z,dir.y-dir.z);
    float clouds = noise(p*8.0f)*noise(p*9.0f)*noise(p*10.0f)*noise(p*11.0f)*sky;
    float3 total = to_float3(sky*0.6f+0.05f+sun+clouds,sky*0.8f+0.075f+_powf(sun,1.5f)+clouds,sky+0.2f+_powf(sun,4.0f)+clouds);
    
    float2 tuv = (swi2(dir,x,z))/dir.y;
    tuv.x /= ratio;
    //float3 ground = swi3(texture(iChannel0,(swi2(dir,x,z))/dir.y),x,x,x)*to_float3(1.1f,1.0f,0.9f);
    float3 ground = swi3(texture(iChannel0,tuv),x,x,x)*to_float3(1.1f,1.0f,0.9f);
    
    ground *= caustics(1000.0f*(swi2(dir,x,z))/dir.y, iTime, iResolution);
    ground += 0.5f * swi3(caustics(1000.0f*(swi2(dir,x,z))/dir.y,iTime, iResolution),x,x,x);//rrr;
    return _mix(total,ground,clamp((sky-0.6f)*64.0f,0.0f,1.0f));
}
    
__DEVICE__ float doModel( float3 pos, float time )
{
    float3 p = pos+to_float3(time*0.2f,0.0f,0.0f)+to_float3(noise(swi2(pos,x,z)),0.0f,noise(swi2(pos,x,z)+8.0f))*0.2f;//Distort coordinates
    float height = 0.1f*_powf(noise(swi2(p,x,z)+to_float2(time*0.7f,time*0.6f))*0.5f+noise(swi2(p,x,z)*8.0f+to_float2_s(time))*0.35f+noise(swi2(p,x,z)*16.0f+to_float2(0.0f,time*0.5f))*0.1f+noise(swi2(p,x,z)*24.0f)*0.05f,0.25f);
    float model = p.y-height;
    return model;
}
__DEVICE__ float3 doMaterial(in float3 rd, in float3 nor, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, float refmul, float refoff, float ratio )
{
    float3 ref = doBackground(reflect(rd,nor), iTime, iResolution, iChannel0, ratio);
    return _mix(doBackground(_refract_f3(rd,nor,0.8f, refmul, refoff), iTime, iResolution,iChannel0, ratio),ref,clamp(dot(ref,to_float3_s(1.0f/3.0f))*1.5f,0.0f,1.0f));
}

__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, float iTime )
{
    float res = 1.0f;
    float t = 0.5f;                 // selfintersection avoidance distance
    float h = 1.0f;
    for( int i=0; i<40; i++ )         // 40 is the max numnber of raymarching steps
    {
        h = doModel(ro + rd*t, iTime);
        res = _fminf( res, 64.0f*h/t );   // 64 is the hardness of the shadows
        t += clamp( h, 0.02f, 2.0f );   // limit the max and min stepping distances
    }
    return clamp(res,0.0f,1.0f);
}

__DEVICE__ float3 doFog( in float3 rd, in float dis, in float3 mal, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, float ratio)
{
    float3 col = mal;
    col = _mix(doBackground(rd, iTime, iResolution, iChannel0, ratio),col,1.0f-clamp(dis*dis/90.0f,0.0f,1.0f));

    return col;
}

__DEVICE__ float calcIntersection( in float3 ro, in float3 rd, float iTime )
{
    const float maxd = 10.0f;           // max trace distance
    const float precis = 0.001f;        // precission of the intersection
    float h = precis*2.0f;
    float t = 0.0f;
    float res = -1.0f;
    for( int i=0; i<90; i++ )          // max number of raymarching iterations is 90
    {
        if( h<precis||t>maxd ) break;
        h = doModel( ro+rd*t, iTime );
        t += h*0.8f;
    }

    if( t<maxd ) res = t;
    return res;
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime )
{
    const float eps = 0.002f;             // precision of the normal computation

    const float3 v1 = to_float3( 1.0f,-1.0f,-1.0f);
    const float3 v2 = to_float3(-1.0f,-1.0f, 1.0f);
    const float3 v3 = to_float3(-1.0f, 1.0f,-1.0f);
    const float3 v4 = to_float3( 1.0f, 1.0f, 1.0f);

  return normalize( v1*doModel( pos + v1*eps, iTime ) + 
                    v2*doModel( pos + v2*eps, iTime ) + 
                    v3*doModel( pos + v3*eps, iTime ) + 
                    v4*doModel( pos + v4*eps, iTime ) );
}



__DEVICE__ mat3 calcLookAtMatrix( in float3 ro, in float3 ta, in float roll )
{
    float3 ww = normalize( ta - ro );
    float3 uu = normalize( cross(ww,to_float3(_sinf(roll),_cosf(roll),0.0f) ) );
    float3 vv = normalize( cross(uu,ww));
    return to_mat3_f3( uu, vv, ww );
}

__KERNEL__ void CoolLakeJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 0.3f); 
  CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.1f);

  float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
  float2 m = swi2(iMouse,x,y)/iResolution;
  
  float ratio = iResolution.x/iResolution.y;
  
  // camera movement
  float3 ro, ta;
  doCamera( &ro, &ta, iTime, m );
float IIIIIIIIIIIIIIII;
  // camera matrix
  mat3 camMat = calcLookAtMatrix( ro, ta, 0.0f );  // 0.0f is the camera roll
    
  // create view ray
  float3 rd = normalize( mul_mat3_f3(camMat , to_float3_aw(swi2(p,x,y),2.0f) )); // 2.0f is the lens length

  float3 col = doBackground(rd, iTime, iResolution, iChannel0, ratio);

  // raymarch
  float t = calcIntersection( ro, rd, iTime );
  if( t>-0.5f )
  {
      // geometry
      float3 pos = ro + t*rd;
      float3 nor = calcNormal(pos, iTime);

      // materials
      float3 mal = doMaterial(rd, nor, iTime, iResolution, iChannel0, refmul, refoff, ratio );

      col = doFog( rd, t, mal, iTime, iResolution, iChannel0, ratio );
  }
    // gamma
  col = pow_f3( clamp(col,0.0f,1.0f), to_float3_s(0.4545f) );
     
  fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}