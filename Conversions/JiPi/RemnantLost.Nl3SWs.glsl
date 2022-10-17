

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// https://www.shadertoy.com/view/Nl3SWs

#define USE_TEX_NOISE_DERIVATIVE
// Uses a texture to store derivative noise data.
// Needs only one texture look up.
// We can't set buffer sizes, so the speed up is good not that great,
// because there's still an enormous amount of texture cache thrashing.
// It really needs a 256x256 buffer.

// PLEASE DO SELECTABLE BUFFER SIZES GUYS!

#define THRESHOLD .1
#define FAR 2000.0
#define SCALE 2.8
#define MINRAD2 .25
const float minRad2 = clamp(MINRAD2, 1.0e-9, 1.0);
const float absScalem1 = abs(SCALE - 1.0);
const float AbsScaleRaisedTo1mIters = pow(abs(SCALE), float(1-10));
const vec3 surfaceColour1 = vec3(.4, .0, 0.);
const vec3 surfaceColour2 = vec3(.4, .4, 0.4);
const vec3 surfaceColour3 = vec3(.4, 0.1, 0.00);
const vec4 scale =vec4(SCALE, SCALE, SCALE, abs(SCALE)) / minRad2;

const vec3 sunLight  = normalize( vec3(  1.1, 0.8,  -0.8 ) );
const vec3 sunColour = vec3(1.0, .8, .7);
const vec3 cloudColour = vec3(.35, .25, .25);

vec3 cameraPos;
float gTime = 0.0;


//-------------------------------------------------------------------------------------------------------

#ifdef USE_TEX_NOISE_DERIVATIVE

// Uses one texture look up...
vec3 noiseD(in vec2 x) 
{
    vec2 f = fract(x);
//    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
//    vec2 du = 30.*f*f*(f*(f-2.0)+1.);
    vec2 u = f*f*(3.0-2.0*f);
    vec2 du = 6.0*f*(1.0-f);


    ivec2 p = ivec2(floor(x));
	vec4 n = texelFetch(iChannel0, p & TWRAP, 0);

	return vec3(n.x + n.y * u.x + n.z * u.y + n.w * u.x*u.y,
				du * (n.yz + n.w*u.yx));
}

#else

// iq's original code from 'elevated'...
vec3 noiseD(in vec2 x )
{
    vec2 f = fract(x);
    
//    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
//    vec2 du = 30.*f*f*(f*(f-2.0)+1.0);
    vec2 u = f*f*(3.0-2.0*f);
    vec2 du = 6.0*f*(1.0-f);

    ivec2 p = ivec2(floor(x));
    float a = texelFetch(iChannel0, p&TWRAP, 0 ).x;
	float b = texelFetch(iChannel0, (p+ivec2(1,0))&TWRAP, 0 ).x;
	float c = texelFetch(iChannel0, (p+ivec2(0,1))&TWRAP, 0 ).x;
   	float d = texelFetch(iChannel0, (p+ivec2(1,1))&TWRAP, 0 ).x;

	return vec3(a + (b-a) * u.x+(c-a) *u.y+(a-b-c+d)*u.x*u.y,
				du*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));
}

#endif

//-------------------------------------------------------------------------------------------------------
// Basic 3D noise using texture channel...
float noise( in vec3 p )
{
    vec3 f = fract(p);
    p = floor(p);
	f = f*f*(3.0-2.0*f);
	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = textureLod( iChannel3, (uv+ 0.5)/256.0, 0.0).yx;
	return mix( rg.x, rg.y, f.z );
}

//-------------------------------------------------------------------------------------------------------

#define ANG2 1.2
#define ANG3 1.4
const mat2 rotMat = mat2(cos(ANG2), sin(ANG3), -sin(ANG3), cos(ANG2)) * 2.1;

//-------------------------------------------------------------------------------------------------------
float terrain( in vec2 p, float z)
{
	p = p*0.0015;
    vec2  d = vec2(0.0);
    float a = 0.0, b = 120.0;
    
    // Decrease iteration detail with distance...
    int iter = 13-int(log2(z*.2+.02));
    iter = clamp(iter, 2,13);
    
	for (int i = 0; i < iter; i++)
	{
       vec3 n = noiseD(p);
        
        d += n.yz;
        a += b*n.x/(1.+dot(d,d));
		b *= 0.47;
        p = rotMat*p;
	} 

	return a;
}

