

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
 * The sun, the earth and the moon by batersy
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 * Contact: seplanely@gmail.com
 */

// update history
// 22/03/16: Add movement to sun and moon, and moon phase in a month, 
// atmosphere reference on https://www.shadertoy.com/view/tdSXzD
// 22/03/31: Add simple cloud and repost, 
// reference on https://www.shadertoy.com/view/MdGfzh
// will keep on updating new things here

// Camera config
const float fov = tan(PI / 3.);
const float cameraheight = 00.;

// Ray Marching config
const int steps = 16;
const int stepss = 6;
const int stepsCloud = 18;
const int stepssCloud = 6;

// sun and moon cycle
const float dayTime = 24.;  // how many time in a day
const float monthDay = 30.;  // how many days in a month
// earth latitude, range PI / 2  PI / 2
float latitude = 35. / 180. * PI;

// circle light smoothstep
float sunSmooth = .02;
float moonSmooth = .2;
// planet size zoom
float sunZoom = 20. / fov;
float moonZoom = 30. / fov;

// cloud config
const float cloudCoverage = .6;  //cloud generate quantity
const float cloudPadding = .0;  //cloud thickness padding
const float segmaMax = .004;  // scattering coefficient in max cloud density
// 0: Basic scatering/transmittance integration
// 1: Improved Scattering integration by SebH, see https://www.shadertoy.com/view/XlBSRz
// 2: Improved Scattering integration with ambient light, see https://www.shadertoy.com/view/MdGfzh
#define CLOUD_RENDER_TYPE 2
// 0: fixed step shadow
// 1: increased step shadow
#define CLOUD_SHADOW_RENDER_TYPE 1

vec3 sphTexureMap(in vec3 pos, in vec4 sph) {
    float theta = asin(pos.y / sph.w);
    float phi = asin(pos.x / (sph.w * cos(theta)));
    vec2 uv = vec2(phi, theta);
    uv = (uv + PI / 2.) / PI;
    uv.x /= 2.;
    uv.x = mod(uv.x + 0.26, 1.);
    vec3 col = texture(iChannel1, uv).rgb;
    return col;
}

float heightGradient(float cloudType, float h) {
    float center = .1 + cloudType * .4;
    float range = .05 + cloudType * .35;
    float solid = .02 + cloudType * .28;
    float gradient = smoothstep(center - range, center - solid, h);
    gradient -= smoothstep(center + range, center + solid, h);
    return clamp(gradient, 0., 1.);
}

float cloudNoise(vec2 p) {
    float perlin = perlinNoiseFbm2D(p, 2, 4., cloudCoverage);
    float voronoi = voronoiNoiseFbm2D(p, 2, 12.);
    float perlinWorley = remap(perlin, voronoi, 1., 0., 1.);
    float baseShape = perlinWorley;
#if 0
    float highFreq =  0.625 * voronoiNoise2D(p, 4.) +
        		0.250 * voronoiNoise2D(p, 8.) +
        		0.125 * voronoiNoise2D(p, 16.);
    baseShape = remap(perlinWorley, -highFreq, 1., 0., 1.);
#endif
    return baseShape;
}

float cloudDensity(vec3 p) {
    vec2 samplePos = p.xz / 10000.;
    samplePos.x += iTime / 5.;
    samplePos.y += iTime / 20.;
    samplePos += clamp((p.z - p.x) / p.y, 0., 1000.);
    float heightTop = cloudNoise(samplePos) + cloudPadding;
    float heightBottom = (1.-heightTop) * .2;
    float baseCloud = 0.;
    float heightFraction = getHeightFraction(p);
    if(heightFraction < heightTop && heightFraction > heightBottom) baseCloud = .3;
    //return density;
    /*float Rc = Re + cloudHeightTop;
    float horizon = sqrt(Rc * Rc - Re * Re) * 2.;
    vec2 samplePos = p.xz / 30000.;
    samplePos.x += iTime / 50.;
    samplePos.y += iTime / 200.;
    vec3 tex = texture(iChannel2, samplePos).rgb;
    float baseCloud = tex.r;*/

    /*samplePos /= 4.;
    vec3 tex = texture(iChannel2, samplePos).rgb;
    float cloudType = tex.g;
    float heightFraction = getHeightFraction(p);
    float gradient = heightGradient(cloudType, heightFraction);
    //baseCloud *= gradient;*/
    return baseCloud;
}

