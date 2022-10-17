

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
    vec4 U = decode(texel(ch0, pos).zw);
    vec4 P = textureLod(ch1, pos/R, 0.);
    vec4 D = pixel(ch2, pos);
    float ang = atan(D.w, D.z);
    float mag = length(D.zw);
    vec3 bord = smoothstep(border_h-1.,border_h-3.,border(pos))*vec3(1.);
	vec3 rho = vec3(1.,1.7,4.)*(0.*abs(U.z)+5.*smoothstep(0.1, 0.25, P.w));
    // Output to screen
    fragColor = vec4(sqrt(rho)*(0.15+hsv2rgb(vec3(ang, 1., mag))) + bord,0);
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


float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}


float border(vec2 p)
{
    float bound = -sdBox(p - R*0.5, R*0.5); 
    float box = sdBox(Rot(0.5*time)*(p - R*0.5), R*vec2(0.1, 0.01));
    float drain = -sdBox(p - R*vec2(0.9, 0.05), vec2(50));
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
    uint m = floatBitsToUint(x.y);
    return vec4(unpack(v),unpack(m)*128.); 
}

vec2 encode(vec4 x)
{
    uint v = pack(x.xy);
    uint m = pack(x.zw/128.);
    return vec2(uintBitsToFloat(v),uintBitsToFloat(m)); 
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
    return 20.*dx*exp(-dot(dx,dx));
}

void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    ivec2 p = ivec2(pos);
        
    //particle position
    vec2 x = vec2(0.);
    //particle velocity, mass and grid distributed density
    vec4 vm = vec4(0.); 
    vec2 F = vec2(0., -0.0004);
    vec2 dF = vec2(0.);
    
    //reintegration advection
    //basically sum over all updated neighbors 
    //that fall inside of this pixel
    //this makes the tracking conservative
    range(i, -2, 2) range(j, -2, 2)
    {
        vec4 data = texel(ch0, p + ivec2(i,j));
        vec4 vm0 = decode(data.zw);
        vec2 x0 = data.xy; //update position
        //how much mass falls into this pixel(is exact, 0 or full)
        vm.w += vm0.z*G((pos - x0)/1.5);
        float D = step(max(abs(pos.x - x0.x),
                           abs(pos.y - x0.y)), 0.5);
        float D1 = 0.25*step(max(abs(pos.x - x0.x),
                          		 abs(pos.y - x0.y)), 1.00);
        
        float pvm = vm0.z;
        vm0.z *= (pvm<4.*mass)?D:D1;
        if(pvm>=4.*mass && pvm > 0.) 
        {
            x0 = mix(x0, pos, 0.5);
        }
           
        //add weighted positions by mass
        x += x0*vm0.z;
        //add weighted velocities by mass
        vm.xy += vm0.xy*vm0.z;
        //add mass
        vm.z += vm0.z;
        dF += vm0.z*Force(pos - x0)*(1. - D);
    }
    vm.z = clamp(vm.z, 0., 2.);
    
    if(vm.z != 0. || any(isnan(vm))) //not vacuum
    {
        //normalize
        x /= vm.z;
        vm.xy /= vm.z;
        
        vec3 dx = vec3(-1.,0,1.);
        vec2 px = x;
        vec3 rand = hash32(pos+x)-0.5;
        
    	//update velocity
         //border 
    	vec3 N = bN(px);
        float vdotN = step(abs(N.z), border_h)*dot(N.xy, vm.xy);
        vm.xy = vm.xy - 0.5*(N.xy*vdotN + N.xy*abs(vdotN));
        F += N.xy*step(abs(N.z), border_h)/N.z;
        
        //global force field
        
        vec4 GF = pixel(ch1, px);

        F += (-GF.xy + (GF.zw - vm.xy)*0.15 - 0.01*vm.xy*step(N.z, border_h + 5.)); 
        if(iMouse.z > 0.)
        {
            vec2 dm =(iMouse.xy - iMouse.zw)/10.; 
            float d = distance(iMouse.xy, x)/20.;
            F += 0.01*dm*exp(-d*d);
        }
   	    vm.xy += 0.4*F*dt/(0.0001+vm.z);
        //velocity limit
        float v = length(vm.xy);
        vm.xy /= (v > 1.)?v:1.;
        x = x + (vm.xy + dF)*dt; 
    }
    else
    {
        x = pos;
        vm.xyz = vec3(0.);
        if(distance(pos, R*vec2(0.55, 0.75)) < 1. || distance(pos, R*vec2(0.45, 0.75)) < 1.)
        {
            vm.xyz = vec3(0., -0.52, 3.*mass);
        }
        if(distance(pos, R*vec2(0.1, 0.75)) < 5.)
        {
            vm.xyz = vec3(0.5, -0.5, 3.*mass);
        }
    }
   
    
    //initial condition
    if(iFrame < 1)
    {
        //random
        vec3 rand = hash32(pos);
        if(rand.z < 0.05) 
        {
            x = pos;
            vm = vec4(0.3*vec2(2.*rand.xy - 1.), 4.*mass, mass);
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
#define Radius 3
void mainImage( out vec4 U, in vec2 pos )
{
    R = iResolution.xy; time = iTime;
    vec4 avm = vec4(0.);
    float sum = 1.;
    range(i, -Radius, Radius)
    {
        ivec2 p = ivec2(pos) + ivec2(i,0);
        if(p.x >= 0 && p.x < int(R.x))
        {
            float k = G(vec2(i,0)*1.5/float(Radius));
            vec4 d = decode(texel(ch0, p).zw);
        	avm += vec4(d.xy*d.z, d.zw)*k;
        	sum += k;
        }
    }
    U = avm/sum;
    U.xy = avm.xy/(avm.z+0.0001); 
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define Radius 3
void mainImage( out vec4 U, in vec2 pos )
{
    vec4 avm = vec4(0.);
    float sum = 1.;
    range(i, -Radius, Radius)
    {
        ivec2 p = ivec2(pos) + ivec2(0,i);
        float k = G(vec2(0,i)*1.5/float(Radius));
        vec4 d = texel(ch0, p);
        avm += vec4(d.xy*d.z, d.zw)*k;
        sum += k;
    }
    U = avm/sum;
    U.z = 0.5*U.z*clamp(pow(abs(U.z/0.07), 7.) - 1., -1., 1.); //water
    //U.z = 1.2*U.z;//gas
    U.xy = avm.xy/(avm.z+0.0001); 
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//force field

void mainImage( out vec4 U, in vec2 pos )
{
   vec3 dx = vec3(-1., 0., 1.);
   U.xy = 0.5*vec2(texel(ch0, pos + dx.zy).z - texel(ch0, pos + dx.xy).z,
                   texel(ch0, pos + dx.yz).z - texel(ch0, pos + dx.yx).z);
   //average velocity
   vec4 a = texel(ch0, pos); 
   U.zw = a.xy;
}