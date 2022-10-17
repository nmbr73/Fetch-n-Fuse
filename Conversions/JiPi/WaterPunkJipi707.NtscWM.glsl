

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

#define MAX_STEPS 40
#define MAX_DIST 40.
#define SURF_DIST .005
#define samples 32
#define LOD 2

float hash(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 184.5453);
}

float noise(vec2 n) {
    const vec2 d = vec2(0.0, 1.0);
    vec2 b = floor(n), f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
    return mix(mix(hash(b), hash(b + d.yx), f.x), mix(hash(b + d.xy), hash(b + d.yy), f.x), f.y);
}

float sdSphere( vec3 p, float s ) { return length(p)-s; }


float smin( float a, float b, float k ) {
    float h = max(k-abs(a-b), 0.0);
    return min(a, b) - h*h*0.25/k; 
}


float getDist(vec3 p) {
    float matId;
    float final = MAX_DIST;
    float iTime = iTime; 
    p = p - vec3(0.,0.5, 5.);
    p.x = abs(p.x);
    vec3 tempP = p;
    for (int i = 0; i < 10; i++) {
        float fi = float(i + 1)  + floor(float(i) / 5.);
        vec3 pos = p;
        float xmov = -dot(p.xy, tempP.xy + tempP.xy * fi * 0.8) * 3.;
        float ymov = sign(mod(fi, 2.)) - dot(tempP.xy, tempP.xy) * 0.2 - xmov * 0.2;

        vec2 xy = vec2(xmov, ymov);
        
        pos.xy += xy * 0.2;
        pos.xy -= noise(pos.xy * 15. / fi) * 0.3;
        pos.xy += (vec2(sin(iTime + fi) * 2., cos(iTime / 2. - fi) * 0.5) * 0.1 * fi);  

        pos.z += sin(iTime * cos(float(i * 4))) * 0.5;
        float r = sin(fi) * 0.2;
        float n = min(sin(pos.z * float(i) * 5.), cos(pos.x * pos.y * float(i) * 10.)) * 0.1;
        float bubble = sdSphere(pos + vec3(n) * 0.1 - vec3(0.05), r);
        final = smin(final, bubble, 0.3 + final * 0.04);
        
        tempP = pos;
    }

    
    return final;
}

float rayMarch(vec3 ro, vec3 rd) {
	float dO=0.;
    float matId = -1.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float res = getDist(p);
        float dS = res;
        dO += dS;
        
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

vec3 normals(vec3 p, float of ) {
	float d = getDist(p);
    vec2 e = vec2(of, 0);
    
    vec3 n = d - vec3(
        getDist(p-e.xyy),
        getDist(p-e.yxy),
        getDist(p-e.yyx));
    
    return normalize(n);
}

float diffuse(vec3 p, vec3 n, vec3 lp) {
    vec3 l = normalize(lp-p);
    float dif = clamp(dot(n, l), 0., 1.);

    return dif;
}

float specular(vec3 rd, vec3 ld, vec3 n) {    
    vec3 reflection = reflect(-ld, n);
    float spec = max(dot(reflection, -normalize(rd)), 0.); 
    return spec;
}

float gaussian(vec2 i) {
const float sigma = float(samples) * .25;
    return exp( -.5* dot(i/=sigma,i) ) / ( 6.28 * sigma*sigma );
}

vec4 blur(sampler2D sp, vec2 U, vec2 scale) {
    const int  sLOD = 1 << LOD;
    vec4 O = vec4(0);
    int s = samples/sLOD;
    
    for ( int i = 0; i < s*s; i++ ) {
        vec2 d = vec2(i%s, i/s)*float(sLOD) - float(samples)/2.;
        O += gaussian(d) * texture(sp, U + scale * d);
    }
    
    return O / O.a;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    vec3 col = vec3(0.0);
    
    float iTime = iTime * 2.;

	vec3 ro = vec3(.0, 0., 1.);
    vec3 rd = normalize(vec3(uv.x, uv.y + 0.2 , 2.));
    vec3 ld =  vec3(0., 0., 1.);
    float d = rayMarch(ro, rd);
    vec3 p = ro + rd * d;
    vec3 n = normals(p, 0.003);
    float dif = diffuse(p, n, ld); 
    float spec = specular(rd, ld, n) * 0.1;
    float fresnel = smoothstep(0.5, 0.2, dot(-rd, n));
    vec3 dispersion = vec3(noise(n.xy * 2.7), noise(n.xy * 3.), noise(n.xy * 3.3)) * 0.4; 
    
    vec2 camUV = fragCoord / iResolution.xy;
    vec3 cam1 =  texture(iChannel0, camUV).xyz * 0.9;
    camUV += n.xy * 0.05 * dif;
    vec3 cam2 = blur(iChannel0, camUV, vec2(0.002)).xyz * 0.9;

    col = dif * cam2;
    col += spec;
    col += cam2 * 0.15;         
    col += dispersion;
    col += fresnel * 0.2;
    
    if (d > MAX_DIST) { col = vec3(cam1);  }

    fragColor = vec4(col, 1.0);
}