float cloudEnergy(float opticalDepth) {
    float beer = exp(-opticalDepth);
    float powder = .5;
    //float powder = 1. - beer * beer;
    return 2. * beer * powder;
}

vec3 volumetricAtmosphereShadow(in vec3 ro, in vec3 sd) {
    float opticalDepthLightR = 0., opticalDepthLightM = 0.;
    float Ls = rayAtmosphereIntersect(ro, sd);
    if (Ls > 0.) {
        float dls = Ls / float(stepss);
        vec3 samplePositionLight = ro + 0.5 * dls * sd;
        for (int j = 0; j < stepss; ++j) {
            float heightLight = getHeight(samplePositionLight);
            opticalDepthLightR += exp(-heightLight / Hr) * dls;
            opticalDepthLightM += exp(-heightLight / Hm) * dls;
            samplePositionLight += dls * sd;
        }
    }
    return betaR * opticalDepthLightR + betaM * 1.1 * opticalDepthLightM;
}

vec3 rayMarchingAtmosphere(vec3 ro, vec3 rd, vec3 sd, float I, out vec3 transmittance) {
    rd.y = abs(rd.y);
    transmittance = vec3(1);
	float L = rayAtmosphereIntersect(ro, rd);
    float mu = dot(rd, sd); // mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
    float phaseR = 3. / (16. * PI) * (1. + mu * mu);
    float phaseM = 3. / (8. * PI) * ((1. - g2) * (1. + mu * mu)) / ((2. + g2) * pow(1. + g2 - 2. * g * mu, 1.5));
    vec3 sumR = vec3(0), sumM = vec3(0); // mie and rayleigh contribution
    
	float dl = L / float(steps);
    vec3 samplePosition = ro + 0.5 * dl * rd;
	for (int i = 0; i < steps; ++i) {
        float height = getHeight(samplePosition);
        // compute optical depth for light
        float hr = exp(-height / Hr) * dl;
        float hm = exp(-height / Hm) * dl;

        vec3 shadow = exp(-volumetricAtmosphereShadow(samplePosition, sd));
        transmittance *= exp(-(betaR * hr + betaM * 1.1 * hm));
        sumR += transmittance * shadow * hr; 
        sumM += transmittance * shadow * hm;
        samplePosition += dl * rd;
	}

	return (sumR * betaR * phaseR + sumM * betaM * phaseM) * I;
}

float volumetricShadow(in vec3 ro, in vec3 sd) {
    float opticalDepthLight = 0.;
#if CLOUD_SHADOW_RENDER_TYPE==0
    vec2 intersectPosLight = rayCloudIntersect(ro, sd);
    float dl = intersectPosLight.y / float(stepssCloud);
    vec3 pos = ro + 0.5 * dl * sd;
    for (int i = 0; i < stepssCloud; ++i) {
        opticalDepthLight += segmaMax * cloudDensity(pos) * dl;
        pos += dl * sd;
    }
#endif
#if CLOUD_SHADOW_RENDER_TYPE==1
    float dl = (cloudHeightTop - cloudHeightBottom) / 20.;
    float d = dl * .5;
    for(int i=0; i<stepssCloud; ++i) {
        vec3 pos = ro + sd * d;
        float norY = clamp(getHeightFraction(pos), 0., 1.);
        if(norY > 1.) return opticalDepthLight;
        opticalDepthLight += segmaMax * cloudDensity(pos) * dl;
        dl *= 1.3;
        d += dl;
    }
#endif
    return opticalDepthLight;
}

