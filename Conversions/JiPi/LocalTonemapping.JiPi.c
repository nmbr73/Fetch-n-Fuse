
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float _powcf(float x, float y) {
    float ret = _powf(x,y);
    if (isnan(ret)) {
        ret = 0.0001f;
    }
    return ret;
}


#define PI 3.14159265f
#define dt 1.0f
#define R iResolution


		#define saturate(a) clamp( a, 0.0, 1.0 )

		//uniform sampler2D tDiffuse;

		//uniform float exposure;

		//varying vec2 vUv;

		__DEVICE__ float3 RRTAndODTFit( float3 v ) {

			float3 a = v * ( v + 0.0245786f ) - 0.000090537f;
			float3 b = v * ( 0.983729f * v + 0.4329510f ) + 0.238081f;
			return a / b;

		}

		__DEVICE__ float3 ACESFilmicToneMapping( float3 color ) {

		// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
			const mat3 ACESInputMat = to_mat3_f3(
                                            to_float3( 0.59719f, 0.07600f, 0.02840f ), // transposed from source
                                            to_float3( 0.35458f, 0.90834f, 0.13383f ),
                                            to_float3( 0.04823f, 0.01566f, 0.83777f )
                                          );

		// ODT_SAT => XYZ => D60_2_D65 => sRGB
			const mat3 ACESOutputMat = to_mat3_f3(
                                            to_float3(  1.60475f, -0.10208f, -0.00327f ), // transposed from source
                                            to_float3( -0.53108f,  1.10813f, -0.07276f ),
                                            to_float3( -0.07367f, -0.00605f,  1.07602f )
                                          );

			color = mul_mat3_f3(ACESInputMat , color);

		// Apply RRT and ODT
			color = RRTAndODTFit( color );

			color = mul_mat3_f3(ACESOutputMat , color);

		// Clamp to [0, 1]
			return saturate( color );
		}



__DEVICE__ float4 LuminanceShader( __TEXTURE2D__ tOriginal, float2 vUv, float shadows, float highlights, float exposure)
{ 

      // Tonemap three syntetic exposures and produce their luminances.
      float3 inpColor = swi3(texture( tOriginal, vUv ),x,y,z) * exposure;
      float _highlights = sqrt(dot(clamp(ACESFilmicToneMapping(inpColor * highlights), 0.0f, 1.0f), to_float3(0.1f,0.7f,0.2f)));
      float _midtones = sqrt(dot(clamp(ACESFilmicToneMapping(inpColor), 0.0f, 1.0f), to_float3(0.1f,0.7f,0.2f)));
      float _shadows = sqrt(dot(clamp(ACESFilmicToneMapping(inpColor * shadows), 0.0f, 1.0f), to_float3(0.1f,0.7f,0.2f)));
			float3 cols = to_float3(_highlights, _midtones, _shadows);
			return to_float4_aw(cols, 1.0f);
}


__DEVICE__ float4 ExposureWeightsShader( __TEXTURE2D__ tDiffuse, float2 vUv, float sigmaSq)
{ 
      // Compute the synthetic exposure weights.
			float3 diff = swi3(texture(tDiffuse, vUv),x,y,z) - to_float3_s(0.5f);
      float3 weights = (exp_f3(-0.5 * diff * diff * sigmaSq));
             weights /= dot(weights, to_float3_s(1.0f)) + 0.00001f;
			return to_float4_aw(weights, 1.0f);
}


__DEVICE__ float4 BlendShader( __TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights, float2 vUv)
{ 
      // Blend the exposures based on the blend weights.
			float3 weights = swi3(texture(tWeights, vUv),x,y,z);
      float3 exposures = swi3(texture(tExposures, vUv),x,y,z);
              weights /= dot(weights, to_float3_s(1.0f)) + 0.0001f;
			return to_float4_aw(to_float3_s(dot(exposures * weights, to_float3_s(1.0f))), 1.0f);
}

__DEVICE__ float4 BlendLaplacianShader( __TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights, __TEXTURE2D__ tExposuresCoarser, __TEXTURE2D__ tAccumSoFar, float2 vUv, float boostLocalContrast)
{
      // Blend the Laplacians based on exposure weights.
      float accumSoFar = texture( tAccumSoFar, vUv ).x;
      float3 laplacians = swi3(texture(tExposures, vUv),x,y,z) - swi3(texture(tExposuresCoarser, vUv),x,y,z);
      float3 weights = swi3(texture(tWeights, vUv),x,y,z) * (boostLocalContrast > 0.0 ? abs_f3(laplacians) + 0.00001f : to_float3_s(1.0f));
      weights /= dot(weights, to_float3_s(1.0f)) + 0.00001f;
      float laplac = dot(laplacians * weights, to_float3_s(1.0f));
			return to_float4_aw(to_float3_s(accumSoFar + laplac), 1.0f);  
}

