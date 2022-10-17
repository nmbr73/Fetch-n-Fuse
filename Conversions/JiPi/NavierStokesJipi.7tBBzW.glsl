

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//based on https://www.shadertoy.com/view/4lyyzc by wyatt
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
    R = iResolution.xy;
    
   	vec4 v = T(U);
    vec2 dz = v.xy-vec2(.5,0);
    C.xyz = max(vec3(0),sin(1.5+5.*(v.w)*vec3(1,2,3)));
    float ndz = length(dz);
    C *= (0.7+.5*ndz);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define k 0.25
#define l .7
#define m 0.0
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u,a,b,c,d;
 	float ds = 0.;
 	vec2 g = vec2(0);
    vec2 vdv = vec2(0);
 	
    float s = 0.5;
 	
    u = T(U); U -= u.xy*s;
    a = T(A); b = T(B); c = T(C); d = T(D);
    A -=a.xy*s; B -=b.xy*s; C -=c.xy*s; D -=d.xy*s;     
    g += vec2(a.z-c.z,b.z-d.z);
    //vdv += vec2(u.x*(c.x-a.x)+u.y*(d.x-b.x), u.x*(c.y-a.y)+u.y*(d.y-b.y))/2.; 	
 	Q = T(U);
 	vec4 N = 0.25*(a+b+c+d);
    float div = 0.25*((c.x-a.x)+(d.y-b.y));
 	Q.z = N.z-k*div;
 	Q.xy += g*l;
    //Q.xy -= vdv*m;
 	//Q *= 0.9999;
 	if (iFrame < 1) Q = vec4(.5,0,0,0);
 	if (length(U-vec2(0.075,0.15)*R) < 4.) Q.xyw = vec3(Q.xy*0.5+0.5*vec2(-1.0,0),1.);
    if (length(U-vec2(0.075,0.5)*R) < 16.) Q.xyw=vec3(0.,0.,1.);
    if (length(U-vec2(0.115,0.75)*R)+ length(U-vec2(0.055,0.82)*R)< R.x/12.) Q.xyw=vec3(0.,0.,1.);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy=vec2(.5,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define k 0.25
#define l .7
#define m 0.0
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u,a,b,c,d;
 	float ds = 0.;
 	vec2 g = vec2(0);
    vec2 vdv = vec2(0);
 	
    float s = 0.5;
 	
    u = T(U); U -= u.xy*s;
    a = T(A); b = T(B); c = T(C); d = T(D);
    A -=a.xy*s; B -=b.xy*s; C -=c.xy*s; D -=d.xy*s;     
    g += vec2(a.z-c.z,b.z-d.z);
    //vdv += vec2(u.x*(c.x-a.x)+u.y*(d.x-b.x), u.x*(c.y-a.y)+u.y*(d.y-b.y))/2.; 	
 	Q = T(U);
 	vec4 N = 0.25*(a+b+c+d);
    float div = 0.25*((c.x-a.x)+(d.y-b.y));
 	Q.z = N.z-k*div;
 	Q.xy += g*l;
    //Q.xy -= vdv*m;
 	//Q *= 0.9999;
 	if (iFrame < 1) Q = vec4(.5,0,0,0);
 	if (length(U-vec2(0.075,0.15)*R) < 4.) Q.xyw = vec3(Q.xy*0.5+0.5*vec2(-1.0,0),1.);
    if (length(U-vec2(0.075,0.5)*R) < 16.) Q.xyw=vec3(0.,0.,1.);
    if (length(U-vec2(0.115,0.75)*R)+ length(U-vec2(0.055,0.82)*R)< R.x/12.) Q.xyw=vec3(0.,0.,1.);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy=vec2(.5,0);
}