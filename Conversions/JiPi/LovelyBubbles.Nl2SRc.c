
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Use a lookup texture in Buffer A for thin film interference instead of calculating it at every intersection
#define USE_THIN_FILM_LOOKUP 1

//float N_Air = 1.0f;
//float N_Water = 1.33f;

#define PI 3.141592654f

// used to prevent loop unrolling
// This will be zero but the compiler doesn't know that as iFrame is a uniform
#define ZERO 0 // _fminf(iFrame,0)

// https://en.wikipedia.org/wiki/Fresnel_equations
__DEVICE__ float FresnelS(float ni, float nt, float cosi, float cost)
{
    return ((nt * cosi) - (ni * cost)) / ((nt * cosi) + (ni * cost));
}

__DEVICE__ float FresnelP(float ni, float nt, float cosi, float cost)
{
    return ((ni * cosi) - (nt * cost)) / ((ni * cosi) + (nt * cost));
}

__DEVICE__ float Fresnel(float ni, float nt, float cosi, float cost )
{    
    float Rs = FresnelS( ni, nt, cosi, cost );
    float Rp = FresnelP( ni, nt, cosi, cost );

    return (Rs * Rs + Rp * Rp) * 0.5f;
}

__DEVICE__ float FresnelR0(float ni, float nt)
{
    float R0 = (ni-nt) / (ni+nt);
    R0 *= R0;
    return R0;
}

// https://en.wikipedia.org/wiki/Snell%27s_law
__DEVICE__ float GetCosT( float ni, float nt, float cosi )
{
    float n = ni/nt;
    float sinT2 = n*n*(1.0f-cosi*cosi);
    
    // Total internal reflection
    if (sinT2 >= 1.0f)
    {
        return 1.0f;
    } 

    float cost = _sqrtf(1.0f - sinT2);
    return cost;
}


// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
    p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float SmoothNoise3d(float3 p)
{
    float3 fl = _floor(p);
    float3 fr = p - fl;
    
    float3 ot = fr*fr*(3.0f-2.0f*fr);
    float3 zt = 1.0f - ot;
    
    
    float result = 0.0f;
    
    result += hash13(fl + to_float3(0,0,0)) * (zt.x * zt.y * zt.z);
    result += hash13(fl + to_float3(1,0,0)) * (ot.x * zt.y * zt.z);

    result += hash13(fl + to_float3(0,1,0)) * (zt.x * ot.y * zt.z);
    result += hash13(fl + to_float3(1,1,0)) * (ot.x * ot.y * zt.z);

    result += hash13(fl + to_float3(0,0,1)) * (zt.x * zt.y * ot.z);
    result += hash13(fl + to_float3(1,0,1)) * (ot.x * zt.y * ot.z);

    result += hash13(fl + to_float3(0,1,1)) * (zt.x * ot.y * ot.z);
    result += hash13(fl + to_float3(1,1,1)) * (ot.x * ot.y * ot.z);

    return result;
}



__DEVICE__ float Noise(float3 p, float o)
{
  
  const mat3 m3 = to_mat3( 0.00f,  0.80f,  0.60f,
                          -0.80f,  0.36f, -0.48f,
                          -0.60f, -0.48f,  0.64f );
  
    float result = 0.0f;
    float a = 1.0f;
    float t= 0.0f;
    float f = 0.5f;
    float s= 2.0f;
    
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = mul_mat3_f3(m3 , p) * s; a = a * f;
    result = result / t;
    
    return result;
}



// Spectrum to xyz approx function from http://jcgt.org/published/0002/02/01/paper.pdf
// Inputs:  Wavelength in nanometers
__DEVICE__ float xFit_1931( float wave )
{
    float t1 = (wave-442.0f)*((wave<442.0f)?0.0624:0.0374),
          t2 = (wave-599.8f)*((wave<599.8f)?0.0264:0.0323),
          t3 = (wave-501.1f)*((wave<501.1f)?0.0490:0.0382);
    return 0.362f*_expf(-0.5f*t1*t1) + 1.056f*_expf(-0.5f*t2*t2)- 0.065f*_expf(-0.5f*t3*t3);
}
__DEVICE__ float yFit_1931( float wave )
{
    float t1 = (wave-568.8f)*((wave<568.8f)?0.0213:0.0247),
          t2 = (wave-530.9f)*((wave<530.9f)?0.0613:0.0322);
    return 0.821f*_expf(-0.5f*t1*t1) + 0.286f*_expf(-0.5f*t2*t2);
}
__DEVICE__ float zFit_1931( float wave )
{
    float t1 = (wave-437.0f)*((wave<437.0f)?0.0845:0.0278),
          t2 = (wave-459.0f)*((wave<459.0f)?0.0385:0.0725);
    return 1.217f*_expf(-0.5f*t1*t1) + 0.681f*_expf(-0.5f*t2*t2);
}

