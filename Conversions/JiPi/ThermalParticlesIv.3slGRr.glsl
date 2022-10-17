

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
void mainImage( out vec4 C, in vec2 U )
{
    R = iResolution.xy;
    vec4 
        a = T(U+vec2(1,0)),
        b = T(U-vec2(1,0)),
        c = T(U+vec2(0,1)),
        d = T(U-vec2(0,1));
        
    vec4 g = vec4(a.zw-b.zw,c.zw-d.zw);
    vec2 dz = g.xz;
    vec2 dw = g.yw;
   	vec4 v = T(U);
    float r = smoothstep(0.,2.,length(U-abs(P(U).xy)));
    vec3 n = normalize(vec3(dz+dw,.1));
    vec3 t = texture(iChannel2,n).xyz;
    C.xyz = (t*0.2+0.9)*(0.5+0.5*(r*2.-1.)*sin(.9+r+v.w+.6*v.z*vec3(1,2,3)));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r, inout vec4 mu) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z-t.w+Q.w);
    mu += 0.25*t;
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u),mu=vec4(0);
 	Q.z = 0.25*(X(U,u,Q,e,mu)+X(U,u,Q,-e,mu)+X(U,u,Q,e.yx,mu)+X(U,u,Q,-e.yx,mu));
	float r = smoothstep(3.,1.,length(U-abs(p.xy)));
 	Q.w =  mix(0.9*mu.w,sign(p.x),r);
 	Q.xy = mix(Q.xy*0.9,p.zw,r);
 	Q.z*= 0.995;
 	if (iMouse.z > 0.) Q.w = mix(Q.w,100.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy = vec2(0);

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r, inout vec4 mu) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z-t.w+Q.w);
    mu += 0.25*t;
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u),mu=vec4(0);
 	Q.z = 0.25*(X(U,u,Q,e,mu)+X(U,u,Q,-e,mu)+X(U,u,Q,e.yx,mu)+X(U,u,Q,-e.yx,mu));
	float r = smoothstep(3.,1.,length(U-abs(p.xy)));
 	Q.w =  mix(0.9*mu.w,sign(p.x),r);
 	Q.xy = mix(Q.xy*0.9,p.zw,r);
 	Q.z*= 0.995;
 	if (iMouse.z > 0.) Q.w = mix(Q.w,100.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy = vec2(0);

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r, inout vec4 mu) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z-t.w+Q.w);
    mu += 0.25*t;
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u),mu=vec4(0);
 	Q.z = 0.25*(X(U,u,Q,e,mu)+X(U,u,Q,-e,mu)+X(U,u,Q,e.yx,mu)+X(U,u,Q,-e.yx,mu));
	float r = smoothstep(3.,1.,length(U-abs(p.xy)));
 	Q.w =  mix(0.9*mu.w,sign(p.x),r);
 	Q.xy = mix(Q.xy*0.9,p.zw,r);
 	Q.z*= 0.995;
 	if (iMouse.z > 0.) Q.w = mix(Q.w,100.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy = vec2(0);

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Voronoi based particle tracking

vec2 R;float N;
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
 	swap(U,Q,vec2(2,0));
 	swap(U,Q,vec2(0,-2));
 	swap(U,Q,vec2(-2,0));
 	swap(U,Q,vec2(0,2));
 	vec4 t = T(Q.xy);
 	vec2 e = vec2(3.5,0);
 	vec4 
        a = T(Q.xy+e.xy),
        b = T(Q.xy+e.yx),
        c = T(Q.xy-e.xy),
        d = T(Q.xy-e.yx);
 	Q.zw = Q.zw-0.06*vec2(a.z-c.z,b.z-d.z)-.1*vec2(a.w-c.w,b.w-d.w);
 	Q.zw = clamp(Q.zw,-.9,.9);
 	Q.xy = sign(Q.xy)*(abs(Q.xy)+Q.zw);
 	
 	if (iFrame < 1 && U.x>20.&&R.x-U.x>20.&&U.y>20.&&R.y-U.y>20.) {
       Q = vec4(floor(U/10.+0.5)*10.,0,0);
    }
 	
 	if (abs(Q.x)<10.)Q.z=0.9*abs(Q.z);
 	if (R.x-abs(Q.x)<10.)Q.z=-0.9*abs(Q.z);
 	if (abs(Q.y)<10.)Q.w=0.9*abs(Q.w);
 	if (R.y-abs(Q.y)<10.)Q.w=-0.9*abs(Q.w);
}