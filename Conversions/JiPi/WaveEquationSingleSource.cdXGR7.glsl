

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texelFetch(iChannel0,ivec2(fragCoord),0)*.1+vec4(.5);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(0.,0.,0.,1.);
    vec2 waveSrc = iMouse.xy / vec2(iResolution.y);
    if (iMouse.x == 0. && iMouse.y == 0.)
    {
        float s = 0.4;
        waveSrc = .3 * vec2(cos(iTime*s),sin(2.*iTime*s))  + iResolution.xy/iResolution.y*.5;
    }
    float waveSrcSize = .005;
    float waveSrcFreq = 5.1;
    //float waveLength = 2.;
    float waveSpeed = 3.3;
    float dampRate = 0.3;
    
    vec2 uv = fragCoord / iResolution.y;
    
    float d2 = dot(uv - waveSrc,uv-waveSrc);
    fragColor.xy = exp(-d2/(waveSrcSize*waveSrcSize)) * vec2(sin(waveSrcFreq*iTime),waveSrcFreq*cos(waveSrcFreq*iTime));
    
    vec4 prevPixel = texelFetch(iChannel0,ivec2(fragCoord),0);
    float h = prevPixel.x;
    float dh_dt = prevPixel.y;
    
    float L = 0.;
    for ( int i=0;i<5;++i)
    {
        for ( int j=0;j<5;++j)
        {
            L -= texelFetch(iChannel0,ivec2(int(fragCoord.x)+i-2,int(fragCoord.y)+j-2),0).x;
        }
    }
    L += 25.*h;
    
    float d2h_dt2 = -L*(waveSpeed*waveSpeed);
    dh_dt += d2h_dt2 * iTimeDelta;
    h += dh_dt * iTimeDelta;
    
    h *= exp(-iTimeDelta*dampRate);
    
    fragColor.x += h;
    fragColor.y += dh_dt;
}