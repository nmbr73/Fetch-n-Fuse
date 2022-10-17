

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Copyright Inigo Quilez, 2022 - https://iquilezles.org/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// Just a remix of https://www.shadertoy.com/view/MdfGRX

//------------------------------------------------------------------
// noise
//------------------------------------------------------------------
float hash( float n )
{
    return fract( n*17.0*fract( n*0.3183099 ) );
}

float noise1( in float x )
{
    float p = floor(x);
    float w = fract(x);
    float u = w*w*(3.0-2.0*w);
    return mix(hash(p+0.0),hash(p+1.0),u);
}

float noise( in vec3 x )
{
#if 0
    vec3 p = floor(x);
    vec3 w = fract(x);
    vec3 u = w*w*(3.0-2.0*w);
    float n = 1.0*p.x + 317.0*p.y + 157.0*p.z;
    return mix( mix( mix(hash(n+  0.0),hash(n+  1.0),u.x),
                     mix(hash(n+317.0),hash(n+318.0),u.x),u.y),
                mix( mix(hash(n+157.0),hash(n+158.0),u.x),
                     mix(hash(n+474.0),hash(n+475.0),u.x),u.y),u.z);   
#else
    return textureLod(iChannel0,x/32.0,0.0).x;
#endif    
}

//------------------------------------------------------------------

vec4 map( vec3 p, float time )
{
    // density
	float den = 0.2 - p.y;

    // invert space	
	p = -7.0*p/dot(p,p);

    // twist space	
	float co = cos(0.8*den);
	float si = sin(0.8*den);
	p.xz = mat2(co,-si,si,co)*p.xz;

    // cloud	
	float f;
    float t = time + 9.0;
	vec3 q = p                           - vec3(0.0,t*0.2,0.0);
    f  = 0.500000*noise( q ); q = q*2.21 - vec3(0.0,t*0.4,0.0);
    f += 0.250000*noise( q ); q = q*2.15 - vec3(0.0,t*0.8,0.0);
    f += 0.125000*noise( q ); q = q*2.13 - vec3(0.0,t*1.6,0.0);
    f += 0.062500*noise( q ); q = q*2.05 - vec3(0.0,t*3.2,0.0);
    f += 0.031250*noise( q );

	den = den + 4.0*f + 0.015;
	
    vec3 col = mix( vec3(0.8), vec3(0.5), den ) + 0.02*sin(p);
	
	return vec4( col, den );
}

vec3 raymarch( in vec3 ro, in vec3 rd, in vec2 pixel, float time )
{
    // lightining
    float li = 1.0;
    li *= smoothstep(0.6,0.65,noise1( time*11.2 + 6.1 ));
    li *= smoothstep(0.4,0.45,noise1( time*1.1 + 6.1 ));

    // raymarch
    vec4 sum = vec4( 0.0 );
    
    const float stepFactor = 0.5;

	// with dithering
    float t = 0.05 *fract(sin(iTime+pixel.x*11.0+17.0*pixel.y)*1.317);    
	for( int i=0; i<256; i++ )
	{
		vec3 pos = ro + t*rd;
		vec4 col = map( pos, time );

        if( col.w>0.0 )
        {
            float len = length(pos);
            float at = smoothstep(2.0,0.0,len);
            col.xyz *= mix( 2.5*vec3(0.3,0.4,0.5), 0.9*vec3(0.4,0.45,0.55), clamp( (pos.y-0.1)/2.0, 0.0, 1.0 ) );
            col.xyz *= 1.0 + 0.15*at + 1.5*li*at;

            //if( li>0.001 )
            {
            vec3 dir = pos/len;
            float nn = max(0.0,col.w - map( pos-dir*0.05, time ).w);
            col.xyz += 2.0*li*(0.5+1.5*at)*nn*vec3(0.8,0.8,0.8)*(1.0-col.w);
            }

            // fog
            col.xyz *= 1.15*exp2(-t*0.1);

            // compose		
            col.a *= stepFactor;
            col.rgb *= col.a;
            sum = sum + col*(1.0 - sum.a);	
            if( sum.a > 0.99 ) break;
        }

		t += 0.1*stepFactor;
	}

	return clamp( sum.xyz, 0.0, 1.0 );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime;

    vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
	
    // camera
    vec3 ro = 4.0*normalize(vec3(1.0, 1.5, 0.0));
	vec3 ta = vec3(0.0, 1.0, 0.0);
	float cr = 0.4*cos(0.4*iTime);
	
    // shake		
	ro += 0.01*(-1.0+2.0*noise1(3.1*time));
	ta += 0.01*(-1.0+2.0*noise1(3.3*time));
	
	// build ray
    vec3 ww = normalize( ta - ro);
    vec3 uu = normalize(cross( vec3(sin(cr),cos(cr),0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    vec3 rd = normalize( p.x*uu + p.y*vv + 2.0*ww );
	
    // raymarch	
    
	vec3 col = raymarch( ro, rd, fragCoord, time );

    // color grade
    col = col*col*(3.0-2.0*col);
    col = col*0.5 + 0.5*col*col*(3.0-2.0*col);
    col *= 1.2;
    
    // vignette
	vec2 q = fragCoord.xy / iResolution.xy;
	col *= 0.1 + 0.9*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.15 );

    fragColor = vec4( col, 1.0 );
}