__DEVICE__ float4 FinalCombinePassShader( __TEXTURE2D__ tDiffuse, __TEXTURE2D__ tOriginal, __TEXTURE2D__ tOriginalMip, float2 vUv, float4 mipPixelSize, float exposure)
{

      // Guided upsampling.
      // See https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/
      float momentX = 0.0f;
      float momentY = 0.0f;
      float momentX2 = 0.0f;
      float momentXY = 0.0f;
      float ws = 0.0f;
      for (int dy = -1; dy <= 1; dy += 1) {
          for (int dx = -1; dx <= 1; dx += 1) {
              float x = texture(tOriginalMip, vUv + to_float2(dx, dy) * swi2(mipPixelSize,z,w)).y;
              float y = texture(tDiffuse, vUv + to_float2(dx, dy) * swi2(mipPixelSize,z,w)).x;
              float w = _expf(-0.5f * (float)(dx*dx + dy*dy) / (0.7f*0.7f));
              momentX += x * w;
              momentY += y * w;
              momentX2 += x * x * w;
              momentXY += x * y * w;
              ws += w;
          }
      }
      momentX /= ws;
      momentY /= ws;
      momentX2 /= ws;
      momentXY /= ws;
      float A = (momentXY - momentX * momentY) / (max(momentX2 - momentX * momentX, 0.0f) + 0.00001f);
      float B = momentY - A * momentX;
      
      // Apply local exposure adjustment as a crude multiplier on all RGB channels.
      // This is... generally pretty wrong, but enough for the demo purpose.
      float3 texel = swi3(texture(tDiffuse, vUv),x,y,z);
      float3 texelOriginal = sqrt_f3(_fmaxf(ACESFilmicToneMapping(swi3(texture(tOriginal, vUv),x,y,z) * exposure), to_float3_s(0.0f)));
      float luminance = dot(swi3(texelOriginal,x,y,z), to_float3(0.1f,0.7f,0.2f)) + 0.00001f;
      float finalMultiplier = _fmaxf(A * luminance + B, 0.0f) / luminance;
      // This is a hack to prevent super dark pixels getting boosted by a lot and showing compression artifacts.
      float lerpToUnityThreshold = 0.007f;
      finalMultiplier = luminance > lerpToUnityThreshold ? finalMultiplier 
                                                         : _mix(1.0f, finalMultiplier, (luminance / lerpToUnityThreshold) * (luminance / lerpToUnityThreshold));
      float3 texelFinal = sqrt_f3(_fmaxf(ACESFilmicToneMapping(swi3(texture(tOriginal, vUv),x,y,z) * exposure * finalMultiplier), to_float3_s(0.0f)));
      return to_float4_aw(texelFinal, 1.0f);
}



// Texturefunktionen
__DEVICE__ float4 ExposureWeightsShaderTex( float4 tDiffuse, float sigmaSq)
{ 
      // Compute the synthetic exposure weights.
			float3 diff = swi3(tDiffuse,x,y,z) - to_float3_s(0.5f);
      float3 weights = (exp_f3(-0.5 * diff * diff * sigmaSq));
             weights /= dot(weights, to_float3_s(1.0f)) + 0.00001f;
			return to_float4_aw(weights, 1.0f);
}


__DEVICE__ float4 BlendShaderTex( float4 tExposures, float4 tWeights)
{ 
      // Blend the exposures based on the blend weights.
			float3 weights = swi3(tWeights,x,y,z);
      float3 exposures = swi3(tExposures,x,y,z);
              weights /= dot(weights, to_float3_s(1.0f)) + 0.0001f;
			return to_float4_aw(to_float3_s(dot(exposures * weights, to_float3_s(1.0f))), 1.0f);
}

