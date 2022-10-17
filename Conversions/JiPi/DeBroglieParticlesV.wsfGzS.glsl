

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
vec4 L ( vec2 U ) {return texture(iChannel3,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
    R = iResolution.xy;
    vec4 
        a = T(U+vec2(1,0)),
        b = T(U-vec2(1,0)),
        c = T(U+vec2(0,1)),
        d = T(U-vec2(0,1)),
        e = P(U+vec2(1,0)),
        f = P(U-vec2(1,0)),
        g = P(U+vec2(0,1)),
        h = P(U-vec2(0,1)),
        i = P(U+vec2(1,0)),
        j = P(U-vec2(1,0)),
        k = P(U+vec2(0,1)),
        l = P(U-vec2(0,1)),
        p = P(U);
    float o = 0.;
    o += length(abs(p.xy)-abs(e.xy));
    o += length(abs(p.xy)-abs(f.xy));
    o += length(abs(p.xy)-abs(g.xy));
    o += length(abs(p.xy)-abs(h.xy));
    o += length(abs(p.xy)-abs(i.xy));
    o += length(abs(p.xy)-abs(j.xy));
    o += length(abs(p.xy)-abs(k.xy));
    o += length(abs(p.xy)-abs(l.xy));
    vec4 gr = vec4(a.zw-b.zw,c.zw-d.zw);
    vec2 dz = gr.xz;
    vec2 dw = 10.*gr.yw;
   	vec4 v = T(U-dz);
    C.xyz = 0.5-0.5*sin(0.005*o*vec3(2,1,0)+L(U).xyz-length(v.xy)*vec3(1.2,1.3,1.2)+v.w*vec3(1,2,3));
    vec3 n = normalize(vec3(dz+dw,.01));
    vec4 tx = vec4(.6,.9,1,1)*texture(iChannel2,reflect(vec3(0,0,1),n));
    C = (C*0.8+0.2)*(0.9+0.1*tx);

}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Fluid
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 P (vec2 U) { // access particle buffer
    return texture(iChannel1,U/R);}
vec4 t (vec2 U) { // access buffer
	return texture(iChannel0,U/R);
}
vec4 T (vec2 U) {
    // sample things where they were, not where they are
	U -= 0.5*t(U).xy;
	U -= 0.5*t(U).xy;
    return t(U);
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = T(U);
   vec4 // neighborhood
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0)),
       	mu = n+e+s+w;
   // xy : velocity, z : pressure, w : spin
   C.x = 0.25*(n.x+e.x+s.x+w.x+w.z-e.z+s.w*C.w-n.w*C.w);
   C.y = 0.25*(n.y+e.y+s.y+w.y+s.z-n.z+w.w*C.w-e.w*C.w);
   C.z = 0.25*(mu.z+s.y-n.y+w.x-e.x);
   C.w = 0.25*(n.x-s.x+w.y-e.y-mu.w);

   //particle interaction
    vec4 p = P(U);
	float r = smoothstep(5.,0.,length(U-abs(p.xy)));
 	C.zw =  mix(C.zw,vec2(-1,0.5*sign(p.x)),r);
 	C.xy = mix(C.xy*0.999,p.zw,r);
    
    // Boundary Conditions
 	if (iMouse.z > 0.) C.w = mix(C.w,4.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) C = vec4(0);
    if (U.x < 5.||U.y < 5.||R.x-U.x < 5.||R.y-U.y < 5.) C.xz*=0.25;
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) C.xzw*=0.;

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Fluid
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 P (vec2 U) { // access particle buffer
    return texture(iChannel1,U/R);}
vec4 t (vec2 U) { // access buffer
	return texture(iChannel0,U/R);
}
vec4 T (vec2 U) {
    // sample things where they were, not where they are
	U -= 0.5*t(U).xy;
	U -= 0.5*t(U).xy;
    return t(U);
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = T(U);
   vec4 // neighborhood
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0)),
       	mu = n+e+s+w;
   // xy : velocity, z : pressure, w : spin
   C.x = 0.25*(n.x+e.x+s.x+w.x+w.z-e.z+s.w*C.w-n.w*C.w);
   C.y = 0.25*(n.y+e.y+s.y+w.y+s.z-n.z+w.w*C.w-e.w*C.w);
   C.z = 0.25*(mu.z+s.y-n.y+w.x-e.x);
   C.w = 0.25*(n.x-s.x+w.y-e.y-mu.w);

   //particle interaction
    vec4 p = P(U);
	float r = smoothstep(5.,0.,length(U-abs(p.xy)));
 	C.zw =  mix(C.zw,vec2(-1,0.5*sign(p.x)),r);
 	C.xy = mix(C.xy*0.999,p.zw,r);
    
    // Boundary Conditions
 	if (iMouse.z > 0.) C.w = mix(C.w,4.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) C = vec4(0);
    if (U.x < 5.||U.y < 5.||R.x-U.x < 5.||R.y-U.y < 5.) C.xz*=0.25;
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) C.xzw*=0.;

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
    R = iResolution.xy;
    C = T(U);
    vec4 p = P(U);
    C = max(
        C * (1.-vec4(0.02,0.01,0.03,0.01)),
        4.*vec4(smoothstep(3.,0.,length(abs(p.xy)-U)))
        );
	if (iFrame < 1) C = vec4(0);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Voronoi based particle tracking
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}//sample fluid
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}//sample particles
void swap (vec2 U, inout vec4 Q, vec2 u) {
    vec4 p = P(U+u);
    float dl = length(U-abs(Q.xy)) - length(U-abs(p.xy));
    Q = mix(Q,p,float(dl>=0.));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	Q = P(U);
 	swap(U,Q,vec2(1,0));
 	swap(U,Q,vec2(0,1));
 	swap(U,Q,vec2(0,-1));
 	swap(U,Q,vec2(-1,0));
 	swap(U,Q,vec2(1,1));
 	swap(U,Q,vec2(-1,1));
 	swap(U,Q,vec2(1,-1));
 	swap(U,Q,vec2(-1,-1));
 	vec4 t = T(abs(Q.xy));
 	vec2 e = vec2(2,0);
 	float s = sign(Q.x);
 	Q.xy = abs(Q.xy);
 	vec4 
        a = T(Q.xy+e.xy),
        b = T(Q.xy+e.yx),
        c = T(Q.xy-e.xy),
        d = T(Q.xy-e.yx);
 	vec2 z = vec2(a.z-c.z,b.z-d.z);
 	vec2 w = vec2(a.w-c.w,b.w-d.w);
 	if (s > 0.) Q.zw += (-0.5*z+s*w.yx*vec2(-1,1));
 	Q.zw = clamp(Q.zw,-.8,.8);
 	 Q.xy+=Q.zw;
 	Q.xy *= s;
 	if (iFrame < 1 || (iMouse.z > 0.&& length(iMouse.xy)<40.)){
     	Q = vec4(-100);
        if (U.x>20.&&R.x-U.x>20.&&U.y>20.&&R.y-U.y>20.)
       	Q = vec4((length(U-0.5*R)<0.33*R.x?1.:-1.)*ceil(U/15.-0.5)*15.,0,0);
    }
 	
 	if (abs(Q.x)<10.)Q.z=0.9*abs(Q.z);
 	if (R.x-abs(Q.x)<10.)Q.z=-0.9*abs(Q.z);
 	if (abs(Q.y)<10.)Q.w=0.9*abs(Q.w);
 	if (R.y-abs(Q.y)<10.)Q.w=-0.9*abs(Q.w);
}