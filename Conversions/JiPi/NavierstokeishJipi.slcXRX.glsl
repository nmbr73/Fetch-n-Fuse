

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#ifdef FAKETHREED

struct C_Sample
{
	vec3 vAlbedo;
	vec3 vNormal;
};
	
C_Sample SampleMaterial(const in vec2 vUV, sampler2D sampler,  const in vec2 vTextureSize, const in float fNormalScale)
{
	C_Sample result;
	
	vec2 vInvTextureSize = vec2(1.0) / vTextureSize;
	
	vec3 cSampleNegXNegY = texture(sampler, vUV + (vec2(-1.0, -1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXNegY = texture(sampler, vUV + (vec2( 0.0, -1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXNegY = texture(sampler, vUV + (vec2( 1.0, -1.0)) * vInvTextureSize.xy).rgb;
	
	vec3 cSampleNegXZerY = texture(sampler, vUV + (vec2(-1.0, 0.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXZerY = texture(sampler, vUV + (vec2( 0.0, 0.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXZerY = texture(sampler, vUV + (vec2( 1.0, 0.0)) * vInvTextureSize.xy).rgb;
	
	vec3 cSampleNegXPosY = texture(sampler, vUV + (vec2(-1.0,  1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSampleZerXPosY = texture(sampler, vUV + (vec2( 0.0,  1.0)) * vInvTextureSize.xy).rgb;
	vec3 cSamplePosXPosY = texture(sampler, vUV + (vec2( 1.0,  1.0)) * vInvTextureSize.xy).rgb;

	// convert to linear	
	vec3 cLSampleNegXNegY = cSampleNegXNegY * cSampleNegXNegY;
	vec3 cLSampleZerXNegY = cSampleZerXNegY * cSampleZerXNegY;
	vec3 cLSamplePosXNegY = cSamplePosXNegY * cSamplePosXNegY;

	vec3 cLSampleNegXZerY = cSampleNegXZerY * cSampleNegXZerY;
	vec3 cLSampleZerXZerY = cSampleZerXZerY * cSampleZerXZerY;
	vec3 cLSamplePosXZerY = cSamplePosXZerY * cSamplePosXZerY;

	vec3 cLSampleNegXPosY = cSampleNegXPosY * cSampleNegXPosY;
	vec3 cLSampleZerXPosY = cSampleZerXPosY * cSampleZerXPosY;
	vec3 cLSamplePosXPosY = cSamplePosXPosY * cSamplePosXPosY;

	// Average samples to get albdeo colour
	result.vAlbedo = ( cLSampleNegXNegY + cLSampleZerXNegY + cLSamplePosXNegY 
		    	     + cLSampleNegXZerY + cLSampleZerXZerY + cLSamplePosXZerY
		    	     + cLSampleNegXPosY + cLSampleZerXPosY + cLSamplePosXPosY ) / 9.0;	
	
	vec3 vScale = vec3(0.3333);
	
	#ifdef USE_LINEAR_FOR_BUMPMAP
		
		float fSampleNegXNegY = dot(cLSampleNegXNegY, vScale);
		float fSampleZerXNegY = dot(cLSampleZerXNegY, vScale);
		float fSamplePosXNegY = dot(cLSamplePosXNegY, vScale);
		
		float fSampleNegXZerY = dot(cLSampleNegXZerY, vScale);
		float fSampleZerXZerY = dot(cLSampleZerXZerY, vScale);
		float fSamplePosXZerY = dot(cLSamplePosXZerY, vScale);
		
		float fSampleNegXPosY = dot(cLSampleNegXPosY, vScale);
		float fSampleZerXPosY = dot(cLSampleZerXPosY, vScale);
		float fSamplePosXPosY = dot(cLSamplePosXPosY, vScale);
	
	#else
	
		float fSampleNegXNegY = dot(cSampleNegXNegY, vScale);
		float fSampleZerXNegY = dot(cSampleZerXNegY, vScale);
		float fSamplePosXNegY = dot(cSamplePosXNegY, vScale);
		
		float fSampleNegXZerY = dot(cSampleNegXZerY, vScale);
		float fSampleZerXZerY = dot(cSampleZerXZerY, vScale);
		float fSamplePosXZerY = dot(cSamplePosXZerY, vScale);
		
		float fSampleNegXPosY = dot(cSampleNegXPosY, vScale);
		float fSampleZerXPosY = dot(cSampleZerXPosY, vScale);
		float fSamplePosXPosY = dot(cSamplePosXPosY, vScale);	
	
	#endif
	
	// Sobel operator - http://en.wikipedia.org/wiki/Sobel_operator
	
	vec2 vEdge;
	vEdge.x = (fSampleNegXNegY - fSamplePosXNegY) * 0.25 
			+ (fSampleNegXZerY - fSamplePosXZerY) * 0.5
			+ (fSampleNegXPosY - fSamplePosXPosY) * 0.25;

	vEdge.y = (fSampleNegXNegY - fSampleNegXPosY) * 0.25 
			+ (fSampleZerXNegY - fSampleZerXPosY) * 0.5
			+ (fSamplePosXNegY - fSamplePosXPosY) * 0.25;

	result.vNormal = normalize(vec3(vEdge * fNormalScale, 1.0));	
	
	return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	vec2 vUV = fragCoord.xy / iResolution.xy;
	
	C_Sample materialSample;
		
	float fNormalScale = 10.0;
	materialSample = SampleMaterial( vUV, iChannel0, iChannelResolution[0].xy, fNormalScale );
	
	// Random Lighting...
	
	float fLightHeight = 0.2;
	float fViewHeight = 2.0;
	
	vec3 vSurfacePos = vec3(vUV, 0.0);
	
	vec3 vViewPos = vec3(0.5, 0.5, fViewHeight);
			
	vec3 vLightPos = vec3( vec2(sin(iTime),cos(iTime)) * 0.25 + 0.5 , fLightHeight);
		
	if( iMouse.z > 0.0 )
	{
		vLightPos = vec3(iMouse.xy / iResolution.xy, fLightHeight);
	}
	
	vec3 vDirToView = normalize( vViewPos - vSurfacePos );
	vec3 vDirToLight = normalize( vLightPos - vSurfacePos );
		
	float fNDotL = clamp( dot(materialSample.vNormal, vDirToLight), 0.0, 1.0);
	float fDiffuse = fNDotL;
	
	vec3 vHalf = normalize( vDirToView + vDirToLight );
	float fNDotH = clamp( dot(materialSample.vNormal, vHalf), 0.0, 1.0);
	float fSpec = pow(fNDotH, 10.0) * fNDotL * 0.5;
	
	vec3 vResult = materialSample.vAlbedo * fDiffuse + fSpec;
	
	vResult = sqrt(vResult);
	
	#ifdef SHOW_NORMAL_MAP
	vResult = materialSample.vNormal * 0.5 + 0.5;
	#endif
	
	#ifdef SHOW_ALBEDO
	vResult = sqrt(materialSample.vAlbedo);
	#endif
	
	fragColor = vec4(vResult,1.0);
}

#else

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 uv = fragCoord/iResolution.xy;

    #ifdef WATERPAINTING
    
    fragColor = texture( iChannel0, uv );
    
    #else 
    
    fragColor = texture( iChannel0, uv );// * vec4( 0.5, 0.2, 2.0, 1.0 );
    
    #endif
    
    //fragColor += 0.2 * texture( iChannel1, uv );// * vec4( 1.5, 0.1, 0.3, 1 );
    
}

#endif
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Undefine the next line for camera input.
//#define WATERPAINTING
// Heavily bitting P_Malin's https://www.shadertoy.com/view/XdlGz8 for the fake 3D effect.
#define FAKETHREED
const float forceVector = 10.0;
const float forceColour = 10.0;
const float dx = 0.5;
const float dt = dx * dx * 0.5;
const float siz = 0.05;
const float di = 1.25;
const float alp = ( dx * dx ) / dt;
const float rbe = 1.0 / ( 4.0 + alp );
const float vo = 10.0;
const float vf = 0.0025;
const float mul = 10.0;
const float e = 0.05;

float dis( vec2 uv, vec2 mou )
{

    return length( uv - mou );

}

float cir( vec2 uv, vec2 mou, float r )
{

    float o = smoothstep( r, r - 0.05, dis( uv, mou ) );
    
    return o;

}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define keyTex iChannel3
#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float arrow_density = 0.2;
const float arrow_length = 0.95;

const vec3 luma = vec3(0.2126, 0.7152, 0.0722);

float segm(in vec2 p, in vec2 a, in vec2 b) //from iq
{
	vec2 pa = p - a;
	vec2 ba = b - a;
	float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1.);
	return length(pa - ba*h)*20.*arrow_density;
}

float cur( vec2 uv )
{
    
    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float top = texture( iChannel0, vec2( x, y + ypi ) ).r;
    float lef = texture( iChannel0, vec2( x - xpi, y ) ).r;
    float rig = texture( iChannel0, vec2( x + xpi, y ) ).r;
    float dow = texture( iChannel0, vec2( x, y - ypi ) ).r;
    
    float dY = ( top - dow ) * 0.5;
    float dX = ( rig - lef ) * 0.5;
    
    return dX * dY;
}

vec2 vor( vec2 uv )
{
    
    vec2 pre = uv;
    
    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;

    vec2 dir = vec2( 0 );
    dir.y = ( cur( vec2( x, y + ypi ) ) ) - ( cur( vec2( x, y - ypi ) ) );
    dir.x = ( cur( vec2( x + xpi, y ) ) ) - ( cur( vec2( x - xpi, y ) ) );
    
    dir = normalize( dir );
    
    if( length( dir ) > 0.0 )
    
    uv -= dt * vo * cur( uv ) * dir;
    
    return uv;
    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 p = fragCoord.xy / iResolution.y;
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 mou = iMouse.xy / iResolution.y;
    p *= mul;
    mou *= mul;
    
    float fO = 0.0;
    fO += texture( iChannel1, vor( uv ) ).r + texture( iChannel1, vor( uv ) ).g + texture( iChannel1, vor( uv ) ).b;
   	fO *= 0.333;
    
    vec2 ep = vec2( e, 0 );
    vec2 rz= vec2( 0 );
    vec2 fra = fract( uv );

    float t0 = 0.0, t1 = 0.0, t2 = 0.0;
    t0 += texture( iChannel0, uv ).a * dt * vf;
    t1 += texture( iChannel0, uv + ep.xy ).a * dt * vf;
    t2 += texture( iChannel0, uv + ep.yx ).a * dt * vf;
    vec2 g = vec2( ( t1 - t0 ), ( t2 - t0 ) ) / ep.xx;
    vec2 t = vec2( -g.y, g.x );

    p += 0.9 * t + g * 0.3;
    rz += t;
    
    vec2 fld = rz;
    
    if( cir( p, mou, siz * mul ) > 0.1 && iMouse.z > 0.5 )
            
        fld += forceVector * texture( iChannel2, uv ).xy;
    
    float o = 0.0;
    
    if( iFrame <= 4 || KEY_I < 0.5 )
    
    o = texture( iChannel0, uv ).a * 0.99;
    
    fO += o;
    
    if( uv.y < 0.00 || uv.x < 0.00 || uv.x > 1.0 || uv.y > 1.0 ) o *= 0.0;
    
    fragColor = vec4( 0, fld, fO );
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
float hash( vec2 a )
{

    return fract( sin( a.x * 3433.8 + a.y * 3843.98 ) * 45933.8 );

}

float noise( vec2 uv )
{
    
    vec2 lv = fract( uv );
    lv = lv * lv * ( 3.0 - 2.0 * lv );
    vec2 id = floor( uv );
    
    float bl = hash( id );
    float br = hash( id + vec2( 1, 0 ) );
    float b = mix( bl, br, lv.x );
    
    float tl = hash( id + vec2( 0, 1 ) );
    float tr = hash( id + vec2( 1 ) );
    float t = mix( tl, tr, lv.x );
    
    float c = mix( b, t, lv.y );
    
    return c;

}

float fbm( vec2 uv )
{

	float f = noise( uv * 4.0 );
    f += noise( uv * 8.0 ) * 0.5;  
    f += noise( uv * 16. ) * 0.25; 
    f += noise( uv * 32. ) * 0.125; 
    f += noise( uv * 64. ) * 0.0625;
    f /= 2.0;
    
    return f;

}

float cur( vec2 uv )
{
    
    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float top = texture( iChannel0, vec2( x, y + ypi ) ).r;
    float lef = texture( iChannel0, vec2( x - xpi, y ) ).r;
    float rig = texture( iChannel0, vec2( x + xpi, y ) ).r;
    float dow = texture( iChannel0, vec2( x, y - ypi ) ).r;
    
    float dY = ( top - dow ) * 0.5;
    float dX = ( rig - lef ) * 0.5;
    
    return dX * dY;
}

vec2 vor( vec2 uv )
{
    
    vec2 pre = uv;
    
    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;

    vec2 dir = vec2( 0 );
    dir.y = ( cur( vec2( x, y + ypi ) ) ) - ( cur( vec2( x, y - ypi ) ) );
    dir.x = ( cur( vec2( x + xpi, y ) ) ) - ( cur( vec2( x - xpi, y ) ) );
    
    dir = normalize( dir );
    
    if( length( dir ) > 0.0 )
    
    uv -= dt * vo * cur( uv ) * dir;
    
    return uv;
    
}

vec2 dif( vec2 uv )
{

    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    vec2 cen = texture( iChannel0, uv ).xy;
    vec2 top = texture( iChannel0, vec2( x, y + ypi ) ).xy;
    vec2 lef = texture( iChannel0, vec2( x - xpi, y ) ).xy;
    vec2 rig = texture( iChannel0, vec2( x + xpi, y ) ).xy;
    vec2 dow = texture( iChannel0, vec2( x, y - ypi ) ).xy;
    
    return ( di * rbe ) * ( top + lef + rig + dow + alp * cen ) * rbe;
    
}

vec2 adv( vec2 uv )
{
    
    // Eulerian.
    vec2 pre = texture( iChannel1, vor( uv ) ).yz;
    pre = iTimeDelta * dt * pre;
    
    uv -= pre;
    
    return uv;
    
}

vec4 forc( vec2 uv, vec2 p, vec2 mou, sampler2D tex, out float cen )
{
    
    vec4 col = vec4( 0 );
    
    #ifdef WATERPAINTING
    
    if( iFrame <= 10 )
    
    col += 0.2 * texture( iChannel2, uv );
    
    if( iMouse.z > 0.5 )
	col += cir( p, mou, siz );
    
    #else
    float tim = iTime * 0.1;
    if( iMouse.z > 0.5 && cir( p, mou, siz ) > 0.1 )
	col += forceColour * vec4( noise( uv + tim ), noise( uv + tim + 1.0 ), noise( uv + tim + 2.0 ), 1 );
    
    #endif
    
    return col;

}

vec2 div( vec2 uv, vec2 p, vec2 mou )
{
    
    float xpi = 1.0 / iResolution.x;
    float ypi = 1.0 / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float cen = texture( iChannel0, uv ).a;
    float top = texture( iChannel0, vec2( x, y + ypi ) ).r;
    float lef = texture( iChannel0, vec2( x - xpi, y ) ).r;
    float rig = texture( iChannel0, vec2( x + xpi, y ) ).r;
    float dow = texture( iChannel0, vec2( x, y - ypi ) ).r;
    
    float dX = dt * ( rig - lef ) * 0.5;
    float dY = dt * ( top - dow ) * 0.5;
    
    vec2 vel = vec2( 0 );
    
    if( iMouse.z > 0.5 && cir( p, mou, siz ) > 0.1 )
    
    vel = forceVector * texture( iChannel3, uv ).xy;
    
    return vec2( dX, dY ) + vel;

}

vec2 pre( vec2 uv, vec2 p, vec2 mou )
{

    vec2 pre = uv;
    
    uv -= ( di * dx * dx ) * div( uv, p, mou );
    
    return uv;

}

vec2 vel( vec2 uv, vec2 p, vec2 mou )
{
    
    vec2 pr = pre( uv, p, mou );
    vec2 die = div( uv, p, mou );
    
    uv += dt * die - pr;
   
    return uv;
    
}

vec4 jac( vec2 uv, vec2 p, vec2 mou, out float cen )
{

    vec4 col = vec4( 0.0 ); float dam = 1.0; vec4 colO = vec4( 0 ); vec2 pre = uv;
    
    vec2 tem = uv;
 
    uv = adv( uv );
    uv -= dt * ( vel( uv, p, mou ) * dif( uv ) );
    col += forc( uv, p, mou, iChannel0, cen );
    colO = texture( iChannel0, uv ) + col * dt;
    colO *= 0.99;
    
    if( pre.y < 0.01 || pre.x < 0.01 || pre.x > 1.0 || pre.y > 1.0 ) colO *= 0.0;
    
    return colO;

}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 uv = fragCoord / iResolution.xy;
    vec2 p = fragCoord / iResolution.y;

    vec2 mou = iMouse.xy / iResolution.y;
    
    float ini = 0.0;
    
    float cen = 0.0;
    
    vec4 colO = jac( uv, p, mou, cen );
    
    fragColor = colO;
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 p = fragCoord / iResolution.y;
    vec2 uv = fragCoord / iResolution.xy;
    vec2 mou = iMouse.xy / iResolution.y;
    vec2 last = textureLod( iChannel0, uv, 0.0 ).zw;
    
    vec2 acc = vec2( 0 );
    
    if( cir( p, mou, siz ) > 0.05 )
    
    	acc += ( mou - last ) * forceVector;
    
    fragColor = vec4( acc, mou );
    
}