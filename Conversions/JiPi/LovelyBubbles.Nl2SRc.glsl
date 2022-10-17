

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Lovely Bubbles
// by @P_Malin
// https://www.shadertoy.com/view/Nl2SRc
//
// Some lovely shadertoy bubbles.
// I've wanted to implement something with thin film interference for a while.


// CAMERA

vec2 GetWindowCoord( vec2 uv )
{
	vec2 window = uv * 2.0 - 1.0;
	window.x *= iResolution.x / iResolution.y;

	return window;	
}

vec3 GetCameraRayDir( vec2 window, vec3 cameraPos, vec3 cameraTarget, float fov )
{
	vec3 forward = normalize( cameraTarget - cameraPos );
	vec3 right = normalize( cross( vec3(0.0, 1.0, 0.0), forward ) );
	vec3 up = normalize( cross( forward, right ) );
							  
	vec3 dir = normalize(window.x * right + window.y * up + forward * fov);

	return dir;
}


// POSTFX

float Vignette( vec2 uv, float size )
{
    float d = length( (uv - 0.5f) * 2.0f ) / length(vec2(1.0));
    
    d /= size;
    
    float s = d * d * ( 3.0f - 2.0f * d );
    
    float v = mix ( d, s, 0.6f );
    
    return max(0.0, 1.0f - v);
}

vec3 ApplyTonemap( vec3 linearCol )
{
	const float kExposure = 0.5;
	
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    vec3 x = linearCol * kExposure;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
}

vec3 ApplyGamma( vec3 linearCol )
{
	const float kGamma = 2.2;

	return pow( linearCol, vec3(1.0/kGamma) );	
}

vec3 ApplyPostFX( vec2 uv, vec3 col )
{    
    col *= 1.3;

    col *= 0.1 + 0.9 * Vignette( uv, 1.0 );

    col *= vec3(1.0, 0.95, 0.8); // warmer
  
    col = ApplyTonemap(col);
	col = ApplyGamma(col);
    
	return col;
}
	


// Scene

float speed = 1.0;

float BubbleOriginForward( float t )
{
    t = t * 30.0;
    if ( t > 0.0)
    {
        t = t / (1.0+t/10.0f);

    }
    return t + iTime * speed;
}

float BubbleOriginInverse( float r )
{
    r = r- iTime * speed;
    if( r > 0.0)
    {
        r = -10.0f * r / (r - 10.0f);
    }
    r = r / 30.0f;
    return r;
}

float Scene_Distance(vec3 pos)
{

    vec3 vPos = pos;
    vPos.x += 3.0;

    float scale = 50.0;
    
    vPos /= scale;

    // wobble
    vec3 offset = vec3(0);
    offset += sin( pos.yzx * 8.91 + iTime * 10.0 ) * 0.001;
    offset += sin( pos.zxy * 7.89 + iTime * 10.0 ) * 0.001;    
    offset *= 0.08;
    
    float f = BubbleOriginForward( vPos.x );
    
    f = floor(f);
    
    float minD = 1000000.0;
    
    for (float b=-1.0; b<=2.0; b+=1.0)
    {
        float p = f + b;
        vec3 o = vPos;
        o.x = BubbleOriginInverse( p );
                
        o.x -= vPos.x;

         float spreadBlend = 1.0 - clamp( vPos.x * 3.0 + 0.2, 0.0, 1.0);
         
         float spread = spreadBlend;
         
         spread *= 0.05;

         o.y += sin(p * 123.3456) * spread;
         o.z += sin(p * 234.5678) * spread;
         
         o += offset;
           
         float rad = sin( p * 456.8342 ) * 0.5 + 0.5;
                             
         float d = length(o) - 0.005f - rad * rad * 0.02f;
         
         minD = min( minD, d );
    }
    
     return minD * scale;
}

vec3 Scene_GetNormal( vec3 pos )
{
    const float delta = 0.0001;
    
    vec4 samples;
    for( int i=ZERO; i<=4; i++ )
    {
        vec4 offset = vec4(0);
        offset[i] = delta;
        samples[i] = Scene_Distance( pos + offset.xyz );
    }
    
    vec3 normal = samples.xyz - samples.www;    
    return normalize( normal );
}    

