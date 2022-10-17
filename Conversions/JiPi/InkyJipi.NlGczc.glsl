

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Music is Save Me by Majik: https://soundcloud.com/majikband/save-me-majik-2

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    // read for accum buffer
    fragColor = 1. - textureLod( iChannel0, uv, 0. );
    
    // tint
    fragColor.xyz *= .8;
    fragColor.xyz += 1.5*vec3(.15,.3,.8);
    
    // vign, treatment
    fragColor *= 1. - .17*length(2. * uv - 1.);
    fragColor.xyz = clamp(fragColor.xyz,0.,1.);
    fragColor.xyz *= 0.5 + 0.5*pow( 16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.1 );
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// sorry this is a mess right now! i will write up a blog post about what the code
// below was written to do and update this later.

float ten_r = 0.04;

float GetAngle( float i, float t )
{
    //float amp = .75; //.25 is subtle wriggling
	//t += amp*sin(1.*t + 4.*iTime + float(i));
    float dir = mod(i,2.) < 0.5 ? 1. : -1.;
    return dir * (1./(.5*i+1.)+1.) * t + i/2.;
}

#define POS_CNT 5
vec2 pos[POS_CNT];

float Potential( int numNodes, vec2 x )
{
    if( numNodes == 0 ) return 0.;
    
    float res = 0.;
    float k = 16.;
    for( int i = 0; i < POS_CNT; i++ )
    {
        if( i == numNodes ) break;
        res += exp( -k * length( pos[i]-x ) );
    }
    return -log(res) / k;
}

void ComputePos_Soft( float t )
{
    for( int i = 0; i < POS_CNT; i++ )
    {
        float a = GetAngle( float(i), t );
        vec2 d = vec2(cos(a),sin(a));
        float r = ten_r;
        
        for( int j = 0; j < 3; j++ )
        {
            r += ten_r-Potential(i,r*d);
        }
        
        pos[i] = r * d;
    }
}


vec3 drawSlice( vec2 uv )
{
    float t = iTime/2.;
    ComputePos_Soft(t);
    float pot = Potential(POS_CNT,uv);
    return vec3(smoothstep(0.03,0.01,pot));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    // sample from previous frame, with slight offset for advection
    fragColor = textureLod( iChannel0, uv*.992, 0. );
    
    // clear on first frame (dont know if this is required)
    if( iFrame == 0 ) fragColor = vec4(0.);
    
    // camera
    uv.x += .1*sin(.7*iTime);
    uv.y += .05*sin(.3*iTime);
    uv = 2. * uv - 1.;
    uv.x *= iResolution.x/iResolution.y;
    
    // draw spots
    vec3 spots = drawSlice( uv );
    
    // accumulate
    fragColor.rgb = fragColor.rgb*.95 + spots;
}
