

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Building smasher" by michael0884. https://shadertoy.com/view/wdGcRK
// 2020-11-22 17:42:31

// Fork of "Neo-Hookean Field" by michael0884. https://shadertoy.com/view/3dVyDD
// 2020-10-20 18:45:49

// Fork of "Neo-Hookean 2: Electric Boogaloo" by michael0884. https://shadertoy.com/view/tsVyWR
// 2020-10-16 17:16:16

// Fork of "CA Neo-Hookean" by michael0884. https://shadertoy.com/view/WdGyWR
// 2020-10-13 18:17:06

//used sources 
//https://github.com/nialltl/incremental_mpm/blob/master/Assets/2.%20MLS_MPM_NeoHookean_Multithreaded/MLS_MPM_NeoHookean_Multithreaded.cs
//https://www.seas.upenn.edu/~cffjiang/research/mpmcourse/mpmcourse.pdf

// Fork of "CA Paste" by michael0884. https://shadertoy.com/view/tsGczh
// 2020-10-12 21:02:54

// Fork of "CA Molecular dynamics" by michael0884. https://shadertoy.com/view/3s3cWr
// 2020-10-08 22:00:15

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
#define zoom 0.35

void mainImage( out vec4 col, in vec2 pos )
{    
    //zoom in
    if(isKeyPressed(KEY_SPACE))
    {
    	pos = iMouse.xy + pos*zoom - R*zoom*0.5;
    }
    float rho = 0.001;
    vec2 c = vec2(0.);
    float De = 0.;
    vec2 vel = vec2(0., 0.);
    vec2 grad = vec2(0.);

    float rho2 = 0.;
    //compute the smoothed density and velocity
    range(i, -1, 1) range(j, -1, 1)
    {
        vec2 tpos = floor(pos) + vec2(i,j);
        vec4 data = T(tpos);

        vec2 X0 = DECODE(data.x) + tpos;
        vec2 V0 = DECODE(data.y);
        float M0 = data.z;
        vec2 dx = X0 - pos;
        vec2 dx0 = X0 - tpos;
	    mat2 D0 = mat2(T1(tpos));
        
        float K = GS(dx/radius)/(radius*radius);
        rho += M0*K;
        grad += normalize(dx)*K;
        c += M0*K*DECODE(data.w);
        De += M0*K*abs(deformation_energy(D0));
        vel += M0*K*V0;
        vec2 dsize = destimator(dx0,  data.z);
        float bsdf = sdBox(pos - X0,0.5*dsize);
        //float bsdf = length(pos - X0) - 0.5*length(destimator(dx0));
        rho2 += M0*smoothstep(0.3, -0.3, bsdf)/(dsize.x*dsize.y);
    }

   grad /= rho; 
   c /= rho;
   vel /= rho;
   De /= rho;
    
   //vec3 vc = hsv2rgb(vec3(6.*atan(vel.x, vel.y)/(2.*PI), 1.0, rho*length(vel.xy)));
   float d = 0.5*rho2;
   col.xyz = mix(vec3(0.),1.0*texture(iChannel2, c).xyz, d);
   col.xyz = 1. - col.xyz;
    //col.xyz = vec3(rho2)*0.2;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T(p) texelFetch(iChannel0, ivec2(mod(p,R)), 0)
#define T1(p) texelFetch(iChannel1, ivec2(mod(p,R)), 0)
#define P(p) texture(iChannel0, mod(p,R)/R)
#define C(p) texture(iChannel1, mod(p,R)/R)

#define PI 3.14159265
#define dt 1.0
#define R iResolution.xy

//useful functions
#define GS(x) exp(-dot(x,x))
#define GS0(x) exp(-length(x))
#define CI(x) smoothstep(1.0, 0.9, length(x))
#define Dir(ang) vec2(cos(ang), sin(ang))
#define Rot(ang) mat2(cos(ang), sin(ang), -sin(ang), cos(ang))
#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define velocity_averaging 0.

//squishy solid
#define relax 0.03
#define distribution_size 1.0
//estimation str
#define difd 2.0
//target density
#define trho 0.
//density target strenght
#define rhoe 0.0

//estimating the in-cell distribution size
vec2 destimator(vec2 dx, float M)
{
    //size estimate by in-cell location
    vec2 ds = clamp(1.0 - 2.0*abs(dx), 0.001, 1.0);
    return ds + 0.*max(M/(ds.x*ds.y) - 1.1, 0.)*dt;
}

float deformation_energy(mat2 D)
{
    D = transpose(D)*D;
    return 2.*(D[0][0]*D[0][0] + D[1][1]*D[1][1] - 2.0);
}


// Lamé parameters for stress-strain relationship
#define elastic_lambda 0.6
#define elastic_mu 0.0
#define incompressible_viscosity 1.0


//viscous fluid
/*
#define relax 0.05
#define distribution_size 0.98
// Lamé parameters for stress-strain relationship
#define elastic_lambda 0.2
#define elastic_mu 0.1
#define incompressible_viscousity 0.05
*/ 

//MD force
float MF(vec2 dx, vec2 dv)
{
    return incompressible_viscosity*dot(dx,dv)*GS(0.8*dx);
}


float Ha(vec2 x)
{
    return ((x.x >= 0.)?1.:0.)*((x.y >= 0.)?1.:0.);
}

float Hb(vec2 x)
{
    return ((x.x > 0.)?1.:0.)*((x.y > 0.)?1.:0.);
}

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float opSubtraction( float d1, float d2 ) { return max(-d1,d2); }

vec2 opRepLim(in vec2 p, in vec2 c, in vec2 l)
{
    return p-c*clamp(round(p/c),-l,l);
}
//data packing
#define PACK(X) ( uint(round(65534.0*clamp(0.5*X.x+0.5, 0., 1.))) + \
           65535u*uint(round(65534.0*clamp(0.5*X.y+0.5, 0., 1.))) )   
               
