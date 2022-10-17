

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

void mainImage( out vec4 col, in vec2 pos )
{
    ivec2 p = ivec2(pos);
   
    vec4 data = T(pos); 
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = data.z;
    
    //how much mass falls into this pixel
    vec4 rho = vec4(V, M, 1.)*GS((pos - X)/0.5); 
    vec3 dx = vec3(-3., 0., 3.);

    float ang = atan(V.x, V.y);
    float mag = 0. + 3.*length(V.xy)*rho.z;
    
    float a = pow(smoothstep(fluid_rho*0., fluid_rho*2., rho.z),0.1);
    // Output to screen
    col.xyz += 0.2*a;
    col.xyz += 0.5 - 0.5*cos(2.*vec3(0.3,0.5,1.)*mix(rho.w,rho.z,0.));
    //col.xyz += vec3(1,1,1)*bord;
    col.xyz = tanh(4.*pow(col.xyz,vec3(1.5))) + hsv2rgb(vec3(5.*ang/PI, 1.2, mag));
    col.w=1.0;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T(p) texelFetch(iChannel0, ivec2(mod(p,R)), 0)
#define P(p) texture(iChannel0, mod(p,R)/R)

#define PI 3.14159265
#define dt 1.5
#define R iResolution.xy

//mold stuff 
#define sense_ang 0.4
#define sense_dis 2.5
#define sense_force 0.1
#define trailing 0.
#define acceleration 0.01

//SPH pressure
#define Pressure(rho) 1.*rho
#define fluid_rho 0.2

//useful functions
#define GS(x) exp(-dot(x,x))
#define GS0(x) exp(-length(x))
#define Dir(ang) vec2(cos(ang), sin(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

//data packing
#define PACK(X) ( uint(round(65534.0*clamp(0.5*X.x+0.5, 0., 1.))) + \
           65535u*uint(round(65534.0*clamp(0.5*X.y+0.5, 0., 1.))) )   
               
#define UNPACK(X) (clamp(vec2(X%65535u, X/65535u)/65534.0, 0.,1.)*2.0 - 1.0)              

#define DECODE(X) UNPACK(floatBitsToUint(X))
#define ENCODE(X) uintBitsToFloat(PACK(X))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
    
    vec2 X = vec2(0);
    vec2 V = vec2(0);
    float M = 0.;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        vec2 tpos = pos + vec2(i,j);
        vec4 data = T(tpos);
       
        vec2 X0 = DECODE(data.x) + tpos;
    	vec2 V0 = DECODE(data.y);
    	vec2 M0 = data.zw;
       
        X0 += V0*dt; //integrate position

        //particle distribution size
        float K = 1.3;
        
        vec4 aabbX = vec4(max(pos - 0.5, X0 - K*0.5), min(pos + 0.5, X0 + K*0.5)); //overlap aabb
        vec2 center = 0.5*(aabbX.xy + aabbX.zw); //center of mass
        vec2 size = max(aabbX.zw - aabbX.xy, 0.); //only positive
        
        //the deposited mass into this cell
        float m = M0.x*size.x*size.y/(K*K); 
        
        //add weighted by mass
        X += center*m;
        V += V0*m;
      
        //add mass
        M += m;
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
        M = 1e-6;
    }
    
    X = clamp(X - pos, vec2(-0.5), vec2(0.5));
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
        
    vec4 data = T(pos); 
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = data.z;
    
    if(M != 0.) //not vacuum
    {
        //Compute the SPH force
        vec2 F = vec2(0.);
        vec3 avgV = vec3(0.);
        range(i, -2, 2) range(j, -2, 2)
        {
            vec2 tpos = pos + vec2(i,j);
            vec4 data = T(tpos);

            vec2 X0 = DECODE(data.x) + tpos;
            vec2 V0 = DECODE(data.y);
            float M0 = data.z;
            vec2 dx = X0 - X;
            
            float avgP = 0.5*M0*(Pressure(M) + Pressure(M0)); 
            F -= 0.5*GS(1.*dx)*avgP*dx;
            avgV += M0*GS(1.*dx)*vec3(V0,1.);
        }
        avgV.xy /= avgV.z;

        //slime mold sensors
        float ang = atan(V.y, V.x);
        vec4 dir = sense_dis*vec4(Dir(ang+sense_ang), Dir(ang - sense_ang));
        vec2 sd = vec2(P(X + dir.xy).z, P(X + dir.zw).z);
        F += sense_force*(Dir(ang+PI*0.5)*sd.x + Dir(ang-PI*0.5)*sd.y); 

        //integrate velocity
        V += F*dt/M;
        
        //acceleration for fun effects
        V *= 1. + acceleration;
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.)?1.*v:1.;
    }
    
    //mass decay
    M *= 0.99;
    
    //input
    if(iMouse.z > 0.)
    	M = mix(M, 0.5, GS((pos - iMouse.xy)/13.));
    else
        M = mix(M, 0.5, GS((pos - R*0.5)/13.));
    
    //save
    X = clamp(X - pos, vec2(-0.5), vec2(0.5));
    U = vec4(ENCODE(X), ENCODE(V), M, 0.);
}