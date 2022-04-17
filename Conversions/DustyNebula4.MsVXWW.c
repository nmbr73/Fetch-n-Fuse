
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: RGBA Noise Medium' to iChannel0
// Connect 'Preset: Keyboard' to iChannel1


// "Dusty nebula 4" by Duke
// https://www.shadertoy.com/view/MsVXWW
//-------------------------------------------------------------------------------------
// Based on "Dusty nebula 3" (https://www.shadertoy.com/view/lsVSRW)
// and "Protoplanetary disk" (https://www.shadertoy.com/view/MdtGRl)
// otaviogood's "Alien Beacon" (https://www.shadertoy.com/view/ld2SzK)
// and Shane's "Cheap Cloud Flythrough" (https://www.shadertoy.com/view/Xsc3R4) shaders
// Some ideas came from other shaders from this wonderful site
// Press 1-2-3 to zoom in and zoom out.
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
//-------------------------------------------------------------------------------------


#define swi2S(a,b,c,d) {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 
#define swi3S(a,b,c,d,e) {float3 tmp = e; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z;} 

#define ROTATION
//#define MOUSE_CAMERA_CONTROL

#define DITHERING
#define BACKGROUND

//#define TONEMAPPING

//-------------------
#define pi 3.14159265f
//#define R(p, a)    p=_cosf(a)*p + _sinf(a) * to_float2((p).y, -(p).x)

#define R(p, X,Y, a) {float2 tmp=_cosf(a)*to_float2((p).X,(p).Y)+_sinf(a)*to_float2((p).Y, -(p).X); (p).X=tmp.x;(p).Y=tmp.y;}


__DEVICE__ float rand(float2 co)
{
  return fract_f(_sinf(dot(co*0.123f,to_float2(12.9898f,78.233f))) * 43758.5453f);
}

//=====================================
// otaviogood's noise from https://www.shadertoy.com/view/ld2SzK
//--------------------------------------------------------------
// This spiral noise works by successively adding and rotating sin waves while increasing frequency.
// It should work the same on all computers since it's not based on a hash function like some other noises.
// It can be much faster than other noise functions if you're ok with some repetition.
__DEVICE__ float SpiralNoiseC(float3 p)
{
const float nudge = 0.739513f;  // size of perpendicular vector
float normalizer = 1.0f / _sqrtf(1.0f + nudge*nudge);  // pythagorean theorem on that perpendicular to maintain scale

    float n = 0.0f;  // noise amount
    float iter = 1.0f;
    for (int i = 0; i < 8; i++)
    {
        // add sin and cos scaled inverse with the frequency
        n += -_fabs(_sinf(p.y*iter) + _cosf(p.x*iter)) / iter;  // abs for a ridged look
        // rotate by adding perpendicular and scaling down
        swi2S(p,x,y,swi2(p,x,y) + to_float2(p.y, -p.x) * nudge)
        swi2S(p,x,y,swi2(p,x,y) * normalizer)
        // rotate on other axis
        swi2S(p,x,z,swi2(p,x,z) + to_float2(p.z, -p.x) * nudge);
        swi2S(p,x,z,swi2(p,x,z) * normalizer);
        // increase the frequency
        iter *= 1.733733f;
    }
    return n;
}

__DEVICE__ float SpiralNoise3D(float3 p)
{
  const float nudge = 0.739513f;  // size of perpendicular vector
  float normalizer = 1.0f / _sqrtf(1.0f + nudge*nudge);  // pythagorean theorem on that perpendicular to maintain scale

    float n = 0.0f;
    float iter = 1.0f;
    for (int i = 0; i < 5; i++)
    {
        n += (_sinf(p.y*iter) + _cosf(p.x*iter)) / iter;
        swi2S(p,x,z,swi2(p,x,z) + to_float2(p.z, -p.x) * nudge);
        swi2S(p,x,z,swi2(p,x,z) * normalizer);
        iter *= 1.33733f;
    }
    return n;
}

__DEVICE__ float NebulaNoise(float3 p)
{
   float final = p.y + 4.5f;
    final -= SpiralNoiseC(swi3(p,x,y,z));   // mid-range noise
    final += SpiralNoiseC(swi3(p,z,x,y)*0.5123f+100.0f)*4.0f;   // large scale features
    final -= SpiralNoise3D(p);   // more large scale features, but 3d

    return final;
}

