

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
    vec4 a = texture(iChannel0,u);
    fragColor = a.z*(+sin(a.x*4.+vec4(1,3,5,4))*.2
                     +sin(a.y*4.+vec4(1,3,2,4))*.2+.6);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2 v = u/iResolution.xy;
    vec4 a = A(u);
    vec2 m = +a.xy                      //fluid velocity
             -vec2(0,1)*.01             //gravity
             +float(v.x<.05)*vec2(1,0)  //wall
             +float(v.y<.05)*vec2(0,1)  //wall
             -float(v.x>.95)*vec2(1,0)  //wall
             -float(v.y>.95)*vec2(0,1); //wall
    float s = 0.;
    float z = 4.;//kernel convolution size
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      vec2 c = -m+vec2(i,j);//translate the gaussian 2Dimage using the velocity
      s += exp(-dot(c,c));  //calculate the gaussian 2Dimage
    }}
    if(s==0.){s = 1.;}      //avoid division by zero
              s = 1./s;
    fragColor = vec4(m,s,0);//velocity in .xy
                            //convolution normalization in .z
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
#define B(u) texture(iChannel1,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec4  o = vec4(0);
    float z = 4.;//kernel convolution size
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      vec4  a = A(u+vec2(i,j));        //old velocity in a.xy, mass in a.z
      vec4  b = B(u+vec2(i,j));        //new velocity in b.xy, normalization of convolution in .z
      vec2  c = -b.xy-vec2(i,j);       //translate the gaussian 2Dimage
      float s = a.z*exp(-dot(c,c))*b.z;//calculate the normalized gaussian 2Dimage multiplied by mass
      vec2  e = c*(a.z-.8);            //fluid expands or atracts itself depending on mass
      o.xy += s*(b.xy+e);              //sum all translated velocities
      o.z  += s;                       //sum all translated masses
    }}
    float tz = 1./o.z;
    if(o.z==0.){tz = 0.;}              //avoid division by zero
    o.xy *= tz;                        //calculate the average velocity
    if(iMouse.z>0.)                    //mouse click adds velocity
    {
        vec2 m = 8.*(u-iMouse.xy)/iResolution.y;
        o += vec4(m,0,0)*.1*exp(-dot(m,m));
    }
    if(iFrame==0)
    {
        vec2 m = 3.*(u-iResolution.xy*.5)/iResolution.y;
        o = vec4(0,0,1,1)*exp(-dot(m,m));
    }
    fragColor = o;
}