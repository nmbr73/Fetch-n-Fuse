

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 pos )
{
	vec4 particle = texel(ch0, pos);
    float distr = 4.*gauss(pos - particle.xy, 0.7);
    vec4 b = texel(ch1, pos);
    vec3 c1 = jet_range(length(texel(ch0, b.xy).zw), 0.,0.9);
    vec3 c2 = jet_range(length(texel(ch0, b.zw).zw), 0.,0.9);
    float line = exp(-pow(sdLine(pos, b.xy, b.zw)/0.5,2.));
 	float linel = length(b.xy - b.zw);
    float pl1 = length(pos - b.zw);
    vec3 color = mix(c2,c1,pl1/(linel+0.01)); 
    fragColor = vec4(1.3*color*line + distr, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//voronoi particle tracking 

//loop the vector
vec2 loop_d(vec2 pos)
{
	return mod(pos + size*0.5, size) - size*0.5;
}

//loop the space
vec2 loop(vec2 pos)
{
	return mod(pos, size);
}


void Check(inout vec4 U, vec2 pos, vec2 dx)
{
    vec4 Unb = texel(ch0, loop(pos+dx));
    //check if the stored neighbouring particle is closer to this position 
    if(length(loop_d(Unb.xy - pos)) < length(loop_d(U.xy - pos)))
    {
        U = Unb; //copy the particle info
    }
}

void CheckRadius(inout vec4 U, vec2 pos, float r)
{
    Check(U, pos, vec2(-r,0));
    Check(U, pos, vec2(r,0));
    Check(U, pos, vec2(0,-r));
    Check(U, pos, vec2(0,r));
}

void mainImage( out vec4 U, in vec2 pos )
{
    //this pixel value
    U = texel(ch0, pos);
    
    //check neighbours 
    CheckRadius(U, pos, 1.);
    CheckRadius(U, pos, 2.);
    CheckRadius(U, pos, 3.);
    CheckRadius(U, pos, 4.);
   
    U.xy = loop(U.xy);
    
    vec2 particle_pos = U.xy;
    
    if(iMouse.z > 0.) 
    {
        vec2 dm = (iMouse.xy - U.xy);
        U.zw += dt*normalize(dm)*pow(length(dm)+10., -1.);
        U.zw *= 0.998;
    }
    
    //update the particle
    U.xy += dt*U.zw;
    
    U.xy = loop(U.xy);
    
    
    if(iFrame < 1)
    {
        particle_pos = vec2(10.*floor(pos.x/10.),10.*floor(pos.y/10.));
        U = vec4(particle_pos, hash22(particle_pos) - 0.5);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//simulation variables
#define dt 0.7
#define radius 17.

//definitions
#define size iResolution.xy
#define texel(a, p)  texelFetch(a, ivec2(p), 0)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3
#define PI 3.14159265

//hash functions
//https://www.shadertoy.com/view/4djSRW
float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}


vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}

vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}


//functions
float gauss(vec2 x, float r)
{
    return exp(-pow(length(x)/r,2.));
}
   
float sdLine( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

//a rainbow colormap from Matlab
float interpolate(float val, float y0, float x0, float y1, float x1) 
{
    return (val-x0)*(y1-y0)/(x1-x0) + y0;
}

float base(float val) 
{
    if ( val <= -0.75 ) return 0.0;
    else if ( val <= -0.25 ) return interpolate( val, 0.0, -0.75, 1.0, -0.25 );
    else if ( val <= 0.25 ) return 1.0;
    else if ( val <= 0.75 ) return interpolate( val, 1.0, 0.25, 0.0, 0.75 );
    else return 0.0;
}

vec3 jet_colormap(float v)
{
    return vec3(base(v - 0.5),base(v),base(v + 0.5));
}

vec3 jet_range(float v, float a, float b)
{
    return jet_colormap(2.*clamp((v-a)/(b-a),0.,1.) - 1.);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//voronoi line tracking 

//loop the vector
vec2 loop_d(vec2 pos)
{
	return mod(pos + size*0.5, size) - size*0.5;
}

//loop the space
vec2 loop(vec2 pos)
{
	return mod(pos, size);
}

vec4 loop(vec4 line)
{
	return vec4(mod(line.xy, size),mod(line.zw, size));
}

float is_border(vec2 pos)
{
    vec4 cp = texel(ch1, pos);
    vec4 p0 = texel(ch1, loop(pos+vec2(-1,0.)));
    vec4 p1 = texel(ch1, loop(pos+vec2(1,0.)));
    vec4 p2 = texel(ch1, loop(pos+vec2(0.,-1.)));
    vec4 p3 = texel(ch1, loop(pos+vec2(0.,1.)));
    if(cp!=p0 || cp!=p1 || cp!=p2 || cp!=p3)
    {
        return 1.;
    }
    return 0.;
}

vec4 use_best(vec2 pos, vec4 U1, vec4 U2)
{
    float d1 = sdLine(pos, U1.xy, U1.zw);
    float d2 = sdLine(pos, U2.xy, U2.zw);
    //check if the stored neighbouring line is closer to this position 
    if(d2 < d1)
    {
       return U2; //copy the line info
    }
    else
    {
       return U1;
    }
}


float d(vec2 a, vec2 b)
{
    return length(a-b);
}



float is_direct_neighbour(vec2 p1, vec2 p2)
{
   /* vec2 cpoint = (p1+p2)/2.;
    vec2 cvect = 2.*normalize(p2 - p1);
    //is the center point a boundary between at least 1 of them
    vec4 pp1 = texel(ch1, cpoint + cvect);
    vec4 pp2 = texel(ch1, cpoint - cvect);*/
    if( d(p1,p2) < radius)
    {
        return 1.;
    }
    
    return 0.;
}  

//the boudary line intersection is the source of the line info
void on_center(inout vec4 U, vec2 pos)
{
    vec4 p0 = texel(ch1, loop(pos+vec2(-1,0.)));
    vec4 p1 = texel(ch1, loop(pos+vec2(1,0.)));
    vec4 p2 = texel(ch1, loop(pos+vec2(0.,-1.)));
    vec4 p3 = texel(ch1, loop(pos+vec2(0.,1.)));
    if(p0 != p1 && is_direct_neighbour(p0.xy,p1.xy) > 0.)
    {
       U = use_best(pos, vec4(p0.xy, p1.xy), U);
    }
    if(p2 != p3 && is_direct_neighbour(p2.xy,p3.xy) > 0.)
    {
       U = use_best(pos, vec4(p2.xy, p3.xy), U);
    } 
}


void Check(inout vec4 U, vec2 pos, vec2 dx)
{
    vec4 Unb = loop(texel(ch0, loop(pos+dx)));
    float d1 = sdLine(pos, U.xy, U.zw);
    float d2 = sdLine(pos, Unb.xy, Unb.zw);
    //check if the stored neighbouring line is closer to this position 
    if(d2 < d1)
    {
        U = Unb; //copy the line info
    }
}

void CheckRadius(inout vec4 U, vec2 pos, float r)
{
    Check(U, pos, vec2(-r,0));
    Check(U, pos, vec2(r,0));
    Check(U, pos, vec2(0,-r));
    Check(U, pos, vec2(0,r));
}

void mainImage( out vec4 U, in vec2 pos )
{
    //this pixel value
    U = texel(ch0, pos);
    
    //check neighbours 
    CheckRadius(U, pos, 1.);
    CheckRadius(U, pos, 2.);
    CheckRadius(U, pos, 3.);
    CheckRadius(U, pos, 4.);
    CheckRadius(U, pos, 5.);
    CheckRadius(U, pos, 6.);
    CheckRadius(U, pos, 7.);
    CheckRadius(U, pos, 8.);
    CheckRadius(U, pos, 9.);
    CheckRadius(U, pos, 10.);
    
    //update the line from the particles
    U.xy = loop(texel(ch1, loop(U.xy)).xy);
    U.zw = loop(texel(ch1, loop(U.zw)).xy);
    
    //sort 
    if(length(U.xy) > length(U.zw)) U = U.zwxy;
    
    if(is_direct_neighbour(U.xy, U.zw) < 1.)
    {
        U = vec4(0.);
    }
    
    on_center(U, pos);
}