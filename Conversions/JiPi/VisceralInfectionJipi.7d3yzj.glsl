

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
    Based on:
	Desert Canyon by shane
	https://www.shadertoy.com/view/Xs33Df
    
    I removed shane's comments to avoid passing them as my own. (I have no idea what I'm doing). 
    Please refer to shane's shader for extensive comments.
*/


#define FAR 125.

#define RM_STEPS 128

const float freqA = .15/3.75;
const float freqB = .25/2.75;
const float ampA = 20.;
const float ampB = 2.;

mat2 rot2( float th ){ vec2 a = sin(vec2(1.5707963, 0) + th); return mat2(a, -a.y, a.x); }

float hash(float n){ return fract(cos(n)*45758.5453); }
float hash(vec3 p){ return fract(sin(dot(p, vec3(7, 157, 113)))*45758.5453); }

float getGrey(vec3 p){ return dot(p, vec3(0.299, 0.587, 0.114)); }

float sminP(float a, float b , float s){
    
    float h = clamp(.5 + .5*(b - a)/s, 0., 1.);
    return mix(b, a, h) - h*(1.0-h)*s;
}

float smaxP(float a, float b, float s){
    
    float h = clamp(.5 + .5*(a - b)/s, 0., 1.);
    return mix(b, a, h) + h*(1. - h)*s;
}

vec2 path(in float z){ 

    return vec2(ampA*sin(z * freqA), ampB*cos(z * freqB) + 5.*(sin(z*0.025)  - 5.)); 
}

float map(in vec3 p){
    
    float tx = (-texture(iChannel0, p.xz/22. + p.xy/80.).x-0.4) * 0.95;
    
    vec3 q = sin(p.zyx*.2 - (0.5+tx)*0.5 + iTime*0.13)*sminP(5.0, pow((0.5-tx)*5.5, 0.22), 3.);
    
    float h = q.x*q.y*q.z;
  
    p.xy -= path(p.z);
    
    float tnl = 2.0 - length(p.xy*vec2(.33, .66))*0.5 + h * 0.4 + (1. - tx)*.25;
    
    return tnl - tx*.5 + tnl*0.3; 
}

float getprot(in vec3 p){
    
    float tx = (-texture(iChannel0, p.xz/22. + p.xy/80.).x-0.4) * 0.95;
    
    vec3 q = sin(p.zyx*.2 - (0.5+tx)*0.5 + iTime*0.13)*sminP(5.0, pow((0.5-tx)*5.5, 0.22), 3.);
    
    return q.x*q.y*q.z;// + (1. - tx)*.25- tx*.5; 
}

// Log-Bisection Tracing
// https://www.shadertoy.com/view/4sSXzD
//
// Log-Bisection Tracing by nimitz (twitter: @stormoid)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Contact: nmz@Stormoid.com

float logBisectTrace(in vec3 ro, in vec3 rd){

    float t = 0., told = 0., mid, dn;
    float d = map(rd*t + ro);
    float sgn = sign(d);

    for (int i=0; i<RM_STEPS; i++){

        if (sign(d) != sgn || d < 0.001 || t > FAR) break;
 
        told = t;
           
        t += step(d, 1.)*(log(abs(d) + 1.1) - d) + d;
        
        d = map(rd*t + ro);
    }
    
    if (sign(d) != sgn){

        dn = sign(map(rd*told + ro));
        
        vec2 iv = vec2(told, t); 

        for (int ii=0; ii<8; ii++){ 
            mid = dot(iv, vec2(.5));
            float d = map(rd*mid + ro);
            if (abs(d) < 0.001)break;

            iv = mix(vec2(iv.x, mid), vec2(mid, iv.y), step(0.0, d*dn));
        }

        t = mid; 
    }
    
    return min(t, FAR);
}

vec3 normal(in vec3 p)
{  
    vec2 e = vec2(-1, 1)*.001;   
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}

vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
   
    n = max(n*n, .001);
    n /= (n.x + n.y + n.z );  
    
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

