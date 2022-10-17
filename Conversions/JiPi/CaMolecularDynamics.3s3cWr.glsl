

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Landau Ginzburg fluid" by michael0884. https://shadertoy.com/view/WlXBDf
// 2020-09-21 21:03:05

// Fork of "Everflow" by michael0884. https://shadertoy.com/view/ttBcWm
// 2020-07-19 18:18:22

// Fork of "Paint streams" by michael0884. https://shadertoy.com/view/WtfyDj
// 2020-07-11 22:38:47

vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing	

	return c.z * mix( vec3(1.0), rgb, c.y);
}

const int KEY_SPACE = 32;
bool isKeyPressed(int KEY)
{
	return texelFetch( iChannel3, ivec2(KEY,2), 0 ).x > 0.5;
}


#define radius 1.0
#define zoom 0.3

void mainImage( out vec4 col, in vec2 pos )
{    
    //zoom in
    if(isKeyPressed(KEY_SPACE))
    {
    	pos = iMouse.xy + pos*zoom - R*zoom*0.5;
    }
    float rho = 0.001;
    vec2 vel = vec2(0., 0.);

    //compute the smoothed density and velocity
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = floor(pos) + vec2(i,j);
        vec4 data = T(tpos);

        vec2 X0 = DECODE(data.x) + tpos;
        vec2 V0 = DECODE(data.y);
        float M0 = data.z;
        vec2 dx = X0 - pos;

        float K = GS(dx/radius)/(radius*radius);
        rho += M0*K;
        vel += M0*K*V0;
    }

   vel /= rho;
   vec3 vc = hsv2rgb(vec3(6.*atan(vel.x, vel.y)/(2.*PI), 1.0, rho*length(vel.xy)));
   col.xyz = cos(0.9*vec3(3,2,1)*rho) + 0.*vc;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T(p) texelFetch(iChannel0, ivec2(mod(p,R)), 0)
#define P(p) texture(iChannel0, mod(p,R)/R)
#define C(p) texture(iChannel1, mod(p,R)/R)

#define PI 3.14159265
#define dt 0.5
#define R iResolution.xy

//useful functions
#define GS(x) exp(-dot(x,x))
#define GS0(x) exp(-length(x))
#define CI(x) smoothstep(1.0, 0.9, length(x))
#define Dir(ang) vec2(cos(ang), sin(ang))
#define Rot(ang) mat2(cos(ang), sin(ang), -sin(ang), cos(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//Wyatt thermostat
#define cooling 1.5

//MD force
float MF(vec2 dx)
{
    return -GS(0.75*dx) + 0.13*GS(0.4*dx);
}


//the step functions need to be exactly like this!! step(x,0) does not work!
float Ha(vec2 x)
{
    return ((x.x >= 0.)?1.:0.)*((x.y >= 0.)?1.:0.);
}

float Hb(vec2 x)
{
    return ((x.x > 0.)?1.:0.)*((x.y > 0.)?1.:0.);
}

//particle grid


//data packing
#define PACK(X) ( uint(round(65534.0*clamp(0.5*X.x+0.5, 0., 1.))) + \
           65535u*uint(round(65534.0*clamp(0.5*X.y+0.5, 0., 1.))) )   
               
#define UNPACK(X) (clamp(vec2(X%65535u, X/65535u)/65534.0, 0.,1.)*2.0 - 1.0)              

#define DECODE(X) UNPACK(floatBitsToUint(X))
#define ENCODE(X) uintBitsToFloat(PACK(X))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//particle distribution
vec3 PD(vec2 x, vec2 pos)
{
    return vec3(x, 1.0)*Ha(x - (pos - 0.5))*Hb((pos + 0.5) - x);
}

void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
    
    vec2 X = vec2(0);
    vec2 V = vec2(0);
    float M = 0.;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = T(tpos);
       
        vec2 X0 = DECODE(data.x) + tpos;
    	vec2 V0 = DECODE(data.y);
    	int M0 = int(data.z);
        int M0H = M0/2;
        
        X0 += V0*dt; //integrate position
        
        //the deposited mass into this cell
        vec3 m = (M0 >= 2)?
            (float(M0H)*PD(X0+vec2(0.5, 0.0), pos) + float(M0 - M0H)*PD(X0-vec2(0.5, 0.0), pos))
            :(float(M0)*PD(X0, pos)); 
        
        //add weighted by mass
        X += m.xy;
        V += V0*m.z;
      
        //add mass
        M += m.z;
    }
    
    //normalization
    if(M != 0.)
    {
        X /= M;
        V /= M;
    }
    
    //initial condition
    if(iFrame < 1)
    {
        X = pos;
        V = vec2(0.);
        M = Ha(pos - (R*0.5 - R.x*0.15))*Hb((R*0.5 + R.x*0.15) - pos);
    }
    
    X = X - pos;
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float border(vec2 p)
{
    float bound = -sdBox(p - R*0.5, R*vec2(0.49, 0.49)); 
    float box = sdBox((p - R*vec2(0.5, 0.6)) , R*vec2(0.05, 0.01));
    float drain = -sdBox(p - R*vec2(0.5, 0.7), R*vec2(0.0, 0.0));
    return bound;
}

#define h 1.
vec3 bN(vec2 p)
{
    vec3 dx = vec3(-h,0,h);
    vec4 idx = vec4(-1./h, 0., 1./h, 0.25);
    vec3 r = idx.zyw*border(p + dx.zy)
           + idx.xyw*border(p + dx.xy)
           + idx.yzw*border(p + dx.yz)
           + idx.yxw*border(p + dx.yx);
    return vec3(normalize(r.xy), r.z + 1e-4);
}

void mainImage( out vec4 U, in vec2 pos )
{
    vec2 uv = pos/R;
    ivec2 p = ivec2(pos);
        
    vec4 data = T(pos); 
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.) //not vacuum
    {
        //Compute the force
        vec2 Fa = vec2(0.);
        range(i, -2, 2) range(j, -2, 2)
        {
            vec2 tpos = pos + vec2(i,j);
            vec4 data = T(tpos);

            vec2 X0 = DECODE(data.x) + tpos;
            vec2 V0 = DECODE(data.y);
            float M0 = data.z;
            vec2 dx = X0 - X;
           
            Fa += M0*MF(dx)*dx;
        }
        
        vec2 F = vec2(0.);
        if(iMouse.z > 0.)
        {
            vec2 dx= pos - iMouse.xy;
             F -= 0.003*dx*GS(dx/30.);
        }
        
       	//gravity
        F += 0.001*vec2(0,-1);
        
        //integrate velocity
        V += (F + Fa)*dt/M;
        
        //Wyatt thermostat
        X += cooling*Fa*dt/M;
        
        vec3 BORD = bN(X);
        V += 0.5*smoothstep(0., 5., -BORD.z)*BORD.xy;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.)?1.*v:1.;
    }
    
    //save
    X = X - pos;
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}