// Faffin' about...
#define ANG4 0.785398
#define ANG5 .785398
const mat2 rotRemXZ = mat2(cos(ANG4), sin(ANG4), -sin(ANG4), cos(ANG4));
const mat2 rotRemXY = mat2(cos(ANG5), sin(ANG5), -sin(ANG5), cos(ANG5));
mat3 rot3D;
//-------------------------------------------------------------------------------------------------------
float mapRemnant(vec3 pos) 
{
	
    pos = pos + vec3(300.,200.,-200);

    pos = rot3D * pos;

	
    //pos.zy = rotRemXY * pos.zy;
    //pos.xz = rotRemXZ * pos.xz;

    vec4 p = vec4(pos*0.006,1);

	vec4 p0 = p;
	for (int i = 0; i < 8; i++)
	{
		p.xyz = clamp(p.xyz, -1.0, 1.0) * 2.0 - p.xyz;

		float r2 = dot(p.xyz, p.xyz);
		p *= clamp(max(minRad2/r2, minRad2), 0.0, 1.0);

		p = p*scale + p0;
	}
	float l = ((length(p.xyz) - absScalem1) / p.w - AbsScaleRaisedTo1mIters) /.006;

    return l;
}

vec3 remnantColour(vec3 pos) 
{
    pos = pos + vec3(300.,200.,-200);
	vec4 p = vec4(pos*0.006,1);
    p.zy = rotRemXY * p.zy;
    p.xz = rotRemXZ * p.xz;


	vec4 p0 = p;
	float trap = 1.0;
    
	for (int i = 0; i < 6; i++)
	{
        
		p.xyz = clamp(p.xyz, -1.0, 1.0) * 2.0 - p.xyz;

		float r2 = dot(p.xyz, p.xyz);
		p *= clamp(max(minRad2/r2, minRad2), 0.0, 1.0);

		p = p*scale + p0;
		trap = min(trap, r2);
	}
	// |c.x|: log final distance (fractional iteration count)
	// |c.y|: spherical orbit trap at (0,0,0)
	vec2 c = clamp(vec2( 0.0001*length(p)-1., sqrt(trap) ), 0.0, 1.0);

    float t = mod(length(pos*.006) - gTime*32., 16.0);
    vec3 surf = mix( surfaceColour1, vec3(.1, 2., 5.), smoothstep(0.0, .3, t) * smoothstep(0.6, .3, t));
	return mix(mix(surf, surfaceColour2, c.y), surfaceColour3, c.x);
}


//-------------------------------------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
vec3 getSky(in vec3 rd)
{
	float sunAmount = max( dot( rd, sunLight), 0.0 );
	float v = pow(1.0-max(rd.y,0.0),5.);
	vec3  sky = mix(vec3(.0, .1, .2), cloudColour, v);
	sky = sky + sunColour * pow(sunAmount, 4.0) * .2;
	sky = sky + sunColour * pow(sunAmount, 800.0)*5.;
	return clamp(sky, 0.0, 1.0);
}


//-------------------------------------------------------------------------------------------------------
// Merge grass into the sky background for correct fog colouring...
vec3 applyFog( in vec3  rgb, in vec3 sky, in float dis,in vec3 pos, in vec3 dir)
{
	float fog = exp(-dis*dis* 0.000001);
    fog = clamp(fog-smoothstep(80.0, 0.0, pos.y)*.3, 0.0, 1.0);
    
	return mix(sky, rgb, fog);

}


//-------------------------------------------------------------------------------------------------------
// Calculate sun light...
vec3  DoLighting(in vec3 dif, in vec3 pos, in vec3 nor, in vec3 eyeDir, in float dis)
{
	float h = dot(sunLight,nor);
	vec3 mat = dif * sunColour*(max(h, 0.0)+.04);
    vec3 ref = reflect(eyeDir, nor);
    mat += sunColour * pow(max(dot(ref, sunLight), 0.0), 80.0)*.5;
    
     
    return min(mat, 1.0);
}


//-------------------------------------------------------------------------------------------------------
// Map the whole scene with two objects...
float map(vec3 p, float z)
{
    return  min(p.y-terrain(p.xz, z), mapRemnant(p));
}

//-------------------------------------------------------------------------------------------------------

