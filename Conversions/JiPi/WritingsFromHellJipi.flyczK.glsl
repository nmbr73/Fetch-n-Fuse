

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// just a dummy shader because i needed some feedback of own color (not possible in image tab)
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture(iChannel0,fragCoord.xy/iResolution.xy);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// writing random endless scribbles
// by summing up low band noised curvature

// fragColor: red = writing, blue = burn mask

#define PI2 6.28318530717959
#define PNUM 40

vec2 filterUV1(vec2 uv) 
{
    // iq's improved texture filtering (https://www.shadertoy.com/view/XsfGDn)
	vec2 x=uv*iChannelResolution[1].xy;
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    return (p+f)/iChannelResolution[1].xy;
}

vec4 getPixel(int x, int y)
{
    return texture(iChannel0,vec2(float(x)+.5,float(y)+.5)/iChannelResolution[0].xy);
}

bool isPixel(int x, int y, vec2 fragCoord)
{
    vec2 c=fragCoord/iResolution.xy*iChannelResolution[0].xy;
    return ( int(c.x)==x && int(c.y)==y );
}

vec2 readPos(int i)
{
    return getPixel(i,0).xy;
}

bool writePos(vec2 pos, int i, inout vec4 fragColor, vec2 fragCoord)
{
    if (isPixel(i,0,fragCoord)) { fragColor.xy=pos; return true; }
    return false;
}

vec4 getRand(vec2 pos)
{
    return texture(iChannel1,filterUV1(pos/vec2(400,300)));
}

float dotDist(vec2 pos,vec2 fragCoord)
{
    return length(pos-fragCoord);
}

