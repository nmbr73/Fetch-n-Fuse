
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define HASHSCALE3 to_float3(0.1031f, 0.1030f, 0.0973f)

//#define R iResolution


__DEVICE__ float4 texelFetchC( __TEXTURE2D__ Channel, int2 pos, float2 R)
{
    
    if ( (pos.x) >= 0 && (pos.x) < int(R.x) && (pos.y) > 0 && (pos.y) < int(R.y) )
    {
        return _tex2DVecN( Channel, (pos.x+0.5f)/R.x,(pos.y+0.5f)/R.y,15 ); //!!!
    }
	else
		return to_float4_s(0);
}

//utility functions

//can be used for solid, non-kinematic obstacles
//unless you use a higher than 1.0f bouse value then the balls get stuck
__DEVICE__ float map(float2 p){
    //float d=length(p-to_float2(100.0f,50.0f))-30.0f;
    /*d=_fminf(d,p.x-5.0f);
    d=_fminf(d,165.0f-p.x);
    d=_fminf(d,p.y-5.0f);
    d=_fminf(d,90.0f-p.y);*/
  //return d;
    return 100.0f;
}
__DEVICE__ float2 mapNormal(float2 p)
{
    float2 eps = to_float2(0.0f, 0.001f);
    float2 normal = normalize(to_float2(
        map(p + swi2(eps,y,x)) - map(p - swi2(eps,y,x)),
        map(p + swi2(eps,x,y)) - map(p - swi2(eps,x,y))));
    return normal;
}
//from https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash22(float2 p)
{
    float3 p3 = fract_f3((swi3(p,x,y,x)) * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


//controls moving the particles
__DEVICE__ float4 getParticle(float2 p, float ballRadius, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float R=_ceil(ballRadius);
    float4 np=to_float4_s(-1.0f);
    float s=0.5f;
    for(float i=-R;i<=R;i++){
      for(float j=-R;j<=R;j++){
        float4 prt = texture(iChannel0,(p+to_float2(j,i))/iResolution);
        if(prt.x!=-1.0f || prt.y!=-1.0f || prt.z!=-1.0f || prt.w!=-1.0f ){
          if( _fabs(prt.x+prt.z+j)<=s && _fabs(prt.y+prt.w+i)<=s ){
            np=to_float4(prt.x+prt.z+j,prt.y+prt.w+i,prt.z,prt.w);
          }
        }
      }
    }
    return np;
}

__KERNEL__ void FastElasticNBallCollisionJiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER0(Radius, 0.0f, 10.0f, 2.0f);
    CONNECT_SLIDER1(Distance, 0.0f, 30.0f, 10.0f);
  
    fragCoord+=0.5f;

    float ballRadius= Radius*_sqrtf(Radius);//2.0f*_sqrtf(2.0f);//min of _sqrtf(2.0f), max is how big your computer can handle

    float2 uv = fragCoord/iResolution;
    if(map(fragCoord)<=0.0f){
        fragColor = to_float4_s(-1.0f);
    }else if(iFrame<=5 || Reset){
        //float A=10.0f*_ceil(ballRadius);
        float A=Distance*_ceil(ballRadius);
        float tex = texture(iChannel1, uv).w;
        if((int)(fragCoord.x/A)!=(int)((1.0f+fragCoord.x)/A)&&
           (int)(fragCoord.y/A)!=(int)((1.0f+fragCoord.y)/A) && (!Textur || tex)){
            fragColor = 1.0f*to_float4_f2f2(to_float2_s(0.0f),2.0f*(hash22(fragCoord+to_float2_s(iTime))-0.5f));
        }else{
            fragColor = to_float4_s(-1.0f);
        }
    }else if((int)(fragCoord.x)==(int)(iResolution.x/2.0f)&&
             (int)(fragCoord.y)==(int)(iResolution.y/2.0f)&&
             //texelFetchC(iChannel1, to_int2(32,1),iResolution ).x==1.0f){
             texture(iChannel1, (make_float2(to_int2(32,1))+0.5f)/iResolution).x==1.0f){   // Randprobleme
       
       fragColor = 1.0f*to_float4_f2f2(to_float2_s(0.0f),2.0f*(hash22(fragCoord+to_float2_s(iTime))-0.5f)); //iDate.w
    }else{
        float4 p=getParticle(fragCoord, ballRadius, iResolution,iChannel0);
        if(p.x!=-1.0f){
            //handles ball with too much speed (rarely happens)
            if(length(swi2(p,z,w))>_ceil(ballRadius)){
               swi2S(p,z,w, swi2(p,z,w)*_ceil(ballRadius)/length(swi2(p,z,w)));
            }
            //friction goes here
            //swi2(p,z,w)*=0.99f;
            fragColor = p;
        }else{
            fragColor = to_float4_s(-1.0f);
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//controls particle collisions
//there is no restitution in this sim
//sometimes clumps form as a result with multiple balls colliding at the same time

#define _reflect(I,N) (I-2.0f*dot(N,I)*N)

__DEVICE__ float4 getNewVelocity(float4 mp, float2 p, float ballRadius, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float R=_ceil(ballRadius*2.0f);
    float2 nv=to_float2_s(0.0f);
    float collisionCount=0.0f;
    float4 nMP=mp;
    //check collisions with other balls
    for(float i=-R;i<=R;i++){
      for(float j=-R;j<=R;j++){
        if(i!=0.&&j!=0.0f){
            float4 prt = texture(iChannel0,(p+to_float2(j,i))/iResolution);
          if(prt.x!=-1.0f){
            float2 collision=swi2(mp,x,y)-(swi2(prt,x,y)+to_float2(j,i));
            //vec2 collision1=swi2(mp,x,y)+swi2(mp,z,w)-(swi2(prt,x,y)+to_float2(j,i)-swi2(prt,z,w));
            float d=length(collision);
            float r=ballRadius*2.0f;
          if(d<=r){
            collision = collision/d;
            float a = dot(swi2(mp,z,w),collision);
            float b = dot(swi2(prt,z,w),collision);
    
            //possible to add a fake restitution by applying collisions based on overlap
            /*nv+=(swi2(mp,z,w)+((b-a)*collision))*((r+1.0f)-d);
            collisionCount+=((r+1.0f)-d);*/
            
            //basic 2D elastic collision equation
            nv+=(swi2(mp,z,w)+((b-a)*collision));
            collisionCount+=1.0f;
            
            //here's basic 1D math which creates a lot of clumps
            //Might be useful if that's what you want
            /*nv+=swi2(prt,z,w);
            collisionCount+=1.0f;*/   
            }
          }
        }     
      }
    }
    //check collisions with non-kinematic obstacles
    float d=map(p+swi2(mp,x,y));
    if(d<=ballRadius){
      float2 normal=mapNormal(p+swi2(mp,x,y));
      nv+=_reflect(swi2(mp,z,w),normal);
      collisionCount+=1.0f;
    }
    //apply all collisions
    if(collisionCount>0.0f){
      //nMP.zw=nv/collisionCount;
      nMP.z=nv.x/collisionCount;
      nMP.w=nv.y/collisionCount;
    }
    return nMP;
}
__KERNEL__ void FastElasticNBallCollisionJiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_SLIDER0(Radius, 0.0f, 10.0f, 2.0f);
    
    fragCoord+=0.5f;
    
    float ballRadius=Radius*_sqrtf(Radius);//2.0f*_sqrtf(2.0f);//min of _sqrtf(2.0f), max is how big your computer can handle

    float2 uv = fragCoord/iResolution;
    if(iFrame<=5 || Reset){
        fragColor=_tex2DVecN(iChannel0,uv.x,uv.y,15);
    }else{
        float4 mp=_tex2DVecN(iChannel0,uv.x,uv.y,15);
        if(mp.x!=-1.0f){
            fragColor = getNewVelocity(mp,fragCoord, ballRadius, iResolution, iChannel0);
        }else{
            fragColor = to_float4_s(-1.0f);
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


//main rendering with trails for balls

__KERNEL__ void FastElasticNBallCollisionJiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_SLIDER0(Radius, 0.0f, 10.0f, 2.0f);

    fragCoord+=0.5f;
    
    float ballRadius=Radius*_sqrtf(Radius);//2.0f*_sqrtf(2.0f);//min of _sqrtf(2.0f), max is how big your computer can handle
    

    float2 p = fragCoord;
    float d=100.0f;
    float md=2.0f;
    if(_fmaxf(fract(fragCoord.x/10.0f),fract(fragCoord.y/10.0f))-0.9f>0.0f){
      md/=1.5f;
    }
    if(length(fragCoord-swi2(iMouse,x,y))<200.&&iMouse.z > 0.0f){
      p = swi2(iMouse,x,y)+(fragCoord-swi2(iMouse,x,y))/3.0f;
        d=_fabs(length(fragCoord-swi2(iMouse,x,y))-200.0f)-3.0f;
        md=1.5f;
        if(_fmaxf(fract(fragCoord.x/30.0f),fract(fragCoord.y/30.0f))-0.9f>0.0f){
            md/=1.5f;
        }
    }
    float R=_ceil(ballRadius);
    for(float i=-R;i<=R;i++){
        for(float j=-R;j<=R;j++){
            float4 prt = texture(iChannel0,(_floor(p)+to_float2(j,i))/iResolution);
            if(prt.x!=-1.0f || prt.y!=-1.0f || prt.z!=-1.0f || prt.w!=-1.0f){
                d=_fminf(d,length(p-(_floor(p)+to_float2(j,i)+swi2(prt,x,y)))-ballRadius);
            }

        }
    }
    d=_fminf(d,map(p));
    float trail=0.9f;
    float nc=trail*texture(iChannel1,fragCoord/iResolution).x+(1.0f-trail)*step(-d,0.0f)/md;
    fragColor=to_float4_s(_fminf(nc,step(-d,0.0f)/md));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


//main rendering

__KERNEL__ void FastElasticNBallCollisionJiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    
    fragColor=texture(iChannel0,fragCoord/iResolution);

  SetFragmentShaderComputedColor(fragColor);
}