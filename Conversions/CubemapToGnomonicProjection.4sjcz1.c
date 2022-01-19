
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Cubemap: Uffizi Gallery_0' to iChannel0
// Connect '/presets/tex00.jpg' to iChannel1


/**
 * Cubemap to Gnomonic / Rectilinear unwrapping by Ruofei Du (DuRuofei.com)
 * Link to demo: https://www.shadertoy.com/view/4sjcz1
 * starea @ ShaderToy, License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
 * https://creativecommons.org/licenses/by-nc-sa/3.0f/
 *
 * Reference:
 * [1] Gnomonic projection: https://en.wikipedia.org/wiki/Gnomonic_projection
 * [2] Weisstein, Eric W. "Gnomonic Projection." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/GnomonicProjection.html
 *
 **/
#define PI  3.1415926536f // const float PI = 3.1415926536f;
#define PI_2 (PI * 0.5f) // const float PI_2 = PI * 0.5f;
#define PI2 (PI * 2.0f) // const float PI2 = PI * 2.0f;
#define KEY_SPACE  32 // const int KEY_SPACE = 32;
#define KEY_ENTER  13 // const int KEY_ENTER = 13;
#define KEY_1  49 // const int KEY_1 = 49;
#define KEY_2  50 // const int KEY_2 = 50;
#define KEY_3  51 // const int KEY_3 = 51;



// Forked from fb39ca4's keycode viewer tool @ https://www.shadertoy.com/view/4tt3Wn
__DEVICE__ float keyPressed(int keyCode) {

  return 0.0f; // !!!
  // return texture(iChannel1, to_float2((float(keyCode) + 0.5f) / 256.0f, 0.5f/3.0f)).r;
}

// Main function, convert screen coordinate system to spherical coordinates in gnomonic projection
// screenCoord: [0, 1], centralPoint: [0, 1], FoVScale: to_float2(0.9f, 0.2f) recommended
__DEVICE__ float2 calcSphericalCoordsInGnomonicProjection(in float2 screenCoord, in float2 centralPoint, in float2 FoVScale) {
    float2 cp = (centralPoint * 2.0f - 1.0f) * to_float2(PI, PI_2);  // [-PI, PI], [-PI_2, PI_2]

    // Convert screen coord in gnomonic mapping to spherical coord in [PI, PI/2]
    float2 convertedScreenCoord = (screenCoord * 2.0f - 1.0f) * FoVScale * to_float2(PI, PI_2);
    float x = convertedScreenCoord.x, y = convertedScreenCoord.y;

    float rou = _sqrtf(x * x + y * y), c = atan(rou);
  float sin_c = _sinf( c ), cos_c = _cosf( c );

    float lat = asin(cos_c * _sinf(cp.y) + (y * sin_c * _cosf(cp.y)) / rou);
  float lon = cp.x + _atan2f(x * sin_c, rou * _cosf(cp.y) * cos_c - y * _sinf(cp.y) * sin_c);

  lat = (lat / PI_2 + 1.0f) * 0.5f; lon = (lon / PI + 1.0f) * 0.5f; //[0, 1]

    // uncomment the following if centralPoint ranges out of [0, PI/2] [0, PI]
  // while (lon > 1.0f) lon -= 1.0f; while (lon < 0.0f) lon += 1.0f;
  // while (lat > 1.0f) lat -= 1.0f; while (lat < 0.0f) lat += 1.0f;

    // convert spherical coord to cubemap coord
   return (bool(keyPressed(KEY_SPACE)) ? screenCoord : to_float2(lon, lat)) * to_float2(PI2, PI);
}

// convert cubemap coordinates to spherical coordinates:
__DEVICE__ float3 sphericalToCubemap(in float2 sph) {
    return to_float3(_sinf(sph.y) * _sinf(sph.x), _cosf(sph.y), _sinf(sph.y) * _cosf(sph.x));
}

