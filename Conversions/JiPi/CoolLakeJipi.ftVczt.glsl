

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// https://www.shadertoy.com/view/7tKyz3 Cool Lake [FORK] with Caustics
// Combined Xor's Cool Lake https://www.shadertoy.com/view/MlXGWf
// with Dave_Hoskins' Tileable Water Caustic https://www.shadertoy.com/view/MdlXz8

// (hastily jumbled together by jt)


// Caustics from https://www.shadertoy.com/view/MdlXz8 Tileable Water Caustic by Dave_Hoskins 

// Redefine below to see the tiling...
//#define SHOW_TILING

#define TAU 6.28318530718
#define MAX_ITER 5

vec3 caustics(vec2 fragCoord ) 
{
	float time = iTime;// * .5+23.0;
    // uv should be the 0-1 uv of texture...
	vec2 uv = fragCoord.xy / iResolution.xy;
    
#ifdef SHOW_TILING
	vec2 p = mod(uv*TAU*2.0, TAU)-250.0;
#else
    vec2 p = mod(uv*TAU, TAU)-250.0;
#endif
	vec2 i = vec2(p);
	float c = 1.0;
	float inten = .005;

	for (int n = 0; n < MAX_ITER; n++) 
	{
		float t = time * (1.0 - (3.5 / float(n+1)));
		i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
	}
	c /= float(MAX_ITER);
	c = 1.17-pow(c, 1.4);
	vec3 colour = vec3(pow(abs(c), 8.0));
    colour = clamp(colour + vec3(0.0, 0.35, 0.5), 0.0, 1.0);
    

	#ifdef SHOW_TILING
	// Flash tile borders...
	vec2 pixel = 2.0 / iResolution.xy;
	uv *= 2.0;

	float f = floor(mod(iTime*.5, 2.0)); 	// Flash value.
	vec2 first = step(pixel, uv) * f;		   	// Rule out first screen pixels and flash.
	uv  = step(fract(uv), pixel);				// Add one line of pixels per tile.
	colour = mix(colour, vec3(1.0, 1.0, 0.0), (uv.x + uv.y) * first.x * first.y); // Yellow line
	
	#endif
	return colour;
}


// Cool Lake from https://www.shadertoy.com/view/MlXGWf Xor - Cool Lake by Xor

#define time iTime
float sfract(float n)
{
    return smoothstep(0.0,1.0,fract(n));
}
float rand(vec2 n)
{
 	return fract(abs(sin(dot(n,vec2(5.3357,-5.8464))))*256.75+0.325);   
}

float noise(vec2 n)
{
    float h1 = mix(rand(vec2(floor(n.x),floor(n.y))),rand(vec2(ceil(n.x),floor(n.y))),sfract(n.x));
    float h2 = mix(rand(vec2(floor(n.x),ceil(n.y))),rand(vec2(ceil(n.x),ceil(n.y))),sfract(n.x));
    float s1 = mix(h1,h2,sfract(n.y));
    return s1;
}
void doCamera( out vec3 camPos, out vec3 camTar, in float time, in float mouseX )
{
    vec2 dir = ((iMouse.xy/iResolution.xy)*vec2(1.0,-0.5)+vec2(0.0,0.75))*6.28;
    vec3 pos = vec3(0.0,0.5,0.0);//vec3(noise((time/32.0)*vec2(1.0,0.0)),0.05,noise((time/32.0)*vec2(0.0,1.0)))*20.0;
	camPos =  pos;//vec3(cos(time/4.0)*8.0,1.0,sin(time/4.0)*8.0);
    camTar = pos+vec3(cos(dir.x)*cos(dir.y),sin(dir.y),sin(dir.x)*cos(dir.y));
}
vec3 doBackground( in vec3 dir)
{
    float sky = dot(dir,vec3(0.0,-1.0,0.0))*0.5+0.5;
    float sun = pow(dot(dir,normalize(vec3(1.0,0.7,0.9)))*0.5+0.5,32.0);
    vec2 p = vec2(dir.x+dir.z,dir.y-dir.z);
    float clouds = noise(p*8.0)*noise(p*9.0)*noise(p*10.0)*noise(p*11.0)*sky;
    vec3 total = vec3(sky*0.6+0.05+sun+clouds,sky*0.8+0.075+pow(sun,1.5)+clouds,sky+0.2+pow(sun,4.0)+clouds);
    vec3 ground = texture(iChannel0,(dir.xz)/dir.y).rrr*vec3(1.1,1.0,0.9);
    ground *= caustics(1000.0*(dir.xz)/dir.y);
    ground += 0.5 * caustics(1000.0*(dir.xz)/dir.y).rrr;
    return mix(total,ground,clamp((sky-0.6)*64.0,0.0,1.0));
}
    
