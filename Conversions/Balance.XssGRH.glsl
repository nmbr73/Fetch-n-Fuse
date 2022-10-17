

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 yinYang( in vec2 fragCoord )
{
    // some old unpublished shader utilized
    vec2 uv = fragCoord.xy / iResolution.xy;
	uv -= vec2( .5,.5 );
	float aspect = iResolution.x / iResolution.y;
	uv.x *= aspect;
	vec3 col = vec3( 1.0, 1.0, 1.0 );
	vec2 offset = vec2( uv.y, -uv.x );
	vec2 uv2 = uv + offset * sin( length( uv ) * 20.0 + iTime) * 1.55;
	float lightness = 1.0 * sin( 10.0 *uv2.x ) * cos( 10.0 * uv2.y );
	lightness += 1.0 * cos( 10.0 *uv.x ) * sin( -10.0 * uv.y );
	lightness = pow( clamp(lightness, .0, 1.0 ), .45 );
	lightness *= 2.0 - 2.9 * dot( uv, uv );
	return col * max( lightness, .0 );
}

vec3 sky( vec3 ray )
{
    float horizonLight = .2 - ray.y;
    ray.y = max( .0, ray.y );
    float dayTime = cos( iTime * .05);
    
    vec3 moonDir = normalize( vec3( .1, -.2 * dayTime + .1, .9 ) );
    float moonDot = dot( moonDir, ray );
    float moon = smoothstep( .996, .997, moonDot );
    vec3 ret = vec3( .0, .07, .12);
    ret += moon * vec3( 1.5, .6, .4 );
    ret += smoothstep( .9, 1.0, moonDot ) * vec3( .1, .0, .0 );
    float dayTimeN = .5 + .5 * dayTime;
    vec3 horizonColor =  (1.0 -  dayTimeN) * vec3( .1, .95, .4 ) + dayTimeN * vec3( 1., .3, -.1 );
    ret += horizonLight * horizonColor ;
    ret *= 3.0;
    ret *= clamp( .15 * iTime - .6, .0, 1.0 );
    
    float yinYangVal = clamp( iTime, .0, 1.0 );
    
    yinYangVal *= max( cos( iTime * .1 ) - .3, .0 );
    yinYangVal *= yinYangVal * yinYangVal;
    
    ret += yinYangVal * yinYang( (ray.xy + vec2(.25, .0)) * iResolution.x * 1.6 );
    
    return ret;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
	uv -= vec2( .5,.7 );
    uv *= .5;
	float aspect = iResolution.x / iResolution.y;
	uv.x *= aspect;
    
    vec3 ray = vec3( uv.x, uv.y, 1.0 );
    ray = normalize( ray );
    
    if( ray.y >= .0 )
    {
        fragColor = vec4( sky(ray), 1.0 );
        return;
    }
    
    vec2 targetUv = vec2( .5 + ray.x * .07 / -ray.y, ray.z * .07 / -ray.y );
    targetUv *= iResolution.y / 300.;
    targetUv.y += sin( iTime * .03 );
    targetUv.x += cos( iTime * .03 );
    
    float tex = texture( iChannel0, fract( targetUv ) ).x - .5;
    targetUv *= vec2( 1.0 - .502 * tex );
    targetUv *= 200.0 / iResolution.y;
    
    vec2 scale = 1. / iResolution.xy;
    vec2 offs = vec2(1.0, 1.0) * scale;
    
    tex = texture( iChannel0, fract( targetUv ) ).x;
    
    float texL = texture( iChannel0, fract( targetUv - vec2(offs.x, .0) ) ).x;
    float texD = texture( iChannel0, fract( targetUv - vec2(.0, offs.y) ) ).x;
    vec3 normal = vec3( tex - texL, .4, tex - texD );
    normal /= length( normal );
    
    vec3 reflection = normalize( ray - 2. * dot( ray, normal ) * normal );
    
    float fresnel = ( 1.0 - 3.0 * dot( normal, reflection ) );
    fragColor = vec4( sky(reflection) * fresnel, 1.0 );
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
#define TONE(f,i) sin((f)*float(i)*1.424759e-4)
#define BOOST_SUB(f,fr) -50.0*abs( fract( f/fr  -.5 ) - .5)


#define ATT1(t) max(sin((t)*0.75+2.0),.0)
#define ATT2(t) max(sin((t)*1.00-2.0),.0)
#define ATT3(t) max(sin((t)*1.25+2.0),.0)
#define ATT4(t) max(sin((t)*1.33+4.0),.0)

#define COUNT 1024
#define COUNT_F 1024.0

#define BOOST \
        float \
        boost = max( boost1_1 + BOOST_SUB( f,tone1_1 ),.0); \
        boost = max( boost1_2 + BOOST_SUB( f,tone1_2 ),boost); \
        boost = max( boost1_3 + BOOST_SUB( f,tone1_3 ),boost); \
        boost = max( boost1_4 + BOOST_SUB( f,tone1_4 ),boost); \
        boost = max( boost2_1 + BOOST_SUB( f,tone2_1 ),boost); \
        boost = max( boost2_2 + BOOST_SUB( f,tone2_2 ),boost); \
        boost = max( boost2_3 + BOOST_SUB( f,tone2_3 ),boost); \
        boost = max( boost2_4 + BOOST_SUB( f,tone2_4 ),boost); \
        boost = min( boost, .1 ); \

float ratios [4] = float[4]( 1.0, 1.25, 1.5, .75 );
vec2 mainSound( int samp, float time )
{
    float f = 20.0;
    float factor = 1.0;
    float domPhase = time * .25;
    int index1 = int( floor( ( domPhase + 1.5707963267 ) / 6.2831853071 ) ) & 3;
    int index2 = int( floor( ( domPhase - 1.5707963267 ) / 6.2831853071 ) ) & 3;
    
    float tone1_1 = 220.0 * ratios[ index2 ];
    float tone1_2 = 275.0 * ratios[ index2 ];
    float tone1_3 = 330.0 * ratios[ index2 ];
    float tone1_4 = 440.0 * ratios[ index2 ];
    
    float tone2_1 = 330.0 * ratios[ index1 ];
    float tone2_2 = 577.5 * ratios[ index1 ];
    float tone2_3 = 247.5 * ratios[ index1 ];
    float tone2_4 = 412.5 * ratios[ index1 ];
    
    float domFactor = clamp( sin( domPhase ) * 3.0 + .25, .0, 1.0 );
    float resFactor = clamp( sin( domPhase ) * -3.0 + .25, .0, 1.0 );
    
    float lt = time;
    
    float boost1_1 = resFactor * ( ATT1(lt) + .3*ATT1(lt - .2 ) + .1*ATT1(lt - .4 ) );
    float boost1_2 = resFactor * ( ATT2(lt) + .3*ATT2(lt - .2 ) + .1*ATT2(lt - .4 ) );
    float boost1_3 = resFactor * ( ATT3(lt) + .3*ATT3(lt - .2 ) + .1*ATT3(lt - .4 ) );
    float boost1_4 = resFactor * ( ATT4(lt) + .3*ATT4(lt - .2 ) + .1*ATT4(lt - .4 ) );
    float boost2_1 = domFactor * ( ATT1(lt) + .3*ATT1(lt - .2 ) + .1*ATT1(lt - .4 ) );
    float boost2_2 = domFactor * ( ATT2(lt) + .3*ATT2(lt - .2 ) + .1*ATT2(lt - .4 ) );
    float boost2_3 = domFactor * ( ATT3(lt) + .3*ATT3(lt - .2 ) + .1*ATT3(lt - .4 ) );
    float boost2_4 = domFactor * ( ATT4(lt) + .3*ATT4(lt - .2 ) + .1*ATT4(lt - .4 ) );
    
    float left = .0;
    for( int i = 1; i < COUNT; ++i )  
    {
        f = 90.0 + ( 7.501 + 2.10 * sin( float(i) ) ) * float( i );
        BOOST
        float fc = factor*factor; fc*=factor;
        left += TONE( f,samp ) * ( min(f/500.0, 1.0)*fc + boost * 24.0 ) * fc;
        factor -= 1.0 / COUNT_F;  
    }
    
    factor = 1.0;
    
    float right = .0;
    for( int i = 1; i < COUNT; ++i )  
    {
        f = 92.0 + ( 5.499 + .010 * sin( float(i) ) ) * float( i );
        BOOST
        float fc = factor*factor; fc*=fc;
        right += TONE( f,samp ) * ( min(f/500.0, 1.0)*fc + boost * 24.0 ) * fc;
        factor -= 1.0 / COUNT_F;
    }
    
    float rotationPhase = time * .7;
    float outRight =  sin( rotationPhase ) * left + cos( rotationPhase ) * right;
    float outLeft =  sin( rotationPhase ) * right + cos( rotationPhase ) * left;
    float waveValue = .8 + .2 * sin( sin( time ) * 3.0 + 1.5 * time );
    time -= .5;
    waveValue *= .8 + .2 * sin( sin( time ) * 3.0 + 1.5 * time );
    waveValue *= 6.0;
    
    return min( time * .2, 1.0) *  ( vec2( outLeft, outRight ) *.8 + vec2( outLeft + outRight ) * .1 )* waveValue / COUNT_F;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    waterUpd( fragColor, fragCoord, iResolution.xy, iTime, iChannel0 );
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    waterUpd( fragColor, fragCoord, iResolution.xy, iTime, iChannel0 );
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
float moise2(vec2 c, float t)
{
	return fract(sin(dot(vec2(18.69781,79.98463),fract(c + t))) * 4958.1694);
}

void waterUpd( out vec4 fragColor, in vec2 fragCoord, vec2 resolution, float time, sampler2D sampler )
{
    vec2 uv = fragCoord.xy / resolution;
    float val = moise2( uv, floor( time * 4. ) * .1234);
    float rain = 1.0 - step( val, .9998 );
    rain *= max( 1.0 - fract( time * 4.) - .7, .0 );

    if( time < 2.0 )
    {
        fragColor = vec4( vec3( .5 ), 1.0 );
        return;
    }
    
    vec2 uv1 = fract( ( fragCoord.xy + vec2( 1., .0 )) / resolution );
    vec2 uv2 = fract( ( fragCoord.xy + vec2( -1., .0 )) / resolution );
    vec2 uv3 = fract( ( fragCoord.xy + vec2( 0., 1.0 )) / resolution );
    vec2 uv4 = fract( ( fragCoord.xy + vec2( 0., -1.0 )) / resolution );
    
    vec4 inCenter = texture( sampler, uv );
    float neighbourAvg = 
        texture( sampler, uv1 ).x + 
        texture( sampler, uv2 ).x +
        texture( sampler, uv3 ).x +
        texture( sampler, uv4 ).x;
    float curSpd = inCenter.y - .5;
    float curAlt = inCenter.x - .5; 
    float spring = neighbourAvg * .25 - .5 - curAlt;
    spring -= curAlt * .2;
    curSpd *= .996;
    curSpd += spring * .32;
    curSpd += rain * 100.0;
    curAlt += curSpd * .15;
    curAlt *= .996;
    curSpd = clamp( curSpd, -.5, .5 ) + .5;
    curAlt = clamp( curAlt, -.5, .5 ) + .5;
    fragColor = vec4( curAlt, curSpd, .0, 1.0 );
    
}