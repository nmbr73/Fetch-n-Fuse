

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define samples 30.
#define glow_size 0.07 
#define glow_brightness 4.

float hash(vec2 p)
{
   return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float c=0.;
    vec2 uv=fragCoord/iResolution.xy;
    float mt=mod(iTime,5.);
     for (float i=0.; i<samples; i++) {
        float t=i*.354185+mt;
        float a=hash(uv+t)*6.28;
        float l=hash(uv+t+13.3548)*glow_size;
        vec2 smp = vec2(cos(a),sin(a))*l;
      	c+=step(.1,texture(iChannel0, uv+smp).x)*(glow_size-l*.9)/glow_size; 
     }
    vec4 part = texture(iChannel0, uv);
    vec3 uvcol=vec3(normalize(abs(uv+.1)),1.);
    vec3 col=(c/samples)*uvcol*glow_brightness+step(.1,part.x)*uvcol;
	col*=vec3(.8,.6,.15);
	col*=1.-abs(uv.x-.5)*2.;
    fragColor = vec4(col,1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Particles idea taken from https://www.shadertoy.com/view/ll3SWs

#define A 6

float hash1(float i)
{
   return fract(sin(i*.156854) * 43758.5453);
}

float hash2(vec2 p)
{
   return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 randCoord(float i) {
    return vec2(hash1(i),hash1(i+12.54682));
}

float arrivingParticle(vec2 coord, out vec4 partData) {
	partData = vec4(0);
    float c=0.;
    for (int i=-A; i<A; i++) {
        for (int j=-A; j<A; j++) {
            vec2 arrCoord = coord + vec2(i,j);
            vec4 data = texture(iChannel0, arrCoord/iResolution.xy);
            if (dot(data,data)<.1) continue;
            vec2 nextCoord = data.xy + data.zw;
            vec2 offset = abs(coord - nextCoord);
			// somehow I got this fluid-like effect changing the 
            // "greedly pick one particle" algorithm 
            // for an average of arriving particles and 
            // changing the condition below 
            if (length(offset)<1.7) { 
                partData += data;
				c++;
            }
        }
    }
    partData/=c;
    return c;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord/iResolution.xy;
    float t=iTime*3.;
    vec2 co = uv-randCoord(floor(t))*vec2(1.,.6);
    if (fract(t)<.3 && length(co)<.05) {
        fragColor = vec4(fragCoord.xy, 4.*normalize(co)*(1.-hash2(uv)*.5)+vec2(0.,3.));
		return;
    }
    if (fragCoord.y<30.+sin(uv.x*5.+t)*10.) {
        fragColor = vec4(fragCoord.xy,0,0);
		return;
    }
    vec4 partData;
    float p = arrivingParticle(fragCoord, partData);
    if (p<1.) {
    	fragColor = vec4(0.);
        return;
    }
    partData.xy+=partData.zw;
    partData.zw*=.99;
    partData.zw-=vec2(0.,.05);
    if (partData.y<30.) partData.w*=-1.;
    partData.w=max(-4.,partData.w);
    fragColor = partData;
}