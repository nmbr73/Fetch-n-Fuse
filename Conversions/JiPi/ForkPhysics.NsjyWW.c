
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define CUBECOUNT 16u


__DEVICE__ float4 readTex3(uint cx,uint cy, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return _tex2DVecN(iChannel3,((float)(cx)+0.5f)/iResolution.x,((float)(cy)+0.5f)/iResolution.y,15);
}

__DEVICE__ float3 getCubePos(uint ci, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return swi3(readTex3(ci,0u,iResolution,iChannel3),x,y,z);
}
__DEVICE__ float4 getCubeQuat(uint ci, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return readTex3(ci,1u,iResolution,iChannel3);
}
__DEVICE__ float3 rotate(float4 quat,float3 v)
{
    float sinsqr = (1.0f-quat.w*quat.w);
    if (sinsqr!=0.0f)
    {
        v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
        v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
    }
    return v;
}
__DEVICE__ float3 cubeTransform(uint ci,float3 lp, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    lp.z*=0.5f;
    return getCubePos(ci,iResolution,iChannel3) + rotate(getCubeQuat(ci,iResolution,iChannel3),lp);
}

__DEVICE__ float4 getCubePlane(uint ci,uint k, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    float3 norm = to_float3( k%3u==1u?1.0:0.0, k%3u==0u?1.0:0.0, k%3u==2u?2.0:0.0)*(float(k/3u)*-2.0f+1.0f);
    norm = rotate(getCubeQuat(ci,iResolution,iChannel3),norm);
    float offset = 1.0f + dot(getCubePos(ci,iResolution,iChannel3),norm);
    return to_float4_aw(norm,offset);
}


__DEVICE__ float3 getWCubeVert(uint ci,uint j, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return cubeTransform(ci,to_float3((float)(j&1u),(float)((j&2u)/2u),(float)((j&4u)/4u))*2.0f-1.0f,iResolution,iChannel3);
}
__DEVICE__ float3 rotateAxisAngle(float3 axis,float angle,float3 v)
{
  v = v*_cosf(angle) + axis*((v*axis) * (1.0f-_cosf(angle))) + cross(v,axis)*_sinf(angle);
    return v;
}

//uint pixelx,pixely;

__DEVICE__ float4 findSeparatingPlane_planA( float2 iResolution, __TEXTURE2D__ iChannel3, uint pixelx, uint pixely)
{

    uint cia = pixelx/6u;
    uint cib = pixely/5u;
    
    if (cia>=CUBECOUNT) {  return to_float4_s(0.0f); } //discard;
    if (cia<=cib)       {  return to_float4_s(0.0f); } //discard;
    
    
    float bestoffset=-1e30;
    float3 bestplane;
    float bestplaneoffset;
    
//    for(uint m=0;m<2;m++)
//    for(uint k=0;k<30;k++)
    uint k = (pixelx%6u)+(pixely%5u)*6u;
    {
        float3 sep;
        float3 edgea = to_float3( k%3u==0u?1.0:0.0, k%3u==1u?1.0:0.0, k%3u==2u?1.0:0.0);
        edgea = rotate(getCubeQuat(cia,iResolution,iChannel3),edgea);
        float3 edgeb = to_float3( k/3u%3u==0u?1.0:0.0, k/3u%3u==1u?1.0:0.0, k/3u%3u==2u?1.0:0.0);
        edgeb = rotate(getCubeQuat(cib,iResolution,iChannel3),edgeb);
        if (k%15u<9u)
        { 
            // edge crossings
            if (length(cross(edgea,edgeb))<0.001f)
                sep = to_float3(0.0f,0.0f,0.0f);  // parallel edges fail
            else
              sep = normalize(cross(edgea,edgeb));
        }
        else
        {  // normals
            if (k<9u+3u)
            {
                sep = edgea;
            }
            else
            {
                sep =  to_float3( k%3u==0u?1.0:0.0, k%3u==1u?1.0:0.0, k%3u==2u?1.0:0.0);
                sep = rotate(getCubeQuat(cib,iResolution,iChannel3),sep);
            }
        }
        if (k>=15u) sep=-sep;
        
        if (cib==0u)
        {
            sep = to_float3(0.0f,-1.0f,0.0f);
        }
        
        float minoffset = -1e30;
        for(uint j=0u;j<8u;j++)
        {
            float3 v = getWCubeVert(cia,j,iResolution,iChannel3);
            if (dot(v,sep)>minoffset)
            {
                minoffset = dot(v,sep);
            }
        }

        float maxoffset = 1e30;
        for(uint j=0u;j<8u;j++)
        {
            float3 v = getWCubeVert(cib,j,iResolution,iChannel3);
            if (dot(v,sep)<maxoffset)
            {
                maxoffset = dot(v,sep);
            }
        }
        float offset = -minoffset+maxoffset;
        
//        if (offset>bestoffset && offset!=0.0f) // no improvement
        {
            bestoffset = offset;
            bestplaneoffset = (minoffset+maxoffset)*0.5f;
            bestplane = sep;
        }
    }
    
    if (bestoffset>=0.0f)
    {
        return to_float4(0.0f,9999.0f,0.0f,0.0f);
    }

    return to_float4_aw(-bestplane*(2.0f-bestoffset),-bestplaneoffset);
}