// iq: https://iquilezles.org/articles/distfunctions
float lineDist(vec2 a,vec2 b,vec2 p)
{
    vec2 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

vec4 drawDot(vec2 pos,float r, vec2 fragCoord)
{
    return vec4(clamp(r-length(pos-fragCoord),0.,1.)/r*3.);
}

#define N(x) (x.yx*vec2(1,-1))

// gives a parametric position on a pentagram with radius 1 within t=0..5
// (maybe there's more elegant ways to do this...)
vec2 pentaPos(float t)
{
    float w=sqrt((5.+sqrt(5.))*.5);
    float s=sqrt(1.-w*w*.25);
    float ang=-floor(t)*PI2*2./5.;
    vec2 x=vec2(cos(ang),sin(ang));
    return -N(x)*s+x*w*(fract(t)-.5);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time=float(iFrame)*1./60.;
    vec2 uv=fragCoord/iResolution.xy;
    float v=0.;
    for(int i=0;i<50;i++) v+=texture(iChannel0,getRand(vec2(i,0)).xy).x/50.;
    fragColor = texture(iChannel0,uv);
    int pnum = int(min(iResolution.y/50.0,float(PNUM-1)));
    bool write=false;
    for(int i=0;i<PNUM;i++)
    {
        bool isMouse = (i==pnum);
        // breaking here if i>pnum didnt work in windows (failed to unloll loop)
        if(i<=pnum) {
        vec2 pos;
            
	    pos=readPos(i);
        vec2 oldpos=pos;
        
    	float ang = (getRand(pos)+getRand(pos+vec2(1,3)*time)).x*PI2;
    	pos+=vec2(.7,0)
            +vec2(4,5)*vec2(cos(15.*time+float(i)),
                            .5*sin(15.*time+float(i)+.5)+
                            .5*sin(21.*time+float(i)+.5))*getRand(pos).x;
            //+vec2(.2,2)*vec2(cos(ang),sin(ang));
    	//vec4 c = drawDot(mod(pos,iResolution.xy),2.5,fragCoord);

        if(isMouse) 
        {
            pos=iMouse.xy;
            if(iMouse.xy==vec2(0) && mod(iTime+5.,37.7)>18.)
            {
                pos=pentaPos(iTime*.5)*.45*iResolution.y+iResolution.xy*.5;
                pos+=(getRand(pos*.6+iTime*vec2(.1,1.)).xy-.5)*7./500.*iResolution.y;
            }
        	if(length(oldpos-pos)>40.) oldpos=pos;
        }
                
        vec2 mpos=mod(pos,iResolution.xy);
        //float dd = dotDist(mpos,fragCoord);
        float dd = lineDist(mpos,oldpos-(pos-mpos),fragCoord);
    	vec4 c = vec4(clamp((isMouse?5.:3.)-dd,0.,1.9),0,max(0.,1.-dd/40.),0);
        if(mpos==oldpos-(pos-mpos)) c=vec4(0.); // ignore 0-length segments
        if(getRand(pos*.3+time).z>.8 && !isMouse) 
            pos+=vec2(10,0);
        else
    		fragColor = max(fragColor,c);        

        if(writePos(pos, i, fragColor,fragCoord)) write=true;
        }
    }

    if(!write)
    {
       fragColor.z=max(-1.,fragColor.z-.002);
       fragColor.x=max(0.,fragColor.x-.003);
    }
        
    if(iTime<2.) 
    {
        fragColor=vec4(0,0,.6,0);
	    for(int i=0;i<PNUM;i++)
    	{
            if(i<=pnum){
                vec4 rnd=texture(iChannel1,vec2(float(i)+.5,.5)/iChannelResolution[1].xy);
        	    writePos(vec2(20.+rnd.x*40.,iResolution.y/float(pnum)*float(i+1)),i,fragColor,fragCoord);     
            }
        }
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

#define RotNum 5
//#define SUPPORT_EVEN_ROTNUM

#define Res  iChannelResolution[0]
#define Res1 iChannelResolution[1]

#define keyTex iChannel3
#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float ang = 2.0*3.1415926535/float(RotNum);
mat2 m = mat2(cos(ang),sin(ang),-sin(ang),cos(ang));
mat2 mh = mat2(cos(ang*0.5),sin(ang*0.5),-sin(ang*0.5),cos(ang*0.5));

vec4 randS(vec2 uv)
{
    return texture(iChannel1,uv*Res.xy/Res1.xy)-vec4(0.5);
}

vec2 getGradBlue(vec2 pos)
{
    float eps=1.4;
    vec2 d=vec2(eps,0);
    return vec2(
		 texture(iChannel0,fract((pos+d.xy)/Res.xy)).z
        -texture(iChannel0,fract((pos-d.xy)/Res.xy)).z,
		 texture(iChannel0,fract((pos+d.yx)/Res.xy)).z
        -texture(iChannel0,fract((pos-d.yx)/Res.xy)).z
    )/(eps*2.);
}

float getRot(vec2 pos, vec2 b)
{
    vec2 p = b;
    float rot=0.0;
    for(int i=0;i<RotNum;i++)
    {
        vec2 v=texture(iChannel0,fract((pos+p)/Res.xy)).xy;

        rot+=dot(v,p.yx*vec2(1,-1));
        p = m*p;
    }
    return rot/float(RotNum)/dot(b,b);
}

vec4 getC2(vec2 uv) 
{
    // line 0 holds writer infos so take 1st line instead
    if(uv.y*iChannelResolution[2].y<1.) uv.y+=1./iChannelResolution[2].y;
	return texture(iChannel2,uv);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 pos = fragCoord.xy;
    float rnd = randS(vec2(float(iFrame)/Res.x,0.5/Res1.y)).x;
    
    vec2 b = vec2(cos(ang*rnd),sin(ang*rnd));
    vec2 v=vec2(0);
    float bbMax=0.7*Res.y*1.; bbMax*=bbMax;
    for(int l=0;l<8;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        vec2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=p.yx*getRot(pos+p,-mh*b);
#else
            // this is faster but works only for odd RotNum
            v+=p.yx*getRot(pos+p,b);
#endif
            p = m*p;
        }
        b*=2.0;
    }
    vec4 c2=getC2(fract(uv));
    float strength = clamp(1.-1.*c2.z,0.,1.);
    fragColor=texture(iChannel0,fract((pos+v*strength*(2./*+2.*iMouse.y/Res.y*/)*vec2(-1,1)*1.0)/Res.xy));
    fragColor=mix(fragColor,c2.xxzw*vec4(1,-1,1,1),.3*clamp(1.-strength,0.,1.));
    
    // damping
    fragColor.xy=mix(fragColor.xy,vec2(.0),.02);
    
    // add a little "motor" in the center
    //vec2 scr=(fragCoord.xy/Res.xy)*2.0-vec2(1.0);
    //fragColor.xy += (0.01*scr.xy / (dot(scr,scr)/0.1+0.3));
    
    if(iFrame<=4 || KEY_I>0.5) fragColor=texture(iChannel2,fragCoord.xy/Res.xy);
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float getVign(vec2 fragCoord)
{
	float vign=1.;
    
	float rs=length(fragCoord-iResolution.xy*.5)/iResolution.x/.7;	
    vign*=1.-rs*rs*rs;
    
    vec2 co=2.*(fragCoord.xy-.5*iResolution.xy)/iResolution.xy;
	vign*=cos(0.75*length(co));
    vign*=0.5+0.5*(1.-pow(co.x*co.x,16.))*(1.-pow(co.y*co.y,16.));
    
    return vign;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // camera rattle
    vec4 rattle=texture(iChannel2,vec2(iTime*.1234*.5,.5/256.));
	vec2 uv = fract(((fragCoord.xy / iResolution.xy-.5)*(1.+rattle.z*.01)+.5) + rattle.xy*.005);
    vec4 c = texture(iChannel0,uv);
    vec4 old = texture(iChannel1,fragCoord.xy / iResolution.xy);
    // brightness flickering
    vec4 flicker=texture(iChannel2,vec2(iTime*.2,.5/256.));
    
    // yellow-red fade
    fragColor= 1.5*mix(abs(c.xxww*vec4(1,.2,0,1)),.6*abs(c.xxxw),(1.-smoothstep(.35,.45,c.z))*(1.-smoothstep(.25,.35,c.x)));
    
    fragColor+=
        +abs(c.yyyw)*vec4(.4,.4,.3,1)                            // bright core
        +(.8+.2*flicker)*vec4(1,1,.5,1)*clamp(c.zzzw-.5,0.,1.);  // halo
    
    // mix bg image
    fragColor=mix(vec4(1),vec4(.2,.12,.06,0)*1.2+.4*texture(iChannel3,uv).x*vec4(1,1,1,0),-fragColor+1.1);

    fragColor*=(flicker*.25+.75)*2.3*fragColor;     // fragColor^2 contrast
    fragColor*=getVign(fragCoord);                  // vignetting
    fragColor=mix(fragColor,old*vec4(.7,1,1,1),.6); // slight motion blur (camera latency)
}