vec3 doBumpMap( sampler2D tex, in vec3 p, in vec3 nor, float bumpfactor){
   
    const float eps = .001;
    vec3 grad = vec3( getGrey(tex3D(tex, vec3(p.x - eps, p.y, p.z), nor)),
                      getGrey(tex3D(tex, vec3(p.x, p.y - eps, p.z), nor)),
                      getGrey(tex3D(tex, vec3(p.x, p.y, p.z - eps), nor)));
    
    grad = (grad - getGrey(tex3D(tex, p, nor)))/eps; 
            
    grad -= nor*dot(nor, grad);          
                      
    return normalize(nor + grad*bumpfactor);
}

float calculateAO( in vec3 p, in vec3 n, float maxDist )
{
	float ao = 0., l;
	const float nbIte = 6.;

    for(float i=1.; i< nbIte+.5; i++){
    
        l = (i + hash(i))*.5/nbIte*maxDist;
        
        ao += (l - map( p + n*l))/(1. + l);
    }
	
    return clamp(1. - ao/nbIte, 0., 1.);
}

float curve(in vec3 p){

    const float eps = .05, amp = 4., ampInit = .5;

    vec2 e = vec2(-1, 1)*eps; 
    
    float t1 = map(p + e.yxx), t2 = map(p + e.xxy);
    float t3 = map(p + e.xyx), t4 = map(p + e.yyy);
    
    return clamp((t1 + t2 + t3 + t4 - 4.*map(p))*amp + ampInit, 0., 1.);
}


void mainImage(out vec4 fragColor, in vec2 fragCoord){

	vec2 u = (fragCoord - iResolution.xy*.5)/iResolution.y;
	
	vec3 lookAt = vec3(0, 0, 18.+iTime*1.9);
	vec3 ro = lookAt + vec3(0, .0, -.25);

	lookAt.xy += path(lookAt.z);
	ro.xy += path(ro.z);

    float FOV = 3.14159/1.5; 
    vec3 forward = normalize(lookAt - ro);
    vec3 right = normalize(vec3(forward.z, 0, -forward.x )); 
    vec3 up = cross(forward, right);

    vec3 rd = normalize(forward + FOV*u.x*right + FOV*u.y*up);
    
	rd.xy = rot2( path(lookAt.z).x/64. )*rd.xy;
    
    vec3 lp = vec3(FAR*.5, FAR, FAR) + vec3(0, 0, ro.z);

	float t = logBisectTrace(ro, rd);
    
    vec3 fog = vec3(1, .9, .7);
    //vec3 fog = vec3(1, .3, .3);

    vec3 col = fog;
    
    if (t < FAR){
    
        vec3 sp = ro+t*rd;
        vec3 sn = normal(sp);

        vec3 ld = lp-sp;
        ld /= max(length(ld), 0.001); 
    
        const float tSize1 = 1./6.;

        sn = doBumpMap(iChannel1, sp*tSize1, sn, .007/(1. + t/FAR));
        
        float shd = 0.9;
        float curv = curve(sp)*.7 +.1;
        float ao = calculateAO(sp, sn, 10.);
        
        float dif = max( dot( ld, sn ), 0.); 
        float spe = pow(max( dot( reflect(-ld, sn), -rd ), 0. ), 5.)*2.0; 
        float fre = clamp(1.0 + dot(rd, sn), 0., 1.);

		float Schlick = pow( 1. - max(dot(rd, normalize(rd + ld)), 0.), 1.5);
		float fre2 = mix(.2, 1., Schlick);
       
        float amb = fre*fre2 + .76*ao;
        
        col = clamp(mix(vec3(.5, .4, .3), vec3(.25, .45, .125),(sp.y+1.)*.15), vec3(.25, .45, .125), vec3(1));
        
        curv = smoothstep(0., .7, curv);
        col *= vec3(curv*0.9, curv*.45, curv*.25)*1.0;
        
        float prot = smoothstep(-0.0, -3.0, getprot(sp));
        
        col = mix(col, vec3(0.9, 0.6, 0.4), prot);
        
        col = (col*(dif + .1) + fre2*spe)*shd*ao + amb*col; 
        
    }
    
    col = mix(col, fog, sqrt(smoothstep(FAR - 85., FAR, t)));
    
    col = pow(max(col, 0.), vec3(.85));

    u = fragCoord/iResolution.xy;
    col *= pow(16.*u.x*u.y*(1. - u.x)*(1. - u.y) , .0625);

	fragColor = vec4(clamp(col, 0., 1.), 1);
}