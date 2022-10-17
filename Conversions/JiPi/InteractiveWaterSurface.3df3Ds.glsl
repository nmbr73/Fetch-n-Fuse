

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 c = texelFetch(iChannel0, ivec2(fragCoord), 0);
    
    // normal
    float dhx = dFdx(c.x);
    float dhy = dFdy(c.x);
    vec3  n   = vec3(dhx, dhy, 1.0);
    n=normalize(n);
    
    vec3  light   = normalize(vec3(3,-4,5));
    float diffuse = dot(light, n);
    float spec    = pow(max(0.,-reflect(light, n).z),100.);
    
    vec3 l = vec3(max(diffuse,0.0) + spec);
    vec3 hv = vec3((c.xy + 1.0) / 2.0, 0.5);
    hv = clamp(hv, 0.0, 1.0);
    l = (abs(fragCoord.x - (iResolution.x*0.4+0.5)) < 1.0)  ? vec3(0.0) : l;
    fragColor = vec4(mix(hv, l, 0.9), 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 xy = ivec2(fragCoord);
    ivec2 sxy = iMouse.z > 0.0 ? ivec2(iMouse.xy) : (iTime<5.0 ? ivec2(vec2(0.66,0.5)*iResolution.xy) : ivec2(100000));
    
    // get velocity from buffer
    vec2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution.xy);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution.xy);
    
    fragColor = vec4(hv, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 xy = ivec2(fragCoord);
    ivec2 sxy = iMouse.z > 0.0 ? ivec2(iMouse.xy) : (iTime<5.0 ? ivec2(vec2(0.66,0.5)*iResolution.xy) : ivec2(100000));
    
    // get velocity from buffer
    vec2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution.xy);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution.xy);
    
    fragColor = vec4(hv, 0.0, 0.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//#define LENS

const float K_OVER_M = 700.0;

const float ATTENUATION = 0.999;

const float AMPLITUDE = 5.0;

const float PULSATION = 40.0;

const float DT = 0.012;

const float MARGIN = 50.0;

//--------------------------------------------
//--------------------------------------------
float elasticity(ivec2 xy, vec2 rxy)
{
#ifdef LENS
    vec2 delta = vec2(xy) - vec2(0.5,0.5)*rxy;
    float r = rxy.x * 0.2;
    return dot(delta,delta) > r*r ? 2.0 : 1.0;
#else
    return xy.x > int(rxy.x*0.4) ? 3.0 : 1.0;
#endif
}

//--------------------------------------------
//--------------------------------------------
float height(sampler2D HV, ivec2 sxy, ivec2 xy, float t)
{
    float s = 1.0 - smoothstep(0.0, 12.0, length(vec2(sxy - xy)));
    
    float h = texelFetch(HV, xy, 0).x;
    h += s * AMPLITUDE * sin(PULSATION * t);
    return h;
}

//--------------------------------------------
//--------------------------------------------
float margin(vec2 xy, vec2 wh)
{
    float a0 = smoothstep(0.0, MARGIN, xy.x);
    float a1 = smoothstep(0.0, MARGIN, xy.y);  a0 = min(a0, a1);
    a1 = smoothstep(0.0, MARGIN, wh.x - xy.x); a0 = min(a0, a1);
    a1 = smoothstep(0.0, MARGIN, wh.y - xy.y); a0 = min(a0, a1);
    return pow(a0, 0.1);
}

//--------------------------------------------
//--------------------------------------------
float acceleration(sampler2D HV, ivec2 sxy, ivec2 xy, float t, vec2 rxy)
{
	// get height and velocity from buffer B
    float h = height(HV, sxy, xy, t);
    
    // get heights of neighbours
    float h0 = height(HV, sxy, xy - ivec2(1,0), t);
    float h1 = height(HV, sxy, xy + ivec2(1,0), t);
    float h2 = height(HV, sxy, xy - ivec2(0,1), t);
    float h3 = height(HV, sxy, xy + ivec2(0,1), t);
    
    // sum of (hi - h) is proportional to the elastic force
    float delta_h = h0 + h1 + h2 + h3 - 4.0 * h;
    
    // acceleration = (k / mass) * delta_h    
    float a = K_OVER_M * delta_h;
    
    return elasticity(xy, rxy) * a;
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 xy = ivec2(fragCoord);
    ivec2 sxy = iMouse.z > 0.0 ? ivec2(iMouse.xy) : (iTime<5.0 ? ivec2(vec2(0.66,0.5)*iResolution.xy) : ivec2(100000));
    
    // get velocity from buffer
    vec2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution.xy);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution.xy);
    
    fragColor = vec4(hv, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 xy = ivec2(fragCoord);
    ivec2 sxy = iMouse.z > 0.0 ? ivec2(iMouse.xy) : (iTime<5.0 ? ivec2(vec2(0.66,0.5)*iResolution.xy) : ivec2(100000));
    
    // get velocity from buffer
    vec2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution.xy);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution.xy);
    
    fragColor = vec4(hv, 0.0, 0.0);
}