#define UNPACK(X) (clamp(vec2(X%65535u, X/65535u)/65534.0, 0.,1.)*2.0 - 1.0)              

#define DECODE(X) UNPACK(floatBitsToUint(X))
#define ENCODE(X) uintBitsToFloat(PACK(X))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Reintegration tracking

float particleBox(vec2 x, vec2 s)
{
    return float(sdBox(x, s) < 0.);
}

float particleArch(vec2 x, vec2 s)
{
    return float(opSubtraction(sdBox(x + vec2(0, s.y*0.4), s*vec2(0.5, 0.9)), sdBox(x, s)) < 0.);
}

float Building(vec2 x, vec2 s)
{
    vec2 room_s = s.y*vec2(0.12);
    vec2 rep_s = vec2(0.15)*s.x;
    float rooms = sdBox(opRepLim(x + vec2(0., -0.1*room_s.y), rep_s, vec2(30.0)), room_s);
    float sd = opSubtraction(rooms, sdBox(x, s));
    return float(sd < 0.);
}

void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
    
    vec2 X = vec2(0);
    vec2 V = vec2(0);
    float M = 0.;
    vec2 C = vec2(0.);
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
        vec2 tpos = vec2(i,j);
        vec4 data = T(pos + tpos);
       
        vec2 X0 = DECODE(data.x) + tpos;
    	vec2 V0 = DECODE(data.y);
        
        
        //particle distribution size
        vec2 K = destimator(X0 - tpos , data.z);
       
        X0 += V0*dt; //integrate position

        //box overlaps
        vec2 aabb0 = max(X0 - K*0.5, -0.5); 
        vec2 aabb1 = min(X0 + K*0.5, 0.5); 
        vec2 size = max(aabb1 - aabb0, 0.); 
        vec2 center = 0.5*(aabb0 + aabb1);

        //the deposited mass into this cell
        vec3 m = data.z*vec3(center, 1.0)*size.x*size.y/(K.x*K.y);
        
        //add weighted by mass
        X += m.xy;
        V += V0*m.z;
      	C += m.z*DECODE(data.w);
        //add mass
        M += m.z;
    }
    
    //normalization
    if(M != 0.)
    {
        X /= M;
        V /= M;
        C /= M;
    }
    
    //initial condition
    if(iFrame < 1)
    {
        X = pos;
        V = vec2(0.);
        vec4 nya = texture(iChannel2, clamp(1.8*X*vec2(0.166,1.)/R, vec2(0.),vec2(0.1666, 1.0)));
        M = max(max(Building(X - R*vec2(0.5,0.32), R*vec2(0.4,0.3)),
            particleBox(X - R*vec2(0.1,0.9), R*vec2(0.0))), 
                particleBox(X - R*vec2(0.5,0.12), R*vec2(0.47, 0.3)));
        
        C = mod(3.*pos/R, 1.);
        X = vec2(0.);
    }
    
    U = vec4(ENCODE(X), ENCODE(V), M, ENCODE(C));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//deformation gradient advection and update