__DEVICE__ float map(float3 p,float iMouse_x,float iTime)
{
  #ifdef ROTATION
  R(p,x,z, iMouse_x*0.008f*pi+iTime*0.1f)
  #endif

  float NebNoise = _fabs(NebulaNoise(p/0.5f)*0.5f);

  return NebNoise+0.03f;
}
//--------------------------------------------------------------

// assign color to the media
__DEVICE__ float3 computeColor( float density, float radius )
{
  // color based on density alone, gives impression of occlusion within
  // the media
  float3 result = _mix( to_float3(1.0f,0.9f,0.8f), to_float3(0.4f,0.15f,0.1f), density );

  // color added to the media
  float3 colCenter = 7.0f*to_float3(0.8f,1.0f,1.0f);
  float3 colEdge = 1.5f*to_float3(0.48f,0.53f,0.5f);
  result *= _mix( colCenter, colEdge, _fminf( (radius+0.05f)/0.9f, 1.15f ) );

  return result;
}

__DEVICE__ bool RaySphereIntersect(float3 org, float3 dir, out float *near, out float *far)
{
  float b = dot(dir, org);
  float c = dot(org, org) - 8.0f;
  float delta = b*b - c;
  if( delta < 0.0f)
    return false;
  float deltasqrt = _sqrtf(delta);
  *near = -b - deltasqrt;
  *far = -b + deltasqrt;
  return *far > 0.0f;
}

// Applies the filmic curve from John Hable's presentation
// More details at : http://filmicgames.com/archives/75
__DEVICE__ float3 ToneMapFilmicALU(float3 _color)
{
  
  _color = _fmaxf(to_float3_s(0.0f), _color - to_float3_s(0.004f));
  _color = (_color * (6.2f*_color + to_float3_s(0.5f))) / (_color * (6.2f * _color + to_float3_s(1.7f)) + to_float3_s(0.06f));
  return _color;
}

__KERNEL__ void DustyNebula4Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

  const float KEY_1 = 49.5f/256.0f;
  const float KEY_2 = 50.5f/256.0f;
  const float KEY_3 = 51.5f/256.0f;
  float key = 0.0f;
    // key += 0.7f*texture(iChannel1, to_float2(KEY_1,0.25f)).x;
    // key += 0.7f*texture(iChannel1, to_float2(KEY_2,0.25f)).x;
    // key += 0.7f*texture(iChannel1, to_float2(KEY_3,0.25f)).x;

  // ro: ray origin
  // rd: direction of the ray
  float3 rd = normalize(to_float3_aw((swi2(fragCoord,x,y)-0.5f*iResolution)/iResolution.y, 1.0f)); // verrueckt!!!
  float3 ro = to_float3(0.0f, 0.0f, -6.0f+key*1.6f);



  #ifdef MOUSE_CAMERA_CONTROL
  R(rd,y,z, -iMouse.y*0.01f*pi*2.0f)
  R(rd,x,z, iMouse.x*0.01f*pi*2.0f)
  R(ro,y,z, -iMouse.y*0.01f*pi*2.0f)
  R(ro,x,z, iMouse.x*0.01f*pi*2.0f)
  #else
    
  R(rd,y,z, -pi*3.93f)
  R(rd,x,z, pi*3.2f)
  R(ro,y,z, -pi*3.93f)
  R(ro,x,z, pi*3.2f)
  #endif

  #ifdef DITHERING
  float2 dpos = ( fragCoord / iResolution );
  float2 seed = dpos + fract_f(iTime);
  #endif

  // ld, td: local, total density
  // w: weighting factor
  float ld=0.0f, td=0.0f, w=0.0f;

  // t: length of the ray
  // d: distance function
  float d=1.0f, t=0.0f;

  const float h = 0.1f;

  float4 sum = to_float4_s(0.0f);

  float min_dist=0.0f, max_dist=0.0f;

