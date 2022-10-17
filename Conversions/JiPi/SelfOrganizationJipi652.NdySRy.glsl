

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float T(vec2 U) {
	vec4 a = A(U);
    vec4 b = B(U);
    vec4 c = C(U);
    vec4 d = D(U);
    return dot(a,a)+dot(b,b)+dot(c.xyz,c.xyz)+dot(d.xyz,d.xyz);
}
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U);
    vec4 b = B(U);
    vec4 c = C(U);
    vec4 d = D(U);
    float
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0));
    vec3 g = normalize(vec3(e-w,n-s,1));
    c = .1*vec4(c.x,0.5*c.x+0.5*c.y,c.y,1);
    d = .1*vec4(d.x,0.5*d.x+0.5*d.y,d.y,1);
    Q = .8*(sqrt(c*c+10.*d*d+.01*(a*a+5.*b*b)));
    Q.xyz = mix(Q.xyz,normalize(Q.xyz),min(1.,length(Q.xyz)));
	Q *= dot(reflect(g,vec3(0,0,1)),normalize(vec3(1)))*0.3+0.7;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy

#define A(U) texture(iChannel0, (U)/R)
#define B(U) texture(iChannel1, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

//controls :

#define all 1.
#define M   0.2
#define PN  .5
#define RGB 0.5

#define loss 0.03

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    Q += B(U);
    //if (iFrame < 1) Q = 5.*sin(.01*length(U-0.5*R)*vec4(1,2,3,4));
    if (iFrame < 1) Q = texture(iChannel3,U/iResolution.xy)*5. - 2.5;//5.*sin(.01*length(U-0.5*R)*vec4(1,2,3,4));
    
    
	if(iMouse.z>0.&&length(U-iMouse.xy)<24.||(length(U-0.5*R)<2.&&iFrame<2)) Q = vec4(0);
	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = B(U);
    vec4 a = A(U);
    vec4 c = C(U);
    vec4 dm = 1./4.*(
    	A(U+vec2(1,0))+
        A(U+vec2(0,1))+
        A(U-vec2(1,0))+
        A(U-vec2(0,1))
    )-a;
    vec4 dmb = 1./4.*(
    	B(U+vec2(1,0))+
        B(U+vec2(0,1))+
        B(U-vec2(1,0))+
        B(U-vec2(0,1))
    )-Q;
    Q += dm + loss*dmb;
    float mag = sqrt(a.w*a.w+dot(c.xy,c.xy)+dot(a.xyz,a.xyz));
    //if (length(a.xyz)>0.) Q.xyz += RGB*3./6.*a.xyz/length(a.xyz);
	if (abs(a.w)>0.) Q.w+= M*1./6.*a.w/abs(a.w);
    if (mag > 0.) Q -= all*a/mag;
	if(iMouse.z>0.&&length(U-iMouse.xy)<24.||(length(U-0.5*R)<2.&&iFrame<2)) Q = vec4(0);
	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = C(U);
    Q += D(U);
    //if (iFrame < 1) Q = 5.*sin(.01*length(U-0.5*R)*vec4(4,5,3,4));
    if (iFrame < 1) Q = texture(iChannel0,U/iResolution.xy)*5. - 2.5;//
    
	if(iMouse.z>0.&&length(U-iMouse.xy)<24.||(length(U-0.5*R)<2.&&iFrame<2)) Q = vec4(0);
	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q *= 0.;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = D(U);
    vec4 a = A(U);
    vec4 c = C(U);
    vec4 dm = 1./4.*(
    	C(U+vec2(1,0))+
        C(U+vec2(0,1))+
        C(U-vec2(1,0))+
        C(U-vec2(0,1))
    )-c;
    vec4 dmd = 1./4.*(
    	D(U+vec2(1,0))+
        D(U+vec2(0,1))+
        D(U-vec2(1,0))+
        D(U-vec2(0,1))
    )-Q;
    Q += dm + loss*dmd;
    float mag = sqrt(a.w*a.w+dot(c.xy,c.xy)+dot(a.xyz,a.xyz));
    //if (length(c.xy)>0.) Q.xy += 2./6.*PN*c.xy/length(c.xy);
    if (mag > 0.) Q.xy -= all*c.xy/mag;
	if(iMouse.z>0.&&length(U-iMouse.xy)<24.||(length(U-0.5*R)<2.&&iFrame<2)) Q = vec4(0);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q *= 0.;
}