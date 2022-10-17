

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 hash22(vec2 p)
{//https://www.shadertoy.com/view/4djSRW
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy)*2.-1.;

}
vec2 vf (vec2 v) {
	return texture(iChannel0, v).xy-vec2(0.1,0);
}
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
float ff (vec2 U, vec2 o) {
    float q = 0.25*iResolution.x;
	vec2 V = floor(U*q+0.5 + o)/q;
    V += 0.1*hash22(floor(V*iResolution.xy))/q;
    
    vec2 v;
    v = vf(V);
    float a = 1e3;
    
    for (int i = 0; i < 3; i++) {
        v = 0.5*vf(V);
        a = min(a,float(1+i)*ln(U, V, V+v));
        V += v;
    }
    
    return max(1.-iResolution.x*0.4*a,0.);
}
void mainImage( out vec4 C, in vec2 U )
{
    U = U/iResolution.xy;
    
    float c = 0.;
    for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
        c += 0.3*ff(U,vec2(x,y));
    }
    }
    
    
    vec4 g = texture(iChannel0,U);
   	C.xyz = vec3(c);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1 || U.x < 3.) {
        
        Co = vec4(0.1,0,0,0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        float to = 0.;
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.04,0.04,1,0.01));
        me.w += 0.9*(100.*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        if (length(U-vec2(0.2,0.5)*ur)<10.||length(iMouse.xy-U)<5.) me.xyw *= 0.;
        Co = me;
        Co.xyz = clamp(Co.xyz, -40., 40.);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1 || U.x < 3.) {
        
        Co = vec4(0.1,0,0,0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        float to = 0.;
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.04,0.04,1,0.01));
        me.w += 0.9*(100.*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        if (length(U-vec2(0.2,0.5)*ur)<10.||length(iMouse.xy-U)<5.) me.xyw *= 0.;
        Co = me;
        Co.xyz = clamp(Co.xyz, -40., 40.);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1 || U.x < 3.) {
        
        Co = vec4(0.1,0,0,0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        float to = 0.;
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.04,0.04,1,0.01));
        me.w += 0.9*(100.*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        if (length(U-vec2(0.2,0.5)*ur)<10.||length(iMouse.xy-U)<5.) me.xyw *= 0.;
        Co = me;
        Co.xyz = clamp(Co.xyz, -40., 40.);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1 || U.x < 3.) {
        
        Co = vec4(0.1,0,0,0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        float to = 0.;
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.04,0.04,1,0.01));
        me.w += 0.9*(100.*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        if (length(U-vec2(0.2,0.5)*ur)<10.||length(iMouse.xy-U)<5.) me.xyw *= 0.;
        Co = me;
        Co.xyz = clamp(Co.xyz, -40., 40.);
    }
}