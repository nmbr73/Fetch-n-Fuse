

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// trying to resemle some chalk on blackboard hand drawing style


#define PI2 6.28318530717959
#define N(a) ((a).yx*vec2(1,-1))
#define AngleNum 5
#define SampNum 24

#define Res0 vec2(textureSize(iChannel0,0).xy)
#define Res1 vec2(textureSize(iChannel1,0).xy)
#define Res2 vec2(textureSize(iChannel2,0).xy)

#define Res  iResolution.xy

#define randSamp iChannel1
#define colorSamp iChannel0

#define zoom 1.


vec4  getRand(vec2 pos)
{
    return texture(iChannel1,pos/Res1/iResolution.y*1200.);
}

#define UVScale (vec2(Res0.y/Res0.x,1)/Res.y)

const float BlackFill=1.;
const float bright=1.1;
const float contourStrength=1.;
const float reflection=.5;
const float reflectStrength=.3;
const float reflectSize=.35;
#define reflectPos (Res*(.2+.15*sin(iTime+vec2(0,2.))))
const vec3 paperTint=vec3(1,.85,.9)*.8;

#define OutlineOffs 0.

vec4 getCol(vec2 pos)
{
    pos=(pos-.5*Res.xy)*zoom+.5*Res.xy;
    vec2 r0=texture(iChannel0,.5/Res).zw;
    vec2 sc=mix(r0.y/Res0.y,r0.x/Res0.x,.5)/Res;    // compromise between "fit all" and "fit one"
    vec2 uv = pos*sc+.5*(r0/Res0-Res*sc);
    uv=clamp(uv,.5/Res0,1.-.5/Res0);
    return (1.-bright*texture(iChannel0,uv).zzzw);
}

vec2 getGrad(vec2 pos, float eps)
{
    pos=(pos-.5*Res.xy)*zoom+.5*Res.xy;
    vec2 r0=texture(iChannel0,.5/Res).zw;
    vec2 sc=mix(r0.y/Res0.y,r0.x/Res0.x,.5)/Res;    // compromise between "fit all" and "fit one"
    vec2 uv = pos*sc+.5*(r0/Res0-Res*sc);
    uv=clamp(uv,.5/Res0,1.-.5/Res0);
    return (contourStrength)*texture(iChannel0,uv).xy*r0/Res.xy;
}

const float flicker=1.;
const float flickerFreq=10.;

float htPattern(vec2 pos, float phase)
{
    float pat=0.;
    float cnt=0.;
    vec2 offs=vec2(.001,.1)*floor(iTime*flickerFreq)/10.*flicker;
    float phaseOffs = 10.*getRand(floor(iTime*flickerFreq)*vec2(.01,.1)).x*flicker;
    vec2 gr=/*getGrad(floor(pos/13.)*13.,1.)+*/1.01*normalize(pos-.5*Res);
    for(float ang=0.0;ang<PI2;ang+=PI2/4.3)
    {
        vec2 b=normalize(sin(vec2(0,PI2/4.) + ang + phase + phaseOffs + 0.6 )*vec2(.5,1.5));
        vec2 uv=((pos.x-pos.y*pos.y*.0004)*b+(pos.y+pos.x*pos.x*.0004)*N(b))/Res1*vec2(7,.3)*.3;
        pat+=.5*texture(iChannel1,uv*.25+offs).x;
        pat+=1.*texture(iChannel1,uv+offs).x;
        cnt+=1.5;
    }
    return pat/cnt; 
}

float halfTone(float val,vec2 pos, float phase)
{
    return smoothstep(.6,1.4,val+htPattern(pos,phase));
}

