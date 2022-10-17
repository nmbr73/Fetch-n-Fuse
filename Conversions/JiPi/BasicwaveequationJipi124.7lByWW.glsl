

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	fragColor = texture(iChannel0,fragCoord.xy / iResolution.xy)*.5+.5;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//in which BufA is t-dt, BufB is t and BufA becomes t+dt



vec4 getA(ivec2 p, vec2 R){
    vec4 middle    = getX(iChannel1,p,R);   	
    vec4 up        = getX(iChannel1,p+ivec2( 0, 1),R);
    vec4 down      = getX(iChannel1,p+ivec2( 0,-1),R);
    vec4 right     = getX(iChannel1,p+ivec2( 1, 0),R);
    vec4 left      = getX(iChannel1,p+ivec2(-1, 0),R);
    vec4 upright   = getX(iChannel1,p+ivec2( 1, 1),R);
    vec4 upleft    = getX(iChannel1,p+ivec2(-1, 1),R);
    vec4 downright = getX(iChannel1,p+ivec2( 1,-1),R);
    vec4 downleft  = getX(iChannel1,p+ivec2(-1,-1),R);
        
   	return (-8.*middle + up + left + right + down + upright + upleft + downright + downleft)/(3.*dx*dx);  
}

#define R iResolution.xy
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
   	ivec2 p = ivec2(fragCoord);
    fragColor = 2.*getX(iChannel1,p,R)-getX(iChannel0,p,R)+getA(p,R)*dt*dt;
    if(iMouse.z>0. && distance(fragCoord,iMouse.xy)<10.) fragColor = vec4(1.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//in which BufB is t-dt, BufA is t and BufA becomes t+dt

vec4 getA(ivec2 p, vec2 R){
    vec4 middle    = getX(iChannel1,p,R);   	
    vec4 up        = getX(iChannel1,p+ivec2( 0, 1),R);
    vec4 down      = getX(iChannel1,p+ivec2( 0,-1),R);
    vec4 right     = getX(iChannel1,p+ivec2( 1, 0),R);
    vec4 left      = getX(iChannel1,p+ivec2(-1, 0),R);
    vec4 upright   = getX(iChannel1,p+ivec2( 1, 1),R);
    vec4 upleft    = getX(iChannel1,p+ivec2(-1, 1),R);
    vec4 downright = getX(iChannel1,p+ivec2( 1,-1),R);
    vec4 downleft  = getX(iChannel1,p+ivec2(-1,-1),R);
        
   	return (-8.*middle + up + left + right + down + upright + upleft + downright + downleft)/(3.*dx*dx);  
}

#define R iResolution.xy
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
   	ivec2 p = ivec2(fragCoord);
    fragColor = 2.*getX(iChannel1,p,R)-getX(iChannel0,p,R)+getA(p,R)*dt*dt;
    if(iMouse.z>0. && distance(fragCoord,iMouse.xy)<10.) fragColor = vec4(1.);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

vec4 texelFetchC( sampler2D Channel, ivec2 pos, vec2 iR)
{
    
    if ( (pos.x) >= 0 && (pos.x) < int(iR.x) && (pos.y) > 0 && (pos.y) < int(iR.y) )
    {
        return texture( Channel, (vec2(pos)+0.5)/iR.xy );
    }
	else
		return vec4(0);
}

const float dx = 0.2;
const float dt = 0.08;

vec4 getX(sampler2D g, ivec2 p, vec2 iR){
    //return texelFetch(g,p,0);
    return texelFetchC(g,p,iR);
    //return texture(g,(vec2(p)+0.5)/R);
}