// this alg isn't good, beacuse it finds some local maximum instead of the best solution, 
// but it works ok if the cubes are uintersecting, a separating plane cannot be put, 
// but the plane with the least overlap is found, which is needed in the solver.
__DEVICE__ float4 findSeparatingPlane_planB(float2 iResolution, __TEXTURE2D__ iChannel3, uint pixelx, uint pixely)  
{
    if (pixelx<=pixely)      {  return to_float4_s(0.0f); } //discard;
    if (pixelx>=(CUBECOUNT)) {  return to_float4_s(0.0f); } //discard;    

    uint cia = pixelx;
    uint cib = pixely;

    if ((length(getCubePos(cia,iResolution,iChannel3)-getCubePos(cib,iResolution,iChannel3))>6.0f && cib!=0u)) { return to_float4_s(0.0f); } //discard;
    
    float3 sep = normalize(getCubePos(cib,iResolution,iChannel3)-getCubePos(cia,iResolution,iChannel3));
    float offset =0.0f;
    float dangle = 0.2f;
    float lastoffset=-1e30;
    float3 lastsep=sep;
    float3 diff = to_float3(0.0f,0.0f,0.0f);
    
    for(uint k=0u;k<64u;k++)
    {
        if (cib==0u)
        {
            sep = to_float3(0.0f,-1.0f,0.0f);
        }
        
        float minoffset = -1e30;
        float3 minvert=to_float3(0.0f,0.0f,0.0f);
        for(uint j=0u;j<8u;j++)
        {
            float3 v = getWCubeVert(cia,j,iResolution,iChannel3);
            if (dot(v,sep)>minoffset)
            {
                minoffset = dot(v,sep);
                minvert = v;
            }
        }

        float maxoffset = 1e30;
        float3 maxvert=to_float3(0.0f,0.0f,0.0f);
        for(uint j=0u;j<8u;j++)
        {
            float3 v = getWCubeVert(cib,j,iResolution,iChannel3);
            if (dot(v,sep)<maxoffset)
            {
                maxoffset = dot(v,sep);
                maxvert = v;
            }
        }
        offset = dot(maxvert-minvert,sep);
        
        dangle*=1.2f;
        if (offset<lastoffset) // no improvement
        {
            sep=lastsep;
            dangle*=0.5f/1.2f;
            offset = lastoffset;
        }
        else
        {
           diff = maxvert-minvert;
        }
        
        float3 axis = normalize(cross(diff,sep));
        lastsep = sep;
        lastoffset = offset;
        sep = rotateAxisAngle(axis,dangle,sep);
        offset = (maxoffset+minoffset)*0.5f;

    }

    return to_float4_aw(sep,offset);
}


__DEVICE__ float3 edge(uint i,uint j)
{
    float3 pa,pb;
    if (i<8u)
    {
        pa.x = (float)(i%4u<2u?1:-1);
        pa.y = (float)((i+1u)%4u<2u?1:-1);
        pa.z = (float)(i/4u<1u?1:-1);

        pb.x = (float)((i+1u)%4u<2u?1:-1);
        pb.y = (float)((i+2u)%4u<2u?1:-1);
        pb.z = (float)(i/4u<1u?1:-1);
    }
    else
    {
        pa.x = (float)(i%4u<2u?1:-1);
        pa.y = (float)((i+1u)%4u<2u?1:-1);
        pa.z = -1.0f;
        pb = to_float3_aw(swi2(pa,x,y),1.0f);
                
    }
    
    return j==0u?pa:pb;
}

__DEVICE__ float4 findCollisionPouint( float2 iResolution, __TEXTURE2D__ iChannel3, uint pixelx, uint pixely)
{
  
    uint ci = pixelx/12u;
    uint cj = pixely/4u;
    if (cj>=ci) { return to_float4_s(0.0f); } //discard;
    
    if (length(getCubePos(ci,iResolution,iChannel3)-getCubePos(cj,iResolution,iChannel3))>6.0f && cj!=0u) // bounding check
    {
        return to_float4(0.0f,0.0f,0.0f,0.0f);
    }
    
    uint j = pixelx%12u;
    
    if (pixely%4u<2u) // swap the two cubes to check collision both ways
    {
        uint t = ci;
        ci = cj;
        cj = t;
    }

    float3 pa = cubeTransform(cj,edge(j,0u),iResolution,iChannel3); // a world space edge of cube j
    float3 pb = cubeTransform(cj,edge(j,1u),iResolution,iChannel3);
    float ea=0.0f;
    float eb=1.0f;
    for(uint l=0u;l<((ci==0u)?1u:6u);l++) // clamp it with the 6 planes of cube i
    {
        float4 pl = getCubePlane(ci,l,iResolution,iChannel3);
        pl/=length(swi3(pl,x,y,z));
        if (_fabs(dot(swi3(pl,x,y,z),pb-pa))>0.0001f)
        {
            float e = -(dot(swi3(pl,x,y,z),pa)-pl.w)/dot(swi3(pl,x,y,z),pb-pa);
            if (dot(pb-pa,swi3(pl,x,y,z))>0.0f)
            {
                eb=_fminf(eb,e);
            }
            else
            {
                ea=_fmaxf(ea,e);
            }
        }
        else
        {
            ea=999999.0f; // edge is parallel to plane
        }
    }
    
    float3 coll = pa+(pb-pa)*((pixely%2u==0u)?ea:eb);
    if (eb<=ea || cj==0u)
    {
        coll = to_float3(0.0f,0.0f,0.0f);
    }
    
    
    return  to_float4_aw(coll,0.0f);
}