const float BGAlpha=0.;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord+0.0*sin(iTime*1.*vec2(1,1.7))*iResolution.y/400.;
    vec2 pos0=pos;
    vec3 col = vec3(0);
    vec3 col2 = vec3(0);
    float sum=0.;
  	vec2 g0=getGrad(pos,1.);
  	float dang=PI2/float(AngleNum);
    for(int i=0;i<AngleNum;i++)
    {
        float ang=dang*float(i)+.1;
        vec2 v=sin(vec2(PI2/4.,0)+ang);
        for(int j=0;j<SampNum;j++)
        {
            vec2 dpos  = v.yx*(vec2(1,-1)*float(j)*iResolution.y/25./float(SampNum)+OutlineOffs*Res.x/15.);
            vec2 dpos2 = v.xy*(float(j*j)/float(SampNum*SampNum)*.3
                               //*(length(100.*g0)) // higher/lower gradients get curved/hatched
                               +.08)*iResolution.y/25.;
	        vec2 g;
            float fact=1.;
            float fact2;

            for(float s=-1.;s<=1.;s+=2.)
            {
                vec2 pos2=pos+1.0/zoom*(s*dpos+dpos2);
                vec2 pos3=pos+1.0/zoom*(s*dpos+dpos2).yx*vec2(1,-1)*2.;
                float ht=1.;
            	g=getGrad(pos2,1.)*ht;
                g*=pow(getRand(pos2*.8*iResolution.y/1080.).x*2.,4.*sqrt(iResolution.y/1200.));
                
            	float fact3=dot(g,v)-.5*abs(dot(g,v.yx*vec2(1,-1)))/**(1.-getVal(pos2))*/;
            	fact2=dot(normalize(g+vec2(.0001)),v.yx*vec2(1,-1));
             
                fact3=clamp(fact3,0.,0.05);
                fact2=abs(fact2);
                
                fact3*=1.-1.*float(j)/float(SampNum);
                fact*=fact3;
            	col += .3*fact3;
             	sum+=fact2;
            }
          	col += 2.*pow(fact,.5);
        }
    }
    col/=float(SampNum*AngleNum)*.75/sqrt(iResolution.y);
    col2/=sum;
    col=1.-col*1.2;
    col*=col*col;
    

    vec2 s=sin(pos.xy*.1/sqrt(iResolution.y/400.));
    float r=length(pos-iResolution.xy*.5)/iResolution.x;
    float vign=1.-r*r*r;
    vec3 c=getCol(pos).xyz;
    float bright=dot(getCol(pos).xyz,vec3(.3333));
    float blackTone=halfTone(bright*1.5+.25,(pos0-Res*.5)*zoom,floor(sqrt(bright)*8.)/8.*2.7);

    blackTone = mix(1.,     blackTone,BlackFill);
    float refl=clamp(pow((col.x*(blackTone)),1.),0.,1.);
    vec3 col3= paperTint;
    col3*=vec3(col.x)*blackTone;
    col3+=.1*getRand(pos*.7).xxx;
    col3*=(1.-.65*texture(iChannel2,fragCoord/iResolution.xy).xyz);
    
	fragColor = vec4(col3*.9+.2*getRand(pos*.7).xyz-.2*getRand(pos*.7-.6).xyz,1);
	float reflEnv=clamp((sin(fragCoord.x/iResolution.x*7.+2.5+1.7*iTime))*(fragCoord.y/iResolution.y),0.,1.);
	vec2 reflp=reflectPos*vec2(1,-1)+vec2(0,iResolution.y); 
	if(iMouse.x>0.5) reflp=iMouse.xy;
	reflEnv = exp(-pow(length(reflp-fragCoord)/iResolution.x/reflectSize,2.));
	fragColor.xyz=1.-fragColor.xyz;
	fragColor.xyz+=refl*vec3(.8,.9,1.)*1.1*reflEnv*reflectStrength;
	fragColor.w=mix(1.,1.-min(min(fragColor.x,fragColor.y),fragColor.z),BGAlpha);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// trying to resemle some chalk on blackboard hand drawing style

// this buffer holds the prerendered the gradient

#define Res0 vec2(textureSize(iChannel0,0).xy)
#define Res1 vec2(textureSize(iChannel1,0).xy)

#define Res  iResolution.xy

#define randSamp iChannel1
#define colorSamp iChannel0


vec4 getRand(vec2 pos)
{
    return texture(iChannel1,pos/Res1/iResolution.y*1200.);
}

#define UVScale (vec2(Res0.y/Res0.x,1)/Res.y)

