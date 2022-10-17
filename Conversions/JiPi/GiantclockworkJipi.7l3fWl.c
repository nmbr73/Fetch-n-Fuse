
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Cubemap: Uffizi Gallery Blurred_0' to iChannel1
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// created by florian berger (flockaroo) - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// giant clockwork

//uncomment this to get some periodiv dynamc dolly-zoom
//#define DOLLY_ZOOM


#define Res0 iResolution //to_float2(textureSize(iChannel0,0))

#ifndef PI2
#define PI2 6.28318530718f
#endif
#define GEAR_W 0.27f

__DEVICE__ float gear(float3 p, int numTeeth, float w)
{
    float lpxy=length(swi2(p,x,y));
    float d=10000.0f;
    float ang=_atan2f(p.y,p.x);
    d=_fminf(d,length(p+to_float3_aw(swi2(p,x,y)/lpxy,0)*0.1f*_sinf(ang*(float)(numTeeth)))-1.0f);
    d=_fmaxf(d,_fabs(p.z)-GEAR_W*w);
    d=_fmaxf(d,0.75f-lpxy);
    return d;
}

__DEVICE__ float4 inverseQuat(float4 q)
{
    //return to_float4(-swi3(q,x,y,z),q.w)/length(q);
    // if already normalized this is enough
    return to_float4_aw(-1.0f*swi3(q,x,y,z),q.w);
}

__DEVICE__ float4 multQuat(float4 a, float4 b)
{
    return to_float4_aw(cross(swi3(a,x,y,z),swi3(b,x,y,z)) + swi3(a,x,y,z)*b.w + swi3(b,x,y,z)*a.w, a.w*b.w - dot(swi3(a,x,y,z),swi3(b,x,y,z)));
}

__DEVICE__ float3 transformVecByQuat( float3 v, float4 q )
{
    return v + 2.0f * cross( swi3(q,x,y,z), cross( swi3(q,x,y,z), v ) + q.w*v );
}

__DEVICE__ float4 axAng2Quat(float3 ax, float ang)
{
  float zzzzzzzzzzzzzz;
    return to_float4_aw(normalize(ax),1)*swi4(sin_f2(to_float2_s(ang*0.5f)+to_float2(0,PI2*0.25f)),x,x,x,y);
}

__DEVICE__ float allGears(float3 p, float rot, float tilt, float iTime)
{
    //gears in to 1st quadrant
    //...we need those only if the truchet cell is rotated
    //if(p.x<0.0f) p.xy=swi2(p,y,x)*to_float2(1,-1);
    //if(p.x<0.0f) p.xy=swi2(p,y,x)*to_float2(1,-1);
    
    float r0=0.205f;
    float4 quat=to_float4(0,0,0,1);
    float d=1000.0f;
    int N=2;
    int numTeeth=16;
    float3 p0=p;
    for(int i=0;i<N+1;i++)
    {
        float ang=PI2/4.0f*(float)(i)/(float)(N);
        float c2=-_cosf(ang*4.0f)*0.5f+0.5f;
        float3 pos0=to_float3_aw(cos_f2(ang-to_float2(0,PI2/4.0f)),0.0f)*0.5f;
        pos0-=normalize(pos0)*c2*0.05f*(0.5f+0.5f*_cosf(tilt)*_cosf(tilt));
        pos0+=to_float3(0,0,1)*c2*0.02f*_sinf(tilt)*_sinf(tilt);
        int nteeth=numTeeth-3*(i%2);
        float r=r0*float(nteeth)/(float)(numTeeth);
        float gearAng=iTime*((float)(i%2)*2.0f-1.0f)*r0/r-c2*0.25f;
        quat=axAng2Quat(to_float3(0,0,1),rot*gearAng);
        quat=multQuat(quat,axAng2Quat(to_float3(0,1,0),tilt));
        quat=multQuat(quat,axAng2Quat(to_float3(0,0,1),-ang));
        p=fract(p0)-to_float3(0,0,0.5f);
        d=_fminf(d,gear(transformVecByQuat(p-pos0,quat)/r,nteeth,r/r0)*r);
        p=fract(swi3(p0,z,x,y)*to_float3( 1,-1, 1))-to_float3(0,0,0.5f);
        d=_fminf(d,gear(transformVecByQuat(p-pos0,quat)/r,nteeth,r/r0)*r);
        p=fract(swi3(p0,y,z,x)*to_float3(-1,-1, 1))-to_float3(0,0,0.5f);
        d=_fminf(d,gear(transformVecByQuat(p-pos0,quat)/r,nteeth,r/r0)*r);
    }
    return d;
}

