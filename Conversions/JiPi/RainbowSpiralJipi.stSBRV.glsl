

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Noise animation - Flow
// 2014 by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/MdlXRS
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// Contact the author for other licensing options



//Somewhat inspired by the concepts behind "flow noise"
//every octave of noise is modulated separately
//with displacement using a rotated vector field

//normalization is used to created "swirls"
//usually not a good idea, depending on the type of noise
//you are going for.

//Sinus ridged fbm is used for better effect.

#define time iTime*0.1
#define tau 6.2831853
// details 0.0 - 1.0
#define details 0.3

mat2 makem2(in float theta){float c = cos(theta);float s = sin(theta);return mat2(c,-s,s,c);}
float hash(vec2 p) {
    p  = 50.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

float noise( in vec2 x ) {
    vec2 i = floor( x );
    vec2 f = fract( x );
	
	vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}
mat2 m2 = mat2( 0.80,  0.60, -0.60,  0.80 );

vec3 hsb2rgb( in vec3 c ){
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),
                             6.0)-3.0)-1.0,
                     0.0,
                     1.0 );
    rgb = rgb*rgb*(3.0-2.0*rgb);
    return c.z * mix( vec3(1.0), rgb, c.y);
}

float grid(vec2 p)
{
	float s = sin(p.x)*cos(p.y);
	return s;
}

float flow(in vec2 p)
{
	float z=2.;
	float rz = 0.;
	vec2 bp = p;
	for (float i= 1.;i < 7.;i++ )
	{
		bp += time*1.5;
		vec2 gr = vec2(grid(p*3.-time*2.),grid(p*3.+4.-time*2.))*0.4;
		gr = normalize(gr)*0.4;
		gr *= makem2((p.x+p.y)*.3+time*10.);
		p += gr*0.5;
		
		rz+= (sin(noise(p)*8.)*0.5+0.5) /z;
		
		p = mix(bp,p,.5);
		z *= 1.7;
		p *= 2.5;
		p*=m2;
		bp *= 2.5;
		bp*=m2;
	}
	return rz;	
}

float spiral(vec2 p,float scl) 
{
	float r = length(p);
	r = log(r);
	float a = atan(p.y, p.x);
	return abs(mod(scl*(r-2./scl*a),tau)-1.)*2.;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 p = fragCoord.xy / iResolution.xy-0.5;
    vec3 color = vec3(1.0);
        
        
    // rainbow part
    float angle = atan(p.y,p.x)+iTime;
    color = hsb2rgb(vec3((angle/tau)+0.5,length(p)*2.0,1.0));

    
	p.x *= iResolution.x/iResolution.y;
	p*= 3.;
	float rz = flow(p) * details;
	p /= exp(mod(time*9.,2.1));
	rz += (6.-spiral(p,3.))*.5;

	vec3 col = abs(vec3(.2,0.07,0.01)/rz);
    
	fragColor = vec4(col * color,1.0);
}