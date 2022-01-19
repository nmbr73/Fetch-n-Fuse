

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texelFetch(iChannel0,ivec2(fragCoord),0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const int   map[]=int[]
//0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
(15,14,13, 3,11, 5, 6, 1, 7, 9,10, 2,12, 4, 8, 0);
const int undomap[]=int[]
(15,7,11,3,13,5,6,8,14,9,10,4,12,2,1,0);
ivec2 modIvec2(ivec2 a,ivec2 b){
    return ivec2(int(mod(float(a.x),float(b.x))),int(mod(float(a.y),float(b.y))));
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    int isInverted;//"a" to invert time
    if(texelFetch(iChannel2,ivec2(65,2),0).x>0.)isInverted=1;
    else isInverted=0;
    
    vec2 uv = fragCoord/iResolution.xy;
    ivec2 worldSize=ivec2(iResolution.xy/renderScale);
    vec2 edge=(iResolution.xy/renderScale-fragCoord);
    vec2 t=uv+vec2(1.-1./renderScale);
    int thift = getShift(iFrame);
    if(1.>min(min(fragCoord.x,fragCoord.y),min(edge.x,edge.y))){
        fragColor = vec4(1-thift,0.0,0.0,1.0);//on border
        return ;
    }if(max(t.x,t.y)>1.){
        fragColor = vec4(thift,0.0,0.0,1.0);//outside screen
        return;
    }
    if(iFrame<freezeFrames){//on start
        int state=0;
        ivec2 pos=ivec2(fragCoord)-ivec2(10,10);
        if(pos.x>=0&&pos.y>=0)
        if(pos.x<seedLen.x&&pos.y<seedLen.y)state = seed[pos.y*seedLen.x+pos.x];
        fragColor = vec4(float(mod(float(1-thift+state),2.)));
        //fragColor = texture(iChannel1,uv+iMouse.xy/iResolution.xy);
        return;
    }if(mod(float(iFrame-freezeFrames),loopTime)!=0.){
        fragColor = texture(iChannel0,uv);
        return;
    }
    if(mouseSize>length(iMouse.xy/renderScale-fragCoord)){
        if(iMouse.z>0.5){fragColor = vec4(thift,0.5,0.0,1.0);return;}
        if(iMouse.w>0.5){fragColor = vec4(1-thift,0.5,0.0,1.0);return;}
    }
    int shift1 = int(mod(float(isInverted+thift),2.));
    ivec2 pos = (ivec2(fragCoord)+shift1)/2*2-shift1;
    ivec2 dif = (ivec2(fragCoord)-pos);
    int n = dif.x+2*dif.y;
    ivec2 c=ivec2(fragCoord);
    int sum;
    if(true){
    sum=int(
        +1*int(population*texelFetch(iChannel0,pos+ivec2(0,0),0).x)
        +2*int(population*texelFetch(iChannel0,pos+ivec2(1,0),0).x)
        +4*int(population*texelFetch(iChannel0,pos+ivec2(0,1),0).x)
        +8*int(population*texelFetch(iChannel0,pos+ivec2(1,1),0).x)
    );
    }else{/*
        ivec2 pos1=pos;
        ivec2 worldSize1=worldSize -ivec2(10);
        ivec2 worldSize2=worldSize1-ivec2(0);
        sum+=1*int(population*texelFetch(iChannel0,pos+ivec2(0,0),0).x);
        
        pos1=modIvec2(pos+worldSize1+ivec2(1,0),worldSize1);
        sum+=2*int(population*texelFetch(iChannel0,pos1,0).x);
        
        pos1=modIvec2(pos+worldSize1+ivec2(0,1),worldSize1);
        sum+=4*int(population*texelFetch(iChannel0,pos1,0).x);
        
        pos1=modIvec2(pos+worldSize1+ivec2(1,1),worldSize1);
        sum+=8*int(population*texelFetch(iChannel0,pos1,0).x);
    */}
    if(bool(isInverted))sum=(map[sum]>>n)&1;
    else sum=(undomap[sum]>>n)&1;
    
    fragColor = vec4(vec3(sum),1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

const float renderScale=2.;
const float loopTime=1.;
const float population=1.5;//1.<pop<2.;
const float mouseSize=6.;
const int freezeFrames=4;
int getShift(int iFrame){
    return int(float(0)+mod(float(iFrame-freezeFrames)/abs(loopTime),2.));
}
ivec2 seedLen=ivec2(12);

const int seed[]=int[](
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                             
0);
/*
const int seed_destroys_vertical[]=int[](
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                             
0);
const int seed_reflect[]=int[](
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                             
0);
const int seed_moveThrough[]=int[](
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 0,1, 1,0, 0,0, 0,0, 0,0,
                             
0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
0,0, 1,0, 0,1, 0,0, 0,0, 0,0,
                             
0);*/
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    if(getShift(iFrame)==1)
    fragColor = texelFetch(iChannel0,ivec2(fragCoord/renderScale),0);
    else
    fragColor = texelFetch(iChannel1,ivec2(fragCoord),0);
}