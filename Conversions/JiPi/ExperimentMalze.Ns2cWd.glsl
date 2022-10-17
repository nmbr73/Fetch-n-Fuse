

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Voronoi Tracking Experiment 1

// based on the shader https://www.shadertoy.com/view/tlKGDh of michael0884

// use mouse for add particles
// use spacebar for clear the screen

vec4 getPheromone(vec2 p)
{
	return texelFetch(iChannel1, ivec2(loop(p, iResolution.xy)), 0);
}

vec4 getPheromoneInv(vec2 p)
{
	return getPheromone(p);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float e = 100.0 / min(iResolution.x, iResolution.y);
	float f = getPheromoneInv(fragCoord).x;
	float fx = (f-getPheromoneInv(fragCoord + vec2(1,0)).x)/e;
	float fy = (f-getPheromoneInv(fragCoord + vec2(0,1)).x)/e;
	vec3 n = normalize(vec3(0,0,1) - vec3(fx,fy,0.0));
	
	float diff = max(dot(vec3(0,0,1), n), 0.0);
	float spec = pow(max(dot(normalize(lightDirection), reflect(vec3(0,0,1),n)), 0.0), specularPower);
		
    fragColor.rgb = lightDiffuse * diff + lightSpecular * spec; 
	fragColor.rgb *= 1.5;
	
    //fragColor = getPheromone(fragCoord);
    
	fragColor.a = 1.0;
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// particle
// x,y => pos [0 > iResolution.xy]
// z   => angle [0 > 2pi]
// abs(w) => sensor angle [0 > pi]
// if (w > 0.0) => activation

vec4 getParticle(vec2 p)
{
	return texelFetch(iChannel0, ivec2(loop(p, iResolution.xy)), 0);
}

vec4 getPheromone(vec2 p)
{
	return texelFetch(iChannel1, ivec2(loop(p, iResolution.xy)), 0);
}

void SelectIfNearestNeighbor(inout vec4 pnb, vec2 p, vec2 dx)
{
    vec4 p_nb = getParticle(p + dx);
    
	if(length(loop_d(p_nb.xy - p, iResolution.xy)) < length(loop_d(pnb.xy - p, iResolution.xy)))
    {
        pnb = p_nb;
    }
}

void SearchForNearestNeighbor(inout vec4 pnb, vec2 p, float ring)
{
	// sides
    SelectIfNearestNeighbor(pnb, p, vec2(-ring,0));
    SelectIfNearestNeighbor(pnb, p, vec2(ring,0));
    SelectIfNearestNeighbor(pnb, p, vec2(0,-ring));
    SelectIfNearestNeighbor(pnb, p, vec2(0,ring));
	
	// corners
	SelectIfNearestNeighbor(pnb, p, vec2(-ring));
    SelectIfNearestNeighbor(pnb, p, vec2(ring,-ring));
    SelectIfNearestNeighbor(pnb, p, vec2(-ring,ring));
    SelectIfNearestNeighbor(pnb, p, vec2(ring));
}

void EmitParticle(vec2 g, inout vec4 p)
{
	float rand = Random(g + p.xy).x;
	
	p.xy = g; // pos
	p.z = rand * 6.28318; // angle

    // sensor angle and activation
    p.w = mix(sensor_angle_rad_inf, sensor_angle_rad_sup, rand);
}

void MoveParticle(inout vec4 p)
{
    // left sensor
	float an = p.z + p.w;
    vec2 sleft = p.xy + sensor_distance * vec2(cos(an), sin(an));
    
    // right sensor
	an = p.z - p.w;
    vec2 sright = p.xy + sensor_distance * vec2(cos(an), sin(an));
    
    float diff_angle = 
        getPheromone(sleft).x - 
        getPheromone(sright).x;
	
    p.z += dt * sensor_strenght * tanh(0.3 * diff_angle);
	p.xy += dt * particle_speed * vec2(cos(p.z), sin(p.z));
    
	p.xy = loop(p.xy, iResolution.xy);
}

void PaintByMouse(vec2 g, inout vec4 p)
{
	if (iMouse.z > 0.0)
	{
		if (length(g - iMouse.xy) < uMouseRadius)
		{
			EmitParticle(g, p);
		}
	}
}

void mainImage( out vec4 fragParticles, in vec2 fragCoord )
{
	fragParticles = getParticle(fragCoord);
	
	SearchForNearestNeighbor(fragParticles, fragCoord, 1.0);
    SearchForNearestNeighbor(fragParticles, fragCoord, 2.0);
    SearchForNearestNeighbor(fragParticles, fragCoord, 3.0);
	
	MoveParticle(fragParticles);
	PaintByMouse(fragCoord, fragParticles);

	if (iFrame < 1) // reset 
	{
		fragParticles = vec4(0);
        
        // start shape
        vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;
        uv.y += sin(uv.x * 5.0) * 0.3;
        uv.x = mod(uv.x, 0.1);
        float st = 5.0 / iResolution.y;
        if (length(uv) < st)
            EmitParticle(fragCoord, fragParticles);
	}
    
    if (reset(iChannel3))
        fragParticles = vec4(0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define dt 0.25

// mouse
const float uMouseRadius = 1.0;

// particle
const float particle_speed = 5.0;

// pheromones
const float gauss_coef = 1.4;
const float decay = 0.15;

// sensor
const float sensor_strenght = 20.0;
const float sensor_distance = 20.0;
const float sensor_angle_rad_inf = 1.0;
const float sensor_angle_rad_sup = 1.8;

// shading
const vec3 lightDiffuse = vec3(0.191553,0.267195,0.373984);
const vec3 lightSpecular = vec3(0.243903,1,0);
const vec3 lightDirection = vec3(0.08232,-0.24085,-0.58841);
const float specularPower = 20.0;

// borderless 
vec2 loop_d(vec2 p, vec2 s){
	return mod(p + s * 0.5, s) - s * 0.5;
}

vec2 loop(vec2 p, vec2 s){
	return mod(p, s);
}

vec2 Random(vec2 p){
	vec3 a = fract(p.xyx * vec3(123.34,234.35,345.65));
	a += dot(a, a + 34.45);
	return fract(vec2(a.x * a.y, a.y * a.z));
}

bool reset(sampler2D sam) {
    return texture(sam, vec2(32.5/256.0, 0.5) ).x > 0.5;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// pheromone
// x   => pheromone quantity

vec4 getParticle(vec2 p)
{
	return texelFetch(iChannel0, ivec2(loop(p, iResolution.xy)), 0);
}

vec4 getPheromone(vec2 p)
{
	return texelFetch(iChannel1, ivec2(loop(p, iResolution.xy)), 0);
}

void DiffusePheromones(vec2 g, inout vec4 fragPheromone)
{
    // laplacian
	float v = 0.0;
    v += getPheromone(g + vec2(-1, 0)).x; // l
	v += getPheromone(g + vec2( 0, 1)).x; // t
	v += getPheromone(g + vec2( 1, 0)).x; // r
	v += getPheromone(g + vec2( 0,-1)).x; // b
	v -= 4.0 * fragPheromone.x;
    
	fragPheromone += dt * v;
}

void mainImage( out vec4 fragPheromone, in vec2 fragCoord )
{
    fragPheromone = getPheromone(fragCoord);
    
    DiffusePheromones(fragCoord, fragPheromone);
	
	// write pheromones for each particles
	vec4 p = getParticle(fragCoord);
	if (p.w > 0.0)
	{
		float gauss = exp(-pow(length(fragCoord - p.xy)/gauss_coef,2.));
		fragPheromone += dt * gauss;
	}
	
	// dissipation  
	fragPheromone -= dt * decay * fragPheromone;
    
    if (iFrame < 1 || reset(iChannel3)) // reset 
		fragPheromone = vec4(0);
}