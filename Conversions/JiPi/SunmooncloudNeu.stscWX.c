
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

__DEVICE__ mat3 transpose(mat3 m)
{
    return(to_mat3(m.r0.x,m.r1.x,m.r2.x, m.r0.y,m.r1.y,m.r2.y, m.r0.z,m.r1.z,m.r2.z)); 	
}

// Environment
__DEVICE__ const float Re = 6360e3; //planet radius //6360e3 actual 6371km
__DEVICE__ const float Ra = 6380e3; //atmosphere radius //6380e3 troposphere 8 to 14.5km
__DEVICE__ const float Rm = 1738e3;  // moon radius
__DEVICE__ const float Rs = 695500e3;  // sun radius
__DEVICE__ const float Lse = 149600000e3;  // distance from sun to earth
__DEVICE__ const float Lem = 384400e3;  // distance from earth to moon
__DEVICE__ const float Is = 20.0f; //sun light power, 10.0f is normal
__DEVICE__ const float Im = 0.5f; //moon light power
__DEVICE__ const float g = 0.5f; //light concentration 0.76f //0.45f //0.6f  0.45f is normaL
__DEVICE__ const float g2 = 0.5f * 0.5f;// g * g;

// cloud
__DEVICE__ const float cloudHeightTop = 40e2;
__DEVICE__ const float cloudHeightBottom = 15e2;
__DEVICE__ const float cloudForwardScatteringG = 0.8f;
__DEVICE__ const float cloudBackwardScatteringG = -0.2f;
__DEVICE__ const float cloudScatteringLerp = 0.5f;
__DEVICE__ const float cloudMinTransmittance = 0.1f;
__DEVICE__ const float cloudMaxDistance = 30e3;  //max render distance
#define CLOUDS_AMBIENT_COLOR_TOP (to_float3(149.0f, 167.0f, 200.0f)*1.5f/255.0f)
#define CLOUDS_AMBIENT_COLOR_BOTTOM (to_float3(39.0f, 67.0f, 87.0f)*1.5f/255.0f)

__DEVICE__ const float Hr = 8e3; //Rayleigh scattering top //8e3
__DEVICE__ const float Hm = 1.2e3; //Mie scattering top //1.3e3





#define PI  3.14159265358979323846f
#define TWO_PI  6.28318530717958647692f

// keyboard control
//const int KEY_LEFT  = 37;
//const int KEY_UP    = 38;
//const int KEY_RIGHT = 39;
//const int KEY_DOWN  = 40;
// East direction




#define perlinRange  (_sqrtf(2.0f) / 2.0f)
__DEVICE__ const float coverage = 1.0f;
__DEVICE__ const int surround = 2;

__DEVICE__ mat3 getCamera( in float3 ro, in float3 ta) {
    float3 cw = normalize(ro - ta);
    float3 up = to_float3(0,1,0);
    //if(abs_f3(cw) == up)
      if(_fabs(cw.x) == up.x && _fabs(cw.y) == up.y && _fabs(cw.z) == up.z)
           up = to_float3(0,0,1);
    float3 cu = normalize(cross(up, cw));
    float3 cv = normalize(cross(cw, cu));
    return to_mat3_f3(cu, cv, cw);
}

