

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define AA 1
#define EPS .001
#define MAX 50.

vec2 hash2(vec2 p)
{
    return fract(sin(p*mat2(98,-64,-73,69))*397.)*.8;
}
float height(vec2 p)
{
    return p.x+sin(p.y*.3)*3.-iTime;
}
float cell(vec3 p)
{
    vec2 f = floor(p.xz);
    float d = 4.;
    
    for(int X = -1; X<=1; X++)
    for(int Y = -1; Y<=1; Y++)
    {
        vec2 h = f+vec2(X,Y);
        h += hash2(h)-p.xz;
        
        vec3 c = vec3(h,p.y+1.);
        vec2 R = normalize(sin(c.xy+p.xz));
        mat2 r = mat2(R,-R.y,R);

        float off = height(p.xz+c.xy);
        c.z -= sqrt(abs(off))-1.;
        c.z = max(c.z,0.);

        float s = .13*smoothstep(-.2,.2,off);
        c.xy *= r;
        float w = .15;
        d = min(d, length(max(abs(c)-s,0.))+s-w);
    }
    
    return d;
}
float dist(vec3 p)
{
    return min(p.y+1.,cell(p));
}
vec3 normal(vec3 p)
{
    vec2 e = vec2(-2,2)*EPS;
    return normalize(dist(p+e.xxy)*e.xxy+dist(p+e.xyx)*e.xyx+
    dist(p+e.yxx)*e.yxx+dist(p+e.y)*e.y);
}
vec3 color(vec3 p,vec3 r)
{
    float off = height(p.xz);
    float s = smoothstep(-.2,.2,off);
    
    float l = cell(vec3(p.x,-2,p.z));
    float e = smoothstep(.02,0.,l);


    vec3 n = normal(p);
    float ao = clamp(dist(p+n*.2)/.2,.1,1.);
    vec3 sd = normalize(vec3(3,2,-1));
    float dl = max(.3+.7*dot(n,sd),0.);
    float sl = max(dot(reflect(r,n),sd)*1.2-1.,0.);
    
    for(float i = .02;i<.5; i*=1.3)
    {
        dl *= clamp(1.5-i/(i+dist(p+sd*i*2.)),.0,1.);
    }
    vec3 sh = mix(vec3(.1,.15,.2),vec3(1),dl);
    
    vec3 col = mix(vec3(0.7,1,.2),vec3(1,0.4,0.1),s);
    return mix(vec3(.5,.7,.8),col*min((p.y+1.1)/.4,1.),e)*sh*sqrt(ao)+sl;
}
vec4 march(vec3 p,vec3 r)
{
    vec4 m = vec4(p+r,1);
    for(int i = 0;i<200;i++)
    {
        float s = dist(m.xyz);
        m += vec4(r,1)*s;
        
        if (s<EPS || m.w>MAX) return m;
    }
    return m;
}
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec3 p = vec3(iTime-2.,.5+.5*cos(iTime*.2),1);
    vec3 col = vec3(0);
    for(int X = 0;X<AA;X++)
    for(int Y = 0;Y<AA;Y++)
    {
        vec2 c = fragCoord+vec2(X,Y)/float(AA)-.5;
        vec3 r = normalize(vec3(c-vec2(.5,.6)*iResolution.xy,iResolution.y));
    
        vec4 m = march(p,r);
        float fog = smoothstep(MAX*.4,MAX,m.w);
        col += mix(color(m.xyz,r),exp(-vec3(13,7,4)*r.y*r.y-.2),fog);
    }
    col /= float(AA*AA);
    fragColor = vec4(pow(col,vec3(.45)),1);
}