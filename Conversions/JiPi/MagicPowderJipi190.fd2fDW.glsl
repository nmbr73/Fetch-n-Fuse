

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define samples 100 
#define size 0.04
#define brightness 3.
#define part_color vec3(1.,.7,.4)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float c=0.;
    vec2 uv=fragCoord/iResolution.xy;
     for (int i=0; i<samples; i++) {
        float a=hash(uv+float(i)*.123)*6.28;
        float l=hash(uv+float(i)*.454)*size;
        vec2 smp = vec2(cos(a),sin(a))*l;
        smp.y*=iResolution.x/iResolution.y;
        vec4 part=texture(iChannel0, uv+smp);
      	c+=length(part.zw)*(size-l*.9)/size; 
     }
    vec4 part = texture(iChannel0, uv);
    vec3 col=(c/float(samples))*part_color*brightness*5.;
    vec2 uvc = (uv-.5)*2.;
    col*=1.-length(uvc*uvc*uvc);
    fragColor = vec4(col,1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Particles idea taken from https://www.shadertoy.com/view/ll3SWs

#define A 3
#define speed 3.
#define size .02
#define fade .99

// check if there is an arriving particle at this pixel in next frame
float arrivingParticle(vec2 coord, out vec4 partData) {
	partData = vec4(0);
    float c=0.;
    // scan area from -D to D
    for (int i=-A; i<A; i++) {
        for (int j=-A; j<A; j++) {
            // position to check
            vec2 arrCoord = coord + vec2(i,j);
            vec4 data = texture(iChannel0, arrCoord/iResolution.xy);
            
            // no particles here
            if (dot(data,data)<.1) continue;

            // get next position of particle
            vec2 nextCoord = data.xy + data.zw;
            
            // add the particle if it's within range
			vec2 offset = abs(nextCoord-coord);
            if (offset.x <.5 && offset.y <.5) {
                partData += data;
				c++;
            }
        }
    }
    partData/=c; //average pos and speeds of resulting particle
    return c;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float t=float(iFrame)*.1;
	vec2 uv = fragCoord/iResolution.xy;
	vec2 m = iMouse.xy/iResolution.xy;
    vec2 uvm=uv-m;
    
    //draw particles
    if (t<35.) uvm=.5-uv-vec2(sin(t)*.7,cos(t)*.6)*.14*iTime;    
    if ((iMouse.z>0.||iTime<5.) && step(length(uvm),size)>0.) {
        fragColor = vec4(fragCoord.xy, 
                         speed*normalize(uvm+vec2(hash(uv+1.5465+t), hash(uv+2.5648+t))-.5)
                         *(.3+hash(uv+t)*.7))*step(length(uvm),.2);
		return;
    }

    // get the data of a particle arriving at this pixel 
    vec4 partData;
    float p = arrivingParticle(fragCoord, partData);
   
    // no particles, empty pixel
    if (p<1.) {
    	fragColor = vec4(0.);
        return;
    }
    
    partData.xy+=partData.zw;
    partData.zw*=fade;

    //set particle data
    fragColor = partData;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
float hash(vec2 p)
{
   return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}