vec4 rayMarchingCloud(vec3 ro, vec3 rd, vec3 sd, vec3 light) {
    rd.y = abs(rd.y);
    vec2 intersectPos = rayCloudIntersect(ro, rd);
    if(intersectPos.x > cloudMaxDistance) return vec4(0,0,0,1);
    vec3 scatteredLight = vec3(0);
    float transmittance = 1.;
    float mu = dot(rd, sd);
    float phase = mix(HenyeyGreenstein(mu, cloudForwardScatteringG), 
        HenyeyGreenstein(mu, cloudBackwardScatteringG), cloudScatteringLerp);

	float dl = (intersectPos.y - intersectPos.x) / float(stepsCloud);
    float h = hash13(rd + fract(iTime) );
    // prevent from banding
    intersectPos.x -= dl * h;
    vec3 samplePosition = ro + (intersectPos.x + 0.5 * dl) * rd;
	for (int i = 0; i < stepsCloud; ++i) {
        float sigmaS = segmaMax * cloudDensity(samplePosition);
        if(sigmaS > 0.) {
            float sigmaE = sigmaS;
            float tran = cloudEnergy(sigmaE * dl);
            float shadow = cloudEnergy(volumetricShadow(samplePosition, sd));
#if CLOUD_RENDER_TYPE==0
            scatteredLight += light * sigmaS * phase * shadow * transmittance * dl;
            transmittance *= tran;
#endif
#if CLOUD_RENDER_TYPE==1
            vec3 S = light * sigmaS * phase * shadow;
            vec3 Sint = S * (1. - tran) / sigmaE;
            scatteredLight += transmittance * Sint;
            transmittance *= tran;
#endif
#if CLOUD_RENDER_TYPE==2
            float norY = clamp(getHeightFraction(samplePosition), 0., 1.);
            vec3 ambientLight = mix( CLOUDS_AMBIENT_COLOR_BOTTOM, CLOUDS_AMBIENT_COLOR_TOP, norY );
            vec3 S = ambientLight * light.b / Is * sigmaS  + light * sigmaS * phase * shadow;
            vec3 Sint = S * (1. - tran) / sigmaE;
            scatteredLight += transmittance * Sint;
            transmittance *= tran;
#endif
        }
        
        if(transmittance < cloudMinTransmittance) break;
        samplePosition += dl * rd;
	}
    
	return vec4(scatteredLight, transmittance);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 p = (2.0*fragCoord.xy-iResolution.xy) / iResolution.y;
    p.xy *= fov;
    vec3 color = vec3(0);
    
    // camera position, earth center as coordinate center
	vec3 ro = vec3(0, cameraheight + Re, 0);
    // look at position
    vec3 ta, direction;
    vec2 mouse = iMouse.xy / iResolution.xy;
    if (mouse.xy == vec2(0)) mouse.xy = vec2(.5);
    float leftPressed = texelFetch(iChannel0, ivec2(KEY_LEFT,0),0 ).x;
    float rightPressed = texelFetch(iChannel0, ivec2(KEY_RIGHT,0),0 ).x;
    float upPressed = texelFetch(iChannel0, ivec2(KEY_UP,0),0 ).x;
    float downPressed = texelFetch(iChannel0, ivec2(KEY_DOWN,0),0 ).x;
    if(leftPressed > 0. || mouse.x < .3)
        direction = taEast;
    else if(rightPressed > 0. || mouse.x > .7)
        direction = taWest;
    else if(upPressed > 0. || mouse.y > .7)
        direction = taUp;
    else if(downPressed > 0. || mouse.y < .3)
        direction = taNorth;
    else
        direction = taSouth;
    ta = ro + direction;

    // primary ray
    mat3 camera = getCamera(ro, ta);
    mat3 cameraInverse = transpose(camera);
    vec3 rd = camera * normalize(vec3(p.xy, -1.));
    
    // used to calculate sunlight of cloud
    vec3 atmosphereTrans;
    float phaseS = mod(iTime * TWO_PI / dayTime, TWO_PI);
    // sun direction
    vec3 sdCenter = vec3(-cos(phaseS), sin(phaseS) * cos(latitude), sin(phaseS) * -sin(latitude));
    vec3 sdCamera = normalize(sdCenter * Lse - ro);
    // consider sunlight has different intensity in a day
    float sunIntensity = Is * (abs(sin(phaseS)) + .0);
    if(phaseS < PI) color += rayMarchingAtmosphere(ro, rd, sdCamera, sunIntensity, atmosphereTrans);
    // paint sun
    vec3 inverseSunPos = cameraInverse * sdCamera;
    float sunScreenRadius = Rs / disGroundToPlanet(Re, Lse, phaseS) * sunZoom;
    if(phaseS < PI && sdCamera.y > 0.05 && rd.y > 0.05) {
        float sunOffset = sunScreenRadius / length(p + inverseSunPos.xy / inverseSunPos.z);
        color += atmosphereTrans * smoothstep(sunSmooth, 1., sunOffset);
    }

    // moon direction simplified being opposite to sun
    float phaseM = mod(phaseS + PI, TWO_PI);
    vec3 mdCenter = -sdCenter;
    vec3 mdCamera = normalize(mdCenter * Lem - ro);
    float moonDay = mod(iTime / dayTime, monthDay);
    float phaseDifference = moonDay * TWO_PI / monthDay;
    float moonIntensity = Im * abs(sin(phaseM)) * (1. + sin(phaseDifference));
    if(phaseM < PI) color += rayMarchingAtmosphere(ro, rd, mdCamera, moonIntensity, atmosphereTrans);
    vec3 inverseMoonPos = cameraInverse * rd;
    vec2 inverseMoonPos2D = -inverseMoonPos.xy / inverseMoonPos.z;
    vec3 inverseMoonPosCenter = cameraInverse * mdCamera;
    vec2 inverseMoonPosCenter2D = -inverseMoonPosCenter.xy / inverseMoonPosCenter.z;

    // moon phase and shader
    vec3 moonro = vec3(0);
    vec2 moonScreenPos = inverseMoonPos2D - inverseMoonPosCenter2D;
    vec3 moonrd = camera * normalize(vec3(moonScreenPos, -1.));
    float moonDis = disGroundToPlanet(Re, Lem, phaseM);
    vec4 moonSphere = vec4(direction * moonDis, Rm * moonZoom);
    float t = raySphereIntersect(moonro, moonrd, moonSphere);
    if(phaseM < PI && mdCamera.y > 0.05 && rd.y > 0.05 && t > 0.) {
        // ray tracing
        vec3 pos = moonro + t*moonrd;
        color += (sphTexureMap(pos, moonSphere) + .4);
        vec3 nor = sphNormal( pos, moonSphere);
        vec3 msd = vec3(cos(phaseDifference), 0, sin(phaseDifference));
        float shadow = clamp( dot(nor, msd), 0.0, 1.0 );
        color *= shadow;
    }
    
    // because a part of atmosphere under the cloud
    atmosphereTrans = .1 + .9 * atmosphereTrans;
    if(phaseS < PI && rd.y > 0.) {
        vec4 cloud = rayMarchingCloud(ro, rd, sdCamera, sunIntensity * atmosphereTrans);
        color = cloud.rgb + color.rgb * cloud.a;
    } else if(phaseS > PI && rd.y > 0.) {
        vec4 cloud = rayMarchingCloud(ro, rd, mdCamera, moonIntensity * atmosphereTrans);
        color = cloud.rgb + color.rgb * cloud.a;
    }
	fragColor = vec4(pow(color, vec3(1.0/2.2)), 1.);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Environment
const float Re = 6360e3; //planet radius //6360e3 actual 6371km
const float Ra = 6380e3; //atmosphere radius //6380e3 troposphere 8 to 14.5km
const float Rm = 1738e3;  // moon radius
const float Rs = 695500e3;  // sun radius
const float Lse = 149600000e3;  // distance from sun to earth
const float Lem = 384400e3;  // distance from earth to moon
const float Is = 20.; //sun light power, 10.0 is normal
const float Im = .5; //moon light power
const float g = 0.5; //light concentration .76 //.45 //.6  .45 is normaL
const float g2 = g * g;

// cloud
const float cloudHeightTop = 40e2;
const float cloudHeightBottom = 15e2;
const float cloudForwardScatteringG = .8;
const float cloudBackwardScatteringG = -.2;
const float cloudScatteringLerp = .5;
const float cloudMinTransmittance = .1;
const float cloudMaxDistance = 30e3;  //max render distance
#define CLOUDS_AMBIENT_COLOR_TOP (vec3(149., 167., 200.)*1.5/255.)
#define CLOUDS_AMBIENT_COLOR_BOTTOM (vec3(39., 67., 87.)*1.5/255.)

const float Hr = 8e3; //Rayleigh scattering top //8e3
const float Hm = 1.2e3; //Mie scattering top //1.3e3

vec3 betaM = vec3(21e-6); //normal mie // vec3(21e-6)
//vec3 bM = vec3(50e-6); //high mie

//Rayleigh scattering (sky color, atmospheric up to 8km)
vec3 betaR = vec3(3.8e-6, 23.5e-6, 50.1e-6);
//vec3 betaR = vec3(5.8e-6, 13.5e-6, 33.1e-6); //normal earth
//vec3 bR = vec3(5.8e-6, 33.1e-6, 13.5e-6); //purple
//vec3 bR = vec3( 63.5e-6, 13.1e-6, 50.8e-6 ); //green
//vec3 bR = vec3( 13.5e-6, 23.1e-6, 115.8e-6 ); //yellow
//vec3 bR = vec3( 5.5e-6, 15.1e-6, 355.8e-6 ); //yeellow
//vec3 bR = vec3(3.5e-6, 333.1e-6, 235.8e-6 ); //red-purple

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;

// keyboard control
const int KEY_LEFT  = 37;
const int KEY_UP    = 38;
const int KEY_RIGHT = 39;
const int KEY_DOWN  = 40;
// East direction
vec3 taEast = vec3(-1,0,0);
// West direction
vec3 taWest = vec3(1,0,0);
// North direction
vec3 taNorth = vec3(0,0,1);
// South direction
vec3 taSouth = vec3(0,0,-1);
// Upward direction
vec3 taUp = vec3(0,1,0);

// noise control
const mat2 m2 = mat2(  0.80,  0.60,
                      -0.60,  0.80 );
const float perlinRange = sqrt(2.) / 2.;
const float coverage = 1.;
const int surround = 2;

mat3 getCamera( in vec3 ro, in vec3 ta) {
    vec3 cw = normalize(ro - ta);
    vec3 up = vec3(0,1,0);
    if(abs(cw) == up)
        up = vec3(0,0,1);
    vec3 cu = normalize(cross(up, cw));
    vec3 cv = normalize(cross(cw, cu));
    return mat3(cu, cv, cw);
}

float remap(const float originalValue, const float originalMin, const float originalMax, const float newMin, const float newMax) {
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float HenyeyGreenstein(float costheta, float g) {
	const float k = 0.079577471546;  // 1/(4pi)
    float g2 = g * g;
	return k*(1.0-g2)/(pow(1.0+g2-2.0*g*costheta, 1.5));
}

float hash(vec2 p) {
    p  = 100.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

float hash13(vec3 p3) {
    p3  = fract(p3 * 1031.1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float perlinNoise2D( in vec2 p) {
    vec2 i = floor( p );
    vec2 f = fract( p );

	vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( hash(i + vec2(0.0,0.0)), 
                     hash(i + vec2(1.0,0.0)), u.x),
                mix( hash(i + vec2(0.0,1.0)), 
                     hash(i + vec2(1.0,1.0)), u.x), u.y);
}

float perlinNoiseFbm2D( vec2 p, const int octaves, float frequency, float coverage ) {
    p *= frequency;
    float G = 0.5;
    float f = 1.0;
    float a = .5;
    float t = 0.0;

    for( int i=0; i<octaves; i++ ) {
        t += a*perlinNoise2D( m2 * f * p);
        f *= 2.0;
        a *= G;
    }
    float range = -perlinRange + 2. * coverage * perlinRange;
    t += range;
    if(t > 0.)
        t = remap(t, 0., range + perlinRange, 0., 1.);
    //t = abs(-2. * t + 1.);
    return t;
}

float voronoiNoise2D(in vec2 x, float frequency) {
    x *= frequency;
	vec2 xi = floor(x);
	vec2 xf = fract(x);
    float m_dist = 1.;
    for( int x=-surround; x<=surround; x++ ) {
        for( int y=-surround; y<=surround; y++ ) {
            // Neighbor place in the grid
            vec2 neighbor = vec2(float(x),float(y));

            //vec2 point = vec2(abs(perlinNoise2D(xi + neighbor)), hash(xi + neighbor));
            // Vector between the pixel and the point
            vec2 diff = neighbor + abs(hash(xi + neighbor)) - xf;

            float dist = length(diff);

            // Keep the closer distance
            if(dist < m_dist) {
                m_dist = min(m_dist, dist);
            }
        }
    }
    return m_dist;
}

float voronoiNoiseFbm2D( vec2 p, const int octaves, float frequency ) {
    float G = 0.5;
    float f = 1.0;
    float a = 0.5;
    float t = 0.0;
    float w = 0.0;

    for( int i=0; i<octaves; i++ ) {
        t += a*voronoiNoise2D( p * f, frequency );
        f *= 2.0;
        w += a;
        a *= G;
    }

    return t / w;
}

float getHeight(vec3 p) {
	return length(p) - Re;
}

float getHeightFraction(in vec3 p) {
    float height = length(p) - Re;
    float height_fraction = (height - cloudHeightBottom) / (cloudHeightTop - cloudHeightBottom);
    return clamp(height_fraction, 0., 1.);
}

float rayAtmosphereIntersect(in vec3 ro, in vec3 rd) {
	float b = dot(ro, rd);
    float d = dot(ro, ro);
	float c = Ra*Ra - d;
	float det = sqrt(b * b + c);
	return c / (det + b);
}

float raySphereIntersect(in vec3 ro, in vec3 rd, vec4 sph) {
	vec3 oc = ro - sph.xyz;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - sph.w*sph.w;
	float h = b*b - c;
	if( h<0.0 ) return -1.0;
	return -b - sqrt( h );
}

vec2 rayCloudIntersect(in vec3 ro, in vec3 rd) {
	float b = dot(ro, rd);
    float d = dot(ro, ro);
    float Rct = Re + cloudHeightTop;
    float Rcb = Re + cloudHeightBottom;
    if(d < Rcb * Rcb) {
        vec2 c = vec2(Rcb*Rcb, Rct*Rct) - d;
        vec2 det = sqrt(b * b + c);
        return c / (det + b);
    } else {
        float c = Rct*Rct - d;
        float det = sqrt(b * b + c);
        return vec2(0, c / (det + b));
    }
}

float disGroundToPlanet(in float earthRadius, in float orbitRadius, in float angle) {
    float radio = earthRadius / orbitRadius;
    return orbitRadius * sqrt(1. + radio * radio - 2. * radio * sin(angle));
}

vec3 sphNormal( in vec3 pos, in vec4 sph ) {
    return normalize(pos-sph.xyz);
}