__KERNEL__ void ForkPhysicsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel3)
{
    fragCoord+=0.5f;

    uint pixelx = (uint)(_floor(fragCoord+0.01f).x);
    uint pixely = (uint)(_floor(fragCoord+0.01f).y);
    
    if (pixelx>=(CUBECOUNT*12u))
    {
        pixelx-=(CUBECOUNT*12u);
        fragColor = findSeparatingPlane_planA(iResolution,iChannel3, pixelx, pixely);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    fragColor = findCollisionPouint(iResolution,iChannel3, pixelx, pixely);   
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


//#define CUBECOUNT 16u
#define STATIC_CUBE_COUNT 1u

//float gravity = -0.003f;

//__DEVICE__ float4 readTex3(uint cx,uint cy)
//{
//    return texture(iChannel3,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
//}
__DEVICE__ float4 readTex2(uint cx,uint cy, float2 iResolution, __TEXTURE2D__ iChannel2)
{
    return _tex2DVecN(iChannel2,((float)(cx)+0.5f)/iResolution.x,((float)(cy)+0.5f)/iResolution.y,15);
}
__DEVICE__ float4 readTex0(uint cx,uint cy, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return _tex2DVecN(iChannel0,((float)(cx)+0.5f)/iResolution.x,((float)(cy)+0.5f)/iResolution.y,15);
}

/*
__DEVICE__ float3 getCubePos(uint ci)
{
    return readTex3(ci,0u).xyz;
}
__DEVICE__ float4 getCubeQuat(uint ci)
{
    return readTex3(ci,1u).xyzw;
}
*/

__DEVICE__ float3 getCubeVel(uint ci,float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return swi3(readTex3(ci,2u,iResolution,iChannel3),x,y,z);
}
__DEVICE__ float3 getCubeRotVel(uint ci,float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return swi3(readTex3(ci,3u,iResolution,iChannel3),x,y,z);
}


//uint pixelx,pixely;

//float3 pos;
//float4 quat;
//float3 vel;
//float3 rotvel;

__DEVICE__ float4 findbestsepplane(float2 iResolution, __TEXTURE2D__ iChannel0, uint pixelx, uint pixely)
{
  
    pixelx-=CUBECOUNT;
    uint cia = pixelx;
    uint cib = pixely;
    if (cia>=CUBECOUNT) {  return to_float4_s(0.0f); } //discard;
    if (cib>=cia)       {  return to_float4_s(0.0f); } //discard;
    float best=1e30;
    float4 bestsep;
    for(uint m=0u;m<30u;m++)
    {
        float4 sep = readTex0(cia*6u+m%6u+CUBECOUNT*12u,cib*5u+m/6u,iResolution,iChannel0);
        if (length(swi3(sep,x,y,z))<best)
        {
            best = length(swi3(sep,x,y,z));
            bestsep = sep;
        }
    }
    return to_float4_aw(normalize(swi3(bestsep,x,y,z)),bestsep.w);;
}

__KERNEL__ void ForkPhysicsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;

    uint pixelx = uint(_floor(fragCoord+0.01f).x);
    uint pixely = uint(_floor(fragCoord+0.01f).y);
    
    fragColor = to_float4(0.0f,0.0f,0.0f,0.0f);
    
    if (pixelx>=(CUBECOUNT)) 
    {
        fragColor = findbestsepplane(iResolution, iChannel0, pixelx, pixely);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    if (pixely>=4u || pixely<2u) { SetFragmentShaderComputedColor(fragColor); return; } //discard; // just output velocity and rotational velocity
    uint cubei = pixelx;
    
    
    float3 pos = getCubePos(cubei,iResolution,iChannel3);
    float4 quat = getCubeQuat(cubei,iResolution,iChannel3);
    float3 vel = getCubeVel(cubei,iResolution,iChannel3);
    float3 rotvel = getCubeRotVel(cubei,iResolution,iChannel3);
    
    if (cubei>=STATIC_CUBE_COUNT)
    {
    // apply forces (just the changes)
    for(uint i=0u;i<CUBECOUNT-1u;i++)
    {
        uint ci,cj;
        float scaler;
        if (i<cubei)
        {
          ci = cubei;
            cj = i;
            scaler  = 1.0f;
     // if the other cube cannot be pushed away, because its's the floor or other unmovable, 
          // this one moves double amount
            if (cj<STATIC_CUBE_COUNT) scaler = 2.0f; 
        }
        else
        {
           ci = i+1u;
           cj = cubei;
           scaler = -1.0f; // applying the opposite forces on the cube pair
        }
        if (!(length(getCubePos(ci,iResolution,iChannel3)-getCubePos(cj,iResolution,iChannel3))>6.0f && cj!=0u)) // bounding check
        for(uint j=0u;j<48u;j++)
        {
            float3 forcepos = swi3(readTex0(ci*12u+j%12u,cj*4u+j/12u,iResolution,iChannel0),x,y,z);
            if (forcepos.x!=0.0f)
            {
              float3 force = swi3(readTex2(ci*12u+j%12u,cj*4u+j/12u,iResolution,iChannel2),x,y,z);
                
                const float RotationalImmobilityTensor = 1.8f;
                force *= scaler;
                vel += force;
                rotvel -= cross(forcepos-pos,force)/RotationalImmobilityTensor;
            }
        }
    }
    

    }
    
    fragColor = to_float4_aw(vel, 0.0f);
    if (pixely==3u) fragColor = to_float4_aw(rotvel, 0.0f);
    

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


//#define CUBECOUNT 16
/*
__DEVICE__ float4 readTex3(int cx,int cy)
{
    return texture(iChannel3,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
__DEVICE__ float4 readTex2(int cx,int cy)
{
    return texture(iChannel2,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
*/

__DEVICE__ float4 readTex1(int cx,int cy,float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return _tex2DVecN(iChannel1,((float)(cx)+0.5f)/iResolution.x,((float)(cy)+0.5f)/iResolution.y,15);
}

/*
__DEVICE__ float4 readTex0(int cx,int cy)
{
    return texture(iChannel0,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}

__DEVICE__ float3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
__DEVICE__ float4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}
*/
__DEVICE__ float3 getCubeVel1(int ci,float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return swi3(readTex1(ci,2,iResolution,iChannel1),x,y,z);
}
__DEVICE__ float3 getCubeRotVel1(int ci,float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return swi3(readTex1(ci,3,iResolution,iChannel1),x,y,z);
}
__DEVICE__ float3 getCubeVelQP(int ci,float3 querypos,float2 iResolution, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3)
{
    return cross(querypos-getCubePos(ci,iResolution,iChannel3),getCubeRotVel1(ci,iResolution,iChannel1))+getCubeVel1(ci,iResolution,iChannel1);
}



//int pixelx,pixely;

__KERNEL__ void ForkPhysicsFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;

    int pixelx = int(_floor(fragCoord+0.01f).x);
    int pixely = int(_floor(fragCoord+0.01f).y);
    
  
    int ci = pixelx/12;
    int cj = pixely/4;
    if (cj>=ci)        { SetFragmentShaderComputedColor(fragColor); return; } //discard;
    if (ci>=CUBECOUNT) { SetFragmentShaderComputedColor(fragColor); return; } //discard;
    
    if ((length(getCubePos(ci,iResolution,iChannel1)-getCubePos(cj,iResolution,iChannel1))>6.0f && cj!=0))  // bounding check
    {
        fragColor = to_float4(0.0f,0.0f,0.0f,0.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    int forceid = pixelx%12+pixely%4*12;
    float3 totalForce;
  
    int i = forceid;
    int lpx = ci*12+i%12;
    int lpy = cj*4+i/12;

    totalForce = swi3(readTex2(lpx,lpy,iResolution,iChannel2),x,y,z);
    float3 collpos = swi3(readTex0(lpx,lpy,iResolution,iChannel0),x,y,z);

    if (collpos.x!=0.0f) // x==0 means no collision at the force denoted by this pixel
    {
        float3 veldiff = getCubeVelQP(cj,collpos,iResolution,iChannel1,iChannel3)-getCubeVelQP(ci,collpos,iResolution,iChannel1,iChannel3);

        float3 collisNormal = swi3(readTex1(ci+CUBECOUNT,cj,iResolution, iChannel1),x,y,z);

        totalForce += veldiff*0.022f;

        float perpart = dot(collisNormal,totalForce);
        float3 tangentialpart = totalForce-collisNormal*perpart;

        const float FrictionConstant = 0.7f;

        if (length(tangentialpart)>perpart*FrictionConstant)
        {
            tangentialpart *= (perpart*FrictionConstant)/length(tangentialpart);
            totalForce = tangentialpart + collisNormal*perpart;
        }


        if (perpart<0.0f)
        {
            totalForce = to_float3(0.0f,0.0f,0.0f);
        }

    }
    else
    {
        totalForce= to_float3(0.0f,0.0f,0.0f);
    }
     
    
    if (iFrame==0) totalForce = to_float3(0.0f,0.0f,0.0f);
    fragColor = to_float4_aw(totalForce,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


//#define CUBECOUNT 16
//#define STATIC_CUBE_COUNT 1

#define BUFD

#define PI (_acosf(0.0f)*2.0f)
//float gravity = -0.0045f;

/*
__DEVICE__ float4 readTex3(int cx,int cy)
{
    return texture(iChannel3,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
__DEVICE__ float4 readTex2(int cx,int cy)
{
    return texture(iChannel2,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
__DEVICE__ float4 readTex1(int cx,int cy)
{
    return texture(iChannel1,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}

__DEVICE__ float4 readTex0(int cx,int cy)
{
    return texture(iChannel0,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
__DEVICE__ float3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
__DEVICE__ float4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}
*/

__DEVICE__ float3 getCubeVel(int ci,float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return swi3(readTex3(ci,2,iResolution, iChannel3),x,y,z);
}
__DEVICE__ float3 getCubeRotVel(int ci,float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return swi3(readTex3(ci,3,iResolution,iChannel3),x,y,z);
}

/*
__DEVICE__ float3 rotate(float4 quat,float3 v)
{
    float sinsqr = (1.0f-quat.w*quat.w);
    if (sinsqr!=0.0f)
    {
        v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
        v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
    }
    return v;
}
*/
//int pixelx,pixely;

__DEVICE__ float3 rotateAxis(float3 axis,float3 v) // the length of the axis defines angle
{
    float len = length(axis);
    if (len!=0.0f)
    {
        axis = normalize(axis);
    v = v*_cosf(len) + axis*((v*axis) * (1.0f-_cosf(len))) + cross(v,axis)*_sinf(len);
    }
    return v;
}


__DEVICE__ float4 rotateRotation(float4 q,float3 axis) // Im sure, there is a simpler way to rotate a rotation :)
{
    float3 x,y,z; // conversion to 3 perpendicular vectors, and rotation
    x = rotateAxis(axis,rotate(q,to_float3(1.0f,0.0f,0.0f)));
    y = rotateAxis(axis,rotate(q,to_float3(0.0f,1.0f,0.0f)));
    z = rotateAxis(axis,rotate(q,to_float3(0.0f,0.0f,1.0f)));
    
    // convert back to quaternion
  float trace = x.x + y.y + z.z; 
  if( trace > 0.0f ) {
    float s = 0.5f / _sqrtf(trace+ 1.0f);
    q.w = 0.25f / s;
    q.x = ( z.y - y.z ) * s;
    q.y = ( x.z - z.x ) * s;
    q.z = ( y.x - x.y ) * s;
  } else {
    if ( x.x > y.y && x.x > z.z ) {
      float s = 2.0f * _sqrtf( 1.0f + x.x - y.y - z.z);
      q.w = (z.y - y.z ) / s;
      q.x = 0.25f * s;
      q.y = (x.y + y.x ) / s;
      q.z = (x.z + z.x ) / s;
    } else if (y.y > z.z) {
      float s = 2.0f * _sqrtf( 1.0f + y.y - x.x - z.z);
      q.w = (x.z - z.x ) / s;
      q.x = (x.y + y.x ) / s;
      q.y = 0.25f * s;
      q.z = (y.z + z.y ) / s;
    } else {
      float s = 2.0f * _sqrtf( 1.0f + z.z - x.x - y.y );
      q.w = (y.x - x.y ) / s;
      q.x = (x.z + z.x ) / s;
      q.y = (y.z + z.y ) / s;
      q.z = 0.25f * s;
    }
  }

  q=normalize(q); // no scaling :)
    
  return q;    
}


//float3 pos;
//float4 quat;
//float3 vel;
//float3 rotvel;

/*
__DEVICE__ void initScene(int cubei)
{
        if (cubei==0) // static floor
        {
            pos = to_float3(0,-1,5);
            vel = to_float3(0.0f,0.0f,0.0f);
            rotvel = to_float3(0.0f,0.0f,0.0f);
            quat = to_float4(0.0f,0.0f,0.0f,1.0f);
            return;
        }
    
    
      cubei--;
        float cubeif = float(cubei);
      int div = 5;
      float3 ro = to_float3_aw(0.0f,PI*(float(cubei)*2.0f)/float(div),0.0f);
      pos = rotateAxis(ro,   to_float3_aw(0.0f,1.0f+float(cubei/div)*2.0f,2.5f));
    
        quat = rotateRotation(normalize(to_float4(0.0f,0.0f,0.0f,1.0f)),ro);
    
        vel = to_float3(-0.00f,0.0f,0.00f);
        rotvel = to_float3(cubeif*-0.001f*_cosf(float(iFrame)),0.0f,cubeif*-0.001f); // randomize start setup
    
      if (cubei==CUBECOUNT-2) // thrown cube
        {
            pos = to_float3(16.0f,2.0f,-1.0f+_sinf(float(iFrame)*1.2f)); // randomize 
            vel = to_float3_aw(-0.37f,0.14f+_sinf(float(iFrame))*0.03f,0.0f);
        }
        
}
*/

__KERNEL__ void ForkPhysicsFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    fragCoord+=0.5f;
  
    float gravity = -0.0045f;

    int pixelx = int(_floor(fragCoord+0.01f).x);
    int pixely = int(_floor(fragCoord+0.01f).y);
    
    fragColor = to_float4(0.0f,0.0f,0.0f,0.0f);
    
    if (pixely>=4)           { SetFragmentShaderComputedColor(fragColor); return; } //discard;
    if (pixelx>=(CUBECOUNT)) { SetFragmentShaderComputedColor(fragColor); return; } //discard;
    int cubei = pixelx;
    
    
    float3 pos = getCubePos(cubei,iResolution,iChannel3);
    float4 quat = getCubeQuat(cubei,iResolution,iChannel3);
    float3 vel = getCubeVel(cubei,iResolution,iChannel3);
    float3 rotvel = getCubeRotVel(cubei,iResolution,iChannel3);

    if (cubei>=STATIC_CUBE_COUNT)
    {
    // apply forces (just the changes)
    for(int i=0;i<CUBECOUNT-1;i++)
    {
        int ci,cj;
        float scaler;
        if (i<cubei)
        {
          ci = cubei;
          cj = i;
          scaler  = 1.0f;
          // if the other cube cannot be pushed away, because its's the floor or other unmovable, 
          // this one moves double amount
          if (cj<STATIC_CUBE_COUNT) scaler = 2.0f; 
        }
        else
        {
           ci = i+1;
           cj = cubei;
           scaler = -1.0f; // applying the opposite forces on the cube pair
        }
        
        if (!(length(getCubePos(ci,iResolution,iChannel3)-getCubePos(cj,iResolution,iChannel3))>6.0f && cj!=0)) // bounding check
        for(int j=0;j<48;j++)
        {
            float3 forcepos = swi3(readTex0(ci*12+j%12,cj*4+j/12,iResolution,iChannel0),x,y,z);
            if (forcepos.x!=0.0f)
            {
              float3 force = swi3(readTex2(ci*12+j%12,cj*4+j/12,iResolution,iChannel2),x,y,z);
                
#ifdef BUFD                
                // add repulsive force
                float4 collisnormal = readTex1(ci+CUBECOUNT,cj,iResolution,iChannel1);
                swi3S(collisnormal,x,y,z, swi3(collisnormal,x,y,z) * _fmaxf(_fabs(dot(forcepos,swi3(collisnormal,x,y,z))-collisnormal.w)-0.01f,0.0f));
                force += swi3(collisnormal,x,y,z)*0.003f;
#endif
                
                const float RotationalImmobilityTensor = 1.8f;
                force *= scaler;
                vel += force;
                rotvel -= cross(forcepos-pos,force)/RotationalImmobilityTensor;
            }
        }
    }

#ifdef BUFD
    // move by adding velocity to position, and rotate
    pos += vel;
    quat = rotateRotation(quat,rotvel);
    vel.y += gravity;
#endif
    }

#ifdef BUFD    
    if (iFrame%(60*8)==0)
    {
      if (cubei==0) // static floor
      {
          pos = to_float3(0,-1,5);
          vel = to_float3(0.0f,0.0f,0.0f);
          rotvel = to_float3(0.0f,0.0f,0.0f);
          quat = to_float4(0.0f,0.0f,0.0f,1.0f);
      }
      else    
      {
        cubei--;
        float cubeif = (float)(cubei);
        int div = 5;
        float3 ro = to_float3(0.0f,PI*((float)(cubei)*2.0f)/(float)(div),0.0f);
        pos = rotateAxis(ro, to_float3(0.0f,1.0f+(float)(cubei/div)*2.0f,2.5f));
      
          quat = rotateRotation(normalize(to_float4(0.0f,0.0f,0.0f,1.0f)),ro);
      
          vel = to_float3(-0.00f,0.0f,0.00f);
          rotvel = to_float3(cubeif*-0.001f*_cosf(float(iFrame)),0.0f,cubeif*-0.001f); // randomize start setup
      
        if (cubei==CUBECOUNT-2) // thrown cube
        {
          pos = to_float3(16.0f,2.0f,-1.0f+_sinf((float)(iFrame)*1.2f)); // randomize 
          vel = to_float3(-0.37f,0.14f+_sinf((float)(iFrame))*0.03f,0.0f);
        }
      }
    }

    fragColor = to_float4_aw(pos, 0.0f);
    if (pixely==1) fragColor = quat;
#endif
    if (pixely==2) fragColor = to_float4_aw(vel, 0.0f);
    if (pixely==3) fragColor = to_float4_aw(rotvel, 0.0f);
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


/*
How it works:

Buf A find collision points. World space edges of a cube are clamped with the planes of the 
other cube. Then the other way around, giving a wireframe of the intersecting parts.
Both line ends will count as a collision, this gives 48 possible collision points per cube pair.
12x4 pixels are allocated for each pair (i>j)

Buf A on the right side of it  stores a separating plane between each pair. While colliding, 
there is no separating plane, but we need the plane with the least reach through. 
As the best plane is usually a cross product of two edges colliding, or a plane of one of the cube,
which collides with a vertex. That counts 30 posibilities per pair, these are stored in 
separate pixels. The offset of the plane right in the middle of the intersection is stored into w. 
The penetration level is stored as the length of the .xyz vector (defining a plane),
which normally has a length of 1.0f, but there is no more component left.

Buf B then selects the best separating plane from Buf A by comparing the length(.xyz) of the 
30 texels, and stores it normalized in the right side of buf B

Buf B in the left side stores updated velocity and rotational velocity after applying
the forces stored in Buf C from last frame. The idea is to use the forces from last frame
as an initial guess for solving the constraints, if they already collided last frame. 
This is verlet integration.

Buf C calculates the forces that would relax the velocity difference at the collision points.
(after having reduced those, by applying forces from last frame in buf B)
It clamps the forces into a friction cone including the force's component, that was already 
added in Buf B. 

Buf D adds the forces stored in Buf C to the velocity and rotational velocity from last frame
(not using the updated velocities in buf B, instead reading buf D) 
It also adds a repulsive force to move the cubes out of penetration.
Buf B has the best separating plane including it's offset, which is used to estimate 
the penetration at a collision point.
Then it updates the translation of the cubes.
The actual rotation is stored as a quaternion. It gets converted to 3 perpendicular
vectors and then back to quaternion after rotating them by the angular velocity (called rotvel)
Velocity is added to position. Gravity is added to velocity after the translation update.

Image: No ray marching



*/

//#define CUBECOUNT 16
//#define PI 3.141592653f
/*
__DEVICE__ float4 readTex3(int cx,int cy)
{
    return texture(iChannel3,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}
__DEVICE__ float4 readTex2(int cx,int cy)
{
    return texture(iChannel2,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}

__DEVICE__ float4 readTex0(int cx,int cy)
{
    return texture(iChannel0,to_float2((float(cx)+0.1f)/iResolution.x,(float(cy)+0.1f)/iResolution.y));
}

__DEVICE__ float3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
__DEVICE__ float4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}
*/

__DEVICE__ float3 getCollision(int ci,int cj,int k, float2 iResolution, __TEXTURE2D__ iChannel0) // for debugging
{
    return swi3(readTex0(ci*12+k%12,cj*4+k/12,iResolution,iChannel0),x,y,z);
}

/*
__DEVICE__ float3 rotate(float4 quat,float3 v)
{
    float sinsqr = (1.0f-quat.w*quat.w);
    if (sinsqr!=0.0f)
    {
      v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
      v=v*quat.w + swi3(quat,x,y,z)*((dot(v,swi3(quat,x,y,z))*(1.0f-quat.w))*(1.0f/sinsqr)) + cross(v,swi3(quat,x,y,z));
    }
    return v;
}

__DEVICE__ float3 rotateAxis(float3 axis,float3 v)
{
    float len = length(axis);
    if (len!=0.0f)
    {
      axis = normalize(axis);
      v = v*_cosf(len) + axis*((v*axis) * (1.0f-_cosf(len))) + cross(v,axis)*_sinf(len);
    }
    return v;
}
*/

__DEVICE__ float4 getCubePlane(int ci,int k,float2 iResolution,__TEXTURE2D__ iChannel3)
{
    float3 norm = to_float3( k/2%3==0?1.0:0.0, k/2%3==1?1.0:0.0, k/2%3==2?2.0:0.0)*(float)(k%2*2-1);
    norm = rotate(getCubeQuat(ci,iResolution,iChannel3),norm);
    float offset = 1.0f + dot(getCubePos(ci,iResolution,iChannel3),norm);
    return to_float4_aw(norm,offset);
}


__DEVICE__ float3 cubeTransform(int ci,float3 lp,float2 iResolution,__TEXTURE2D__ iChannel3)
{
    return getCubePos(ci,iResolution,iChannel3) + rotate(getCubeQuat(ci,iResolution,iChannel3),lp);
}

//float3 rayPos,rayDir;
//float minDist;
//float3 minDistNormal;
//float minDistMaterial;

//float3 sunDir = normalize(to_float3(0.8f,1.0f,-0.3f));

__DEVICE__ void renderCube(int ci,float2 iResolution,__TEXTURE2D__ iChannel3, float3 rayPos,float3 rayDir, float *minDist, float3 *minDistNormal, float *minDistMaterial)
{
    float backMin = 1e30;
    float frontMax = -1e30;
    float3 frontNormal = to_float3(0.0f,0.0f,0.0f);
    
    for(int side=0;side<6;side++)
    {
        float4 plane = getCubePlane(ci,side,iResolution,iChannel3);
        
        float rayTravel = -(dot(rayPos,swi3(plane,x,y,z))-plane.w)/dot(swi3(plane,x,y,z),rayDir);
        if (  dot(swi3(plane,x,y,z),rayDir)<0.0f)
        {
            if (frontMax < rayTravel)
            {
                frontMax = rayTravel;
                frontNormal = swi3(plane,x,y,z);
            }
        }
        else
        {
            if (backMin > rayTravel)
            {
                backMin = rayTravel;
            }
        }
    }
    if (frontMax<backMin) // cube hit
    {
        if (*minDist>frontMax && frontMax>0.0f) // chose closeset cube
        {
            *minDist = frontMax;
            *minDistNormal = frontNormal;
//          *minDistMaterial = 1.0f-fract(float(ci)/float(CUBECOUNT)*94.0f);
            *minDistMaterial = fract((float)(ci)/3.0f)+0.1f;
        }
    }

}


__DEVICE__ float3 backGround(float3 dir,float3 pos, float3 sunDir)
{
  float f = _fmaxf(dir.y,0.0f)*0.5f+0.5f;
  float3 color = 1.0f-to_float3(1,0.85f,0.7f)*f;
  color *= dir.x*-0.3f+1.0f;
  
  if (dot(sunDir,dir)>0.0f) // sun reflected on cubes
  {
   f = _fmaxf(length(cross(sunDir,dir))*10.0f,1.0f);
    
   color += to_float3(1,0.9f,0.7f)*40.0f/(f*f*f*f);
  }
  return color;
}

//float4 debugColor;
//float debugthickness = 0.02f;
__DEVICE__ void debugline(float3 pa,float3 pb, float3 rayPos,float3 rayDir, float4 *debugColor, float debugthickness)
{
    float3 pl = normalize(cross(cross(pa-pb,rayDir),pa-pb));
    float d = -dot(pl,rayPos-pa)/dot(pl,rayDir);
    float3 p = rayPos+rayDir*d;
    if (dot(p-pb,pa-pb)>0.0f && dot(p-pb,pa-pb)<dot(pa-pb,pa-pb) && length(cross(p-pa,normalize(pa-pb)))<debugthickness )
    {
      *debugColor = to_float4(0.9f,0.8f,0.1f,1.0f);
    }
}

__DEVICE__ void debugdot(float3 p, float3 rayPos,float3 rayDir, float4 *debugColor, float debugthickness)
{
    float d = _powf(length(cross(p-rayPos,rayDir))/debugthickness,4.0f);
    *debugColor = _mix(*debugColor,to_float4(0.9f,0.8f,0.1f,1.0f),_fmaxf(1.0f-d,0.0f));
}


__DEVICE__ void debugCollision(int ci,int cj,float2 iResolution,__TEXTURE2D__ iChannel0,__TEXTURE2D__ iChannel2, float3 rayPos,float3 rayDir, float4 *debugColor, float debugthickness)
{
    float4 sep = readTex0(ci+CUBECOUNT*12,cj,iResolution,iChannel0);
    for(int k=0;k<48;k++)
    {
      float3 cpos = getCollision(ci,cj,k,iResolution,iChannel0);
      debugdot(cpos, rayPos,rayDir, debugColor,debugthickness);
      debugline(cpos,cpos + swi3(readTex2(ci*12+k%12,cj*4+k/12,iResolution,iChannel2),x,y,z)*10.0f, rayPos, rayDir, debugColor, debugthickness );
    }
}

__DEVICE__ void renderScene(float2 iResolution,__TEXTURE2D__ iChannel0, float3 rayPos,float3 rayDir, float *minDist, float3 *minDistNormal, float *minDistMaterial, __TEXTURE2D__ iChannel3)
{
    *minDistMaterial = 0.0f;
    *minDist = 1e30;
    *minDistNormal = to_float3(0.0f,1.0f,0.0f);
    
    if (rayDir.y<0.0f)
    {
        *minDist = rayPos.y/-rayDir.y;
    }
    
    for(int i=1;i<(CUBECOUNT);i++)
    {
        if ( length(cross(getCubePos(i,iResolution,iChannel3)-rayPos,rayDir))<3.0f)
        {
        renderCube(i,iResolution,iChannel3, rayPos,rayDir,minDist,minDistNormal,minDistMaterial);
        }
    }
}

__DEVICE__ float3 getDiffuse(float2 iResolution,__TEXTURE2D__ iChannel1, float3 rayPos, float minDistMaterial)
{
        float3 difColor;
        
        float  ratio = iResolution.x/iResolution.y;
        float2 tuv = to_float2(rayPos.z/ratio, rayPos.x);
        
        if (minDistMaterial==0.0f) difColor = swi3(texture(iChannel1,tuv/8.0f),x,y,z); // floord ifColor = swi3(texture(iChannel1,swi2(rayPos,z,x)/8.0f),x,y,z); // floor
        else
        {
            difColor = to_float3(1.0f,1.0f-minDistMaterial,0.3f-0.3f*minDistMaterial); // cube colors
        }
    return difColor;
}

__KERNEL__ void ForkPhysicsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  
  CONNECT_POINT0(Look, 0.0f, 0.0f);
  CONNECT_SLIDER0(LookZ, -10.0f, 10.0f, 0.0f);
  
  CONNECT_POINT1(LookDir, 0.0f, 0.0f);
  CONNECT_SLIDER1(LookDirZ, -10.0f, 10.0f, 0.0f);
  
  fragCoord+=0.5f;

  float minDist;
  float3 minDistNormal;
  float minDistMaterial;

  float3 sunDir = normalize(to_float3(0.8f,1.0f,-0.3f));

  float4 debugColor = to_float4(0.0f,0.0f,0.0f,0.0f);
  float debugthickness = 0.02f;
  
  float3 campos = to_float3(0.0f-fract((float)(iFrame)/(60.0f*8.0f))*4.0f,6.4f-_sinf(iTime/4.0f)*0.5f,-17.0f);
  float2 uv = fragCoord / iResolution;
  float3 pdir = to_float3_aw(uv*2.0f-1.0f,2.0f);
  pdir.y /= iResolution.x/iResolution.y; // wide screen
    pdir = normalize(pdir);
    
    pdir = rotateAxis( to_float3(-0.27f,0.0f,0.0f), pdir);

    float3 rayPos = campos + to_float3(Look.x,Look.y,LookZ);
    float3 rayDir = pdir + to_float3(LookDir.x,LookDir.y,LookDirZ);;

    renderScene(iResolution, iChannel0, rayPos, rayDir, &minDist, &minDistNormal, &minDistMaterial,iChannel3);
//  debugCubeEdges(i);
//  debugdot(getCubePos(i),rayPos, rayDir, &debugColor,debugthickness);
//  for(int j=0;j<8;j++) debugdot(getWCubeVert(i,j,iResolution,iChannel3),rayPos, rayDir, &debugColor,debugthickness);
    
 
//  debugCollision(1,0, iResolution, iChannel0, iChannel2, rayPos, rayDir, &debugColor, debugthickness);
//  debugline(getCubePos(1),g,rayPos,rayDir, &debugColor,debugthickness);    
    
    
    
    if (minDist<1e30)
    {
        minDistNormal = normalize(minDistNormal);
        const float3 sunColor = to_float3(1.0f,0.8f,0.5f)*1.0f;
        const float3 skyColor = to_float3(1.0f,1.2f,1.5f)*0.6f;
        rayPos += rayDir*minDist;
        float3 firstHitPos = rayPos;
        float3 refdir = reflect(rayDir,minDistNormal);
        float f = 1.0f-_fmaxf(dot(minDistNormal,-rayDir),0.0f);
        float fresnel = 0.65f*f*f*f*f*f+0.05f;

        float3 difColor =getDiffuse(iResolution,iChannel1, rayPos, minDistMaterial);
        
        fragColor = to_float4_aw(difColor*((minDistNormal).y*0.5f+0.5f),0.0f);//to_float4_aw(difColor*skyColor*((minDistNormal).y*0.5f+0.5f),0.0f);
        float suncos = dot((minDistNormal),sunDir);
        if (suncos>0.0f)
        {
          // spot sun light pointing on the thrown cube.
          float3 v = cross(sunDir,rayPos-getCubePos(CUBECOUNT-1,iResolution,iChannel3))/20.0f;
          suncos *= _fmaxf(0.0f,1.0f-dot(v,v));
          rayDir = sunDir;
          renderScene(iResolution, iChannel0, rayPos, rayDir, &minDist, &minDistNormal, &minDistMaterial,iChannel3);
          if (minDist==1e30) 
            {
                swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + difColor * suncos * sunColor);
            }
        }
        
        rayPos = firstHitPos;
        rayDir = refdir;
        renderScene(iResolution, iChannel0, rayPos, rayDir, &minDist, &minDistNormal, &minDistMaterial,iChannel3);
        
        float3 refColor;
        if (minDist<1e30)
        {
            rayPos += rayDir * minDist;

            float3 difColor = getDiffuse(iResolution,iChannel1, rayPos, minDistMaterial);
            
            refColor = difColor*(normalize(minDistNormal).y*0.5f+0.5f);
        }
        else
        {
            refColor = backGround(rayDir,rayPos, sunDir);
        }
        swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z),refColor,fresnel));
    }
    else
    {
    fragColor = to_float4_aw(backGround(pdir,campos,sunDir),0.0f);
    }
    
    swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z),swi3(debugColor,x,y,z),debugColor.w));


  SetFragmentShaderComputedColor(fragColor);
}