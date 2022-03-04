

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 u = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0,u).zzzz;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2  a = vec2(0);
    float h = A(u).z;
    float z    = 8.;//kernel convolution size
    float blur = 4./z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
        vec2 c = vec2(i,j)*blur; //c = c.yx*vec2(-1,1);
        float h2 = A(u+vec2(i,j)).z;
        a += c*(h2-h)*exp(-dot(c,c));
    }}
    fragColor = a.xyxy;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
#define B(u) texture(iChannel1,(u)/iResolution.xy)
#define C(u) texture(iChannel2,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec4 t = A(u);
    vec2 m = +t.xy
             +B(u).xy*(t.z-.4)
             +t.z*vec2(0,.0)
             -C(u).x*t.xy*.0;
    float s = 0.;
    float z    = 8.;//kernel convolution size
    float blur = 4./z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      vec2 c = (m+vec2(i,j))*blur;
      s += exp(-dot(c,c));
    }}
    if(s==0.){s = 1.;}
    s = 1./s;
    
    fragColor = vec4(m,s,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
#define B(u) texture(iChannel1,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    float lz = 0.;
    float tz = 0.;
    vec4 a = vec4(0);
    float z    = 8.;//kernel convolution size
    float blur = 4./z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      vec4 t = A(u+vec2(i,j));
      vec4 m = B(u+vec2(i,j));
      vec2 c = (m.xy-vec2(i,j))*blur;
      float z = t.z*exp(-dot(c,c));
      lz   += z*length(m.xy);
      a.xy += z*m.xy;
      a.z  += z*m.z;
      tz   += z;
    }}
    if(tz==0.){tz = 1.;}
    float l = 1./length(a.xy);  if(isinf(l)){l = 0.;}
    a.xy *= l*lz/tz;
    if(iMouse.z>0.)
    {
        vec2 m = 16.*(u-iMouse.xy)/iResolution.y;
        a += vec4(0,0,1,1)*.1*exp(-dot(m,m));
    }
    if(iFrame==0)
    {
        vec2 m = 7.*(u-iResolution.xy*.5)/iResolution.y;
        a = vec4(0,0,1,1)*exp(-dot(m,m));
    }
    fragColor = a;
}