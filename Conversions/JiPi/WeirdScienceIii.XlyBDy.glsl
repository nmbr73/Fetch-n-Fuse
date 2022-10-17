

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
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
   	vec4 v = T(U-10.*dz);
    C.xyz = abs(sin(v.z*v.z+0.5+5.*(v.w-length(dw))*vec3(1.1,1.2,1.3)));
    vec3 n = normalize(vec3(dz,.05));
    vec4 tx = texture(iChannel1,reflect(vec3(0,0,1),n));
    C = (n.x*0.3+0.9)*(C)*(0.9+0.1*tx);
    C *= sqrt(C);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Fluid Algorithm 
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z+Q.w*t.w*(t.w-Q.w));
    return 0.5*(length(r-v+u)-length(r+v-u))+t.z;
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	float P = 0.; Q = T(U-u);
 	Q.z = 0.25*(
       X (U,u,Q, e.xy)+
 	   X (U,u,Q,-e.xy)+
 	   X (U,u,Q, e.yx)+
 	   X (U,u,Q,-e.yx)
    );
 	float l = length(Q.xy);if(l>0.)Q.xy=mix(Q.xy,Q.w*Q.xy/l,0.0001);
 	if (iFrame < 1){if (length(U-0.5*R)<7.)Q=vec4(1); else Q = vec4(0);}
	vec4 mo = texture(iChannel2,vec2(0));
 	l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 5.) Q.xyzw += vec4((5.-l)*(mo.xy-mo.zw)/700.,0,1.-Q.w);
 
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Fluid Algorithm 
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z+Q.w*t.w*(t.w-Q.w));
    return 0.5*(length(r-v+u)-length(r+v-u))+t.z;
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	float P = 0.; Q = T(U-u);
 	Q.z = 0.25*(
       X (U,u,Q, e.xy)+
 	   X (U,u,Q,-e.xy)+
 	   X (U,u,Q, e.yx)+
 	   X (U,u,Q,-e.yx)
    );
 	float l = length(Q.xy);if(l>0.)Q.xy=mix(Q.xy,Q.w*Q.xy/l,0.0001);
 	if (iFrame < 1){if (length(U-0.5*R)<7.)Q=vec4(1); else Q = vec4(0);}
	vec4 mo = texture(iChannel2,vec2(0));
 	l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 5.) Q.xyzw += vec4((5.-l)*(mo.xy-mo.zw)/700.,0,1.-Q.w);
 
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//Fluid Algorithm 
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    Q.xy -= 0.25*r*(t.z-Q.z+Q.w*t.w*(t.w-Q.w));
    return 0.5*(length(r-v+u)-length(r+v-u))+t.z;
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	float P = 0.; Q = T(U-u);
 	Q.z = 0.25*(
       X (U,u,Q, e.xy)+
 	   X (U,u,Q,-e.xy)+
 	   X (U,u,Q, e.yx)+
 	   X (U,u,Q,-e.yx)
    );
 	float l = length(Q.xy);if(l>0.)Q.xy=mix(Q.xy,Q.w*Q.xy/l,0.0001);
 	if (iFrame < 1){if (length(U-0.5*R)<7.)Q=vec4(1); else Q = vec4(0);}
	vec4 mo = texture(iChannel2,vec2(0));
 	l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 5.) Q.xyzw += vec4((5.-l)*(mo.xy-mo.zw)/700.,0,1.-Q.w);
 
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// keep track of mouse
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 p = texture(iChannel0,fragCoord/iResolution.xy);
    if (iMouse.z>0.) {
        if (p.z>0.) fragColor =  vec4(iMouse.xy,p.xy);
    	else fragColor =  vec4(iMouse.xy,iMouse.xy);
    }
    else fragColor = vec4(-iResolution.xy,-iResolution.xy);
}