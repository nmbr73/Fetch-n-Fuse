

// history:
//  - v.01, mac browser crash fixed
//  - v.00, f1rs7 p05t

#define TRACE_STEPS 128
#define TRACE_EPSILON 0.001f
#define REFLECT_EPSILON 0.1f
#define TRACE_DISTANCE 30.0f
#define NORMAL_EPSILON 0.01f
#define REFLECT_DEPTH 4
#define CUBEMAP_SIZE 128






__DEVICE__ float world(float3 at, int NUM_BALLS, POINTERPARAMETER const float3* balls) {
	//return touching_balls(at);
	float sum = 0.0f;
	for (int i = 0; i < NUM_BALLS; ++i) {
		float r = length(balls[i] - at);
		sum += 1.0f / (r * r);
	}
	return 1.0f - sum;
}


__DEVICE__ float3 lookAtDir(in float3 dir, in float3 pos, in float3 at) {
	float3 f = normalize(at - pos);
	float3 r = cross(f, to_float3(0.0f,1.0f,0.0f));
	float3 u = cross(r, f);
	return normalize(dir.x * r + dir.y * u + dir.z * f);
}


__KERNEL__ void BallsAreTouchingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0 ) {

  CONNECT_INTSLIDER0(numBalls,1,20,7);

  float t = iTime * 0.11f;

  #define MAX_BALLS 20

  float3 balls[MAX_BALLS];

	// update_balls(t);
  {

    for (int i = 0; i < numBalls; ++i) {
		balls[i] = 3.0f * to_float3(
                              _sinf(0.3f+(float)(i+1)*t),
                              _cosf(1.7f+(float)(i-5)*t),
                              1.1f*sin(2.3f+(float)(i+7)*t));
	  }
  }



	float aspect = iResolution.x / iResolution.y;
	float2 uv = (fragCoord / iResolution * 2.0f - 1.0f) * to_float2(aspect, 1.0f);

	float3 pos = to_float3(cos(2.0f+4.0f*cos(t))*10.0f, 2.0f+8.0f*cos(t*0.8f), 10.0f*sin(2.0f+3.0f*cos(t)));
	float3 dir = lookAtDir(normalize(to_float3_aw(uv, 2.0f)), swi3(pos,x,y,z), balls[0]);

	fragColor = to_float4_s(0.0f);
	float k = 1.0f;
	for (int reflections = 0; reflections < REFLECT_DEPTH; ++reflections) {

    float4 tpos; // = raymarch(pos, dir, TRACE_DISTANCE);
    {
      float l = 0.0f;
      for (int i = 0; i < TRACE_STEPS; ++i) {
        float d = world(pos + dir * l,numBalls,balls);
        if (d < TRACE_EPSILON*l) break; // if we return here, browser will crash on mac os x, lols
        l += d;
        if (l > TRACE_DISTANCE) break;
      }
      tpos= to_float4_aw(pos + dir * l, l);
    }

		if (tpos.w >= TRACE_DISTANCE) {


      // http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
      // __DEVICE__ float3 cube(in float3 dir)
      {
        float M = max(max(_fabs(dir.x), _fabs(dir.y)), _fabs(dir.z));
        float scale = ((float)(CUBEMAP_SIZE) - 1.0f) / (float)(CUBEMAP_SIZE);
        if (_fabs(dir.x) != M) dir.x *= scale;
        if (_fabs(dir.y) != M) dir.y *= scale;
        if (_fabs(dir.z) != M) dir.z *= scale;
      }


			fragColor += decube_f3(iChannel0, dir);
			break;
		}
		fragColor += to_float4_s(0.1f) * k;
		k *= .6;

    // normal
    float3 at=swi3(tpos,x,y,z);
    {
      
	    float2 e = to_float2(0.0f, NORMAL_EPSILON);
	    at = normalize(to_float3(
              world(at+swi3(e,y,x,x),numBalls,balls)-world(at,numBalls,balls),
						  world(at+swi3(e,x,y,x),numBalls,balls)-world(at,numBalls,balls),
						  world(at+swi3(e,x,x,y),numBalls,balls)-world(at,numBalls,balls)
              ));
    }

		dir = normalize(reflect(dir, at));
		pos = swi3(tpos,x,y,z) + dir * REFLECT_EPSILON;
	}

  fragColor.w=1.0f;
  SetFragmentShaderComputedColor(fragColor);

}