// March the whole scene...
float rayMarch(in vec3 rO, in vec3 rD, in float st)
{
    float t = st;
	float d = 0.;

    float oldT = t;
    
    vec3 p;

	for(int j = min(0, iFrame); j < 160 && t < FAR; j++)
	{
	    p = rO + t*rD;
        d = map(p, t);
        if (abs(d) < THRESHOLD) break;
        oldT = t;
        t += d;// + t * 0.001; // Adding the current 't' thins out the Box too much.
	}
 
	return t;
}

//-------------------------------------------------------------------------------------------------------
vec3 CameraPath( float t )
{
    vec2 p = vec2(240.+1200.0 * sin(1.4*t), 800.0 * cos(1.*t) );
	return vec3(p.x,   terrain(p, 9000.)+95.0+cos(gTime*3.-2.7)*50.0, p.y);
} 


//-------------------------------------------------------------------------------------------------------
vec3 getNormal(vec3 p, float dis)
{
    dis = dis*2./iResolution.y;
	vec2 e = vec2(0,clamp(dis*dis, .001, 144.));
	return normalize(map(p, 0.0)-vec3(map(p - e.yxx, 0.0), map(p - e.xyx, 0.0), map(p - e.xxy, 0.0)));
}

//------------------------------------------------------------------------------
float shadow( in vec3 ro, in vec3 rd, in float dis)
{
	float res = 1.0;
    float t = 20.;
	float h;
	
    for (int i = 0; i < 25; i++)
	{
        vec3 p =  ro + rd*t;

		h = map(p, dis);
		res = min(.2*h / t*t, res);
		t += h+3.;
	}
    return clamp(res, .2, 1.0);
}


//-------------------------------------------------------------------------------------------------------
vec3 getDiffuse(vec3 pos, vec3 dir,  vec3 nor, float dis)
{
    vec3 dif = vec3(0);
    if ((pos.y-terrain(pos.xz, dis)) < THRESHOLD)
    {
        float n = cos(pos.y*.03+pos.x*.01+.8)*.5+.5;
    
        dif = vec3(mix(vec3(.9,.5,.3), vec3(1.0, .8, .7), n));
        float s = max(0.,nor.y*nor.y);
        dif = mix(dif, vec3(s*.3, s*.3,.1), clamp(nor.x+nor.z, 0.0, .8));
     }else
     {
         //float n = noise(pos*.3);
         dif = vec3(.2,0,0);
         dif = remnantColour(pos);
     }
    return dif;
}


//-------------------------------------------------------------------------------------------------------
vec3 cw, cu, cv;
vec3 getCamera(vec2 uv)
{
    vec3 camTar;
	cameraPos = CameraPath(gTime + 0.0);

	camTar	 = CameraPath(gTime + .3);

    camTar.y = cameraPos.y;
	
	float roll = .4*sin(gTime+.5);
	cw = normalize(camTar-cameraPos);
	vec3 cp = vec3(sin(roll), cos(roll),0.0);
	cu = cross(cw,cp);
	cv = cross(cu,cw);
    return normalize(uv.x*cu + uv.y*cv + 1.*cw);;
}


