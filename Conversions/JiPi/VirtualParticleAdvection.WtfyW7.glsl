

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing	

	return c.z * mix( vec3(1.0), rgb, c.y);
}

void mainImage( out vec4 fragColor, in vec2 pos )
{
	R = iResolution.xy; time = iTime;
    //pos = R*0.5 + pos*0.1;
    ivec2 p = ivec2(pos);
    
    //cur particle
    vec4 U = decode(texel(ch0, pos).zw);
    
    //pressure
    vec4 P = textureLod(ch1, pos/R, 0.);
    
    //border render
    vec3 bord = smoothstep(border_h-1.,border_h-3.,border(pos))*vec3(1.);
    
    //particle render
    float rho = 0.;
    range(i, -1, 1) range(j, -1, 1)
    {
        vec4 data = texel(ch0, p + ivec2(i,j));
        vec4 vm0 = decode(data.zw);
        vec2 x0 = data.xy; //update position
        //how much mass falls into this pixel
        rho += 1.*vm0.z*G((pos - x0)/0.5);
    }
    
    // Output to screen
    fragColor = vec4(tanh(4.*vec3(1.,2.,3.)*rho) + bord + 0.*abs(P.x),0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define Bf(p) p
#define Bi(p) ivec2(p)
#define texel(a, p) texelFetch(a, Bi(p), 0)
#define pixel(a, p) texture(a, (p)/R)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3

#define PI 3.14159265

#define loop(i,x) for(int i = 0; i < x; i++)
#define range(i,a,b) for(int i = a; i <= b; i++)

#define dt 1.

#define border_h 5.
vec2 R;
float time;

mat2 Rot(float ang)
{
    return mat2(cos(ang), -sin(ang), sin(ang), cos(ang)); 
}

vec2 Dir(float ang)
{
    return vec2(cos(ang), sin(ang));
}


float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdSph( in vec2 p, in float r )
{
    return length(p) - r; 
}


float border(vec2 p)
{
    float bound = -sdBox(p - R*0.5, R*vec2(0.52, 0.5)); 
    
    float box = sdSph(p - R*vec2(0.2, 0.3), R.x*0.05);
    box =min(box, sdSph(p - R*vec2(0.2, 0.7), R.x*0.05));
    float drain = -sdBox(p - R*vec2(0.9, 0.05), vec2(0));
    return max(drain,min(bound, box));
}

#define h 0.1
vec3 bN(vec2 p)
{
    vec3 dx = vec3(-h,0,h);
    vec4 idx = vec4(-1./h, 0., 1./h, 0.25);
    vec3 r = idx.zyw*border(p + dx.zy)
           + idx.xyw*border(p + dx.xy)
           + idx.yzw*border(p + dx.yz)
           + idx.yxw*border(p + dx.yx);
    return vec3(normalize(r.xy), r.z);
}

uint pack(vec2 x)
{
    x = 65534.0*clamp(0.5*x+0.5, 0., 1.);
    return uint(x.x) + 65535u*uint(x.y);
}

vec2 unpack(uint a)
{
    vec2 x = vec2(a%65535u, a/65535u);
    return clamp(x/65534.0, 0.,1.)*2.0 - 1.0;
}

vec4 decode(vec2 x)
{
    uint v = floatBitsToUint(x.x);
    //uint m = floatBitsToUint(x.y);
    return vec4(unpack(v),x.yy); 
}

vec2 encode(vec4 x)
{
    uint v = pack(x.xy);
    //uint m = pack(x.zw/1.);
    return vec2(uintBitsToFloat(v),x.z); 
}

vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

float G(vec2 x)
{
    return exp(-dot(x,x));
}

float G0(vec2 x)
{
    return exp(-length(x));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define mass 0.1
#define div 0.7

vec2 Force(vec2 dx)
{
    return 0.*dx*exp(-dot(dx,dx));
}

vec2 P(vec2 p)
{
    return pixel(ch1, p).zw;
}

//diffusion amount
#define dif 0.93
vec3 distribution(vec2 x, vec2 p)
{
    vec4 aabb0 = vec4(p - 0.5, p + 0.5);
    vec4 aabb1 = vec4(x - dif*0.5, x + dif*0.5);
    vec4 aabbX = vec4(max(aabb0.xy, aabb1.xy), min(aabb0.zw, aabb1.zw));
    vec2 center = 0.5*(aabbX.xy + aabbX.zw); //center of mass
    vec2 size = max(aabbX.zw - aabbX.xy, 0.); //only positive
    float m = size.x*size.y/(dif*dif); //relative amount
    //if any of the dimensions are 0 then the mass is 0
    return vec3(center, m);
}

void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);
        
    //particle velocity, mass and grid distributed density
    vec4 vm = vec4(0.);
    vec2 F = vec2(0., -0.00);
    vec2 dF = vec2(0.);
    
    //particle position
    vec2 x = pos*vm.z;

    //reintegration advection
    //basically sum over all updated neighbors 
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        vec4 data = texel(ch0, p + ivec2(i,j));
        vec4 vm0 = decode(data.zw);
       
        vec2 vv = vm0.xy;
        vec2 xx = data.xy + vv*dt; //integrate position

        vec3 D = distribution(xx, pos);

        //the deposited mass into this cell
        float m = vm0.z*D.z;
        //local center of mass in this cell
        xx = D.xy; 

        //add weighted positions by mass
        x += xx*m;
        //add weighted velocities by mass
        vm.xy += vv*m;
        //add mass
        vm.z += m;
    }
    
    if(vm.z != 0.)
    {
        //normalize
        x /= vm.z;
        vm.xy /= vm.z;

        //update velocity
        //border 
        vec3 N = bN(x);
        N.z += 0.0001;
        
        if(N.z < 0.) vm.z*=0.99;

        float vdotN = step(abs(N.z), border_h)*dot(N.xy, vm.xy);
        vm.xy = vm.xy - 1.*(N.xy*vdotN + N.xy*abs(vdotN));
        F += N.xy*step(abs(N.z), border_h)/N.z;

         vec3 dx = vec3(-1., 0., 1.) + 1.;
        //global force field
        vec2 pressure = P(x);

        F += 0.4*pressure - 0.01*vm.xy*step(N.z, border_h + 5.);
        if(iMouse.z > 0.)
        {
            vec2 dm =(iMouse.xy - iMouse.zw)/10.; 
            float d = distance(iMouse.xy, x)/20.;
            F += 0.01*dm*exp(-d*d);
        }
        vm.xy += 0.4*F*dt;
        
        //velocity limit
        float v = length(vm.xy);
        vm.xy /= (v > 1.)?v:1.;
    }

    if(pos.x < 1.)
    {
        x = pos;
        vm.xyz = vec3(0.6, 0., 0.5*mass);
    }
    
    //initial condition
    if(iFrame < 1)
    {
        //random
        vec3 rand = hash32(pos);
        if(rand.z < 0.1) 
        {
            x = pos;
            vm = vec4(0.5*(rand.xy-0.5) + vec2(0.5, 0.), 4.*mass, mass);
        }
        else
        {
            x = pos;
        	vm = vec4(0.);
        }
    }
    
    U = vec4(x, encode(vm));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//velocity blur


void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    
	U = texel(ch1, pos);
    vec4 av = vec4(0.); float s = 0.0001;
    range(i, -3, 3) range(j, -3, 3)
    {
        vec2 dx = vec2(i,j);
        vec4 dc = decode(texel(ch0, pos + dx).zw);
        float k = dc.z*G(dx/1.);
        s += k;
        av += k*dc.xyzz;
    }
    U = av/s; 
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//pressure solve

vec2 V(vec2 p)
{
    vec4 d =texel(ch0, p); 
    return d.xy;
}

float P(vec2 p)
{
    return texel(ch1, p).x;
}

void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    
    float b = border(pos);
      
    
    vec3 dx = vec3(-1., 0., 1.);
    //velocity divergence
    float div = 0.5*(V(pos + dx.zy).x - V(pos + dx.xy).x +
                     V(pos + dx.yz).y - V(pos + dx.yx).y);
    //neighbor average
    float L = 0.25*(P(pos + dx.zy) + P(pos + dx.xy) +
                    P(pos + dx.yz) + P(pos + dx.yx));
    U.x = 0.995*L + div;
  
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//pressure solve + gradient

vec2 V(vec2 p)
{
    vec4 d =texel(ch0, p); 
    return d.xy;
}

float sqr(float x)
{
	return x*x;
}

float P(vec2 p)
{
    return -0.08*texel(ch0, p).z+ texel(ch1, p).x;
}

void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    
    float b = border(pos);
      
    if(b > 0. || true) 
    {
        vec3 dx = vec3(-1., 0., 1.);
        //velocity divergence
        float div = 0.5*(V(pos + dx.zy).x - V(pos + dx.xy).x +
                         V(pos + dx.yz).y - V(pos + dx.yx).y);
        //neighbor average
        float L = 0.25*(P(pos + dx.zy) + P(pos + dx.xy) +
                       P(pos + dx.yz) + P(pos + dx.yx));
        U.x = 0.995*L + div;
    }  
    
    
    vec3 dx = vec3(-1., 0., 1.);
    //global force field
    vec2 pressure = 0.5*vec2(P(pos + dx.zy) - P(pos + dx.xy),
                             P(pos + dx.yz) - P(pos + dx.yx));
    U.zw = pressure;
}