float4 debcol;

  if(RaySphereIntersect(ro, rd, &min_dist, &max_dist))
    {

    t = min_dist*step(t,min_dist);

    // raymarch loop
    for (int i=0; i<56; i++)
    {

      float3 pos = ro + t*rd;

      // Loop break conditions.
      if(td>0.9f || d<0.1f*t || t>10.0f || sum.w > 0.99f || t>max_dist) break;

      // evaluate distance function
      float d = map(pos,iMouse.x,iTime);

      // change this string to control density
      d = _fmaxf(d,0.08f);

      // point light calculations
      float3 ldst = to_float3_s(0.0f)-pos;
      float lDist = _fmaxf(length(ldst), 0.001f);

      // star in center
      float3 lightColor=to_float3(1.0f,0.5f,0.25f);
      //swi3(sum,x,y,z)+=(lightColor/(lDist*lDist)/30.0f); // star itself and bloom around the light

      //float3 dmy=(lightColor/(lDist*lDist)/30.0f);
      //sum.x=dmy.x;  sum.y=dmy.y;  sum.z=dmy.z;

   //   swi3S(sum,x,y,z,swi3(sum,x,y,z)+(lightColor/(lDist*lDist)/30.0f)) // star itself and bloom around the light 
debcol = sum;
      if (d<h)
      {
        // compute local density
        ld = h - d;

        // compute weighting factor
        w = (1.0f - td) * ld;

        // accumulate density
        td += w + 1.0f/200.0f;

        float4 col = to_float4_aw( computeColor(td,lDist), td );
col = to_float4_s(0.10f);
//debcol = col;
        // uniform scale density
        col.w *= 0.185f;
        // colour by alpha
  //      swi3S(col,x,y,z,swi3(col,x,y,z) * col.w)
        // alpha blend in contribution
 //       sum = sum + col*(1.0f - sum.w);

      }

      td += 1.0f/70.0f;

      // enforce minimum stepsize
      d = _fmaxf(d, 0.04f);

      #ifdef DITHERING
      // add in noise to reduce banding and create fuzz
      d=_fabs(d)*(0.8f+0.2f*rand(seed*to_float2_s(i)));
      #endif

      // trying to optimize step size near the camera and near the light source
      t += _fmaxf(d * 0.1f * _fmaxf(min(length(ldst),length(ro)),1.0f), 0.02f);

    }

    // simple scattering
    //sum *= 1.0f / _expf( ld * 0.2f ) * 0.6f;

    //sum = clamp( sum, 0.0f, 1.0f );

    //swi3(sum,x,y,z) = swi3(sum,x,y,z)*swi3(sum,x,y,z)*(3.0f-2.0f*swi3(sum,x,y,z));
    sum.x = sum.x*sum.x*(3.0f-2.0f*sum.x);
    sum.y = sum.y*sum.y*(3.0f-2.0f*sum.y);
    sum.z = sum.z*sum.z*(3.0f-2.0f*sum.z);

  }

    #ifdef BACKGROUND
    // stars background
    if (td<0.8f)
    {
        // iq's noise
        float noise;
        {   float3 x = rd*500.0f;
            float3 p = _floor(x);
            float3 f = fract_f(x);
            f = f*f*(3.0f-2.0f*f);
            //float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y);
            //float2 rg = textureLod( iChannel0, (uv+ 0.5f)/256.0f, 0.0f ).yx;
            //noise = 1.0f - 0.82f*_mix( rg.x, rg.y, f.z );

            float uv_x = p.x+37.0f*p.z + f.x;
            float uv_y = p.y+17.0f*p.z + f.y;
            float4 rg = _tex2DVecN( iChannel0, (uv_x+ 0.5f)/256.0f, (uv_y+ 0.5f)/256.0f, 0.0f );
            noise = 1.0f - 0.82f*_mix( rg.y, rg.x, f.z );
        }


        float3 stars = to_float3_s(noise*0.5f+0.5f);



        float3 starbg = to_float3_s(0.0f);
        //starbg = _mix(starbg, vec3(0.8,0.9,1.0), smoothstep(0.99, 1.0, stars)*clamp(dot(vec3(0.0),rd)+0.75,0.0,1.0));
        
        starbg = mix_f3(starbg, 
                        to_float3(0.8f,0.9f,1.0f),
                        smoothstep(to_float3_s(0.99f), to_float3_s(1.0f), stars)*clamp(dot(to_float3_s(0.0f),rd)+0.75f,0.0f,1.0f));
        starbg = clamp(starbg, 0.0f, 1.0f);
        //swi3(sum,x,y,z) += starbg;
        sum.x+=starbg.x;
        sum.y+=starbg.y;
        sum.z+=starbg.z;

    }
  #endif

  #ifdef TONEMAPPING
    fragColor = to_float4_aw(ToneMapFilmicALU(swi3(sum,x,y,z)*2.2f),1.0f);
  #else
    fragColor = to_float4_aw(swi3(sum,x,y,z),1.0f);
  #endif

fragColor=to_float4_aw(swi3(debcol,x,y,z),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}