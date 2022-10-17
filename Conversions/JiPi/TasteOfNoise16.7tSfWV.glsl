

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Taste of Noise 16 by Leon Denise 2022-05-17

// An experiment of lighting with a 3D FBM noise.
// Trying to render organic volumes without raymarching.
// Clic to display the diffent layers, which are from left to right: 
// height, normal, noise, glow, lighting and shape

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // coordinates
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    // value from noise buffer A
    vec3 noise = texture(iChannel0, uv).rgb;
    float gray = noise.x;
    
    // gradient normal from gray value
    vec3 unit = vec3(5./iResolution.xy,0);
    vec3 normal = normalize(vec3(
        TEX(uv + unit.xz)-TEX(uv - unit.xz),
        TEX(uv - unit.zy)-TEX(uv + unit.zy),
        gray*gray));
    
    vec3 dir = normalize(vec3(0,1,.2)); // light direction
    float angle = dot(normal, dir); // light and surface angle
    vec3 color = vec3(.2); // ambient
    float light = pow(angle*.5+.5,10.);
    float soft = .5*smoothstep(.0,.2,gray-.75);
    float glow = .1/(noise.y*noise.y)*noise.z;
    vec3 tint = .5+.5*cos(vec3(1,2,3)+length(uv)*3.+iTime+angle); // iq palette
    color += vec3(1)*light; // specular light
    color += soft; // soft white
    color += tint*glow; // glow rainbow
    color *= gray; // shadows
    
    // display layers when clic
    if (iMouse.z > 0.5)
    {
        if (uv.x < .16) color = vec3(gray);
        else if (uv.x < .33) color = normal*.5+.5;
        else if (uv.x < .5) color = vec3(1.-noise);
        else if (uv.x < .66) color = vec3(glow);
        else if (uv.x < .86) color = vec3(.2+light)*gray;
        else color = vec3(soft);
        if (uv.y < .02) color = .5+.5*cos(vec3(1,2,3)+uv.x*3.+iTime);
    }

    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// The noise animated pass with shape and glow layers

const float speed = .01;
const float scale = 0.08;
const float cycle = 1.5;
const float falloff = 1.8;

// transform linear value into cyclic absolute value
vec3 bend(vec3 v)
{
    return abs(sin(v*cycle*6.283+iTime*6.283*speed*10.));
}

// fractal brownian motion (layers of multi scale noise)
vec3 fbm(vec3 p)
{
    vec3 result = vec3(0);
    float amplitude = 0.5;
    for (float index = 0.; index < 4.; ++index)
    {
        result += bend(texture(iChannel0, p/amplitude).xyz) * amplitude;
        amplitude /= falloff;
    }
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // noise from coordinates
    vec2 uv = (fragCoord.xy - iResolution.xy / 2.)/iResolution.y;
    vec3 noise = fbm(vec3(uv, iTime * speed) * scale);
    
    // fade noise with circle
    noise.x -= .5*smoothstep(.3,0.,abs(length(uv)-.6));
    
    // keyhole shape
    float shape = 10.;
    shape = min(shape, max(0.,length(uv-vec2(0,.05))-.07));
    uv.y += .1;
    shape = min(shape, max(0., moda(uv*rot(1.57), 3.).r-.05));
    
    // add shape to soft white
    noise.x += .5*smoothstep(.1,0.,abs(shape-.03));
    
    // add shape to glow
    noise.y *= smoothstep(.0,.1,abs(shape-.01));
    
    // remove shape
    float hole = smoothstep(.04,.0,shape+.02);
    noise.x -= hole*1.9;
    noise.y += hole;
    
    fragColor = vec4(clamp(noise, 0., 1.), 1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// shortcut to sample texture
#define TEX(uv) texture(iChannel0, uv).r

// polar domain repetition used for the triangle
vec2 moda(vec2 p, float repetitions)
{
	float angle = 2.*3.14/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	a = mod(a,angle) - angle/2.;
	return vec2(cos(a), sin(a))*length(p);
}

// rotation matrix
mat2 rot (float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }