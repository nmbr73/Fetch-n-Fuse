

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 g )
{   	
    vec2 s = iResolution.xy;
    float cc = texture(iChannel0, g/s).w;
    float cc2 = texture(iChannel0, (g-1.)/s).w;
    fragColor = vec4(1)*cc*cc*0.8;
    fragColor += vec4(.7, .4, .2,1)*max(cc2*cc2*cc2 - cc*cc*cc, 0.0)*iResolution.y*.2;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define radius 0.43
#define emitSize 4.0
#define force 1.0
#define constraint 0.01
#define effect 1.0

#define emit(v,s,f) if (length(g-(v)) < emitSize) res.xy = res.xy * (1.0 - f) + f * (s), res.w = 1.0
#define rot(a) vec2(cos(radians(a)),sin(radians(a)))

vec4 calc(sampler2D sam, vec2 g, vec2 s, int i)
{
	vec4 a = texture(sam, (g+vec2(1,0))/s);
	vec4 b = texture(sam, (g+vec2(0,1))/s);
	vec4 c = texture(sam, (g+vec2(-1,0))/s);
	vec4 d = texture(sam, (g+vec2(0,-1))/s);
	vec4 res = texture(sam, (g-texture(sam, g/s).xy)/s);
	vec2 gp = vec2(a.z-c.z,b.z-d.z);
	res.xyz = vec3(
		res.x + gp.x,
		res.y + gp.y - res.w * 0.0001,
		(0.245 /*+ 0.005 * abs((g/s)*2.-1.)*/) * 
        (a.z + b.z + c.z + d.z) - 0.05 * (c.x - a.x + d.y - b.y));
    
    res.w += res.z * 0.01;
    
	if (i < 1) res = vec4(0);
	if (g.x < 1. || g.y < 1. || g.x > s.x - 1. || g.y > s.y - 1.) res.xy *= 0.;
    
    emit(s * 0.5 - vec2(1.5,1) * radius * min(s.x,s.y), rot(45.),1.0);
	emit(s * 0.5 - vec2(-1.5,1) * radius * min(s.x,s.y), rot(135.),0.25);
    
    return res;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    int i = iFrame; if (iMouse.z > 0.) i = 0;
	fragColor = calc(iChannel0, fragCoord, iResolution.xy, i);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	fragColor = calc(iChannel0, fragCoord, iResolution.xy, iFrame);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	fragColor = calc(iChannel0, fragCoord, iResolution.xy, iFrame);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	fragColor = calc(iChannel0, fragCoord, iResolution.xy, iFrame);
}