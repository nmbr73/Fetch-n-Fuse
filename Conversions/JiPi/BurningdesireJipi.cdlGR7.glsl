

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Original shader created by XT95 - flame
//AND
// Original shader created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// modified by JoÃ£o Portela. Heart3

//Wanted to try some shaders on an interactive GLSLEditor developed by me
//https://github.com/rakesh-malviya/GLSLEditor
//Please goto the link and try it


float noise(vec3 p) //Thx to Las^Mercury
{
        vec3 i = floor(p);
        vec4 a = dot(i, vec3(1., 57., 21.)) + vec4(0., 57., 21., 78.);
        vec3 f = cos((p-i)*acos(-1.))*(-.5)+.5;
        a = mix(sin(cos(a)*a),sin(cos(1.+a)*(1.+a)), f.x);
        a.xy = mix(a.xz, a.yw, f.y);
        return mix(a.x, a.y, f.z);
}

float sphere(vec3 p, vec4 spr)
{
        return length(spr.xyz-p) - spr.w;
}

float flame(vec3 p)
{
        float d = sphere(p*vec3(0.4,0.5,1.0), vec4(.0,-1.,.0,1.));
        return d + (noise(p+vec3(.0,iTime*2.,.0)) + noise(p*3.)*.5)*.25*(p.y) ;
}

float scene(vec3 p)
{
        return min(100.-length(p) , abs(flame(p)) );
}

vec4 raymarch(vec3 org, vec3 dir)
{
        float d = 0.0, glow = 0.0, eps = 0.02;
        vec3  p = org;
        bool glowed = false;

        for(int i=0; i<64; i++)
        {
                d = scene(p) + eps;
                p += d * dir;
                if( d>eps )
                {
                        if(flame(p) < .0)
                                glowed=true;
                        if(glowed)
                        glow = float(i)/64.;
                }
        }
        return vec4(p,glow);
}




void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (2.0*fragCoord.xy-iResolution.xy)/iResolution.y;
    p.y -= 0.25;

    // background color
    vec3 bcol = vec3(1.0,0.7,0.8-0.07*p.y)*(1.0-0.35*length(p));

    // animate
    float tt = abs(sin(iTime))*1.5;
    float ss = pow(tt,.2)*0.5 + 0.5;
    ss -= ss*0.2*sin(tt*6.2831*3.0)*exp(-tt*4.0);
    p *= vec2(0.5,1.5) + ss*vec2(0.5,-0.5);


    // shape
    float a = atan(p.x,p.y)/3.141593;
    float r = length(p);
    float h = abs(a);
    float d = (13.0*h - 22.0*h*h + 10.0*h*h*h)/(6.0-5.0*h);

        // color
        float s = 1.0-0.5*clamp(r/d,0.0,1.0);
        s = 0.75 + 0.75*p.x;
        s *= 1.0-0.25*r;
        s = 0.5 + 0.6*s;
        s *= 0.5+0.5*pow( 1.0-clamp(r/d, 0.0, 1.0 ), 0.1 );
        vec3 hcol = vec3(1.0,0.5*r,0.3)*s;

    vec3 col = mix( bcol, hcol, smoothstep( -0.01, 0.01, d-r) );

    vec2 vXT = -1.0 + 2.0 * fragCoord.xy / iResolution.xy;
    vXT.x *= iResolution.x/iResolution.y;

    vec3 orgXT = vec3(0., -2., 4.);
    vec3 dirXT = normalize(vec3(vXT.x*1.6, -vXT.y, -1.5));

    vec4 pXT = raymarch(orgXT, dirXT);
    float glowXT = pXT.w;

    vec4 colXT = mix(vec4(0.7,0.4,.1,1.), vec4(0.1,.5,1.,1.), pXT.y*(-0.02)+.4);

    fragColor = mix(vec4(col,1.0),colXT, pow(glowXT*2.,4.));

    //fragColor = vec4(col,1.0);
}


/*
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
        vec2 v = -1.0 + 2.0 * fragCoord.xy / iResolution.xy;
        v.x *= iResolution.x/iResolution.y;

        vec3 org = vec3(0., -2., 4.);
        vec3 dir = normalize(vec3(v.x*1.6, -v.y, -1.5));

        vec4 p = raymarch(org, dir);
        float glow = p.w;

        vec4 col = mix(vec4(1.,.5,.1,1.), vec4(0.1,.5,1.,1.), p.y*.02+.4);

        fragColor = mix(vec4(0.), col, pow(glow*2.,4.));
        //fragColor = mix(vec4(1.), mix(vec4(1.,.5,.1,1.),vec4(0.1,.5,1.,1.),p.y*.02+.4), pow(glow*2.,4.));

}
*/
