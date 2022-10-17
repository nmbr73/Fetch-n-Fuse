

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
    v = texture(iChannel0, v).xy;
	return v;
}
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
float ff (vec2 U, vec2 o) {
    float q = 0.3*iResolution.x;
	vec2 V = floor(U*q+0.5 + o)/q;
    V += 0.5*hash22(floor(V*iResolution.xy))/q;
    
    vec2 v;
    v = vf(V);
    float a = 1e3;
    
    for (int i = 0; i < 4; i++) {
        v = 0.5*vf(V );
        a = min(a,1.2*ln(U, V, V+v));
        V += v;
    }
    
    return max(1.-iResolution.x*0.4*a,0.);
}
void mainImage( out vec4 C, in vec2 U )
{
    
    float c = 0.;
    for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
        c += 0.33*ff(U/iResolution.xy,vec2(x,y));
    }
    }
    
    
    
    vec4 me = texture(iChannel0,U/iResolution.xy);
   	C.xyz = vec3(c)*(.3+5.*me.w);
    C.y *= length(me.xy)*10.;
    C.x *= max(0.,0.2+me.z);
    C.z *= max(0.,0.2-me.z);
    C*=vec4(1,1.5,2,1)*(1.+0.5*C*C);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec2 hash23(vec3 p3)
{//https://www.shadertoy.com/view/4djSRW
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}
vec2 ur, U;
vec4 t (vec2 v, int a, int b) {return texture(iChannel2,((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel2,(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1) {
        Co = vec4(0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 6; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        
        if (U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=-1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=1.;}
        if (U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=-1.;}
        else if (U.x<1.||ur.x-U.x<1.||ur.y-U.y<1.||U.y<1.) me.xy*=0.;
        
        
        
        float o = 0., m=10.;
        vec2 y = U/iResolution.xy*m;
        y = fract(y)*2.-1.+hash23(vec3(floor(y),iFrame))*2.-1.;
       	me.w = me.w*.99 + 2.*(1.+clamp(-0.2*me.z*(me.z)*me.z,0.,2.))*smoothstep(0.004,0.,length(y)/m);
        
        
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
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
    if (iFrame < 1) {
        Co = vec4(0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 6; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        
        if (U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=-1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=1.;}
        if (U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=-1.;}
        else if (U.x<1.||ur.x-U.x<1.||ur.y-U.y<1.||U.y<1.) me.xy*=0.;
        
        
        
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
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
    if (iFrame < 1) {
        Co = vec4(0);
    } else {
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        for (int i = 0; i < 3; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        vec4 me = t(v);
        for (int i = 0; i < 6; i++) {
            vec2 tmp = t(v).xy;
            v -= tmp;
        }
        me.zw = t(v).zw;
        for (int i = 0; i < 6; i++) {
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
        me = mix(me,ne,vec4(0.06,0.06,1,0.0));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.);
        vec4 pr = vec4(e.z,w.z,n.z,s.z);
        me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        vec4 mouse = texture(iChannel1,vec2(0.5));
        float q = ln(U,mouse.xy,mouse.zw);
        vec2 mo = mouse.xy-mouse.zw;
        float l = length(mo);
        if (l>0.) {
            mo = normalize(mo)*min(l,.1);
        	me += exp(-q*q)*vec4(normalize(mo.xy),0,0);
        }
        
        
        if (U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=-1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.7*ur.y)<2.) {me.x=.1;me.z=1.;}
        if (U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=1.;}
        if (ur.x-U.x<2.&&abs(U.y-0.3*ur.y)<2.) {me.x=-.1;me.z=-1.;}
        else if (U.x<1.||ur.x-U.x<1.||ur.y-U.y<1.||U.y<1.) me.xy*=0.;
        
        
        
        
        Co = me;
        Co.xyz = clamp(Co.xyz, -vec3(.5,.5,40.), vec3(.5,.5,40.));
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// MOUSE
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 p = texture(iChannel0,fragCoord/iResolution.xy);
    if (iMouse.z>0.) {
        if (p.z>0.) fragColor =  vec4(iMouse.xy,p.xy);
    	else fragColor =  vec4(iMouse.xy,iMouse.xy);
    }
    else fragColor = vec4(-iResolution.xy,-iResolution.xy);
}