//particle distribution
vec3 PD(vec2 x, vec2 pos)
{
    return vec3(x, 1.0)*Ha(x - (pos - 0.5))*Hb((pos + 0.5) - x);
}

void mainImage( out vec4 U, in vec2 pos )
{
    ivec2 p = ivec2(pos);
    
    //deformation gradient
   	mat2 D = mat2(0);
    float M = 0.;
    
    //basically integral over all updated neighbor distributions
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -1, 1) range(j, -1, 1)
    {
         vec2 tpos = vec2(i,j);
        vec4 data = T(pos + tpos);
       
        vec2 X0 = DECODE(data.x) + tpos;
    	vec2 V0 = DECODE(data.y);
        mat2 D0 = mat2(T1(tpos + pos));
        
        //particle distribution size
        vec2 K = destimator(X0 - tpos , data.z);
       
        X0 += V0*dt; //integrate position

        //box overlaps
        vec2 aabb0 = max(X0 - K*0.5, -0.5); 
        vec2 aabb1 = min(X0 + K*0.5, 0.5); 
        vec2 size = max(aabb1 - aabb0, 0.); 
        vec2 center = 0.5*(aabb0 + aabb1);

        //the deposited mass into this cell
        vec3 m = data.z*vec3(center, 1.0)*size.x*size.y/(K.x*K.y);
       
        //add deformation grad weighted by mass
        D += D0*m.z;
      	
        //add mass
        M += m.z;
    }
    
    //normalization
    if(M != 0.)
    {
       D /= M;
    }
	else D = mat2(1.0);
    
    //initial condition
    if(iFrame < 1)
    {
        D = mat2(1.0);
    }

    U = vec4(D);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 normalize2(vec2 a)
{
    return all(equal(a, vec2(0.)))?vec2(0.):normalize(a);
}

