

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float modValue = 512.0f;

float permuteX(float x)
{
    float t = ((x * 67.0) + 71.0) * x;
	return mod(t, modValue);
}

float permuteY(float x)
{
    float t = ((x * 73.0) + 83.0) * x;
	return mod(t, modValue);
}

float shiftX(float value)
{
    return fract(value * (1.0 / 73.0));
}

float shiftY(float value)
{
    return fract(value * (1.0 / 69.0));
}

vec2 rand(vec2 v)
{
    v = mod(v, modValue);
    float rX = permuteX(permuteX(v.x) + v.y);
    float rY = permuteY(permuteY(v.x) + v.y);
    return vec2(shiftX(rX), shiftY(rY));
}

float worleyNoise(vec2 uv)
{
    vec2 p = floor(uv);
    vec2 f = fract(uv);
    float dis = 1e9f;
    int range = 1;
    
    vec2 findPos, findJitPos;
    for(int i = -range; i <= range; i++)
    {
        for(int j = -range; j <= range; j++)
        {
            vec2 b = vec2(i, j);
            vec2 jitPos = b - f + rand(p + b);
            float len = dot(jitPos, jitPos);
            if (dis > len)
            {
				dis = len;
                findPos = b;
                findJitPos = jitPos;
            }
        }
    }
    
    dis = 1e9f;
    range = 2;
    for(int i = -range; i <= range; i++)
    {
        for(int j = -range; j <= range; j++)
        {
            vec2 b = findPos + vec2(i, j);
            vec2 jitPos = b - f + rand(p + b);
            float len = dot((findJitPos + jitPos) * 0.5f, normalize(jitPos - findJitPos));
            
            if (dis > len)
            {
                dis = len;
            }
        }
    }
    
   	//return smoothstep( 0.0, 0.5, dis );
    return dis;
}

vec2 noise(vec2 uv)
{
    vec2 p = floor(uv);
    vec2 f = fract(uv);
    
    vec2 v = f * f * (3.0 - 2.0 * f);
    
    vec2 result = mix(
        mix(rand(p + vec2(0.0f, 0.0f)), rand(p + vec2(1.0f, 0.0f)), v.x), 
		mix(rand(p + vec2(0.0f, 1.0f)), rand(p + vec2(1.0f, 1.0f)), v.x), 
    v.y);
    
    return result - 0.5f;
}

vec2 fbm(vec2 uv)
{
    vec2 res = vec2(0.0f);
    float s = 0.25f;
    for(int i = 0; i < 5; i++)
    {
		uv *= 2.0f;
        res += s * noise(uv);
        s *= 0.5f;
    }
    return res;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.yy;
    uv *= 4.0f;
    uv += 1.0f;
    
    vec2 I = floor(uv/2.); 
    bool vert = mod(I.x+I.y,2.)==0.; 
    
    float result;
    for(float i = 0.0; i < 5.0; i += 01.50)
    {
        uv *= 1.5f;
    	vec2 jituv = fbm(uv * 1.5f);
        float col = worleyNoise(uv + jituv);
        
        float ground = min(1.0, 1.8 * pow(col, 0.22f));
        result += (1.0 - ground) / exp2(i);
    }

    //if (vert) result = 1.-result; 
    
    vec4 tex = texture(iChannel0, fragCoord/iResolution.xy);
    
    fragColor = result + tex;
    

    //fragColor = vec4(vec3(result), 1.0f);
} 