#ifndef RandTex 
#define RandTex iChannel0
#endif

__DEVICE__ float4 myenv(float3 pos, float3 dir, float period, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
  #ifndef SHADEROO
    return decube_f3(iChannel1,swi3(dir,x,z,y));
  #else
    float azim = _atan2f(dir.y,dir.x);
    float thr  = 0.5f*0.5f*(0.7f*_sinf(2.0f*azim*5.0f)+0.3f*_sinf(2.0f*azim*7.0f));
    float thr2 = 0.5f*0.125f*(0.7f*_sinf(2.0f*azim*13.0f)+0.3f*_sinf(2.0f*azim*27.0f));
    float thr3 = 0.5f*0.05f*(0.7f*_sinf(2.0f*azim*32.0f)+0.3f*_sinf(2.0f*azim*47.0f));
    float br  = smoothstep(thr-0.2f, thr+0.2f, dir.z+0.25f);
    float br2 = smoothstep(thr2-0.2f,thr2+0.2f,dir.z+0.15f);
    float br3 = smoothstep(thr3-0.2f,thr3+0.2f,dir.z);
    float4 r1 = 0.5f*(texture(RandTex,swi2(dir,x,y)*0.01f)-texture(RandTex,swi2(dir,x,y)*0.017f+0.33f));
    float3 skyCol=to_float3(0.9f,1,1.1f)+0.3f*(swi3(r1,x,x,x)*0.5f+swi3(r1,x,y,z)*0.5f);
    //skyCol*=2.5f;
    float4 r2 = 0.5f*(texture(RandTex,swi2(dir,x,y)*0.1f)-texture(RandTex,swi2(dir,x,y)*0.07f-0.33f));
    float3 floorCol = to_float3(0.9f,1.1f,1.0f)*0.8f+0.5f*(swi3(r2,x,x,x)*0.7f+swi3(r2,x,y,z)*0.3f);
    float3 col=_mix(swi3(floorCol,z,y,x),skyCol,br3);
    col=_mix(swi3(floorCol,y,z,x)*0.7f,col,br2);
    col=_mix(swi3(floorCol,x,y,z)*0.7f*0.7f,col,br);
    float3 r=texture(RandTex,to_float2(azim/PI2*0.125f,0.5f)).xyz;
    col*= 1.0f-clamp(((swi3(r,x,x,x)*0.7f+swi3(r,x,z,z)*0.3f)*2.0f-1.0f)*clamp(1.0f-_fabs(dir.z*1.6f),0.0f,1.0f),0.0f,1.0f);
    return to_float4(col*col*to_float3(1.1f,1,0.9f)/**clamp(1.0f+dir.x*0.3f,0.9f,1.2f)*/,1);
  #endif
}

// only 2 configurations per cell: all wheels either "standing" (meshing fully) or "lying" (meshing 45°)
// in either configuration every other cell (3d checkerboard) has to be point-mirrored
// so 4 cell configurations are possible - but only in certain conditions
// here: every second layer in x-dir "lying" configuration
__DEVICE__ float truchetDist(float3 pos, float iTime)
{
    //float rnd=textureLod(iChannel0,(_floor(swi2(pos,x,y))+0.5f+to_float2(7,13)*_floor(pos.z))/Res0,0.0f).x;
    float3 mp=_floor(mod_f3(pos,2.0f))*2.0f-1.0f;
    float fact=mp.x*mp.y*mp.z;
    return allGears(pos*fact*mp.x,1.0f,PI2*0.25f*(mp.x*0.5f+0.5f), iTime);
}

__DEVICE__ float getTime(float2 iResolution, __TEXTURE2D__ iChannel0)
{
    //return texelFetch(iChannel0,to_int2(0,0),0).x;
    return texture(iChannel0,(make_float2(to_int2(0,0))+0.05f)/iResolution).x;
}

__DEVICE__ float getDistance(float3 pos, float iTime)
{
    return truchetDist(pos,iTime);
}

__DEVICE__ float3 getGrad(float3 pos, float delta, float iTime)
{
    delta*=2.0f;
    float3 eps=to_float3(delta,0,0);
    float d=getDistance(pos,iTime);
    return to_float3(getDistance(pos+swi3(eps,x,y,y),iTime)-d, getDistance(pos+swi3(eps,y,x,y),iTime)-d, getDistance(pos+swi3(eps,y,y,x),iTime)-d)/delta;
}

#define RaytraceMaxStepNum 100