//-------------------------------------------------------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

	float m = (iMouse.x/iResolution.x)*50.0;
	gTime = (iTime*.8+m+403.0)*.1;
    vec2 xy = fragCoord.xy / iResolution.xy;
	vec2 uv = (-1.0 + 2.0 * xy) * vec2(iResolution.x/iResolution.y,1.0);

    rot3D = rotateY(0.785398) * rotateX(0.785398);

    if (abs(xy.y -.5) > .37)
	{
		// Top and bottom cine-crop - what a waste! :)
		fragColor=vec4(vec4(0.0));
		return;
	}

    vec3 dir = getCamera(uv);

	vec3 col;
	float dist;

    float st = hash12(uvec2(fragCoord*iTime))*50.;
	dist = rayMarch(cameraPos, dir, st);
    
    vec3 sky = getSky(dir);
    
	if (dist >= FAR)
    {
		// Completely missed the scene...
		col = sky;
	}
	else
	{
        // Render the objcets...
        vec3 pos = cameraPos + dist * dir;
        vec3 nor = getNormal(pos, dist);
        vec3 dif = getDiffuse(pos, dir, nor, dist);
        col = DoLighting(dif, pos, nor,dir, dist);
        col *= shadow( pos, sunLight, dist);
        col = applyFog(col, sky, dist, pos, dir);
	}


    // My usual Sun flare stuff...
	float bri = dot(cw, sunLight)*.75;
	if (bri > 0.0)
	{
		vec2 sunPos = vec2( dot( sunLight, cu ), dot( sunLight, cv ) );
		vec2 uvT = uv-sunPos;
		uvT = uvT*(length(uvT));
		bri = pow(bri, 6.0)*.8;

		// glare = the red shifted blob...
		float glare1 = max(dot(normalize(vec3(dir.x, dir.y+.3, dir.z)),sunLight),0.0)*1.4;
		// glare2 is the cyan ring...
		float glare2 = max(1.0-length(uvT+sunPos*.5)*4.0, 0.0);
		uvT = mix (uvT, uv, -2.3);
		// glare3 is a purple splodge...
		float glare3 = max(1.0-length(uvT+sunPos*5.0)*1.2, 0.0);

		col += bri * vec3(1.0, .0, .0)  * pow(glare1, 12.5)*.1;
		col += bri * vec3(.2, 1.0, 1.) * pow(glare2, 2.0)*3.;
		col += bri * sunColour * pow(glare3, 2.0)*3.5;
	}
    
    // Post screen effects...
    //col = smoothstep(0.0, 1.0, col);
    // Contrast...
    col = col*.3 + (col*col*(3.0-2.0*col))*.7;
    // Gamma...
    col = sqrt(col);
	
	fragColor=vec4(col,1.0);
}

//-------------------------------------------------------------------------------------------------------
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Create derivative noise texture
// PLEASE DO SELECTABLE BUFFER SIZES GUYS!

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float derivHash(ivec2 q2)
{
   uvec2 q = uvec2((q2+10) & TWRAP);  // ...Seeded and wrapped.
   float f = hash12(q);
   return pow(f, 2.)*1.5;
}


void mainImage( out vec4 colour, in vec2 coord )
{

// Draw it ony once to relavent area...

// It seems the buffers are also doubled as I need to draw 2 frames...
  if (iFrame < 2 && coord.x < TSIZE && coord.y < TSIZE)
  {

    vec4 data, n;
    ivec2 co = ivec2(floor(coord));

    float a = derivHash(co);
    float b = derivHash((co+ivec2(1,0)));
    float c = derivHash((co+ivec2(0,1)));
    float d = derivHash((co+ivec2(1,1)));

// Pre-calc all the sums...
    data.x = a;	 		
    data.y = b-a; 			
    data.z = c-a; 			
    data.w = a - b - c + d;

    colour = data;
  }
  else discard;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define TSIZE 256.
#define TWRAP 255


#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xfffffffeU))

float hash12(uvec2 q)
{
	q *= UI2;
	uint n = (q.x ^ q.y) * UI0;
	return float(n) * UIF;
}

vec2 hash22(vec2 vq)
{
    uvec2 q = uvec2(vq);

	q *= UI2;
	q = (q.x ^ q.y) * UI2;
	return vec2(q) * UIF;
}
//----------------------------------------------------------------------------------------
vec2 noise2D( in vec2 n )
{
    vec2 p = floor(n);
    n = fract(n);
    n = n*n*(3.0-2.0*n);
    
    vec2 res = mix(mix( hash22(p), hash22(p+vec2(1.0 ,0.0)),n.x),
                    mix( hash22(p + vec2(0.0,1.0)), hash22(p + vec2(1.0,1.0)),n.x),n.y);
    return res;
}

float sMin( float a, float b, float k )
{
    
	float h = clamp(0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.-h);
}

mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

// Rotation matrix around the Y axis.
mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}

// Rotation matrix around the Z axis.
mat3 rotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<

//----------------------------------------------------------------------------------------
vec2 mainSound( in int samp, float time )
{
    
	// Engine noise...
    vec2 noi = (noise2D(vec2(time*340.0))-.5)*.5 * noise2D(vec2(time*.3));
    noi += (noise2D(vec2(time*600.0))-.5)*.5 * noise2D(vec2(time*.2+20.0));
    noi += (noise2D(vec2(time*40.0))-.5)*.8 *  noise2D(vec2(time*2.+100.0));
    
    
    noi *= smoothstep(0.0, 4.0,time) * smoothstep(180.0, 170.0,time);
    return noi;
}