

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 o, in vec2 i )
{
    vec2 uv = i/R.xy;
    o = vec4(texture(iChannel0,uv)/1e2);
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Store all the particles here
void mainImage( out vec4 o, in vec2 i )
{
    vec2 uv = i/R.xy;
    
    o = texture(iChannel0,uv);
    
    float v = exp2(7.-floor(.7*T+T*T/20.));
    
    if(T<.3){
        o.xyzw = vec4(floor(uv.x/v*R.x)*v/R.x*.8+.1,0,0,5.1);
    }
    
    o.xy+=o.zw/R.xy*2.;
    vec4 r = hash44(vec4(floor(i/v),F,iMouse.x));
    r.z = sqrt(-2.*log(r.z));
    r.w *= 6.28318;
    r.zw = r.z*vec2(cos(r.w),sin(r.w))*.4;
    o.zw+=r.zw*(.1+T/2000.);
    o.w -= .6/(100.+T*30.);
    float l = length(o.zw);
    o.zw*=max(0.,(pow(l,.9+r.x*.1)*(1.-T/30.))/l);
    float t = atan(o.w,o.z);
    if(o.w>0.)
        t += .01*sign(o.z);
    o.zw = length(o.zw)*vec2(cos(t),sin(t));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//This is the first pass of the rendering/search

void mainImage( out vec4 o, in vec2 i )
{
    o = vec4(0);
    for(int a = 0; a < i1; a++){
        vec4 r = hash44(vec4(i,F,a));//randomly read points from buffer A
        vec4 p = texture(iChannel0,r.xy);
        float l = length(p.xy*R.xy-i);
        if(l < length(o.xy*R.xy-i)){//save only the closest to this pixel
            o = p;
        }
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 o, in vec2 i )
{
    vec2 uv = i/R.xy;
    o = texture(iChannel1,uv);
    for(int a = 0; a < i1; a++){
        vec4 r = hash44(vec4(i,F,a));//Transform this uniform random into a normal distribution
        r.z = sqrt(-2.*log(r.z));
        r.w *= 6.28318;
        r.zw = r.z*vec2(cos(r.w),sin(r.w))*s;
        vec4 p = texture(iChannel0,(i+r.xy)/R.xy);//sample random nearby points
        if(p.xy!=vec2(0)&&length(p.zw)>.001)
        	o += vec4(length(p.zw),.5+.5*sin(p.z),.5+.5*cos(4.*p.w),1)/(1.+exp((2.+T/5.)*length(p.xy*R.xy-i))); //add a gaussian to the accumulated image from the particle 
    }
    if(T<.3){
        o = vec4(0);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define CL(x) clamp(x,0.,.5)
#define R iResolution
#define F iFrame
#define T mod(float(iFrame)/60.,20.)
#define PI 2.*asin(1.)
#define E exp(1.)
vec4 hash44(vec4 p4)
{
	p4 = fract(p4  * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+19.19);
    return fract((p4.xxyz+p4.yzzw)*p4.zywx);
}
float s = 15.;	//search radius

int i1 = 100;
int i2 = 100;

vec2 cis(float t){
    return cos(t - vec2(0,PI/2.));
}
vec2 cexp(vec2 z) {
    return exp(z.x)*cis(z.y);
}
vec2 clog(vec2 z) {
    return vec2(log(length(z)),atan(z.y,z.x));
}