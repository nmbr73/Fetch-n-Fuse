

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//CC0 1.0 Universal https://creativecommons.org/publicdomain/zero/1.0/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.

vec3 erot(vec3 p, vec3 ax, float ro) {
    return mix(dot(p,ax)*ax,p,cos(ro))+sin(ro)*cross(ax,p);
}

float WaveletNoise(vec3 p, float z, float k) {
    // https://www.shadertoy.com/view/wsBfzK
    float d=0.,s=1.,m=0., a;
    for(float i=0.; i<5.; i++) {
        vec3 q = p*s, g=fract(floor(q)*vec3(123.34,233.53,314.15));
    	g += dot(g, g+23.234);
		a = fract(g.x*g.y)*1e3 +z*(mod(g.x+g.y, 2.)-1.); // add vorticity
        q = (fract(q)-.5);
        //random rotation in 3d. the +.1 is to fix the rare case that g == vec3(0)
        //https://suricrasia.online/demoscene/functions/#rndrot
        q = erot(q, normalize(tan(g+.1)), a);
        d += sin(q.x*10.+z)*smoothstep(.25, .0, dot(q,q))/s;
        p = erot(p,normalize(vec3(-1,1,0)),atan(sqrt(2.)))+i; //rotate along the magic angle
        m += 1./s;
        s *= k; 
    }
    return d/m;
}

float super(vec3 p) {
    return sqrt(length(p*p));
}

float box(vec3 p, vec3 d) {
    vec3 q = abs(p)-d;
    return super(max(q,0.))+min(0.,max(q.x,max(q.y,q.z)));
}

vec3 distorted_p;
float scene(vec3 p) {
    //different noise for each dimension
    p.x += WaveletNoise(p/2., iTime*3., 1.15)*.3;
    p.y += WaveletNoise(p/2.+10., iTime*3., 1.15)*.3;
    p.z += WaveletNoise(p/2.+20., iTime*3., 1.15)*.3;
    distorted_p = p;
    return box(p,vec3(1))-.3;
}

vec3 norm(vec3 p) {
    mat3 k = mat3(p,p,p)-mat3(0.001);
    return normalize(scene(p) - vec3(scene(k[0]),scene(k[1]),scene(k[2])));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-0.5*iResolution.xy)/iResolution.y;
    vec2 mouse = (iMouse.xy-0.5*iResolution.xy)/iResolution.y;

    vec3 cam = normalize(vec3(1.2,uv));
    vec3 init = vec3(-7.,0,0);
    
    float yrot = 0.5;
    float zrot = iTime*.2;
    if (iMouse.z > 0.) {
        yrot += -4.*mouse.y;
        zrot = 4.*mouse.x;
    }
    cam = erot(cam, vec3(0,1,0), yrot);
    init = erot(init, vec3(0,1,0), yrot);
    cam = erot(cam, vec3(0,0,1), zrot);
    init = erot(init, vec3(0,0,1), zrot);
    
    vec3 p = init;
    bool hit = false;
    for (int i = 0; i < 250 && !hit; i++) {
        float dist = scene(p);
        hit = dist*dist < 1e-6;
        p+=dist*cam*.9;
        if (distance(p,init)>50.) break;
    }
    vec3 local_coords = distorted_p;
    vec3 n = norm(p);
    vec3 r = reflect(cam,n);
    float ss = smoothstep(-.05,.05,scene(p+vec3(.05)/sqrt(3.)));
    float tex = WaveletNoise(local_coords*3., 0., 1.5)+.5;
    float diff = mix(length(sin(n*2.)*0.5+0.5)/sqrt(3.),ss,.7)+.1;
    float spec = length(sin(r*4.)*0.5+0.5)/sqrt(3.);
    float specpow = mix(3.,10.,tex);
    float frens = 1.-pow(dot(cam,n),2.)*0.98;
    vec3 col = vec3(0.7,0.2,0.4)*diff + pow(spec,specpow)*frens;
    float bgdot = length(sin(cam*3.5)*0.4+0.6)/sqrt(3.);
    vec3 bg = vec3(.2,.2,.3) * bgdot + pow(bgdot, 10.)*2.;
    fragColor.xyz = hit ? col : bg;
    fragColor = sqrt(fragColor);
    fragColor *= 1.- dot(uv,uv)*.6;
}