

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T (vec2 U) {
	return texture(iChannel3,U/R);
}
vec4 C (vec2 U) {
	return texture(iChannel2,U/R);
}
vec4 D (vec2 U) {return texture(iChannel3,U/R);}
vec4 col (vec2 U) {
	vec4 me = T(U);
    me = T(U);
    vec4 c = C(U);
    vec3 no = normalize(vec3(c.zy,.1));
    return 0.5-0.5*c.z*sin((1.7+1e2*me.w+me.z)*vec4(1,2,3,4));

}
void mainImage( out vec4 Q, in vec2 U )
{
   R = iResolution.xy;
   Q = col(U);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Particle tracking
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 D (vec2 U) {return texture(iChannel3,U/R);}
float man (vec2 U) {return abs(U.x)+abs(U.y);}
void X (vec2 U, inout vec4 Q, vec2 u) {
    vec4 p = A(U+u);
    if (length(p.xy - U) < length(Q.xy-U)) Q = p;
    
}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
 	Q = A(U);
 	vec4 c = C(U);
 	// measure neighborhood
 	for (int x = -1; x <= 1; x++)
     for (int y = -1; y <= 1; y++)
      X(U,Q,vec2(x,y));
    // if neares cell is far, make one up
    if (length(c.xy)<0.1&&man(Q.xy-U)>2.) Q.xyw = vec3(U,sign(floor(3.*c.w+0.5)));
        
 	Q.xy += D(Q.xy).xy;
    // init
    if (iFrame < 1) {
        vec2 u = U;
        U = floor(u);
        Q.xy = U;
        Q.z = 1.;
        Q.w = sign((U.y-0.5*R.y));
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Blur pass 1
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
	Q = vec4(0);
 	for (float i = -N ; i <= N ; i++) {
 		vec4 a = A(U+vec2(i,0));
        vec4 c = C(U+vec2(i,0));
        float s = smoothstep(1.1,.9,length(U+vec2(i,0)-a.xy));
        vec4 x = vec4(0,0,a.w*s,c.z);
        Q += x*sqrt(FORCE_RANGE)/FORCE_RANGE*exp(-i*i*0.5/FORCE_RANGE);
 	}
 		
 	
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Blur pass 2
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
	Q = vec4(0);
 	for (float i = -N ; i <= N ; i++) {
 		vec4 c = B(U+vec2(0,i));
        Q += c*sqrt(FORCE_RANGE)/FORCE_RANGE*exp(-i*i*0.5/FORCE_RANGE);
 	}
 	vec4 
        n = C(U+vec2(0,1)),
        e = C(U+vec2(1,0)),
        s = C(U+vec2(0,-1)),
        w = C(U+vec2(-1,0));
 	Q = clamp(Q,-1.,1.);
   n = C(U+vec2(0,1));
   e = C(U+vec2(1,0));
   s = C(U-vec2(0,1));
   w = C(U-vec2(1,0));
   Q.xy = vec2(e.w-w.w,n.w-s.w);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Fluid
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 D (vec2 U) {return texture(iChannel3,U/R);}
vec4 T (vec2 U) {
    return D(U-D(U).xy);
}
void mainImage( out vec4 Q, in vec2 U )
{
   R = iResolution.xy;
   Q = T(U);
   vec4 n,e,s,w;
   n = T(U+vec2(0,1));
   e = T(U+vec2(1,0));
   s = T(U-vec2(0,1));
   w = T(U-vec2(1,0));
   vec4 c = C(U);
   // xy : velocity, z : pressure, w : spin
   Q.x -= .25*(e.z-w.z+Q.w*(n.w-s.w));
   Q.y -= .25*(n.z-s.z+Q.w*(e.w-w.w));
   Q.z  = .25*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   Q.w  = .25*((n.x-s.x+w.y-e.y)-(n.w+e.w+s.w+w.w));
   Q.xy = mix(Q.xy*.999,0.25*(n+e+s+w).xy,0.2);
   // blurred particles force
   Q.xy += .001*c.z*c.xy;
   // gravity
   Q.y -= c.z/1000.;
   if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
   // boundary conditions
   if( iFrame < 1) Q = vec4(0);
   if (U.x<7.||R.x-U.x<7.||U.y<7.||R.y-U.y<7.) Q.xy *= 0.8;
   vec2 m = (U-iMouse.xy)/dot(U-iMouse.xy,U-iMouse.xy);
    if (iMouse.z > 0.) Q.xy = clamp(Q.xy+.2*(vec2(-m.y,m.x))*exp(-.0001*length(U-iMouse.xy)),vec2(-1),vec2(1));

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// lower to improve frame rate
#define N 6.

#define FORCE_RANGE vec4(0,0,3,3)
