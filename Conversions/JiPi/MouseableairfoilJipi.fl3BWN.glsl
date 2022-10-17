

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 C, in vec2 U )
{
    
    vec4 g = texture(iChannel0,U/iResolution.xy);
	C.xyz = vec3(g.w);
    
    U = U-vec2(0.4,0.5)*iResolution.xy;
    float an = -iMouse.x/iResolution.x,
        co = cos(an), si = sin(an);
    U.xy = mat2(co,-si,si,co)*U.xy;
    U.x*=0.125;
    U.y += (iMouse.y/iResolution.y)*U.x*U.x;
    if (length(U)<6.) C.xyz = vec3(sin(iTime)*0.5+0.5,0.5,1.);
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 hash23(vec3 p3)
{//https://www.shadertoy.com/view/4djSRW
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}
vec2 ur, U;
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1||U.x < 3.||ur.x-U.x < 3.) {
        Co = vec4(0.1,0,0,0);
     
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 9; i++) {
            A += t(A).xy;
            B += t(B).xy;
            C += t(C).xy;
            D += t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        float o = 0., m=20.;
        vec2 y = U/iResolution.xy*m;
        y = fract(y)*2.-1.+hash23(vec3(floor(y),iFrame))*2.-1.;
        me.w = me.w + 0.5*(1.+clamp(-0.2*me.z*(me.z)*me.z,0.,2.))*smoothstep(0.004,0.,length(y)/m);
        
        
    	U = U-vec2(0.4,0.5)*ur;
        float an = -iMouse.x/ur.x,
            co = cos(an), si = sin(an);
        U.xy = mat2(co,-si,si,co)*U.xy;
        U.x*=0.125;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        me.xyw *= step(6.,length(U));
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec2 ur, U;
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1||U.x < 3.||ur.x-U.x < 3.) {
        Co = vec4(0.1,0,0,0);
     
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 9; i++) {
            A += t(A).xy;
            B += t(B).xy;
            C += t(C).xy;
            D += t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
    	U = U-vec2(0.4,0.5)*ur;
        float an = -iMouse.x/ur.x,
            co = cos(an), si = sin(an);
        U.xy = mat2(co,-si,si,co)*U.xy;
        U.x*=0.125;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        me.xyw *= step(6.,length(U));
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 ur, U;
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1||U.x < 3.||ur.x-U.x < 3.) {
        Co = vec4(0.1,0,0,0);
     
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 9; i++) {
            A += t(A).xy;
            B += t(B).xy;
            C += t(C).xy;
            D += t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
    	U = U-vec2(0.4,0.5)*ur;
        float an = -iMouse.x/ur.x,
            co = cos(an), si = sin(an);
        U.xy = mat2(co,-si,si,co)*U.xy;
        U.x*=0.125;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        me.xyw *= step(6.,length(U));
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec2 ur, U;
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1||U.x < 3.||ur.x-U.x < 3.) {
        Co = vec4(0.1,0,0,0);
     
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 2; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 9; i++) {
            A += t(A).xy;
            B += t(B).xy;
            C += t(C).xy;
            D += t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
    	U = U-vec2(0.4,0.5)*ur;
        float an = -iMouse.x/ur.x,
            co = cos(an), si = sin(an);
        U.xy = mat2(co,-si,si,co)*U.xy;
        U.x*=0.125;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        me.xyw *= step(6.,length(U));
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}