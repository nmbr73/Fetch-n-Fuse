
// AUs Fehlermeldungen ersichtlich:
//
// #define _tex2DVecN(texID, X, Y, O)   make_intensity(_tex2DVec4((texID), (X), (Y)), (O))
// #define _tex2DVec4(texID, X, Y)      texID.sample(RowSampler, float2((float)(X), (float)(Y)))
//
// typedef vec<float, 4> float4
// vec<float, 4> _tex2DVec4(?,float x,floaty)
//
// __DEVICE__ inline float4 make_intensity(float4 p_Tex, uchar p_Order)


  // System/Library/PrivateFrameworks/GPUCompiler.framework/Versions/31001/Libraries/lib/clang/31001.192/include/metal/metal_texture
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, int2 offset = int2(0)) const thread
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, bias options, int2 offset = int2(0)) const thread
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, level options, int2 offset = int2(0)) const thread
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, gradient2d options, int2 offset = int2(0)) const thread
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, bias options, int2 offset = int2(0)) const device
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, level options, int2 offset = int2(0)) const device
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, gradient2d options, int2 offset = int2(0)) const device
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, bias options, int2 offset = int2(0)) const constant
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, level options, int2 offset = int2(0)) const constant
  // METAL_FUNC vec<T, 4> sample(sampler s, float2 coord, gradient2d options, int2 offset = int2(0)) const constant


__KERNEL__ void CubemapDebugDisplayFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float2 iChannelResolution[], sampler2D iChannel0)
{
  float x = fragCoord.x;
  float y = fragCoord.y;

  float3 samplePos = to_float3_s(0.0f);

  if (0 && (x>=iChannelResolution[0].x || y>=iChannelResolution[0].y)) {
    fragColor=to_float4(1.0f,0.0f,0.0f,0.0f);
  } else {

    float2 inUV = fragCoord/iResolution;
    //float2 inUV = fragCoord/iChannelResolution[0].xy;


    fragColor = to_float4(0.0f, 0.0f, 0.2f, 0.0f);


    // Crude statement to visualize different cube map faces based on UV coordinates
    int x = int(floor(inUV.x / 0.25f));
    int y = int(floor(inUV.y / (1.0f / 3.0f)));
    if (y == 1) {
      float2 uv = to_float2(inUV.x * 4.0f, (inUV.y - 1.0f/3.0f) * 3.0f);
      uv = 2.0 * to_float2(uv.x - float(x) * 1.0f, uv.y) - 1.0f;
      switch (x) {
        case 0:	// NEGATIVE_X
          samplePos = to_float3(-1.0f, uv.y, uv.x);
          break;
        case 1: // POSITIVE_Z
          samplePos = to_float3(uv.x, uv.y, 1.0f);
          break;
        case 2: // POSITIVE_X
          samplePos = to_float3(1.0, uv.y, -uv.x);
          break;
        case 3: // NEGATIVE_Z
          samplePos = to_float3(-uv.x, uv.y, -1.0f);
          break;
      }
    } else {
      if (x == 1) {
        float2 uv = to_float2((inUV.x - 0.25f) * 4.0f, (inUV.y - float(y) / 3.0f) * 3.0f);
        uv = 2.0f * uv - 1.0f;
        switch (y) {
          case 0: // NEGATIVE_Y
            samplePos = to_float3(uv.x, -1.0f, uv.y);
            break;
          case 2: // POSITIVE_Y
            samplePos = to_float3(uv.x, 1.0f, -uv.y);
            break;
        }
      }
    }

    if ((samplePos.x != 0.0f) && (samplePos.y != 0.0f)) {
      fragColor = unfold_cube_f3(iChannel0,samplePos);
      fragColor.w=1.0f;
    }
  }

   SetFragmentShaderComputedColor(fragColor);
}

/*
  float x = fragCoord.x;
  float y = fragCoord.y;

  if (x>=iChannelResolution[0].x || y>=iChannelResolution[0].y) {
    fragColor=to_float4(1.0f,0.0f,0.0f,0.0f);
  } else {

    //float uv_x = fragCoord.x / iResolution.x;
    //float uv_y = fragCoord.y / iResolution.y;
    float uv_x = fragCoord.x / iChannelResolution[0].x;
    float uv_y = fragCoord.y / iChannelResolution[0].y;
    float uv_z = 0.0f;

    //fragColor = _tex2DVecN(iChannel0,uv_x,uv_y,15);
    fragColor = unfold_cube_pixel(iChannel0,uv_x,uv_y,uv_y);
  }
*/




  // // Normalized pixel coordinates (from 0 to 1)
  // float2 inUV = fragCoord/iResolution.xy;

  // fragColor = to_float4(0.0f, 0.0f, 0.2f, 0.0f);

	// float3 samplePos = to_float3_s(0.0f);

	// // Crude statement to visualize different cube map faces based on UV coordinates
	// int x = int(floor(inUV.x / 0.25f));
	// int y = int(floor(inUV.y / (1.0f / 3.0f)));
	// if (y == 1) {
	// 	float2 uv = to_float2(inUV.x * 4.0f, (inUV.y - 1.0f/3.0f) * 3.0f);
	// 	uv = 2.0 * to_float2(uv.x - float(x) * 1.0f, uv.y) - 1.0f;
	// 	switch (x) {
	// 		case 0:	// NEGATIVE_X
	// 			samplePos = to_float3(-1.0f, uv.y, uv.x);
	// 			break;
	// 		case 1: // POSITIVE_Z
	// 			samplePos = to_float3(uv.x, uv.y, 1.0f);
	// 			break;
	// 		case 2: // POSITIVE_X
	// 			samplePos = to_float3(1.0, uv.y, -uv.x);
	// 			break;
	// 		case 3: // NEGATIVE_Z
	// 			samplePos = to_float3(-uv.x, uv.y, -1.0f);
	// 			break;
	// 	}
	// } else {
	// 	if (x == 1) {
	// 		float2 uv = to_float2((inUV.x - 0.25f) * 4.0f, (inUV.y - float(y) / 3.0f) * 3.0f);
	// 		uv = 2.0f * uv - 1.0f;
	// 		switch (y) {
	// 			case 0: // NEGATIVE_Y
	// 				samplePos = to_float3(uv.x, -1.0f, uv.y);
	// 				break;
	// 			case 2: // POSITIVE_Y
	// 				samplePos = to_float3(uv.x, 1.0f, -uv.y);
	// 				break;
	// 		}
	// 	}
	// }

	// if ((samplePos.x != 0.0f) && (samplePos.y != 0.0f)) {
	// 	//fragColor = to_float4(unfold_cube_pixel(iChannel0, samplePos).rgb, 1.0);
	// 	//fragColor = to_float4(unfold_cube_pixel(iChannel0, samplePos).rgb, 1.0);
	// 	fragColor = to_float4_aw(unfold_cube_pixel(iChannel0, samplePos.x,samplePos.y,samplePos.z), 1.0f);
	// }
