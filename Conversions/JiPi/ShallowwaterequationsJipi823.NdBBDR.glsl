

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<


vec2 screen2uv(in vec2 fragCoord)
{
    return fragCoord / iResolution.xy;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = screen2uv(fragCoord);
    float height = texture(iChannel0, uv).x;
	fragColor = 0.5+vec4(height)*0.5;
	//fragColor = 0.5 + 0.5*texture(iChannel1, uv)*100.0;
    
    return;
    
    float t = dot(normalize(vec3(dFdx(height), dFdy(height), 1.)), normalize(vec3(1.)));
    t = max(0., t);
    
	fragColor = vec4(t);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define Dt	0.5
#define Dx	0.01
#define H	1.0

#define RADIUS		.06
#define INTENSITY	.2

#define DX	vec2(1.0/iResolution.x, 0.0)
#define DY	vec2(0.0, 1.0/iResolution.y)


vec2 screen2world(in vec2 fragCoord)
{
    return (2.0*fragCoord.xy - iResolution.xy)/iResolution.y;
}

vec2 world2screen(in vec2 pos)
{
    return (pos*iResolution.y + iResolution.xy) * 0.5;
}

vec2 screen2uv(in vec2 fragCoord)
{
    return fragCoord / iResolution.xy;
}

vec2 uv2screen(in vec2 uv)
{
    return uv * iResolution.xy;
}

vec2 world2uv(in vec2 pos)
{
    return world2screen(pos) / iResolution.xy;
}

vec2 uv2world(in vec2 uv)
{
    return screen2world(uv2screen(uv));
}

float brushIntensity(float r)
{
    if(r/RADIUS <0.707)
        return INTENSITY;
    return -INTENSITY;
}


float PointSegDistance2(in vec2 p, in vec2 p0, in vec2 p1)
{
    vec2 px0 = p-p0, p10 = p1-p0;
    float h = clamp(dot(px0, p10) / dot(p10, p10), 0.0, 1.0);
    return length(px0 - p10*h);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = screen2uv(fragCoord);
    
    vec2 pos = screen2world(fragCoord);
    
    
    // Retrieving mouse data
    bool mousePressed = iMouse.z > 0.0;
    bool prevMousePressed = texture(iChannel0, uv).y > 0.0;
    
    vec2 mousePos = screen2world(iMouse.xy);
    vec2 prevMousePos = texture(iChannel0, uv).zw;
    
    
    
    float prev_eta = texture(iChannel0, uv).x;
    
    //float dev_speed_x = 0.5*(texture(iChannel1, uv + vec2(1,0)/iResolution.xy).x - texture(iChannel1, uv + vec2(-1,0)/iResolution.xy).x);
    //float dev_speed_y = 0.5*(texture(iChannel1, uv + vec2(0,1)/iResolution.xy).y - texture(iChannel1, uv + vec2(0,-1)/iResolution.xy).y);
    
    
    float dev_speed_x = texture(iChannel1, uv+DX).x - texture(iChannel1, uv-DX).x;
    float dev_speed_y = texture(iChannel1, uv+DY).y - texture(iChannel1, uv-DY).y;
    
    
    float eta = .99*prev_eta - Dt * (dev_speed_x+dev_speed_y);
    
    fragColor = vec4(eta);
    
    
    if (iMouse.x < 10.0) 
    {
        //float dist = length(vec2(cos(float(iFrame) * 0.03), 0.7*sin(2.0*float(iFrame) * 0.03))*vec2(1.2,0.6) - pos);
        vec2 p1 = vec2(cos(float(iFrame) * 0.03), 0.7*sin(2.0*float(iFrame) * 0.03))*vec2(1.2,0.6);
        vec2 p0 = vec2(cos(float(iFrame-1) * 0.03), 0.7*sin(2.0*float(iFrame-1) * 0.03))*vec2(1.2,0.6);
        float dist = PointSegDistance2(pos, p0, p1);
        if (dist < RADIUS) 
        {
            fragColor += vec4(brushIntensity(dist));
            //fragColor -= vec4(brushIntensity(length(p1-pos)));
        }
    } 
    else 
    {
       
        //fragColor = vec4(0.0);
        if(mousePressed && prevMousePressed)
        {
            //float dist = length(screen2world(iMouse.xy)-pos);
            float dist = PointSegDistance2(pos, mousePos, prevMousePos);
            if(dist < RADIUS)
            {
            	fragColor += vec4(brushIntensity(dist));
            	//fragColor -= vec4(brushIntensity(length(mousePos-pos)));
            }
        }
    }
    
    // Back-uping mouse data
    fragColor.y = iMouse.z;
    fragColor.zw = mousePos;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define Dt	0.5
#define Dx	0.01
#define B	0.02
#define G	1.0

#define DX	ivec2(1, 0)
#define DY	ivec2(0, 1)


vec2 screen2world(in vec2 fragCoord)
{
    return (2.0*fragCoord.xy - iResolution.xy)/iResolution.y;
}

vec2 world2screen(in vec2 pos)
{
    return (pos*iResolution.y + iResolution.xy) * 0.5;
}

vec2 screen2uv(in vec2 fragCoord)
{
    return fragCoord / iResolution.xy;
}

vec2 uv2screen(in vec2 uv)
{
    return uv * iResolution.xy;
}

vec2 world2uv(in vec2 pos)
{
    return world2screen(pos) / iResolution.xy;
}

vec2 uv2world(in vec2 uv)
{
    return screen2world(uv2screen(uv));
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = screen2uv(fragCoord);
    
    
    vec2 dev_height;
    //dev_height.x = texelFetch(iChannel0, ivec2(fragCoord)+DX, 0).x - texelFetch(iChannel0, ivec2(fragCoord)-DX, 0).x;
    dev_height.x = texture(iChannel0, (vec2(ivec2(fragCoord)+DX)+0.5)/iResolution.xy).x 
                 - texture(iChannel0, (vec2(ivec2(fragCoord)-DX)+0.5)/iResolution.xy).x;
    
    //dev_height.y = texelFetch(iChannel0, ivec2(fragCoord)+DY, 0).x - texelFetch(iChannel0, ivec2(fragCoord)-DY, 0).x;
    dev_height.y = texture(iChannel0, (vec2(ivec2(fragCoord)+DY)+0.5)/iResolution.xy).x 
                 - texture(iChannel0, (vec2(ivec2(fragCoord)-DY)+0.5)/iResolution.xy).x;
    
    
    //vec2 prev_speed = texelFetch(iChannel1, ivec2(fragCoord), 0).xy;
    vec2 prev_speed = texture(iChannel1, (vec2(ivec2(fragCoord))+0.5)/iResolution.xy).xy;
    
    
    vec2 new_speed = prev_speed - Dt*(G*dev_height + B * prev_speed);
    
    
    //*
    // Reflexions
    if(fragCoord.x < 0.5)
    {
        new_speed.x = abs(new_speed.x);
    }
    
    if(fragCoord.y < 0.5)
    {
        new_speed.y = abs(new_speed.y);
    }
    if(fragCoord.x > iResolution.x-1.5)
    {
        new_speed.x = -abs(new_speed.x);
    }
    
    if(fragCoord.y > iResolution.y-1.5)
    {
        new_speed.y = -abs(new_speed.y);
    }
	//*/
    
    fragColor = vec4(new_speed, vec2(0.0));
}
