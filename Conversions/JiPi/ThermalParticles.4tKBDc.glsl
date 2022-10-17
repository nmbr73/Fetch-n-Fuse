

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
I plan on doing a few iterations of this
the physics is definitely not realistic yet
but it's a good practical start

The idea is that the particles sit in a fluid of smaller particles
their interaction with the fluid determines the force field around them
in this setup, the force is very weak and inaccurate. I plan on improving that.
The particles bounce off each other because of an interaction with the field that is 
the drawing of the particles.

*/


vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{
    R = iResolution.xy;
   	vec4 t = T(U);
   	vec4 p = P(U);
    Q.xyz = 0.5+0.5*sin(1.5*(2.*t.w+t.z*vec3(1,2,3)));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    // the "w" channel contributes to the force interaction
    Q.xy -= 0.25*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u);
 	// The particles sit in a damped fluid
 	Q.z = 0.25*(X(U,u,Q,e)+X(U,u,Q,-e)+X(U,u,Q,e.yx)+X(U,u,Q,-e.yx));
	// the particles interact with the space so i just draw a circle where they are
 	Q.w =  sign(p.x)*smoothstep(2.,0.,length(U-abs(p.xy)));
 	// the pressure is damped so that it doesn't go too crazy
 	Q.z *= 0.96;
 	// make a little pressure and density where the mouse is
 	if (iMouse.z > 0.) Q.w = mix(Q.w,3.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy = vec2(0);

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    // the "w" channel contributes to the force interaction
    Q.xy -= 0.25*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u);
 	// The particles sit in a damped fluid
 	Q.z = 0.25*(X(U,u,Q,e)+X(U,u,Q,-e)+X(U,u,Q,e.yx)+X(U,u,Q,-e.yx));
	// the particles interact with the space so i just draw a circle where they are
 	Q.w =  sign(p.x)*smoothstep(2.,0.,length(U-abs(p.xy)));
 	// the pressure is damped so that it doesn't go too crazy
 	Q.z *= 0.96;
 	// make a little pressure and density where the mouse is
 	if (iMouse.z > 0.) Q.w = mix(Q.w,3.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy = vec2(0);

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
float X (vec2 U, vec2 u, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, v = T(V).xy;
    vec4 t = T (V-v);
    // the "w" channel contributes to the force interaction
    Q.xy -= 0.25*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5*(length(r-v+u)-length(r+v-u))+t.z);
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 u = T(U).xy, e = vec2(1,0);
 	Q = T(U-u);
 	vec4 p = P(U-u);
 	// The particles sit in a damped fluid
 	Q.z = 0.25*(X(U,u,Q,e)+X(U,u,Q,-e)+X(U,u,Q,e.yx)+X(U,u,Q,-e.yx));
	// the particles interact with the space so i just draw a circle where they are
 	Q.w =  sign(p.x)*smoothstep(2.,0.,length(U-abs(p.xy)));
 	// the pressure is damped so that it doesn't go too crazy
 	Q.z *= 0.96;
 	// make a little pressure and density where the mouse is
 	if (iMouse.z > 0.) Q.w = mix(Q.w,3.,exp(-.1*dot(U-iMouse.xy,U-iMouse.xy)));
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
    Q = mix(Q,p,float(dl>0.));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	Q = P(U);
 	// exchange information with neighbors to find particles
 	swap(U,Q,vec2(1,0));
 	swap(U,Q,vec2(0,1));
 	swap(U,Q,vec2(0,-1));
 	swap(U,Q,vec2(-1,0));
 	swap(U,Q,vec2(3,0));
 	swap(U,Q,vec2(0,3));
 	swap(U,Q,vec2(0,-3));
 	swap(U,Q,vec2(-3,0));
 	// find this particle fluid state
 	vec4 t = T(Q.xy);
 	vec2 e = vec2(1,0);
 	// find neighbor fluid states
 	vec4 
        a = T(Q.xy+e.xy),
        b = T(Q.xy+e.yx),
        c = T(Q.xy-e.xy),
        d = T(Q.xy-e.yx);
 	// accelerate this particle in the direction of the z and w fields
 	// z field is force at a distance. the particles displace the fluid and create a pressure field
 	// w field is the discritized particle. its just a drawing of where they are
 	// but when they get close they transfer energies because they interact with the radial drawing of the particle and push away
 	Q.zw = Q.zw*0.95+vec2(a.z-c.z,b.z-d.z)-vec2(a.w-c.w,b.w-d.w);
 	// the particle moves in the diredction of its momentum
 	Q.xy += .1*Q.zw;
 	
 	
 	if (iFrame < 1) { // I don't know why the init looks so cool in practice, I just wanted a grid
     	Q = vec4(floor(U/10.)*10.,U);
    }
}