void mainImage( out vec4 U, in vec2 pos )
{
    vec2 uv = pos/R;
    ivec2 p = ivec2(pos);
        
    vec4 data = T(pos); 
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = data.z;
    float C = data.w;
    mat2 D = mat2(T1(pos));
    
    if(M > 0.01) //not vacuum
    {
        //Compute the velocity gradient matrix
        mat2 B = mat2(0.);
        float a = 0.01;
        float rho = 0.;
        range(i, -1, 1) range(j, -1, 1)
        {
            vec2 tpos = pos + vec2(i,j);
            vec4 data = T(tpos);

            vec2 X0 = DECODE(data.x) + tpos;
            vec2 V0 = DECODE(data.y);
            float M0 = data.z;
            vec2 dx = X0 - X;
            vec2 dv = V0 - V;
			vec2 dsize = clamp(destimator(X0 - tpos, data.z), 0.3, 1.0);
            float weight = M0*GS(1.2*dx);
            rho += M0*weight;
            B += mat2(dv*dx.x,dv*dx.y)*weight;
            a += weight;
        }
        B /= a;
        rho /= a;
      
        float drho = rho - 1.0;
        B -= 0.004*mat2(drho)*abs(drho);
       
        //integrate deformation gradient
       	D += 1.*dt*B*D;
       
        //smoothing
        
        float r = relax + 0.05*smoothstep(-30., 0., -pos.y);
        D = D*(1. - r) + mat2(1.)*r;
        
        //clamp the gradient to not go insane
        D = mat2(clamp(vec4(D), -5.0, 5.0));
    }
    
    //save
    U = vec4(D);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//velocity update

float border(vec2 p)
{
    float bound = -sdBox(p - R*0.5, R*vec2(0.48, 0.48)); 
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

vec2 normalize2(vec2 a)
{
    return all(equal(a, vec2(0.)))?vec2(0.):normalize(a);
}


mat2 strain(mat2 D)
{
    float J = abs(determinant(D)) + 0.001;

    // MPM course, page 46
    float volume = J;

    // useful matrices for Neo-Hookean model
    mat2 F_T = transpose(D);
    mat2 F_inv_T = inverse(F_T);
    mat2 F_minus_F_inv_T = D - F_inv_T;

    // MPM course equation 48
    mat2 P_term_0 = elastic_mu * (F_minus_F_inv_T);
    mat2 P_term_1 = elastic_lambda * log(J) * F_inv_T;
    mat2 P = P_term_0 + P_term_1;

    // equation 38, MPM course
    mat2 stress = (1./J)* P * F_T;

    return volume * stress;
}


void mainImage( out vec4 U, in vec2 pos )
{
    vec2 uv = pos/R;
    ivec2 p = ivec2(pos);
        
    vec4 data = T(pos); 
    mat2 D = mat2(T1(pos));
    vec2 X = DECODE(data.x) + pos;
    vec2 V = DECODE(data.y);
    float M = clamp(data.z, 0., 2.0);
    vec2 C = DECODE(data.w);
    if(M>0.0) //not vacuum
    {
        //Compute the force
      
        vec2 F = vec2(0.);
        float b = 0.;
   
        mat2 local_strain = strain(D);
        if(M > 0.0)
        {
            range(i, -1,1) range(j, -1, 1)
            {
                if(!(i == 0 && j == 0))
                {
                    vec2 tpos = pos + vec2(i,j);
                    vec4 data = T(tpos);

                    vec2 X0 = DECODE(data.x) + tpos;
                    vec2 V0 = DECODE(data.y);
                    float M0 = data.z;
                    vec2 dx = X0 - X;
                    vec2 dv = V0 - V;
                    mat2 D0 = mat2(T1(tpos));
                    float weight = GS(1.2*dx);
                   
                    //F += M0*strain((D0*M + D*M0)/(M+M0))*dx*weight;
                    mat2 strain0 = 0.5*(strain(D0) + local_strain) + mat2(0.6*dot(dx,dv));
                    F += M0*strain0*dx*weight;
                   
                    b += weight;
                }
            }
       
            F /= b;
 			F = clamp(F, -0.1,0.1);
        }
        if(iMouse.z > 0.)
        {
            vec2 dx= pos - iMouse.xy;
            F += 0.02*normalize2(dx)*GS(dx/80.);
        }
        
       	//gravity
        F += 0.001*vec2(0,-1);
        
        //integrate velocity
        V += F*dt;
        //X +=  0.*F*dt;
        
        vec3 BORD = bN(X);
        V += 0.1*smoothstep(0., 5., -BORD.z)*BORD.xy;
        V *= 1. - 0.5*smoothstep(-30., 0., -pos.y);
        
        //velocity limit
        float v = length(V);
        V /= (v > 1.)?1.*v:1.;
    }
    
    //save
    X = X - pos;
    U = vec4(ENCODE(X), ENCODE(V), data.z, ENCODE(C));
}

