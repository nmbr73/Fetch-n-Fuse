
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define SIZE     0.07f
//#define FRICTION 0.25f

__DEVICE__ float hash1( float2  p ) { float n = dot(p,to_float2(127.1f,311.7f)); return fract(_sinf(n)*43758.5453f); }

__DEVICE__ float4 getParticle( float2 id, float2 iResolution, __TEXTURE2D__ iChannel0 )
{
    return texture( iChannel0, (id+0.5f)/iResolution);
}

__DEVICE__ float4 react( in float4 p, in float2 qid, in float2 rid, float2 iResolution, __TEXTURE2D__ iChannel0, float SIZE)
{
    float2 q = swi2(getParticle( qid, iResolution, iChannel0 ),x,y);
    float2 r = swi2(getParticle( rid, iResolution, iChannel0 ),x,y);
    
    float2 m = (q + r) * 0.5f;
    float2 n = normalize(swi2((q - r),y,x) * to_float2(1.0f,-1.0f)) * SIZE * 0.7071f;
    
    swi2S(p,x,y, _mix(swi2(p,x,y), m + n, 0.2f));
    
    return p;
}

__DEVICE__ float4 solveConstraints( in float2 id, in float4 p, float2 iResolution, __TEXTURE2D__ iChannel0, float SIZE )
{
    if( id.x > 0.5f && id.y > 0.5f)  p = react( p, id + to_float2(-1.0f, 0.0f), id + to_float2( 0.0f,-1.0f), iResolution, iChannel0, SIZE);
    if( id.x > 0.5f && id.y < 8.5f)  p = react( p, id + to_float2( 0.0f, 1.0f), id + to_float2(-1.0f, 0.0f), iResolution, iChannel0, SIZE);
    if( id.x < 8.5f && id.y > 0.5f)  p = react( p, id + to_float2( 0.0f,-1.0f), id + to_float2( 1.0f, 0.0f), iResolution, iChannel0, SIZE);
    if( id.x < 8.5f && id.y < 8.5f)  p = react( p, id + to_float2( 1.0f, 0.0f), id + to_float2( 0.0f, 1.0f), iResolution, iChannel0, SIZE);
    return p;
}    

__DEVICE__ float4 move( in float4 p, in float2 id, float2 iResolution, __TEXTURE2D__ iChannel0, float iTimeDelta, float SIZE, float FRICTION )
{
    const float g = 0.6f;

    //float iTimeDelta = 0.01f;

    // acceleration
    swi2S(p,x,y, swi2(p,x,y) + iTimeDelta*iTimeDelta*to_float2(0.0f,-g));
    
    // collide screen
    if( p.x < 0.00f ) { p.x = 0.00f; p.w = _mix(p.w, p.y, FRICTION);}
    if( p.x > 1.77f ) { p.x = 1.77f; p.w = _mix(p.w, p.y, FRICTION);}
    if( p.y < 0.00f ) { p.y = 0.00f; p.z = _mix(p.z, p.x, FRICTION);}        
    if( p.y > 1.00f ) { p.y = 1.00f; p.z = _mix(p.z, p.x, FRICTION);}

    // constrains
    p = solveConstraints( id, p, iResolution, iChannel0, SIZE );
        
    #if 0
    if( id.y > 8.5f )  p.x = 0.05f + 0.1f*id.x, p.y = 0.05f +0.1f*id.y; //swi2(p,x,y) = 0.05f + 0.1f*id;
    #endif
    
    // inertia
    float2 np = 2.0f*swi2(p,x,y) - swi2(p,z,w);
    //swi2(p,z,w) = swi2(p,x,y);
    p.z = p.x;
    p.w = p.y;
    //swi2(p,x,y) = np;
    p.x = np.x;
    p.y = np.y;

    return p;
}