vec4 getCol(vec2 pos)
{
    vec2 uv=pos/Res0.xy;
    uv=clamp(uv,0.5/Res0,1.-.5/Res0);
    vec4 c1=texture(iChannel0,uv);
    float d=clamp(dot(c1.xyz,vec3(-.5,1.,-.5)),0.0,1.0);
    vec4 c2=vec4(.5);
    return min(mix(c1,c2,1.8*d),.7);
}

vec4 getColHT(vec2 pos)
{
 	return smoothstep(.95,1.05,getCol(pos)+getRand(pos*.2));
}

float getVal(vec2 pos)
{
    vec4 c=getCol(pos);
 	return dot(c.xyz,vec3(.333));
}

#define SQR3 1.73205081
vec2 getGrad(vec2 pos, float eps)
{
   	vec3 d=vec3(eps/UVScale/Res0,0);
    pos-=.33*d.xy;
    float v0=getVal(pos);
    return (vec2(getVal(pos+d.xz),getVal(pos+d.zy))-v0)/d.xy;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 ResN = Res0;
    ResN *= min(ResN.y,Res.y)/ResN.y;
    ResN *= min(ResN.x,Res.x)/ResN.x;
    vec2 fc=fragCoord.xy*Res0/ResN;
    fragColor.xy = getGrad(fc,.15);
    fragColor.z=getVal(fc);
    if(fragCoord.x<1. && fragCoord.y<1.)
    fragColor.zw = ResN;
    if(fragCoord.x>ResN.x || fragCoord.y>ResN.y) 
    {
        fragColor=vec4(0,0,0,1);
        //discard;
    }
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// trying to resemle some chalk on blackboard hand drawing style


mat2 ROTM(float ang) { return mat2(cos(ang),sin(ang),-sin(ang),cos(ang)); }

vec2 quad01(int idx) { return idx<3?vec2(idx%2,idx/2):1.-vec2((5-idx)%2,(5-idx)/2); }

vec4 getRand(int idx) {
    ivec2 res=textureSize(iChannel0,0);
    return texelFetch(iChannel0,ivec2(idx%res.x,(idx/res.x)%res.y),0); 
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    float StrokeLen=1.2;
    float StrokeW=.4;
    int NumStrokes=3;
    int NumBatches=40;
    
    vec3 c=vec3(0);
    
    for(int i=0;i<NumStrokes*NumBatches;i++){
    vec2 sc=fragCoord/iResolution.xy*2.-1.;
    int strokeIdx=i;
    int strokeIdx0=strokeIdx;
    strokeIdx=strokeIdx%NumStrokes;
    int batchIdx=strokeIdx0/NumStrokes;
    float ang=float(batchIdx)*.17+floor(iTime*3.);
    float dang=(float(strokeIdx%2)-.5)*StrokeW/StrokeLen*1.2;
    mat2 m=ROTM(dang);
    vec2 sc0=(getRand(batchIdx).zw-.5)*2.;
    sc-=sc0;
    sc=ROTM(ang)*sc;
    //sc0=vec2(0);
    float strokeFact=float(strokeIdx)/float(NumStrokes)-.5;
    //float segFact=sc.x/(StrokeLen*.5);
    sc+=StrokeW*.8*float(NumStrokes)*vec2(0,.7*strokeFact);
    sc=ROTM(dang)*sc;
    vec2 uv=sc/(vec2(StrokeLen,StrokeW));
    //uv=m*uv;
    uv.y+=uv.x*uv.x*1.5;
    uv+=.5;
    vec4 r=textureLod(iChannel0,(uv+vec2(0,i))*vec2(.02,1.),1.7);
    vec3 s;
    s = clamp(vec3(0) + r.x,0.,1.);
    s*=mix(exp(-12.*uv.x)+exp(-12.*(1.-uv.x)),1.,.5);
    float a=1.+r.x;
    a*=1.-smoothstep(0.85,1.,abs(uv.x-.5)*2.);
    a*=1.-smoothstep(0.85,1.,abs(uv.y-.5)*2.);
    a*=.3;
    a=clamp(a,0.,1.);
    c=c*(1.-a)+s*a;
    }
    
    fragColor.xyz=c;
    fragColor.w=1.;
}