#define xyzFit_1931(w) to_float3( xFit_1931(w), yFit_1931(w), zFit_1931(w) ) 

__DEVICE__ float3 XYZtosRGB( float3 XYZ )
{
    // XYZ to sRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
   mat3 m = to_mat3 (
                      3.2404542f, -1.5371385f, -0.4985314f,
                     -0.9692660f,  1.8760108f,  0.0415560f,
                      0.0556434f, -0.2040259f,  1.0572252f );
    
    return mul_f3_mat3(XYZ , m);
}

__DEVICE__ float3 WavelengthToXYZ( float f )
{    
    return xyzFit_1931( f );    
}


// from  https://github.com/amandaghassaei/SoapFlow/blob/main/python/Thin%20Film%20Interference.ipynb
__DEVICE__ float ThinFilmAmplitude( float wavelength, float thickness, float cosi, float N_Air, float N_Water )
{
    float ni = N_Air;
    float nt = N_Water;
    
    float cost = GetCosT( ni, nt, cosi );

    // # The wavelength inside a medium is scaled by the index of refraction.
    // wavelength_soap = wavelength / n_soap
    // wavelength_air = wavelength / n_air
    // # First calc phase shift of reflection at rear surface, based on film thickness.
    // phaseDelta = 2 * thickness / math._cosf(theta) * 2 * math.pi / wavelength_soap  
    // # There is an additional path to compute, the segment AJ from:
    // # https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
    // phaseDelta -= 2 * thickness * math._tanf(theta) * math._sinf(incidentAngle) * 2 * math.pi / wavelength_air
    // Simplified to:
    float phaseDelta = 2.0f * thickness * nt * cost * 2.0f * PI / wavelength;
    
    // https://en.wikipedia.org/wiki/Reflection_phase_change
    if (ni < nt)
        phaseDelta -= PI;
    if (ni > nt)
        phaseDelta += PI;

    float front_refl_amp = Fresnel(cosi, cost, ni, nt);
    float front_trans_amp = 1.0f - front_refl_amp;
    float rear_refl_amp = front_trans_amp * Fresnel(cost, cosi, nt, ni);
    
    rear_refl_amp /= front_refl_amp;
    front_refl_amp = 1.0f;
        
    // http://scipp.ucsc.edu/~haber/ph5B/addsine.pdf
    return _sqrtf(front_refl_amp * front_refl_amp + rear_refl_amp * rear_refl_amp + 2.0f * front_refl_amp * rear_refl_amp * _cosf(phaseDelta));
}

#if 1

__DEVICE__ float3 GetThinFilmColour( float cosi, float thicknessN, float N_Air, float N_Water )
{
    float thicknessMin = 100.0f;//1.0f;
    float thicknessMax = 1500.0f;//2500.0f;
    
    float thickness = _mix(thicknessMin, thicknessMax, thicknessN);

    float3 result = to_float3_s(0.0f);
    
    float t = 0.0f;
    
    float3 white = to_float3_s(0.0f);
    
    for (float wavelength = 380.0f; wavelength<=780.0f; wavelength += 50.0f)
    {
        float amplitude = ThinFilmAmplitude( wavelength, thickness, cosi, N_Air, N_Water );
        
        float3 XYZ = WavelengthToXYZ( wavelength );
    
        white += XYZ;
    
        result += XYZ * amplitude;
        t += 1.0f;
    }

    result = XYZtosRGB( result );
      
    result /= t;
    //result /= white;
    //result = to_float3_s(1.0f);
    
    return result;
}

#else

// The Technical Art of The Last of Us Part II by Waylon Brinck and Steven Tang || SIGGRAPH 2020
// https://youtu.be/tvBIqPHaExQ?t=2873