__DEVICE__ float remap(const float originalValue, const float originalMin, const float originalMax, const float newMin, const float newMax) {
  return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

__DEVICE__ float HenyeyGreenstein(float costheta, float g) {
  const float k = 0.079577471546f;  // 1/(4pi)
  float g2 = g * g;
  return k*(1.0f-g2)/(_powf(1.0f+g2-2.0f*g*costheta, 1.5f));
}

__DEVICE__ float hash(float2 p) {
    p  = 100.0f*fract_f2( p*0.3183099f + to_float2(0.71f,0.113f));
    return -1.0f+2.0f*fract( p.x*p.y*(p.x+p.y) );
}

__DEVICE__ float hash13(float3 p3) {
    p3  = fract_f3(p3 * 1031.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float perlinNoise2D( in float2 p) {
    float2 i = _floor( p );
    float2 f = fract_f2( p );

    float2 u = f*f*(3.0f-2.0f*f);

    return _mix( _mix( hash(i + to_float2(0.0f,0.0f)), 
                       hash(i + to_float2(1.0f,0.0f)), u.x),
                 _mix( hash(i + to_float2(0.0f,1.0f)), 
                       hash(i + to_float2(1.0f,1.0f)), u.x), u.y);
}

__DEVICE__ float perlinNoiseFbm2D( float2 p, const int octaves, float frequency, float coverage ) {
    // noise control
    const mat2 m2 = to_mat2(  0.80f,  0.60f,
                             -0.60f,  0.80f );
    
    p *= frequency;
    float G = 0.5f;
    float f = 1.0f;
    float a = 0.5f;
    float t = 0.0f;

    for( int i=0; i<octaves; i++ ) {
        t += a*perlinNoise2D( mul_mat2_f2(m2 , f * p));
        f *= 2.0f;
        a *= G;
    }
    float range = -perlinRange + 2.0f * coverage * perlinRange;
    t += range;
    if(t > 0.0f)
        t = remap(t, 0.0f, range + perlinRange, 0.0f, 1.0f);
    //t = _fabs(-2.0f * t + 1.0f);
    return t;
}

__DEVICE__ float voronoiNoise2D(in float2 x, float frequency) {
    x *= frequency;
  float2 xi = _floor(x);
  float2 xf = fract_f2(x);
    float m_dist = 1.0f;
    for( int x=-surround; x<=surround; x++ ) {
        for( int y=-surround; y<=surround; y++ ) {
            // Neighbor place in the grid
            float2 neighbor = to_float2((float)(x),(float)(y));

            //vec2 point = to_float2(_fabs(perlinNoise2D(xi + neighbor)), hash(xi + neighbor));
            // Vector between the pixel and the point
            float2 diff = neighbor + _fabs(hash(xi + neighbor)) - xf;

            float dist = length(diff);

            // Keep the closer distance
            if(dist < m_dist) {
                m_dist = _fminf(m_dist, dist);
            }
        }
    }
    return m_dist;
}

__DEVICE__ float voronoiNoiseFbm2D( float2 p, const int octaves, float frequency ) {
    float G = 0.5f;
    float f = 1.0f;
    float a = 0.5f;
    float t = 0.0f;
    float w = 0.0f;

    for( int i=0; i<octaves; i++ ) {
        t += a*voronoiNoise2D( p * f, frequency );
        f *= 2.0f;
        w += a;
        a *= G;
    }

    return t / w;
}

__DEVICE__ float getHeight(float3 p) {
  return length(p) - Re;
}

__DEVICE__ float getHeightFraction(in float3 p) {
    float height = length(p) - Re;
    float height_fraction = (height - cloudHeightBottom) / (cloudHeightTop - cloudHeightBottom);
    return clamp(height_fraction, 0.0f, 1.0f);
}

__DEVICE__ float rayAtmosphereIntersect(in float3 ro, in float3 rd) {
  float b = dot(ro, rd);
    float d = dot(ro, ro);
  float c = Ra*Ra - d;
  float det = _sqrtf(b * b + c);
  return c / (det + b);
}

__DEVICE__ float raySphereIntersect(in float3 ro, in float3 rd, float4 sph) {
  float3 oc = ro - swi3(sph,x,y,z);
  float b = dot( oc, rd );
  float c = dot( oc, oc ) - sph.w*sph.w;
  float h = b*b - c;
  if( h<0.0f ) return -1.0f;
  return -b - _sqrtf( h );
}

__DEVICE__ float2 rayCloudIntersect(in float3 ro, in float3 rd) {
  float b = dot(ro, rd);
    float d = dot(ro, ro);
    float Rct = Re + cloudHeightTop;
    float Rcb = Re + cloudHeightBottom;
    if(d < Rcb * Rcb) {
        float2 c = to_float2(Rcb*Rcb, Rct*Rct) - d;
        float2 det = sqrt_f2(b * b + c);
        return c / (det + b);
    } else {
        float c = Rct*Rct - d;
        float det = _sqrtf(b * b + c);
        return to_float2(0, c / (det + b));
    }
}

__DEVICE__ float disGroundToPlanet(in float earthRadius, in float orbitRadius, in float angle) {
    float radio = earthRadius / orbitRadius;
    return orbitRadius * _sqrtf(1.0f + radio * radio - 2.0f * radio * _sinf(angle));
}

__DEVICE__ float3 sphNormal( in float3 pos, in float4 sph ) {
    return normalize(pos-swi3(sph,x,y,z));
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Lichen' to iChannel1
// Connect Image 'Preset: Keyboard' to iChannel0


/*
 * The sun, the earth and the moon by batersy
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0f International License.
 * Contact: seplanely@gmail.com
 */

// update history
// 22/03/16: Add movement to sun and moon, and moon phase in a month, 
// atmosphere reference on https://www.shadertoy.com/view/tdSXzD
// 22/03/31: Add simple cloud and repost, 
// reference on https://www.shadertoy.com/view/MdGfzh
// will keep on updating new things here

// Camera config
#define fov _tanf(PI / 3.0f)
__DEVICE__ const float cameraheight = 0.0f;

// Ray Marching config
__DEVICE__ const int steps = 16;
__DEVICE__ const int stepss = 6;
__DEVICE__ const int stepsCloud = 18;
__DEVICE__ const int stepssCloud = 6;

// sun and moon cycle
__DEVICE__ const float dayTime = 24.0f;  // how many time in a day
__DEVICE__ const float monthDay = 30.0f;  // how many days in a month
// earth latitude, range PI / 2  PI / 2
__DEVICE__ float latitude = 35.0f / 180.0f * PI;

// circle light smoothstep
__DEVICE__ float sunSmooth = 0.02f;
__DEVICE__ float moonSmooth = 0.2f;


// cloud config
__DEVICE__ const float cloudCoverage = 0.6f;  //cloud generate quantity
__DEVICE__ const float cloudPadding = 0.0f;  //cloud thickness padding
__DEVICE__ const float segmaMax = 0.004f;  // scattering coefficient in max cloud density
// 0: Basic scatering/transmittance integration
// 1: Improved Scattering integration by SebH, see https://www.shadertoy.com/view/XlBSRz
// 2: Improved Scattering integration with ambient light, see https://www.shadertoy.com/view/MdGfzh
#define CLOUD_RENDER_TYPE 2
// 0: fixed step shadow
// 1: increased step shadow
#define CLOUD_SHADOW_RENDER_TYPE 1

__DEVICE__ float3 sphTexureMap(in float3 pos, in float4 sph, __TEXTURE2D__ iChannel1) {
    float theta = asin(pos.y / sph.w);
    float phi = asin(pos.x / (sph.w * _cosf(theta)));
    float2 uv = to_float2(phi, theta);
    uv = (uv + PI / 2.0f) / PI;
    uv.x /= 2.0f;
    uv.x = mod_f(uv.x + 0.26f, 1.0f);
    float3 col = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    return col;
}

__DEVICE__ float heightGradient(float cloudType, float h) {
    float center = 0.1f + cloudType * 0.4f;
    float range = 0.05f + cloudType * 0.35f;
    float solid = 0.02f + cloudType * 0.28f;
    float gradient = smoothstep(center - range, center - solid, h);
    gradient -= smoothstep(center + range, center + solid, h);
    return clamp(gradient, 0.0f, 1.0f);
}

__DEVICE__ float cloudNoise(float2 p) {
    float perlin = perlinNoiseFbm2D(p, 2, 4.0f, cloudCoverage);
    float voronoi = voronoiNoiseFbm2D(p, 2, 12.0f);
    float perlinWorley = remap(perlin, voronoi, 1.0f, 0.0f, 1.0f);
    float baseShape = perlinWorley;
#if 0
    float highFreq =  0.625f * voronoiNoise2D(p, 4.0f) +
            0.250f * voronoiNoise2D(p, 8.0f) +
            0.125f * voronoiNoise2D(p, 16.0f);
    baseShape = remap(perlinWorley, -highFreq, 1.0f, 0.0f, 1.0f);
#endif
    return baseShape;
}

__DEVICE__ float cloudDensity(float3 p, float iTime) {
    float2 samplePos = swi2(p,x,z) / 10000.0f;
    samplePos.x += iTime / 5.0f;
    samplePos.y += iTime / 20.0f;
    samplePos += clamp((p.z - p.x) / p.y, 0.0f, 1000.0f);
    float heightTop = cloudNoise(samplePos) + cloudPadding;
    float heightBottom = (1.0f-heightTop) * 0.2f;
    float baseCloud = 0.0f;
    float heightFraction = getHeightFraction(p);
    if(heightFraction < heightTop && heightFraction > heightBottom) baseCloud = 0.3f;
    //return density;
    /*float Rc = Re + cloudHeightTop;
    float horizon = _sqrtf(Rc * Rc - Re * Re) * 2.0f;
    float2 samplePos = swi2(p,x,z) / 30000.0f;
    samplePos.x += iTime / 50.0f;
    samplePos.y += iTime / 200.0f;
    float3 tex = _tex2DVecN(iChannel2,samplePos.x,samplePos.y,15).rgb;
    float baseCloud = tex.x;*/

    /*samplePos /= 4.0f;
    float3 tex = _tex2DVecN(iChannel2,samplePos.x,samplePos.y,15).rgb;
    float cloudType = tex.y;
    float heightFraction = getHeightFraction(p);
    float gradient = heightGradient(cloudType, heightFraction);
    //baseCloud *= gradient;*/
    return baseCloud;
}

__DEVICE__ float cloudEnergy(float opticalDepth) {
    float beer = _expf(-opticalDepth);
    float powder = 0.5f;
float zzzzzzzzzzzzzzzzz;
    //float powder = 1.0f - beer * beer;
    return 2.0f * beer * powder;
}

__DEVICE__ float3 volumetricAtmosphereShadow(in float3 ro, in float3 sd, float3 betaR, float3 betaM) {
    float opticalDepthLightR = 0.0f, opticalDepthLightM = 0.0f;
    float Ls = rayAtmosphereIntersect(ro, sd);
    if (Ls > 0.0f) {
        float dls = Ls / (float)(stepss);
        float3 samplePositionLight = ro + 0.5f * dls * sd;
        for (int j = 0; j < stepss; ++j) {
            float heightLight = getHeight(samplePositionLight);
            opticalDepthLightR += _expf(-heightLight / Hr) * dls;
            opticalDepthLightM += _expf(-heightLight / Hm) * dls;
            samplePositionLight += dls * sd;
        }
    }
    return betaR * opticalDepthLightR + betaM * 1.1f * opticalDepthLightM;
}

__DEVICE__ float3 rayMarchingAtmosphere(float3 ro, float3 rd, float3 sd, float I, out float3 *transmittance, float3 betaR, float3 betaM) {
  rd.y = _fabs(rd.y);
  *transmittance = to_float3_s(1);
  float L = rayAtmosphereIntersect(ro, rd);
  float mu = dot(rd, sd); // mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
  float phaseR = 3.0f / (16.0f * PI) * (1.0f + mu * mu);
  float phaseM = 3.0f / (8.0f * PI) * ((1.0f - g2) * (1.0f + mu * mu)) / ((2.0f + g2) * _powf(1.0f + g2 - 2.0f * g * mu, 1.5f));
  float3 sumR = to_float3_s(0), sumM = to_float3_s(0); // mie and rayleigh contribution
    
  float dl = L / (float)(steps);
  float3 samplePosition = ro + 0.5f * dl * rd;
  for (int i = 0; i < steps; ++i) {
        float height = getHeight(samplePosition);
        // compute optical depth for light
        float hr = _expf(-height / Hr) * dl;
        float hm = _expf(-height / Hm) * dl;

        float3 shadow = exp_f3(-1.0f*volumetricAtmosphereShadow(samplePosition, sd, betaR, betaM));
        *transmittance *= exp_f3(-1.0f*(betaR * hr + betaM * 1.1f * hm));
        sumR += *transmittance * shadow * hr; 
        sumM += *transmittance * shadow * hm;
        samplePosition += dl * rd;
  }

  return (sumR * betaR * phaseR + sumM * betaM * phaseM) * I;
}

__DEVICE__ float volumetricShadow(in float3 ro, in float3 sd, float iTime) {
    float opticalDepthLight = 0.0f;
#if CLOUD_SHADOW_RENDER_TYPE==0
    float2 intersectPosLight = rayCloudIntersect(ro, sd);
    float dl = intersectPosLight.y / float(stepssCloud);
    float3 pos = ro + 0.5f * dl * sd;
    for (int i = 0; i < stepssCloud; ++i) {
        opticalDepthLight += segmaMax * cloudDensity(pos, iTime) * dl;
        pos += dl * sd;
    }
#endif
#if CLOUD_SHADOW_RENDER_TYPE==1
    float dl = (cloudHeightTop - cloudHeightBottom) / 20.0f;
    float d = dl * 0.5f;
    for(int i=0; i<stepssCloud; ++i) {
        float3 pos = ro + sd * d;
        float norY = clamp(getHeightFraction(pos), 0.0f, 1.0f);
        if(norY > 1.0f) return opticalDepthLight;
        opticalDepthLight += segmaMax * cloudDensity(pos, iTime) * dl;
        dl *= 1.3f;
        d += dl;
    }
#endif
    return opticalDepthLight;
}

__DEVICE__ float4 rayMarchingCloud(float3 ro, float3 rd, float3 sd, float3 light, float iTime) {
    rd.y = _fabs(rd.y);
    float2 intersectPos = rayCloudIntersect(ro, rd);
    if(intersectPos.x > cloudMaxDistance) return to_float4(0,0,0,1);
    float3 scatteredLight = to_float3_s(0);
    float transmittance = 1.0f;
    float mu = dot(rd, sd);
    float phase = _mix(HenyeyGreenstein(mu, cloudForwardScatteringG), 
                       HenyeyGreenstein(mu, cloudBackwardScatteringG), cloudScatteringLerp);

    float dl = (intersectPos.y - intersectPos.x) / float(stepsCloud);
    float h = hash13(rd + fract(iTime) );
    // prevent from banding
    intersectPos.x -= dl * h;
    float3 samplePosition = ro + (intersectPos.x + 0.5f * dl) * rd;
    for (int i = 0; i < stepsCloud; ++i) {
        float sigmaS = segmaMax * cloudDensity(samplePosition, iTime);
        if(sigmaS > 0.0f) {
            float sigmaE = sigmaS;
            float tran = cloudEnergy(sigmaE * dl);
            float shadow = cloudEnergy(volumetricShadow(samplePosition, sd, iTime));
#if CLOUD_RENDER_TYPE==0
            scatteredLight += light * sigmaS * phase * shadow * transmittance * dl;
            transmittance *= tran;
#endif
#if CLOUD_RENDER_TYPE==1
            float3 S = light * sigmaS * phase * shadow;
            float3 Sint = S * (1.0f - tran) / sigmaE;
            scatteredLight += transmittance * Sint;
            transmittance *= tran;
#endif
#if CLOUD_RENDER_TYPE==2
            float norY = clamp(getHeightFraction(samplePosition), 0.0f, 1.0f);
            float3 ambientLight = _mix( CLOUDS_AMBIENT_COLOR_BOTTOM, CLOUDS_AMBIENT_COLOR_TOP, norY );
            float3 S = ambientLight * light.z / Is * sigmaS  + light * sigmaS * phase * shadow;
            float3 Sint = S * (1.0f - tran) / sigmaE;
            scatteredLight += transmittance * Sint;
            transmittance *= tran;
#endif
        }
        
        if(transmittance < cloudMinTransmittance) break;
        samplePosition += dl * rd;
  }
    
  return to_float4_aw(scatteredLight, transmittance);
}

__KERNEL__ void SunmooncloudNeuFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_POINT0(RightLeft, 0.0f, 0.0f);
    CONNECT_POINT1(UpDown, 0.0f, 0.0f);

    float3 taEast = to_float3(-1,0,0);
    // West direction
    float3 taWest = to_float3(1,0,0);
    // North direction
    float3 taNorth = to_float3(0,0,1);
    // South direction
    float3 taSouth = to_float3(0,0,-1);
    // Upward direction
    float3 taUp = to_float3(0,1,0);

    //Rayleigh scattering (sky color, atmospheric up to 8km)
    float3 betaR = {3.8e-6, 23.5e-6, 50.1e-6};
    //vec3 betaR = to_float3(5.8e-6, 13.5e-6, 33.1e-6); //normal earth
    //vec3 bR = to_float3(5.8e-6, 33.1e-6, 13.5e-6); //purple
    //vec3 bR = to_float3( 63.5e-6, 13.1e-6, 50.8e-6 ); //green
    //vec3 bR = to_float3( 13.5e-6, 23.1e-6, 115.8e-6 ); //yellow
    //vec3 bR = to_float3( 5.5e-6, 15.1e-6, 355.8e-6 ); //yeellow
    //vec3 bR = to_float3(3.5e-6, 333.1e-6, 235.8e-6 ); //red-purple

    float3 betaM = to_float3_s(21e-6); //normal mie // to_float3(21e-6)
    //vec3 bM = to_float3(50e-6); //high mie

    // planet size zoom
    float sunZoom = 20.0f / fov;
    float moonZoom = 30.0f / fov;


    float2 p = (2.0f*fragCoord-iResolution) / iResolution.y;
    //swi2(p,x,y) *= fov;
    p.x *= fov;
    p.y *= fov;
    float3 color = to_float3_s(0);
    
    // camera position, earth center as coordinate center
    float3 ro = to_float3(0, cameraheight + Re, 0);
    // look at position
    float3 ta, direction;
    float2 mouse = swi2(iMouse,x,y) / iResolution;
    //if (swi2(mouse,x,y) == to_float2_s(0)) swi2(mouse,x,y) = to_float2_s(0.5f);
    if (mouse.x == 0.0f && mouse.y == 0.0f) mouse.x = 0.5f, mouse.y = 0.5f;
    
    float leftPressed = RightLeft.x>0.0?1.0f:0.0f;
    float rightPressed = RightLeft.y>0.0?1.0f:0.0f;
    float upPressed = UpDown.x>0.0?1.0f:0.0f;
    float downPressed = RightLeft.y>0.0?1.0f:0.0f;
    
    
    if(leftPressed > 0.0f || mouse.x < 0.3f)
        direction = taEast;
    else if(rightPressed > 0.0f || mouse.x > 0.7f)
        direction = taWest;
    else if(upPressed > 0.0f || mouse.y > 0.7f)
        direction = taUp;
    else if(downPressed > 0.0f || mouse.y < 0.3f)
        direction = taNorth;
    else
        direction = taSouth;
    ta = ro + direction;

    // primary ray
    mat3 camera = getCamera(ro, ta);
    mat3 cameraInverse = transpose(camera);
    float3 rd = mul_mat3_f3(camera , normalize(to_float3_aw(swi2(p,x,y), -1.0f)));
    
    // used to calculate sunlight of cloud
    float3 atmosphereTrans;
    float phaseS = mod_f(iTime * TWO_PI / dayTime, TWO_PI);
    // sun direction
    float3 sdCenter = to_float3(-_cosf(phaseS), _sinf(phaseS) * _cosf(latitude), _sinf(phaseS) * -_sinf(latitude));
    float3 sdCamera = normalize(sdCenter * Lse - ro);
    // consider sunlight has different intensity in a day
    float sunIntensity = Is * (_fabs(_sinf(phaseS)) + 0.0f);
    if(phaseS < PI) color += rayMarchingAtmosphere(ro, rd, sdCamera, sunIntensity, &atmosphereTrans, betaR, betaM);
    // paint sun
    float3 inverseSunPos = mul_mat3_f3(cameraInverse , sdCamera);
    float sunScreenRadius = Rs / disGroundToPlanet(Re, Lse, phaseS) * sunZoom;
    if(phaseS < PI && sdCamera.y > 0.05f && rd.y > 0.05f) {
        float sunOffset = sunScreenRadius / length(p + swi2(inverseSunPos,x,y) / inverseSunPos.z);
        color += atmosphereTrans * smoothstep(sunSmooth, 1.0f, sunOffset);
    }

    // moon direction simplified being opposite to sun
    float phaseM = mod_f(phaseS + PI, TWO_PI);
    float3 mdCenter = -sdCenter;
    float3 mdCamera = normalize(mdCenter * Lem - ro);
    float moonDay = mod_f(iTime / dayTime, monthDay);
    float phaseDifference = moonDay * TWO_PI / monthDay;
    float moonIntensity = Im * _fabs(_sinf(phaseM)) * (1.0f + _sinf(phaseDifference));
    if(phaseM < PI) color += rayMarchingAtmosphere(ro, rd, mdCamera, moonIntensity, &atmosphereTrans, betaR, betaM);
    float3 inverseMoonPos = mul_mat3_f3(cameraInverse , rd);
    float2 inverseMoonPos2D = -1.0f*swi2(inverseMoonPos,x,y) / inverseMoonPos.z;
    float3 inverseMoonPosCenter = mul_mat3_f3(cameraInverse , mdCamera);
    float2 inverseMoonPosCenter2D = -1.0f*swi2(inverseMoonPosCenter,x,y) / inverseMoonPosCenter.z;

    // moon phase and shader
    float3 moonro = to_float3_s(0);
    float2 moonScreenPos = inverseMoonPos2D - inverseMoonPosCenter2D;
    float3 moonrd = mul_mat3_f3(camera , normalize(to_float3_aw(moonScreenPos, -1.0f)));
    float moonDis = disGroundToPlanet(Re, Lem, phaseM);
    float4 moonSphere = to_float4_aw(direction * moonDis, Rm * moonZoom);
    float t = raySphereIntersect(moonro, moonrd, moonSphere);
    if(phaseM < PI && mdCamera.y > 0.05f && rd.y > 0.05f && t > 0.0f) {
        // ray tracing
        float3 pos = moonro + t*moonrd;
        color += (sphTexureMap(pos, moonSphere,iChannel1) + 0.4f);
        float3 nor = sphNormal( pos, moonSphere);
        float3 msd = to_float3(_cosf(phaseDifference), 0, _sinf(phaseDifference));
        float shadow = clamp( dot(nor, msd), 0.0f, 1.0f );
        color *= shadow;
    }
    
    // because a part of atmosphere under the cloud
    atmosphereTrans = 0.1f + 0.9f * atmosphereTrans;
    if(phaseS < PI && rd.y > 0.0f) {
        float4 cloud = rayMarchingCloud(ro, rd, sdCamera, sunIntensity * atmosphereTrans, iTime);
        color = swi3(cloud,x,y,z) + swi3(color,x,y,z) * cloud.w;
    } else if(phaseS > PI && rd.y > 0.0f) {
        float4 cloud = rayMarchingCloud(ro, rd, mdCamera, moonIntensity * atmosphereTrans, iTime);
        color = swi3(cloud,x,y,z) + swi3(color,x,y,z) * cloud.w;
    }
  fragColor = to_float4_aw(pow_f3(color, to_float3_s(1.0f/2.2f)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}