// convert screen coordinate system to cube map coordinates in rectilinear projection
__DEVICE__ float3 calcCubeCoordsInGnomonicProjection(in float2 screenCoord, in float2 centralPoint, in float2 FoVScale) {
  return sphericalToCubemap( calcSphericalCoordsInGnomonicProjection(screenCoord, centralPoint, FoVScale) );
}

// the inverse function of calcSphericalCoordsInGnomonicProjection()
__DEVICE__ float2 calcEquirectangularFromGnomonicProjection(in float2 sph, in float2 centralPoint) {
    float2 cp = (centralPoint * 2.0f - 1.0f) * to_float2(PI, PI_2);
  float cos_c = _sinf(cp.y) * _sinf(sph.y) + _cosf(cp.y) * _cosf(sph.y) * _cosf(sph.y - cp.y);
    float x = _cosf(sph.y) * _sinf(sph.y - cp.y) / cos_c;
    float y = ( _cosf(cp.y) * _sinf(sph.y) - _sinf(cp.y) * _cosf(sph.y) * _cosf(sph.y - cp.y) ) / cos_c;
    return to_float2(x, y) + to_float2(PI, PI_2);
}

// Forked from: https://www.shadertoy.com/view/MsXGz4, press enter for comparison
__DEVICE__ float3 iqCubemap(in float2 q, in float2 mo, float aspect) {
    float2 p = -1.0f + 2.0f * q;
    p.x *= aspect; // iResolution.x / iResolution.y;

    // camera
  float an1 = -6.2831f * (mo.x + 0.25f);
  float an2 = clamp( (1.0f-mo.y) * 2.0f, 0.0f, 2.0f );
    float3 ro = 2.5f * normalize(to_float3(_sinf(an2)*_cosf(an1), _cosf(an2)-0.5f, _sinf(an2)*_sinf(an1)));
    float3 ww = normalize(to_float3(0.0f, 0.0f, 0.0f) - ro);
    float3 uu = normalize(cross( to_float3(0.0f, -1.0f, 0.0f), ww ));
    float3 vv = normalize(cross(ww, uu));
    return normalize( p.x * uu + p.y * vv + 1.4f * ww );
}

__KERNEL__ void CubemapToGnomonicProjectionFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

  float2 q = fragCoord / iResolution;

  // Modify this to adjust the field of view
    float2 FoVScale = to_float2(0.45f, 0.4f);
    if (bool(keyPressed(KEY_1))) {
        FoVScale = to_float2(0.225f, 0.2f);
    } else if (bool(keyPressed(KEY_2))) {
        FoVScale = to_float2(1.0f, 1.0f);
    } else if (bool(keyPressed(KEY_3))) {
        FoVScale = to_float2(0.5f, 0.5f);
    }

    // central / foveated point, swi2(iMouse,x,y) corresponds to longitude and latitude
    float2 centralPoint = (length(swi2(iMouse,x,y)) < 1e-4) ? to_float2(0.25f, 0.5f) : (swi2(iMouse,x,y) / iResolution);

    // press enter to compare with iq's cubemaps https://www.shadertoy.com/view/MsXGz4
    float3 dir = (!bool(keyPressed(KEY_ENTER))) ? calcCubeCoordsInGnomonicProjection(q, centralPoint, FoVScale) : iqCubemap(q, centralPoint, iResolution.x / iResolution.y);

  //float3 col = _tex2DVecN(iChannel0,dir.x,dir.y,15).rgb;
  float4 tmp = decube_f3(iChannel0,dir);
  float3 col = to_float3(tmp.x,tmp.y,tmp.z);

    // test if the inverse function works correctly by pressing z
    if (bool(keyPressed(90))) {
      float2 sph = calcSphericalCoordsInGnomonicProjection(q, centralPoint, FoVScale);
        sph = calcEquirectangularFromGnomonicProjection(sph, centralPoint);
        col = to_float3_aw(sph / to_float2(PI2, PI), 0.5f);
    }

    col *= 0.25f + 0.75f * _powf( 16.0f * q.x * q.y * (1.0f - q.x) * (1.0f - q.y), 0.15f );

    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}