__DEVICE__ float4 BlendLaplacianShaderTex( float4 tExposures, float4 tWeights, float4 tExposuresCoarser, float4 tAccumSoFar, float boostLocalContrast)
{
      // Blend the Laplacians based on exposure weights.
      float accumSoFar =  tAccumSoFar.x;
      float3 laplacians = swi3(tExposures,x,y,z) - swi3(tExposuresCoarser,x,y,z);
      float3 weights = swi3(tWeights,x,y,z) * (boostLocalContrast > 0.0 ? abs_f3(laplacians) + 0.00001f : to_float3_s(1.0f));
      weights /= dot(weights, to_float3_s(1.0f)) + 0.00001f;
      float laplac = dot(laplacians * weights, to_float3_s(1.0f));
			return to_float4_aw(to_float3_s(accumSoFar + laplac), 1.0f);  
}

__DEVICE__ float4 FinalCombinePassShaderTex( __TEXTURE2D__ tDiffuse, __TEXTURE2D__ tOriginal, __TEXTURE2D__ tOriginalMip, float2 vUv, float4 mipPixelSize, float exposure)
{
float zzzzzzzzzzzzzzzz;
      // Guided upsampling.
      // See https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/
      float momentX = 0.0f;
      float momentY = 0.0f;
      float momentX2 = 0.0f;
      float momentXY = 0.0f;
      float ws = 0.0f;
      for (int dy = -1; dy <= 1; dy += 1) {
          for (int dx = -1; dx <= 1; dx += 1) {
              float x = texture(tOriginalMip, vUv + to_float2(dx, dy) * swi2(mipPixelSize,z,w)).y;
              float y = texture(tDiffuse, vUv + to_float2(dx, dy) * swi2(mipPixelSize,z,w)).x;
              float w = _expf(-0.5f * (float)(dx*dx + dy*dy) / (0.7f*0.7f));
              momentX += x * w;
              momentY += y * w;
              momentX2 += x * x * w;
              momentXY += x * y * w;
              ws += w;
          }
      }
      momentX /= ws;
      momentY /= ws;
      momentX2 /= ws;
      momentXY /= ws;
      float A = (momentXY - momentX * momentY) / (max(momentX2 - momentX * momentX, 0.0f) + 0.00001f);
      float B = momentY - A * momentX;
      
      // Apply local exposure adjustment as a crude multiplier on all RGB channels.
      // This is... generally pretty wrong, but enough for the demo purpose.
      float3 texel = swi3(texture(tDiffuse, vUv),x,y,z);
      float3 texelOriginal = sqrt_f3(_fmaxf(ACESFilmicToneMapping(swi3(texture(tOriginal, vUv),x,y,z) * exposure), to_float3_s(0.0f)));
      float luminance = dot(swi3(texelOriginal,x,y,z), to_float3(0.1f,0.7f,0.2f)) + 0.00001f;
      float finalMultiplier = _fmaxf(A * luminance + B, 0.0f) / luminance;
      // This is a hack to prevent super dark pixels getting boosted by a lot and showing compression artifacts.
      float lerpToUnityThreshold = 0.007f;
      finalMultiplier = luminance > lerpToUnityThreshold ? finalMultiplier 
                                                         : _mix(1.0f, finalMultiplier, (luminance / lerpToUnityThreshold) * (luminance / lerpToUnityThreshold));
      float3 texelFinal = sqrt_f3(_fmaxf(ACESFilmicToneMapping(swi3(texture(tOriginal, vUv),x,y,z) * exposure * finalMultiplier), to_float3_s(0.0f)));
      return to_float4_aw(texelFinal, 1.0f);
 
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: InputTexture' to iChannel0



__KERNEL__ void LocalTonemappingFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_SLIDER0(Exposure, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(Shadows, -1.0f, 3.0f, 0.5f);
    CONNECT_SLIDER2(Highlights, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER3(SigmaSq, 0.0f, 10000.0f, 1000.0f);
    CONNECT_SLIDER4(BoostLocalContrast, -1.0f, 10.0f, 1.0f);
    
    pos+=0.5f;
float AAAAAAAAAAAA; 
    float2 vUv = pos/R;
    
    U = LuminanceShader(iChannel0, vUv, Exposure, Shadows, Highlights); // Buffer A

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel2

__KERNEL__ void LocalTonemappingFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_SLIDER0(Exposure, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(Shadows, -1.0f, 3.0f, 0.5f);
    CONNECT_SLIDER2(Highlights, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER3(SigmaSq, 0.0f, 10000.0f, 1000.0f);
    CONNECT_SLIDER4(BoostLocalContrast, -1.0f, 10.0f, 1.0f);

    pos+=0.5f;
float BBBBBBBBBBBBBBBBB;
    float2 vUv = pos/R;
    
    float4 mips0 = texture(iChannel0, vUv);
    
    float4 mipsWeights0 = ExposureWeightsShaderTex( mips0, SigmaSq);

    //__TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights
    float4 mipsAssemble0 = BlendShaderTex(mips0, mipsWeights0);
    
    //__TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights, __TEXTURE2D__ tExposuresCoarser, __TEXTURE2D__ tAccumSoFar
    U = BlendLaplacianShaderTex(mips0, mipsWeights0, mips0, mipsAssemble0, BoostLocalContrast);
    

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


#define radius 2.0f

__KERNEL__ void LocalTonemappingFuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel0)
{
    pos+=0.5f;  

    float2 vUv = pos/R;
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Organic 1' to iChannel2
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1



__KERNEL__ void LocalTonemappingFuse__Buffer_D(float4 fragColor, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    pos+=0.5f;

    float2 vUv = pos/R;
    
    //initial condition
    if(iFrame < 1 || Reset)
    {
       swi2S(fragColor,x,y, to_float2_s(0.0f));
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Texture: InputTexture' to iChannel2





__KERNEL__ void LocalTonemappingFuse(float4 col, float2 pos, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    
    CONNECT_CHECKBOX0(SingleShaderTest, 0);
    CONNECT_CHECKBOX1(FinalShader, 0);
    
    CONNECT_BUTTON0(Shader, 0, Luminance, ExposureWeights, Blend, BLaplacian, Final);
    
    CONNECT_SLIDER0(Exposure, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(Shadows, -1.0f, 3.0f, 0.5f);
    CONNECT_SLIDER2(Highlights, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER3(SigmaSq, 0.0f, 10000.0f, 1000.0f);
    CONNECT_SLIDER4(BoostLocalContrast, -1.0f, 10.0f, 1.0f);
float IIIIIIIIIIIIIIIIIIII;    
    pos+=0.5f;    

    float2 vUv = pos/R;

    col = texture(iChannel2, vUv);
    
    // Original ACESFilmicToneMappingShader.js
    //tex.rgb *= exposure / 0.6; // pre-exposed, outside of the tone mapping function
		col = to_float4_aw( ACESFilmicToneMapping( swi3(col,x,y,z)) * Exposure / 0.6f, col.w );

    if (Shader == 1) //Luminance
    {
       col = LuminanceShader(iChannel2, vUv, Exposure, Shadows, Highlights);
    }
    else

    if (Shader == 2) //ExposureWeights
    {
       col = ExposureWeightsShader(iChannel2, vUv, SigmaSq);
    }
    else

    if (Shader == 3) //Blend
    {
       col = BlendShader(iChannel2, iChannel2, vUv);
    }
    else

    if (Shader == 4) //BlendLaplacianShader
    {
       col = BlendLaplacianShader(iChannel2, iChannel2, iChannel2, iChannel2, vUv, BoostLocalContrast);
    }
    else

    if (Shader == 5) //FinalCombinePassShader
    {
       col = FinalCombinePassShader(iChannel2, iChannel2, iChannel2, vUv, to_float4_s(0.0f), Exposure);
    }


    if(SingleShaderTest)
    {
      float4 mips0 = LuminanceShader(iChannel2,vUv, Exposure, Shadows, Highlights); // Buffer A
      
      
      float4 mipsWeights0 = ExposureWeightsShaderTex( mips0, SigmaSq);              // Buffer B

      //__TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights
      float4 mipsAssemble0 = BlendShaderTex(mips0, mipsWeights0);
      
      //__TEXTURE2D__ tExposures, __TEXTURE2D__ tWeights, __TEXTURE2D__ tExposuresCoarser, __TEXTURE2D__ tAccumSoFar
      float4 mipsAssemble1 = BlendLaplacianShaderTex(mips0, mipsWeights0, mips0, mipsAssemble0, BoostLocalContrast);
      
      //__TEXTURE2D__ tDiffuse, __TEXTURE2D__ tOriginal, __TEXTURE2D__ tOriginalMip
      col = FinalCombinePassShaderTex(iChannel1, iChannel2, iChannel0, vUv, to_float4_s(0.0f), Exposure);
      
    }

    if(FinalShader)
    {
       float4 mipPixelSize = to_float4_f2f2(iResolution, 1.0f/iResolution);
       col = FinalCombinePassShader(iChannel1, iChannel2, iChannel0, vUv, mipPixelSize, Exposure);
    }

  SetFragmentShaderComputedColor(col);
}