__KERNEL__ void D2DJellyJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0)
{


    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Orig, 1);

    CONNECT_SLIDER0(TimeDelta, -1.0f, 1.0f, 0.01f);
    CONNECT_SLIDER1(SIZE, -1.0f, 1.0f, 0.07f);
    CONNECT_SLIDER2(FRICTION, -1.0f, 1.0f, 0.25f);

    fragCoord+=0.5f;

    float2 id = _floor( fragCoord-0.4f );
    
    if( id.x>9.5f || id.y>9.5f ) 
    { //discard;
         fragColor = to_float4_s(0.0f);
         SetFragmentShaderComputedColor(fragColor); 
         return;
    }
    
    float4 p = getParticle(id, iResolution,iChannel0);
    
    if( iFrame==0 )
    {
        //swi2(p,x,y) = 0.15f + id * SIZE;
        p.x = 0.15f + id.x * SIZE;
        p.y = 0.15f + id.y * SIZE;
        //swi2(p,z,w) = swi2(p,x,y) - 0.01f*to_float2(0.5f+0.5f*hash1(id),0.0f);
        p.z = p.x - 0.01f*(0.5f+0.5f*hash1(id));
        p.w = p.y - 0.01f*(0.0f);
    }
    else
    {
      p = move( p, id, iResolution, iChannel0, TimeDelta, SIZE, FRICTION );
    }
    
    if(Orig)
    {
    
      if(iMouse.z > 0.5f && id.x < 0.5f && id.y < 0.5f){
          swi2S(p,x,y, swi2(iMouse,x,y)/iResolution * to_float2(1.77f, 1.0f));
          //swi2(p,z,w) = swi2(p,x,y);
      }
    }  
    else
    {
      if(iMouse.z > 0.5f){
        swi2S(p,x,y,  swi2(p,x,y) - (swi2(iMouse,x,y)/iResolution * to_float2(1.77f, 1.0f) - 0.0f) / 50.0f);
        swi2S(p,z,w, swi2(p,x,y));
      }  
    }


    fragColor = p;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 4' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Modified version of 2D Cloth https://www.shadertoy.com/view/4dG3R1
// with texture mapping, mouse interaction, and inversion-resistant constraints

// The MIT License
// Copyright © 2016 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//__DEVICE__ float hash1I( float2 p ) { float n = dot(p,to_float2(127.1f,311.7f)); return fract(_sinf(n)*153.4353f); }

__DEVICE__ float4 getParticleI( float2 id, float2 iResolution, __TEXTURE2D__ iChannel0 )
{
    return texture( iChannel0, (id+0.5f)/iResolution );
}

__KERNEL__ void D2DJellyJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    //float2 uv = fragCoord/iResolution.y;
    float2 uv = fragCoord/iResolution;
    
    float ratio = iResolution.x/iResolution.y;
    
    uv.x*=ratio;
    
    float3 f = to_float3_s(0.0f);
    for( int j=1; j<10; j++ )
    for( int i=1; i<10; i++ )
    {
        float2 ij0 = to_float2(i-1,j-1);
        float2 ij1 = to_float2(i  ,j  );
        float4 p00 = getParticleI(to_float2(ij0.x,ij0.y), iResolution,iChannel0);
        float4 p01 = getParticleI(to_float2(ij0.x,ij1.y), iResolution,iChannel0);
        float4 p10 = getParticleI(to_float2(ij1.x,ij0.y), iResolution,iChannel0);
        float4 p11 = getParticleI(to_float2(ij1.x,ij1.y), iResolution,iChannel0);
        float2 d0 = (swi2(uv,y,x) - swi2(p00,y,x)) * to_float2(1.0f, -1.0f);
        float2 d1 = (swi2(uv,y,x) - swi2(p11,y,x)) * to_float2(1.0f, -1.0f);
        float2 n00 = normalize(swi2(p01,x,y) - swi2(p00,x,y));
        float2 n10 = normalize(swi2(p11,x,y) - swi2(p10,x,y));
        float2 n01 = normalize(swi2(p10,x,y) - swi2(p00,x,y));
        float2 n11 = normalize(swi2(p11,x,y) - swi2(p01,x,y));
        float2 uv0 = to_float2( dot(d0, n00),-dot(d0, n01));
        float2 uv1 = to_float2(-dot(d1, n10), dot(d1, n11));
        float2 max_uv = _fmaxf(uv0, uv1);
        float2 texture_uv = (uv0 / (uv0 + uv1) + ij0) / 9.0f;
        float3 col = swi3(_tex2DVecN( iChannel1,texture_uv.x/ratio,texture_uv.y,15),x,y,z);
        float alpha = 1.0f-smoothstep( 0.0f, 0.002f, _fmaxf(max_uv.x, max_uv.y) );
        f = _mix( f, col, alpha);
    }
    //fragColor = to_float4_aw(swi3(f,z,x,y),1.0f);
    fragColor = to_float4_aw(f,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}