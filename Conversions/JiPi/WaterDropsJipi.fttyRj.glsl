

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const float PI = 3.14159265359;
#define time (-iTime*5.0)

const vec3 eps = vec3(0.01, 0.0, 0.0);

float genWave1(float len)
{
	float wave = sin(8.0 * PI * len + time);
	wave = (wave + 1.0) * 0.5; // <0 ; 1>
	wave -= 0.3;
	wave *= wave * wave;
	return wave;
}

float genWave2(float len)
{
	float wave = sin(7.0 * PI * len + time);
	float wavePow = 1.0 - pow(abs(wave*1.1), 0.8);
	wave = wavePow * wave; 
	return wave;
}

float scene(float len)
{
	// you can select type of waves
	return genWave1(len);
}

vec2 normal(float len) 
{
	float tg = (scene(len + eps.x) - scene(len)) / eps.x;
	return normalize(vec2(-tg, 1.0));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	vec2 so = iMouse.xy / iResolution.xy;
	vec2 pos2 = vec2(uv - so); 	  //wave origin
	vec2 pos2n = normalize(pos2);

	float len = length(pos2);
	float wave = scene(len); 

	vec2 uv2 = -pos2n * wave/(1.0 + 5.0 * len);

	fragColor = vec4(texture(iChannel0, uv + uv2));
}