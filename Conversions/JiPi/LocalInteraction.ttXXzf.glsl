

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define A(COORD) texture(iChannel0,(COORD)/iResolution.xy)
float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*min(dot(p-a,b-a),0.)/dot(b-a,b-a));}
void mainImage( out vec4 Q, in vec2 U )
{
    Q  =  A(U);
    vec4 pX  =  A(U + vec2(1,0));
    vec4 pY  =  A(U + vec2(0,1));
    vec4 nX  =  A(U - vec2(1,0));
    vec4 nY  =  A(U - vec2(0,1));
    vec3 n = normalize(vec3(pX.z-nX.z,pY.z-nY.z,1));
    vec3 r = reflect(n,vec3(0,0,-1));
    Q = (0.5+0.5*sin(iTime+atan(Q.x,Q.y)*vec4(3,2,1,4)));
    float d = ln(vec3(.4,.4,6)*iResolution.xyy,
                 vec3(U,0),vec3(U,0)+r)/iResolution.y;
    Q *= exp(-d*d)*.5+.5*exp(-3.*d*d);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(COORD) texture(iChannel0,(COORD)/iResolution.xy)

void mainImage( out vec4 Q, in vec2 U )
{
    U-=.5*iResolution.xy;
    U *= .997;
    float a = .003*sin(.5*iTime-2.*length(U-0.5*iResolution.xy)/iResolution.y);
    U *= mat2(cos(a),-sin(a),sin(a),cos(a));
    U+=.5*iResolution.xy;
    Q  =  A(U);
    // Neighborhood :
    vec4 pX  =  A(U + vec2(1,0));
    vec4 pY  =  A(U + vec2(0,1));
    vec4 nX  =  A(U - vec2(1,0));
    vec4 nY  =  A(U - vec2(0,1));
    vec4 m = 0.25*(pX+nX+pY+nY);
    float b = mix(1.,abs(Q.z),.8);
    Q.xyz += (1.-b)*(0.25*vec3(pX.z-nX.z,pY.z-nY.z,-pX.x+nX.x-pY.y+nY.y)- Q.xyz);

    
    Q = mix(Q,m,b);
    
    if (length(Q.xy)>0.) Q.xy = normalize(Q.xy);
    
    if(iFrame < 1) Q = sin(.01*length(U-0.5*iResolution.xy)*vec4(1,2,3,4));
    
    if (iMouse.z>0.&&length(U-iMouse.xy)<.1*iResolution.y) Q *= 0.;
    
}