__DEVICE__ float3 GetThinFilmColour( float cosi, float thicknessN, float N_Air, float N_Water )
{
    float thicknessMin = 100.0f;//1.0f;
    float thicknessMax = 1500.0f;//2500.0f;
    
    float thickness = _mix(thicknessMin, thicknessMax, thicknessN);

    float3 result = to_float3_s(1.0f);

    float3 rgbLightWavelength = to_float3(700,510,440);
    float extraDistance = thickness / cosi;
    float3 phaseChangeEquivalentShift = to_float3_s(0);
    
    bool doublePhase = true;
    
    if ( !doublePhase )
    {
        phaseChangeEquivalentShift = -rgbLightWavelength * 0.5f;
    }
    
    float3 phaseOffset = to_float3(extraDistance - phaseChangeEquivalentShift) / rgbLightWavelength;
    float3 superpositionAmplitude = _fabs(_cosf(phaseOffset * PI));
    
    float coherenceTerm = 1.0f - clamp( extraDistance * 1.0f / 1500.0f, 0.0f, 1.0f );
    
    superpositionAmplitude = _mix( to_float3_s(1.0f), superpositionAmplitude, coherenceTerm);

    float filmStrength = 2.0f;
    float filmStrengthTerm = filmStrength * cosi;

    result = _mix( result, superpositionAmplitude, filmStrengthTerm );

    //result = superpositionAmplitude;

    return result;
}

#endif
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0




__KERNEL__ void LovelyBubblesFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.5f);

    CONNECT_SLIDER1(N_Air, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(N_Water, -1.0f, 3.0f, 1.33f);
    CONNECT_SLIDER3(speed, -1.0f, 3.0f, 1.0f);


    fragCoord+=0.5f;
    
    //float N_Air = 1.0f;
    //float N_Water = 1.33f;

#if USE_THIN_FILM_LOOKUP
    {
        int segmentCount = 32;
        int segment = iFrame % segmentCount;
        int currSegment = (int)(_floor((fragCoord.y * (float)(segmentCount) / iResolution.y)));
        
        if ( segment != currSegment )
        {
          
            //fragColor = texelFetch( iChannel0, to_int2(fragCoord), 0 );
            fragColor = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
            
            SetFragmentShaderComputedColor(fragColor);
            return;
        }
    }

    float2 uv = fragCoord/iResolution;
        
    float3 result = GetThinFilmColour(uv.x, uv.y, N_Air, N_Water);  

    result += swi3(Color,x,y,z)-0.5f;

    fragColor = to_float4_aw(result,1.0f);
#else
    //discard;
    SetFragmentShaderComputedColor(fragColor);
    return;
#endif

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Lovely Bubbles
// by @P_Malin
// https://www.shadertoy.com/view/Nl2SRc
//
// Some lovely shadertoy bubbles.
// I've wanted to implement something with thin film interference for a while.


// CAMERA

__DEVICE__ float2 GetWindowCoord( float2 uv, float2 iResolution )
{
  float2 window = uv * 2.0f - 1.0f;
  window.x *= iResolution.x / iResolution.y;

  return window;  
}

__DEVICE__ float3 GetCameraRayDir( float2 window, float3 cameraPos, float3 cameraTarget, float fov )
{
  float3 forward = normalize( cameraTarget - cameraPos );
  float3 right = normalize( cross( to_float3(0.0f, 1.0f, 0.0f), forward ) );
  float3 up = normalize( cross( forward, right ) );
                
  float3 dir = normalize(window.x * right + window.y * up + forward * fov);

  return dir;
}


// POSTFX

__DEVICE__ float Vignette( float2 uv, float size )
{
    float d = length( (uv - 0.5f) * 2.0f ) / length(to_float2_s(1.0f));
 
    d /= size;
    
    float s = d * d * ( 3.0f - 2.0f * d );
    
    float v = _mix ( d, s, 0.6f );
    
    return _fmaxf(0.0f, 1.0f - v);
}

__DEVICE__ float3 ApplyTonemap( float3 linearCol )
{
  const float kExposure = 0.5f;
  
    float a = 0.010f;
    float b = 0.132f;
    float c = 0.010f;
    float d = 0.163f;
    float e = 0.101f;

    float3 x = linearCol * kExposure;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
}

__DEVICE__ float3 ApplyGamma( float3 linearCol )
{
  const float kGamma = 2.2f;

  return pow_f3( linearCol, to_float3_s(1.0f/kGamma) );  
}

__DEVICE__ float3 ApplyPostFX( float2 uv, float3 col )
{    
    col *= 1.3f;

    col *= 0.1f + 0.9f * Vignette( uv, 1.0f );

    col *= to_float3(1.0f, 0.95f, 0.8f); // warmer
  
    col = ApplyTonemap(col);
    col = ApplyGamma(col);
    
  return col;
}
  


// Scene



__DEVICE__ float BubbleOriginForward( float t, float iTime, float speed )
{
    t = t * 30.0f;
    if ( t > 0.0f)
    {
        t = t / (1.0f+t/10.0f);
    }
    return t + iTime * speed;
}

__DEVICE__ float BubbleOriginInverse( float r, float iTime, float speed )
{
    r = r- iTime * speed;
    if( r > 0.0f)
    {
        r = -10.0f * r / (r - 10.0f);
    }
    r = r / 30.0f;
    return r;
}

__DEVICE__ float Scene_Distance(float3 pos, float iTime, float speed)
{

    float3 vPos = pos;
    vPos.x += 3.0f;

    float scale = 50.0f;
    
    vPos /= scale;

    // wobble
    float3 offset = to_float3_s(0);
    offset += sin_f3( swi3(pos,y,z,x) * 8.91f + iTime * 10.0f ) * 0.001f;
    offset += sin_f3( swi3(pos,z,x,y) * 7.89f + iTime * 10.0f ) * 0.001f;    
    offset *= 0.08f;
    
    float f = BubbleOriginForward( vPos.x, iTime, speed );
    
    f = _floor(f);
    
    float minD = 1000000.0f;
    
    for (float b=-1.0f; b<=2.0f; b+=1.0f)
    {
      float p = f + b;
      float3 o = vPos;
      o.x = BubbleOriginInverse( p, iTime, speed );
              
      o.x -= vPos.x;

      float spreadBlend = 1.0f - clamp( vPos.x * 3.0f + 0.2f, 0.0f, 1.0f);
       
      float spread = spreadBlend;
      
      spread *= 0.05f;

      o.y += _sinf(p * 123.3456f) * spread;
      o.z += _sinf(p * 234.5678f) * spread;
       
      o += offset;
         
      float rad = _sinf( p * 456.8342f ) * 0.5f + 0.5f;
                           
      float d = length(o) - 0.005f - rad * rad * 0.02f;
       
      minD = _fminf( minD, d );
    }
    
     return minD * scale;
}

union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };

__DEVICE__ float3 Scene_GetNormal( float3 pos, float iTime, float speed )
{
    const float delta = 0.0001f;
    
    //float4 samples;
    A2F samples;
    
    for( int i=ZERO; i<=4; i++ )
    {
        //float4 offset = to_float4_s(0);
        A2F offset;
        offset.F = to_float4_s(0);
        offset.A[i] = delta;
        samples.A[i] = Scene_Distance( pos + swi3(offset.F,x,y,z), iTime, speed );
    }
    
    float3 normal = swi3(samples.F,x,y,z) - swi3(samples.F,w,w,w);    
    return normalize( normal );
}    

__DEVICE__ float Scene_Trace( float3 rayOrigin, float3 rayDir, float minDist, float maxDist, float side, float iTime, float speed )
{
  float t = minDist;

  const int kRaymarchMaxIter = 128;
  for(int i=0; i<kRaymarchMaxIter; i++)
  {    
    float epsilon = 0.0001f * t;
    float d = Scene_Distance( rayOrigin + rayDir * t, iTime, speed ) * side;
    if ( _fabs(d) < epsilon )
    {
      break;
    }
                        
    if ( t > maxDist )
    {
      t = maxDist + 1.0f;
        break;
    }       
    
  t += d;        
  }
    
    return t;
}

__DEVICE__ float3 GetSkyColour( float3 dir, __TEXTURE2D__ iChannel1 )
{
  float3 result = to_float3_s(0.0f);
  
    float3 envMap = swi3(decube_f3(iChannel1,dir),x,y,z);//.rgb;
    envMap = envMap * envMap;
    float kEnvmapExposure = 0.99999f;
    result = -1.0f*log2_f3(1.0f - envMap * kEnvmapExposure);

    return result;  
}

__DEVICE__ float FilmThickness( float3 pos, float iTime )
{
    return Noise(pos * 0.3f, iTime * 0.5f);
}