float Scene_Trace( vec3 rayOrigin, vec3 rayDir, float minDist, float maxDist, float side )
{
	float t = minDist;

    const int kRaymarchMaxIter = 128;
	for(int i=0; i<kRaymarchMaxIter; i++)
	{		
        float epsilon = 0.0001 * t;
		float d = Scene_Distance( rayOrigin + rayDir * t ) * side;
        if ( abs(d) < epsilon )
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

vec3 GetSkyColour( vec3 dir )
{
	vec3 result = vec3(0.0);
	
    vec3 envMap = texture(iChannel1, dir).rgb;
    envMap = envMap * envMap;
    float kEnvmapExposure = 0.99999;
    result = -log2(1.0 - envMap * kEnvmapExposure);

    return result;	
}

float FilmThickness( vec3 pos )
{
    return Noise(pos * 0.3f, iTime * 0.5);
}

void Shade( inout vec3 colour, inout vec3 remaining, vec3 pos, vec3 rayDir, vec3 normal )
{
    float NdotV = max( dot(normal, -rayDir), 0.0 );

    float filmThickness = FilmThickness(pos);

    vec3 reflection = GetSkyColour( reflect( rayDir, normal ) );
    
#if 1
    // Extra highlight
    vec3 LightColour = vec3(1,.9,.7) * 0.8;
    vec3 L = normalize(vec3(1.0, 2.0, 0.0));
    float NdotL = max( dot( normal, L ), 0.0 );
    float NdotH = max( dot( normal, normalize(L-rayDir) ), 0.0 );
    reflection += (pow(NdotH,10000.0) * 10000.0) * NdotL * LightColour;
    //vReflection += (pow(NdotH,1000.0) * 2000.0) * NdotL * LightColour;
    reflection += (pow(NdotH,100.0) * 200.0) * NdotL * LightColour;
    reflection += (pow(NdotH,10.0) * 20.0) * NdotL * LightColour;
#endif     
     
    float ni = N_Air;
    float nt = N_Water;     
    
    float cosi = NdotV;
    float cost = GetCosT( ni, nt, cosi );
    float fresnelA = Fresnel( ni, nt, cosi, cost );
    float fresnelB = Fresnel( nt, ni, cost, cosi );

    float fresnelFactor = 1.0f - (1.0f - fresnelA) * (1.0f - fresnelB);
    
    vec3 fresnel = vec3(fresnelFactor);

    vec3 thinFilmColour;
#if USE_THIN_FILM_LOOKUP
    thinFilmColour = texture(iChannel0, vec2(NdotV, filmThickness) ).rgb;
#else
    thinFilmColour = GetThinFilmColour(NdotV, filmThickness);
#endif    
    fresnel *= thinFilmColour;
    
    colour += reflection * fresnel * remaining;
    remaining *= (1.0f - fresnel);


#if 0
    float fGlassThickness = 0.5;
    vec3 vGlassColor = vec3(1,0.5, 0.25);

	float fOpticalDepth = fGlassThickness / NdotV;
    vec3 vExtinction = exp2( -fOpticalDepth * (1.0 - vGlassColor) ); 
    remaining *= vExtinction;
#endif    
}


vec3 GetSceneColour( vec3 rayOrigin, vec3 rayDir )
{    
    float kFarClip = 200.0;

	vec3 colour = vec3(0);
    vec3 remaining = vec3(1);
    
    float side = 1.0;
    
    float minDist = 0.0;
    
    for( int i=0; i<10; i++ )
    {
        float t = Scene_Trace( rayOrigin, rayDir, minDist, kFarClip, side );
        
        if ( t>=kFarClip )
        {
            break;
        }
        
        minDist = t + 0.1f;
        
        vec3 hitPos = rayOrigin + rayDir * t;

        vec3 normal = Scene_GetNormal( hitPos );

        Shade(colour, remaining, hitPos, rayDir, normal * side );
        
        side = side * -1.0f;
    }
    
    colour += GetSkyColour(rayDir) * remaining; 
	
	return colour;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{        
	vec2 uv = fragCoord.xy / iResolution.xy;

    float heading = 0.3f + sin(iTime * 0.3) * 0.1;

    float elevation = 1.8 + sin(iTime * 0.134) * 0.1;
    
    float fov = 2.5 + sin( iTime * 0.234) * 0.5;
    
    float cameraDist = 10.0;
	vec3 cameraPos = vec3(sin(heading) * sin(-elevation), cos(-elevation), cos(heading) * sin(-elevation)) * cameraDist;
	vec3 cameraTarget = vec3(sin(iTime * 0.1542) * 3.0, 0.0, 0.0);

	vec3 rayOrigin = cameraPos;
	vec3 rayDir = GetCameraRayDir( GetWindowCoord(uv), cameraPos, cameraTarget, fov );
	
	vec3 sceneCol = GetSceneColour( rayOrigin, rayDir );
	
	vec3 final = ApplyPostFX( uv, sceneCol );
    
    //final = texture( iChannel0, fragCoord.xy/iResolution.xy).rgb;
	
	fragColor = vec4(final, 1.0);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
#if USE_THIN_FILM_LOOKUP
    {
        int segmentCount = 32;
        int segment = iFrame % segmentCount;
        int currSegment = int(floor((fragCoord.y * float(segmentCount) / iResolution.y)));
        
        if ( segment != currSegment )
        {
            fragColor = texelFetch( iChannel0, ivec2(fragCoord), 0 );
            return;
        }
    }

    vec2 uv = fragCoord/iResolution.xy;
        
    vec3 result = GetThinFilmColour(uv.x, uv.y);  

    fragColor = vec4(result,1.0);
#else
    discard;
#endif
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// Use a lookup texture in Buffer A for thin film interference instead of calculating it at every intersection
#define USE_THIN_FILM_LOOKUP 1

float N_Air = 1.0f;
float N_Water = 1.33f;

float PI = 3.141592654;

// used to prevent loop unrolling
// This will be zero but the compiler doesn't know that as iFrame is a uniform
#define ZERO min(iFrame,0)

// https://en.wikipedia.org/wiki/Fresnel_equations
float FresnelS(float ni, float nt, float cosi, float cost)
{
    return ((nt * cosi) - (ni * cost)) / ((nt * cosi) + (ni * cost));
}

float FresnelP(float ni, float nt, float cosi, float cost)
{
    return ((ni * cosi) - (nt * cost)) / ((ni * cosi) + (nt * cost));
}

float Fresnel(float ni, float nt, float cosi, float cost )
{    
    float Rs = FresnelS( ni, nt, cosi, cost );
    float Rp = FresnelP( ni, nt, cosi, cost );

    return (Rs * Rs + Rp * Rp) * 0.5;
}

float FresnelR0(float ni, float nt)
{
    float R0 = (ni-nt) / (ni+nt);
    R0 *= R0;
    return R0;
}

// https://en.wikipedia.org/wiki/Snell%27s_law
float GetCosT( float ni, float nt, float cosi )
{
    float n = ni/nt;
    float sinT2 = n*n*(1.0-cosi*cosi);
    
    // Total internal reflection
    if (sinT2 >= 1.0)
    {
        return 1.0;
    } 

    float cost = sqrt(1.0 - sinT2);
    return cost;
}


// https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

float SmoothNoise3d(vec3 p)
{
    vec3 fl = floor(p);
    vec3 fr = p - fl;
    
    vec3 ot = fr*fr*(3.0-2.0*fr);
    vec3 zt = 1.0f - ot;
    
    
    float result = 0.0f;
    
    result += hash13(fl + vec3(0,0,0)) * (zt.x * zt.y * zt.z);
    result += hash13(fl + vec3(1,0,0)) * (ot.x * zt.y * zt.z);

    result += hash13(fl + vec3(0,1,0)) * (zt.x * ot.y * zt.z);
    result += hash13(fl + vec3(1,1,0)) * (ot.x * ot.y * zt.z);

    result += hash13(fl + vec3(0,0,1)) * (zt.x * zt.y * ot.z);
    result += hash13(fl + vec3(1,0,1)) * (ot.x * zt.y * ot.z);

    result += hash13(fl + vec3(0,1,1)) * (zt.x * ot.y * ot.z);
    result += hash13(fl + vec3(1,1,1)) * (ot.x * ot.y * ot.z);

    return result;
}

const mat3 m3 = mat3( 0.00,  0.80,  0.60,
					-0.80,  0.36, -0.48,
					-0.60, -0.48,  0.64 );

float Noise(vec3 p, float o)
{
    float result = 0.0f;
    float a = 1.0f;
    float t= 0.0;
    float f = 0.5;
    float s= 2.0f;
    
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    result = result / t;
    
    return result;
}



// Spectrum to xyz approx function from http://jcgt.org/published/0002/02/01/paper.pdf
// Inputs:  Wavelength in nanometers
float xFit_1931( float wave )
{
    float t1 = (wave-442.0)*((wave<442.0)?0.0624:0.0374),
          t2 = (wave-599.8)*((wave<599.8)?0.0264:0.0323),
          t3 = (wave-501.1)*((wave<501.1)?0.0490:0.0382);
    return 0.362*exp(-0.5*t1*t1) + 1.056*exp(-0.5*t2*t2)- 0.065*exp(-0.5*t3*t3);
}
float yFit_1931( float wave )
{
    float t1 = (wave-568.8)*((wave<568.8)?0.0213:0.0247),
          t2 = (wave-530.9)*((wave<530.9)?0.0613:0.0322);
    return 0.821*exp(-0.5*t1*t1) + 0.286*exp(-0.5*t2*t2);
}
float zFit_1931( float wave )
{
    float t1 = (wave-437.0)*((wave<437.0)?0.0845:0.0278),
          t2 = (wave-459.0)*((wave<459.0)?0.0385:0.0725);
    return 1.217*exp(-0.5*t1*t1) + 0.681*exp(-0.5*t2*t2);
}

#define xyzFit_1931(w) vec3( xFit_1931(w), yFit_1931(w), zFit_1931(w) ) 

vec3 XYZtosRGB( vec3 XYZ )
{
    // XYZ to sRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
   mat3 m = mat3 (
        3.2404542, -1.5371385, -0.4985314,
		-0.9692660,  1.8760108,  0.0415560,
 		0.0556434, -0.2040259,  1.0572252 );
    
    return XYZ * m;
}

vec3 WavelengthToXYZ( float f )
{    
    return xyzFit_1931( f );    
}



// from  https://github.com/amandaghassaei/SoapFlow/blob/main/python/Thin%20Film%20Interference.ipynb
float ThinFilmAmplitude( float wavelength, float thickness, float cosi )
{
    float ni = N_Air;
    float nt = N_Water;
    
    float cost = GetCosT( ni, nt, cosi );

    // # The wavelength inside a medium is scaled by the index of refraction.
    // wavelength_soap = wavelength / n_soap
    // wavelength_air = wavelength / n_air
    // # First calc phase shift of reflection at rear surface, based on film thickness.
    // phaseDelta = 2 * thickness / math.cos(theta) * 2 * math.pi / wavelength_soap  
    // # There is an additional path to compute, the segment AJ from:
    // # https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
    // phaseDelta -= 2 * thickness * math.tan(theta) * math.sin(incidentAngle) * 2 * math.pi / wavelength_air
    // Simplified to:
    float phaseDelta = 2.0 * thickness * nt * cost * 2.0 * PI / wavelength;
    
    // https://en.wikipedia.org/wiki/Reflection_phase_change
    if (ni < nt)
        phaseDelta -= PI;
    if (ni > nt)
        phaseDelta += PI;

    float front_refl_amp = Fresnel(cosi, cost, ni, nt);
    float front_trans_amp = 1.0 - front_refl_amp;
    float rear_refl_amp = front_trans_amp * Fresnel(cost, cosi, nt, ni);
    
    rear_refl_amp /= front_refl_amp;
    front_refl_amp = 1.0f;
        
    // http://scipp.ucsc.edu/~haber/ph5B/addsine.pdf
    return sqrt(front_refl_amp * front_refl_amp + rear_refl_amp * rear_refl_amp + 2.0 * front_refl_amp * rear_refl_amp * cos(phaseDelta));
}

#if 1

vec3 GetThinFilmColour( float cosi, float thicknessN )
{
    float thicknessMin = 100.0;//1.0f;
    float thicknessMax = 1500.0;//2500.0f;
    
    float thickness = mix(thicknessMin, thicknessMax, thicknessN);

    vec3 result = vec3(0.0);
    
    float t = 0.0;
    
    vec3 white = vec3(0.0);
    
    for (float wavelength = 380.0; wavelength<=780.0; wavelength += 50.0)
    {
        float amplitude = ThinFilmAmplitude( wavelength, thickness, cosi );
        
        vec3 XYZ = WavelengthToXYZ( wavelength );
    
        white += XYZ;
    
        result += XYZ * amplitude;
        t += 1.0f;
    }

    result = XYZtosRGB( result );
      
    result /= t;
    //result /= white;
    //result = vec3(1.0);
    
    return result;
}

#else

// The Technical Art of The Last of Us Part II by Waylon Brinck and Steven Tang || SIGGRAPH 2020
// https://youtu.be/tvBIqPHaExQ?t=2873

vec3 GetThinFilmColour( float cosi, float thicknessN )
{
    float thicknessMin = 100.0;//1.0f;
    float thicknessMax = 1500.0;//2500.0f;
    
    float thickness = mix(thicknessMin, thicknessMax, thicknessN);

    vec3 result = vec3(1.0);

    vec3 rgbLightWavelength = vec3(700,510,440);
    float extraDistance = thickness / cosi;
    vec3 phaseChangeEquivalentShift = vec3(0);
    
    bool doublePhase = true;
    
    if ( !doublePhase )
    {
        phaseChangeEquivalentShift = -rgbLightWavelength * .5;
    }
    
    vec3 phaseOffset = vec3(extraDistance - phaseChangeEquivalentShift) / rgbLightWavelength;
    vec3 superpositionAmplitude = abs(cos(phaseOffset * PI));
    
    float coherenceTerm = 1.0 - clamp( extraDistance * 1.0 / 1500., 0., 1. );
    
    superpositionAmplitude = mix( vec3(1.0), superpositionAmplitude, coherenceTerm);

    float filmStrength = 2.0;
    float filmStrengthTerm = filmStrength * cosi;

    result = mix( result, superpositionAmplitude, filmStrengthTerm );

    //result = superpositionAmplitude;

    return result;
}

#endif