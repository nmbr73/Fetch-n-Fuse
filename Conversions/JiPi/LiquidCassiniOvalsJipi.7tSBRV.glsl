

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float t = 0.5 * (1. + sin(iTime * 0.53));
    float t2 = 0.5 * (1. + sin(iTime * 0.31));
    float t3 = 0.5 * (1. + sin(iTime * 0.37));
    float t4 = 0.5 * (1. + sin(iTime * 0.29));
    vec2 p = fragCoord;
    float maxDist = iResolution.y;
    float minX = iResolution.x / 6.;
    float xRange = iResolution.x - minX * 2.;
    float minY = iResolution.y / 6.;
    float yRange = iResolution.y - minY * 2.;
    vec2 a = vec2(minX + xRange * (1. - t2), iResolution.y / 3.);
    vec2 b = vec2(minX + xRange * t2, 2. * iResolution.y / 3.);
    vec2 c = vec2(iResolution.x / 2., minY + yRange * t3);
    vec2 e = vec2(iResolution.x / 2.,  minY + yRange * (1. - t4));
    float r1 = length(p - a);
    float r2 = length(p - b);
    float r3 = length(p - c);
    float r4 = length(p - e);
    float d = iResolution.x / 10. + iResolution.x / 10. * abs(sin(iTime * 0.5));
    float mx = iMouse.x / iResolution.x;
    float my = iMouse.y / iResolution.y;

    
    float distF = (1. - r1 * r2 * r3 * r4 / (d * d * d * d));
    float f = abs(0.5 - distF) / 0.5;
    f = pow(f, 0.3 + 2. * mx);
    float len = length(uv - vec2(0.5, 0.5));
    //vec3 col = vec3(f < 0.5 + 0.5 * mx && f > 0.5 * my ? 1. : 0.);
    vec3 col = vec3(0.7 - f * 0.7, 1. - f, max(0., 0.3 + 0.7 - f * 0.7)); //  //, 0., f);

    fragColor = vec4(col,1.0);
}