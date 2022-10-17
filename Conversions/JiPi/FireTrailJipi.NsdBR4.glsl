

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define scale 10. 
vec2 rand22(vec2 seed)
{
  return fract(vec2(sin(dot(seed,vec2(12.788,72.133))),sin(dot(seed,vec2(12.788,72.133)))) * 522734.567);
}

float voronoise(vec2 uv)
{
    vec2 f = fract(uv);
    f -= .5;
    vec2 i = floor(uv);
    float dist = distance(f,rand22(i) - .5);
    for(int x=-1; x<=1; x++)
    {
        for(int y=-1; y<=1; y++)
        {
            vec2 p = i + vec2(x,y);
            float nDist = distance(f,rand22(p) + vec2(x,y)  - .5);
            if(nDist < dist){
                dist = nDist;
            }
        }
    }
    return dist;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 uv = (fragCoord - .5 * vec2(iResolution.x,0.))/iResolution.y;
    vec2 unscaledUV = (fragCoord - .5 * vec2(iResolution.x,0.))/iResolution.y;
    vec2 texUV = fragCoord/iResolution.xy;
    vec2 m = (iMouse.xy - .5 * vec2(iResolution.x,0.))/iResolution.y;
    uv *= scale;
    float t = iTime * .7;
    float f = 0.;
    //fire noise
    f += voronoise(uv * vec2(3.,1.) - (vec2(-2.5,1.) * t));
    f += voronoise((uv + vec2(13.1312, 1555.23)) * vec2(3.,1.) - (vec2(2.5,1.) * t));
    f = sqrt(f + .1);
    f *= voronoise(uv - (vec2(0.,5.) * t))/2.;
    
    //fire mask
    f *= pow(texture(iChannel0,texUV).x,.8);
    //fill circle in center of fire
    f += clamp(1. - distance(unscaledUV, m)*16.,0.,1.);
    
    //color fire
    vec3 col = vec3(0.);
    col = mix(vec3(1.,5.,2.), vec3(1.,1.,1.), step(.96,f));
    col = mix(vec3(1.,.15,.15), col,smoothstep(.1,.9,f));
    col = mix(vec3(0.), col, step(.1,f));
    
    // Output to screen
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define acc 16384.
float projDist(vec2 p, vec2 a, vec2 b){

    vec2 pa = p - a;
    vec2 ba = b - a;
    
    vec2 t = clamp(dot(pa,ba)/dot(ba,ba),0.,1.) * ba - pa;
    return length(t);
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 texUV = fragCoord/iResolution.xy;
    vec2 uv = fragCoord/iResolution.y;
    vec2 m = iMouse.xy/ iResolution.y;
    
    texUV.y -= .003;
    
    vec4 read = texture(iChannel0, texUV);
    vec2 prev = vec2(float((int(read.w ) & 0xffff0000) >> 16 )/ acc,
                     float(int(read.w ) & 0xffff)/ acc );
    read.xyz *= .96;

    float write = float(((int(m.x * acc) & 0xffff) << 16 ) |
                        ((int(m.y * acc) & 0xffff)));
    
    vec3 col = vec3(0.);
    float size = projDist(uv,prev,m);
    
    col += 1. - smoothstep(-.0,.11,size);
    
    read.xyz += col;
    fragColor = vec4(read.xyz, write);
}