__DEVICE__ float march(inout float3 *pos, float3 dir, float iTime)
{
    float eps=0.002f;
    for( int i=0 ; i<RaytraceMaxStepNum ; i++ )
    {
        float d=getDistance(*pos,iTime);
        *pos+=d*dir*0.7f;
        if(d<eps) return 1.0f;
    }
    return 0.0f;
}


__DEVICE__ void calcDollyZoom(inout float3 *pos, float3 dir0, inout float3 *dir, float fact)
{
    *dir=_mix(*dir,dir0,fact);
    *pos-=fact*dir0;
    *pos+=fact* *dir;
}

__KERNEL__ void GiantclockworkJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    float3 lightCol = to_float3(1.1f,1.0f,0.9f);
    float3 ambCol   = to_float3(0.2f,0.6f,1.0f)*0.25f;
    float3 diffCol  = to_float3(1.0f,0.85f,0.7f);
    float3 lightDir = normalize(to_float3(0.2f,0.5f,1));



    float motion=1.0f;
    float2 myRes=iResolution;
    float3 pos=to_float3(iTime*0.4f*motion-0.5f,0,0);
    float2 scoord=(fragCoord-myRes*0.5f)/myRes.x;
    float phi = iTime*0.333f*0.3f*motion;
    float th = _cosf(iTime*0.23423f*0.3f)*motion;
    if(iMouse.x>0.5f) 
    {
        phi+=-iMouse.x/iResolution.x*PI2/2.0f*3.0f;
        th+=PI2/4.0f-iMouse.y/iResolution.y*PI2/2.0f*3.0f;
    }
    
    // calc view
    float4 quat=to_float4(0,0,0,1);
    quat=multQuat(quat,axAng2Quat(to_float3(0,0,1),phi));
    quat=multQuat(quat,axAng2Quat(to_float3(1,0,0),th));
    float3 dir0 = transformVecByQuat(to_float3( 0,1,0),quat);
    float3 left = transformVecByQuat(to_float3(-1,0,0),quat);
    float3 up   = transformVecByQuat(to_float3( 0,0,1),quat);
    float3 dir=left*scoord.x*2.0f+up*scoord.y*2.0f;
    dir=normalize(dir0+dir);
  #ifdef DOLLY_ZOOM
    calcDollyZoom(&pos,dir0,&dir,0.6f*clamp(-_cosf(iTime*0.1f)*5.0f+0.5f,0.0f,1.0f));
  #endif
float AAAAAAAAAAAAAAAAAA;    
    // do the actual raymarching
    float3 pos0=pos;
    march(&pos,dir,iTime);
    
    float3 bg=_mix(ambCol,lightCol,clamp(dot(pos-pos0,lightDir)*0.2f,-0.5f,1.5f));
    
    // reflection
    float3 n=normalize(getGrad(pos,0.0015f,iTime));
    float3 R=normalize(reflect(pos-pos0,n));
    float4 refl=myenv(pos,R,1.0f,iChannel0,iChannel1);
    
    float3 col=to_float3_s(1);
    
    // ambient occlusion
    float ao=1.0f;
    ao=_fminf(ao,getDistance(pos+n*0.08f,iTime)/0.08f);
    ao=_fminf(ao,getDistance(pos+n*0.04f,iTime)/0.04f);
    ao=_fminf(ao,getDistance(pos+n*0.02f,iTime)/0.02f);
    ao=ao*0.8f+0.2f;
    
    float diff=clamp(dot(n,lightDir),0.0f,1.0f);
    
    // mix diffuse, ambient, ao
    col *= diffCol-0.1f+0.1f*(n*0.5f+0.5f);
    col *= ao;
    col *= lightCol*diff*(1.0f-ambCol)+ambCol;
    //col+=0.1f*cnt/50.0f;
    col+=0.2f*swi3(refl,x,y,z);
    
    // depth fog
    col = _mix(bg,col,_expf(-length(pos-pos0)/7.0f));
    fragColor=to_float4_aw(col,1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// created by florian berger (flockaroo) - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// giant clockwork

#define MIST

__KERNEL__ void GiantclockworkJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;
float IIIIIIIIIIIII;
    float2 uv=fragCoord/iResolution;
  #ifdef MIST
    fragColor=1.5f*texture(iChannel0,uv);//,0.5f);
    fragColor+=1.0f*texture(iChannel0,uv);//,2.5f);
    fragColor+=0.75f*texture(iChannel0,uv);//,4.5f);
    
    fragColor = fragColor/3.25f;
  #else
    fragColor=1.0f*texture(iChannel0,uv,0.0f);
  #endif
        
    fragColor.w=1.0f;

  SetFragmentShaderComputedColor(fragColor);
}