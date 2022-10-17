

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 P )
{
    vec2 uv = P/iResolution.xy;
    
    float mask = mask(texture(iChannel2, uv));
    vec4 c = texelFetch(iChannel0, ivec2(P), 0);
    vec4 col = (texture(iChannel2, uv))*mask;
    col *= 1.-mask;
    vec4 col1 = vec4(1.0, 0., 0.0, 0.0);
    vec4 col2 = vec4(1.0, 1.0, 1.0, 0.0);
    vec4 col3 = vec4(.2, .8, 1.0, 0.0);
    col1 = mix(col1, col3, clamp(c.y*1.5, 0.0, 1.0));
    float ln = min(length(c.xy)*.6, 1.0);
    fragColor = col + ln * (1.-mask) * mix(col1, col2, ln);
    float edge = .55-max(abs(uv.x - .5), abs(uv.y - .5));
    fragColor *=  1.+smoothstep(.41, 0.5, .49-edge)*155.*vec4(.2, 0.9, 0.6, 0.0);
    fragColor = min(fragColor, 1.0);
    vec4 bg = vec4(0.02, 0.0, uv.y * .07, 0.0)*.1;
    fragColor +=bg;
    fragColor = sqrt(fragColor);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 P )
{
    
    fragColor = vec4(0.0);
    
    if(iFrame < 5) return;
    
    vec2 uv = P.xy/iChannelResolution[0].xy;
    vec4 rgba  = texture(iChannel2, uv);
    
    float ba= smoothstep(0.49, 0.5, abs(uv.x - .5));
    float bb= smoothstep(0.49, 0.5, abs(uv.y - .5));

    if(mask(rgba) > 0.)
    {
        
        fragColor = vec4(0.0);
        fragColor.xy = texture(iChannel1, uv + iTime).rg;
        fragColor.x -= .5;
        fragColor.y *= 0.25;
        fragColor.zw = sign(fragColor.xy) * 1.1;
    }
    else
    {
        vec4 ct = texelFetch(iChannel0, ivec2(P) + ivec2( 0.0,  1.0), 0);
        vec4 cr = texelFetch(iChannel0, ivec2(P) + ivec2( 1.0,  0.0), 0);
        vec4 cl = texelFetch(iChannel0, ivec2(P) + ivec2(-1.0,  0.0), 0);
        vec4 cb = texelFetch(iChannel0, ivec2(P) + ivec2( 0.0, -1.0), 0);

        vec4 tr = texelFetch(iChannel0, ivec2(P) + ivec2(  1.0,  1.0), 0);
        vec4 tl = texelFetch(iChannel0, ivec2(P) + ivec2(  1.0, -1.0), 0);
        
        vec4 br = texelFetch(iChannel0, ivec2(P) + ivec2(  1.0, -1.0), 0);
        vec4 bl = texelFetch(iChannel0, ivec2(P) + ivec2( -1.0, -1.0), 0);
        
        vec4 c = texelFetch(iChannel0, ivec2(P), 0);
        vec4 incb = vec4(0.0);

        // incoming from topright
        if(tr.z <= -1.0 && tr.w <= -1.0)
        {
            tr.zw -= vec2(-1.0, -1.0);
            incb = tr;
        }

        // incoming from topleft
        else if(tl.z >= 1.0 && tl.w <= -1.0)
        {
            tl.zw -= vec2(1.0, -1.0);
            incb = tl;
        }
        
        // incoming from bottomright
        if(br.z <= -1.0 && br.w >= 1.0)
        {
            br.zw -= vec2(-1.0, 1.0);
            incb = br;
        }

        // incoming from bottomleft
        else if(bl.z >= 1.0 && bl.w >= 1.0)
        {
            bl.zw -= vec2(1.0, 1.0);
            incb = bl;
        }
        
        
        // incoming from bottom
        else if(cb.w >= 1.0 && abs(cb.z) < 1.0)
        {
            cb.w -= 1.0;
            incb = cb;
        }

        // incoming from top
        else if(ct.w <= -1.0 && abs(ct.z) < 1.0)
        {
            ct.w += 1.0;
            incb = ct;
        }
        
        // incoming from left
        else if(abs(cl.w) < 1.0 && cl.z >= 1.0)
        {
            cl.z -= 1.0;
            incb = cl;
        }

        // incoming from right
        else if(abs(cr.w) < 1.0 && cr.z <= -1.0)
        {
            cr.z += 1.0;
            incb = cr;
        }
        
        
        // delete from this cell if moved out
        if(abs(c.z) > 1.0 || abs(c.w) > 1.0) c = vec4(0.0);
        
         
        
        // add incoming one
        c += incb;
        
        // apply velocity
        c.zw += c.xy * iTimeDelta * 50.;
        if(dot(c, c) > 0. && bb <= 0. && ba <= 0.)
        {
            c.y -= iTimeDelta * iTimeDelta * 25.;
        }
        
        c.xy *= 0.98;
        fragColor = c;
    }

    // vignette
    vec2 edgeDir = (-(uv - 0.5));
    edgeDir.y += sin(uv.x*220. + iTime*2.1);
    edgeDir.x += cos(uv.y*220. + iTime*2.1);
    fragColor.xy += min(1.0, ba + bb)*edgeDir*.005;    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// https://www.shadertoy.com/view/XsXGDM

#define BIAS  4.
#define LUMIN vec3(.2126, .7152, .0722)

float mask(in vec4 fg)
{
	float sf = max(fg.r, fg.b);
	float k = clamp((fg.g - sf) * BIAS, 0., 1.);
	
	if (fg.g > sf) fg = vec4(dot(LUMIN, fg.xyz));
	
	return smoothstep(0.3, 0., (fg * k).r);    
}