__DEVICE__ void Shade( inout float3 *colour, inout float3 *remaining, float3 pos, float3 rayDir, float3 normal, float iTime, float speed, float N_Air, float N_Water, __TEXTURE2D__ iChannel0 , __TEXTURE2D__ iChannel1 )
{
    float NdotV = _fmaxf( dot(normal, -rayDir), 0.0f );

    float filmThickness = FilmThickness(pos, iTime);

    float3 reflection = GetSkyColour( reflect( rayDir, normal ), iChannel1 );
    
#if 1
    // Extra highlight
    float3 LightColour = to_float3(1,0.9f,0.7f) * 0.8f;
    float3 L = normalize(to_float3(1.0f, 2.0f, 0.0f));
    float NdotL = _fmaxf( dot( normal, L ), 0.0f );
    float NdotH = _fmaxf( dot( normal, normalize(L-rayDir) ), 0.0f );
    reflection += (_powf(NdotH,10000.0f) * 10000.0f) * NdotL * LightColour;
    //vReflection += (_powf(NdotH,1000.0f) * 2000.0f) * NdotL * LightColour;
    reflection += (_powf(NdotH,100.0f) * 200.0f) * NdotL * LightColour;
    reflection += (_powf(NdotH,10.0f) * 20.0f) * NdotL * LightColour;
#endif     
     
    float ni = N_Air;
    float nt = N_Water;     
    
    float cosi = NdotV;
    float cost = GetCosT( ni, nt, cosi );
    float fresnelA = Fresnel( ni, nt, cosi, cost );
    float fresnelB = Fresnel( nt, ni, cost, cosi );

    float fresnelFactor = 1.0f - (1.0f - fresnelA) * (1.0f - fresnelB);
    
    float3 fresnel = to_float3_s(fresnelFactor);

    float3 thinFilmColour;
#if USE_THIN_FILM_LOOKUP
    thinFilmColour = swi3(texture(iChannel0, to_float2(NdotV, filmThickness) ),x,y,z);//.rgb;
#else
    thinFilmColour = GetThinFilmColour(NdotV, filmThickness, iTime, speed);
#endif    
    fresnel *= thinFilmColour;
    
    *colour += reflection * fresnel * *remaining;
    *remaining *= (1.0f - fresnel);


#if 0
    float fGlassThickness = 0.5f;
    float3 vGlassColor = to_float3(1,0.5f, 0.25f);

  float fOpticalDepth = fGlassThickness / NdotV;
    float3 vExtinction = _exp2f( -fOpticalDepth * (1.0f - vGlassColor) ); 
    *remaining *= vExtinction;
#endif    
}


__DEVICE__ float3 GetSceneColour( float3 rayOrigin, float3 rayDir, float iTime, float speed, float N_Air, float N_Water, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 )
{    
    float kFarClip = 200.0f;

    float3 colour = to_float3_s(0);
    float3 remaining = to_float3_s(1);
    
    float side = 1.0f;
    
    float minDist = 0.0f;
    
    for( int i=0; i<10; i++ )
    {
        float t = Scene_Trace( rayOrigin, rayDir, minDist, kFarClip, side, iTime, speed );
        
        if ( t>=kFarClip )
        {
            break;
        }
        
        minDist = t + 0.1f;
        
        float3 hitPos = rayOrigin + rayDir * t;

        float3 normal = Scene_GetNormal( hitPos, iTime, speed );

        Shade(&colour, &remaining, hitPos, rayDir, normal * side, iTime, speed, N_Air, N_Water, iChannel0, iChannel1 );
        
        side = side * -1.0f;
    }
    
    colour += GetSkyColour(rayDir, iChannel1) * remaining; 
  
  return colour;
}

__KERNEL__ void LovelyBubblesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
  CONNECT_SLIDER0(ViewZ, -100.0f, 100.0f, 0.0f);

  CONNECT_SLIDER1(N_Air, -1.0f, 10.0f, 1.0f);
  CONNECT_SLIDER2(N_Water, -1.0f, 3.0f, 1.33f);
  CONNECT_SLIDER3(speed, -1.0f, 3.0f, 1.0f);

  fragCoord+=0.5f;
  
//  float N_Air = 1.0f;
//  float N_Water = 1.33f;
  
//  float speed = 1.0f;
        
  float2 uv = fragCoord / iResolution;

  float heading = 0.3f + _sinf(iTime * 0.3f) * 0.1f + ViewXY.x;

  float elevation = 1.8f + _sinf(iTime * 0.134f) * 0.1f + ViewXY.y;
  
  float fov = 2.5f + _sinf( iTime * 0.234f) * 0.5f;
float IIIIIIIIIIIIIIIIIIIII;  
  float cameraDist = 10.0f + ViewZ;
  float3 cameraPos = to_float3(_sinf(heading) * _sinf(-elevation), _cosf(-elevation), _cosf(heading) * _sinf(-elevation)) * cameraDist;
  float3 cameraTarget = to_float3(_sinf(iTime * 0.1542f) * 3.0f, 0.0f, 0.0f);

  float3 rayOrigin = cameraPos;
  float3 rayDir = GetCameraRayDir( GetWindowCoord(uv, iResolution), cameraPos, cameraTarget, fov );
  
  float3 sceneCol = GetSceneColour( rayOrigin, rayDir, iTime, speed, N_Air, N_Water,iChannel0, iChannel1 );
  
  float3 final = ApplyPostFX( uv, sceneCol );
    
    //final = texture( iChannel0, fragCoord/iResolution).rgb;
  
  fragColor = to_float4_aw(final, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}