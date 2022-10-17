

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//calculate 5x5 caustic
#define R iResolution.xy
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
void mainImage (out vec4 Q, vec2 U) {
    Q = vec4(0);
    for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++) {
       vec3 u = normalize(vec3(x,y,1));
       vec2 b = B(U+u.xy).zw;
       Q.x += dot(u,normalize(vec3(b,.005)));
       Q.y += dot(u,normalize(vec3(b,.010)));
       Q.z += dot(u,normalize(vec3(b,.015)));
    }
    Q = Q/20.;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// fluid pressure and velocity
#define R iResolution.xy
vec4 t (vec2 U) {return texture(iChannel0,U/R);}
vec4 A (vec2 U) {return t(U-t(U).xy);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
float ln (vec2 p, vec2 a, vec2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));}
void mainImage (out vec4 Q, vec2 U) {
    vec4 
        b = B(U),
        c = C(U),
        me = Q = A(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    // navier stokes
    Q.x -= 0.25*(e.w-w.w+c.x);
    Q.y -= 0.25*(n.w-s.w+c.y);
    Q.z += 0.125*(me.w-b.x); // vertical velocity feeds back with the height
    Q.w = 0.25*((n.w+e.w+s.w+w.w)-me.z-b.y-(n.y-s.y+e.x-w.x)); // pressure calculation accounting for the vertical force
    // this part is pivotal but I don't remember why I put it there...
    Q.xyz *= min(1.,b.x);
    
    //boundaries
    if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.) Q.xyz *= 0.;
    if (iFrame < 1) Q = vec4(vec2(.1*smoothstep(50.,45.,length(U-0.5*R))),0,1);
    vec4 mo = texture(iChannel3,U/R);
    float l = ln(U,mo.xy,mo.zw);
   	if (length(mo.xy-mo.zw)>0.) Q.xy += 2.*exp(-.05*l*l)*normalize(mo.xy-mo.zw);
    if (length(Q.xy)>0.) Q.xy = normalize(Q.xy)*min(.8,length(Q.xy));

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// fluid height
#define R iResolution.xy
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,(U-A(U).xy)/R);}
void mainImage (out vec4 Q, vec2 U) {
	Q = B(U);
    vec4 a = A(U),
        n = B(U+vec2(0,1)),
        e = B(U+vec2(1,0)),
        s = B(U-vec2(0,1)),
        w = B(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    // basically the schrodinger equation 
    Q.y = m.x-Q.x;
    Q.x += a.z;
    // gradient for making the caustic in Image
    Q.zw = vec2(e.x-w.x,n.x-s.x);
    
    // boundaries
    if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.) Q.x = 1.;
    if (iFrame < 1) Q = vec4(1,0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//Mouse
void mainImage( out vec4 C, in vec2 U )
{
    vec4 p = texture(iChannel0,U/iResolution.xy);
   	if (iMouse.z>0.) {
      if (p.z>0.) C =  vec4(iMouse.xy,p.xy);
    else C =  vec4(iMouse.xy,iMouse.xy);
   }
    else C = vec4(-iResolution.xy,-iResolution.xy);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// fluid vorticity
#define R iResolution.xy
vec4 t (vec2 U) {return texture(iChannel0,U/R);}
vec4 c (vec2 U) {return texture(iChannel2,U/R);}
vec4 C (vec2 U) {return c(U-t(U).xy);}
float ln (vec2 p, vec2 a, vec2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));}
void mainImage (out vec4 Q, vec2 U) {
    vec4 
        c = C(U),
        n = t(U+vec2(0,1)),
        e = t(U+vec2(1,0)),
        s = t(U-vec2(0,1)),
        w = t(U-vec2(1,0));
    // curl
   	Q.z = 0.25*(n.x-s.x+e.y-w.y);
    	n = C(U+vec2(0,1));
        e = C(U+vec2(1,0));
        s = C(U-vec2(0,1));
        w = C(U-vec2(1,0));
    // magnus force
    Q.xy = c.z*vec2(s.z-n.z,w.z-e.z);
    
    if (iFrame < 1) Q = vec4(0);

}