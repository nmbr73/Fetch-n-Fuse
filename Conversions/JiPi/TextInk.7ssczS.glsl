

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4 a = A(U), b = B(U);
    Q = a+b;
    float n = hash(U+vec2(0,1));
    float e = hash(U+vec2(1,0));
    float s = hash(U-vec2(0,1));
    float w = hash(U-vec2(1,0));
    vec3 no = normalize(vec3(e-s,n-s,1));
    
    Q = .9+.05*no.y-sqrt(Q);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.1415926
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
float ln (vec3 p, vec3 a, vec3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
float hash (vec2 p) // Dave H
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 char(vec2 p, int c) 
{
    if (p.x<.0|| p.x>1. || p.y<0.|| p.y>1.) return vec4(0,0,0,1e5);
	return textureGrad( iChannel1, p/16. + fract( vec2(c, 15-c/16) / 16. ), dFdx(p/16.),dFdy(p/16.) );
}

#define P(c) T.x-=.5; tV += char(T,64+c).x*mouseDown;

Main {

    //the two Qs here are responsibile for the dye pooling downwards
    float visc = 0.05; //viscosity or thickness or how runny it is
    
    vec2 dir = vec2(sin(U.y/R.y*10.+iTime*5.) ,sin(U.x/R.x*10.+iTime*5.)+1.0);
    dir = vec2(0.,1.);
    //dir = vec2(hash(vec2(iTime,U.x*10.)),hash(vec2(U.y*10.,iTime))) - 0.5;
    //dir *= 2.;
    Q = A(U+vec2(0,0));
    float xmove = sin((U.x/R.x-0.5)*10. + iTime/1.)/30.;
    //xmove = 0.;
    Q = A(U+vec2(xmove*1000.,visc*Q.w*0.));
    
    
    Q = A(U+dir);
    Q = A(U+vec2(visc*Q.w)*dir);
    //Q = A(U+vec2(0,0.));
    //Q.w is set to 1 when color is added
    vec4 q = vec4(0);
    for (int x = -1; x<= 1; x++)
    for (int y = -1; y<= 1; y++)
    if (x!=0||y!=0)
    {
        
        vec2 u = vec2(x,y); //corner diff
        vec4 a = A(U+u); //corner sample
        float h = hash(U+0.5*u); //corner random
        float m = (length(a.xyz)); //length of color of corner sample
        m = min(m,1.); //cap length at 1
        //m = pow(m,1.2)+visc;
        q += pow(h,6.)*(a-Q)/2.;
    }
    Q += 0.125*q;
    
    
    vec2 uv = U/R;
    uv.x *= R.x/R.y;
    
    float mouseDown = step(0.,iMouse.z);
    
    float FontSize = 32.;
    vec2 position = vec2(0.5)*vec2(R.x/R.y,1.) - vec2(FontSize/64.*3./2.,FontSize/64./2.);
    vec2 T = ( uv - position)*64.0/FontSize;
    
    float tV = char(T,64 + 16).x*mouseDown;
    
    P(1);P(9);P(14);P(20);
    //tV += char(T - vec2(0.5,0.),64 + 2).x*mouseDown;
    //tV += tV2;
    
    vec3 textCol = 1.-vec3(0.3,0.9,0.5);
    
    float textBool = step(0.5,tV);
    
    Q.w = mix(Q.w,1.,textBool);
    Q.xyz = mix(Q.xyz,textCol,textBool);
    
    //vec4 d = D(U);
    vec2 c = vec2(hash(vec2(iTime/450.,460.)*1000.)*R.x,hash(vec2(iTime/100.,0.)*2000.)*R.y);
    vec4 d = vec4(c,c);
    
    if (ln(U,d.xy,d.zw)<.025*R.y){
        //Q = 0.5+0.5*sin(iTime+vec4(3.+ iTime/100.,0.+ iTime/1131.,1.,4));
        //Q = 0.5+0.5*sin(iTime+vec4(0,5./4.,5./8.,4));
        //Q *= 0.2;
       // Q.w = 1.;
    }
    //if (iFrame < 1) Q = 1.-B(U);
    
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
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {

    Q = B(U);
    Q += 5e-4*A(U);
    
}