float doModel( vec3 pos )
{
    vec3 p = pos+vec3(time*0.2,0.0,0.0)+vec3(noise(pos.xz),0.0,noise(pos.xz+8.0))*0.2;//Distort coordinates
    float height = 0.1*pow(noise(p.xz+vec2(time*0.7,time*0.6))*0.5+noise(p.xz*8.0+vec2(time))*0.35+noise(p.xz*16.0+vec2(0.0,time*0.5))*0.1+noise(p.xz*24.0)*0.05,0.25);
    float model = p.y-height;
    return model;
}
vec3 doMaterial(in vec3 rd, in vec3 nor )
{
    vec3 ref = doBackground(reflect(rd,nor));
    return mix(doBackground(refract(rd,nor,0.8)),ref,clamp(dot(ref,vec3(1.0/3.0))*1.5,0.0,1.0));
}
float calcSoftshadow( in vec3 ro, in vec3 rd );

vec3 doFog( in vec3 rd, in float dis, in vec3 mal )
{
    vec3 col = mal;
	col = mix(doBackground(rd),col,1.0-clamp(dis*dis/90.0,0.0,1.0));

    return col;
}

float calcIntersection( in vec3 ro, in vec3 rd )
{
	const float maxd = 10.0;           // max trace distance
	const float precis = 0.001;        // precission of the intersection
    float h = precis*2.0;
    float t = 0.0;
	float res = -1.0;
    for( int i=0; i<90; i++ )          // max number of raymarching iterations is 90
    {
        if( h<precis||t>maxd ) break;
	    h = doModel( ro+rd*t );
        t += h*.8;
    }

    if( t<maxd ) res = t;
    return res;
}

vec3 calcNormal( in vec3 pos )
{
    const float eps = 0.002;             // precision of the normal computation

    const vec3 v1 = vec3( 1.0,-1.0,-1.0);
    const vec3 v2 = vec3(-1.0,-1.0, 1.0);
    const vec3 v3 = vec3(-1.0, 1.0,-1.0);
    const vec3 v4 = vec3( 1.0, 1.0, 1.0);

	return normalize( v1*doModel( pos + v1*eps ) + 
					  v2*doModel( pos + v2*eps ) + 
					  v3*doModel( pos + v3*eps ) + 
					  v4*doModel( pos + v4*eps ) );
}

float calcSoftshadow( in vec3 ro, in vec3 rd )
{
    float res = 1.0;
    float t = 0.5;                 // selfintersection avoidance distance
	float h = 1.0;
    for( int i=0; i<40; i++ )         // 40 is the max numnber of raymarching steps
    {
        h = doModel(ro + rd*t);
        res = min( res, 64.0*h/t );   // 64 is the hardness of the shadows
		t += clamp( h, 0.02, 2.0 );   // limit the max and min stepping distances
    }
    return clamp(res,0.0,1.0);
}

mat3 calcLookAtMatrix( in vec3 ro, in vec3 ta, in float roll )
{
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(sin(roll),cos(roll),0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
    return mat3( uu, vv, ww );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (-iResolution.xy + 2.0*fragCoord.xy)/iResolution.y;
    vec2 m = iMouse.xy/iResolution.xy;
    
    // camera movement
    vec3 ro, ta;
    doCamera( ro, ta, iTime, m.x );

    // camera matrix
    mat3 camMat = calcLookAtMatrix( ro, ta, 0.0 );  // 0.0 is the camera roll
    
	// create view ray
	vec3 rd = normalize( camMat * vec3(p.xy,2.0) ); // 2.0 is the lens length

	vec3 col = doBackground(rd);

	// raymarch
    float t = calcIntersection( ro, rd );
    if( t>-0.5 )
    {
        // geometry
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal(pos);

        // materials
        vec3 mal = doMaterial(rd, nor );

        col = doFog( rd, t, mal );
	}
    // gamma
	col = pow( clamp(col,0.0,1.0), vec3(0.4545) );
	   
    fragColor = vec4( col, 1.0 );
}