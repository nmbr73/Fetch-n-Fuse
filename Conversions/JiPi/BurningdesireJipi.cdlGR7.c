
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Bild' to iChannel0

// Original shader created by XT95 - flame
//AND
// Original shader created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// modified by JoÃ£o Portela. Heart3

//Wanted to try some shaders on an interactive GLSLEditor developed by me
//https://github.com/rakesh-malviya/GLSLEditor
//Please goto the link and try it

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float noise(float3 p) //Thx to Las^Mercury
{
        float3 i = _floor(p);
        float4 a = dot(i, to_float3(1.0f, 57.0f, 21.0f)) + to_float4(0.0f, 57.0f, 21.0f, 78.0f);
        float3 f = cos_f3((p-i)*_acosf(-1.0f))*(-0.5f)+0.5f;
        a = _mix(sin_f4(cos_f4(a)*a),sin_f4(cos_f4(1.0f+a)*(1.0f+a)), f.x);
        swi2S(a,x,y, _mix(swi2(a,x,z), swi2(a,y,w), f.y));
        return _mix(a.x, a.y, f.z);
}

__DEVICE__ float sphere(float3 p, float4 spr)
{
        return length(swi3(spr,x,y,z)-p) - spr.w;
}

__DEVICE__ float flame(float3 p, float iTime)
{
        float d = sphere(p*to_float3(0.4f,0.5f,1.0f), to_float4(0.0f,-1.0f,0.0f,1.0f));
        return d + (noise(p+to_float3(0.0f,iTime*2.0f,0.0f)) + noise(p*3.0f)*0.5f)*0.25f*(p.y) ;
}

__DEVICE__ float scene(float3 p, float iTime)
{
        return _fminf(100.0f-length(p) , _fabs(flame(p,iTime)) );
}

__DEVICE__ float4 raymarch(float3 org, float3 dir, float iTime)
{
        float d = 0.0f, glow = 0.0f, eps = 0.02f;
        float3  p = org;
        bool glowed = false;

        for(int i=0; i<64; i++)
        {
                d = scene(p,iTime) + eps;
                p += d * dir;
                if( d>eps )
                {
                        if(flame(p,iTime) < 0.0f)
                                glowed=true;
                        if(glowed)
                        glow = (float)(i)/64.0f;
                }
        }
        return to_float4_aw(p,glow);
}



__KERNEL__ void BurningdesireJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(FlameOnly, 0);
    CONNECT_CHECKBOX1(AlphaHeart, 0);
    CONNECT_CHECKBOX2(Textur, 0);

    CONNECT_COLOR0(BackgroundColor, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(Color1, 0.7f, 0.4f, 0.1f, 1.0f);    
    CONNECT_COLOR2(Color2, 0.1f, 0.5f, 1.0f, 1.0f);
    
    CONNECT_COLOR3(FlameColor1, 1.0f, 0.5f, 0.1f, 1.0f);    
    CONNECT_COLOR4(FlameColor2, 0.1f, 0.5f, 1.0f, 1.0f);
    
    CONNECT_COLOR5(HeartColor, 1.0f, 0.5f, 0.3f, 1.0f);
    
    CONNECT_POINT0(FlamePoint1XY, 0.0f, 0.0f );
    CONNECT_SLIDER0(FlamePoint1Z, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER1(HeartSize, -1.0f, 5.0f, 1.0f);
    
    CONNECT_SLIDER2(Debug, -1.0f, 5.0f, 0.0f);
    CONNECT_SLIDER3(AlphaThres, 0.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER4(HeartBeat1, 0.0f, 10.0f, 1.5f);
    CONNECT_SLIDER5(HeartBeat2, -1.0f, 1.0f, 0.2f);
    CONNECT_SLIDER6(HeartBeat3, -1.0f, 10.0f, 3.0f);
    CONNECT_SLIDER7(HeartBeat4, -1.0f, 10.0f, 4.0f);



    if(FlameOnly)
    {
        float2 v = -1.0f + 2.0f * fragCoord / iResolution;
        v.x *= iResolution.x/iResolution.y;

        float3 org = to_float3(0.0f, -2.0f, 4.0f) + to_float3_aw(FlamePoint1XY,FlamePoint1Z);
        float3 dir = normalize(to_float3(v.x*1.6f, -v.y, -1.5f));

        float4 p = raymarch(org, dir, iTime);
        float glow = p.w;

        //float4 col = _mix(to_float4(1.0f,0.5f,0.1f,1.0f), to_float4(0.1f,0.5f,1.0f,1.0f), p.y*0.02f+0.4f);
        float4 col = _mix(FlameColor1, FlameColor2, p.y*0.02f+0.4f);

        fragColor = _mix(to_float4_s(0.0f), col, _powf(glow*2.0f,4.0f));
        //fragColor = _mix(to_float4_s(1.0f), _mix(to_float4(1.0f,0.5f,0.1f,1.0f),to_float4(0.1f,0.5f,1.0f,1.0f),p.y*0.02f+0.4f), _powf(glow*2.0f,4.0f));

    }
    else
    {
      float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;
      p.y -= 0.25f;

      // background color
      //float3 bcol = to_float3(1.0f,0.7f,0.8f-0.07f*p.y)*(1.0f-0.35f*length(p));
      float3 bcol = (0.5f-swi3(BackgroundColor,x,y,z)+to_float3(1.0f,0.7f,0.8f-0.07f*p.y))*(1.0f-0.35f*length(p));

      // animate
      float tt = _fabs(_sinf(iTime))*HeartBeat1;//1.5f;
      float ss = _powf(tt,0.2f)*0.5f + 0.5f;
      //ss -= ss*0.2f*_sinf(tt*6.2831f*3.0f)*_expf(-tt*4.0f);
      ss -= ss*HeartBeat2*_sinf(tt*6.2831f*HeartBeat3)*_expf(-tt*HeartBeat4);
      p *= to_float2(0.5f,1.5f) + ss*to_float2(0.5f,-0.5f);

      p += 1.0f-(swi2(iMouse,x,y)/iResolution+0.5f);

      // shape
      float a = _atan2f(p.x,p.y)/3.141593f;
      float r = length(p);
      float h = _fabs(a);
      float d = (13.0f*h - 22.0f*h*h + 10.0f*h*h*h)/(6.0f-5.0f*h) * HeartSize;

          // color
          float s = 1.0f-0.5f*clamp(r/d,0.0f,1.0f);
          s = 0.75f + 0.75f*p.x;
          s *= 1.0f-0.25f*r;
          s = 0.5f + 0.6f*s;
          s *= 0.5f+0.5f*_powf( 1.0f-clamp(r/d, 0.0f, 1.0f ), 0.1f );
          //float3 hcol = to_float3(1.0f,0.5f*r,0.3f)*s;
          float3 hcol = to_float3(HeartColor.x,HeartColor.y*r,HeartColor.z)*s;

      if(Textur)
      {
         float4 tcol = texture(iChannel0, fragCoord/iResolution);
         
         d = 0.0f;
         r = 0.0f;
         
         if (tcol.w > 0.0f + AlphaThres) 
         {
           d = 1.0f; 
           hcol = swi3(tcol,x,y,z);
         }
      }

      float3 col = _mix( bcol, hcol, smoothstep( -0.01f, 0.01f, d-r) );

      float2 vXT = -1.0f + 2.0f * fragCoord / iResolution;
      vXT.x *= iResolution.x/iResolution.y;

      float3 orgXT = to_float3(0.0f, -2.0f, 4.0f) + to_float3_aw(FlamePoint1XY,FlamePoint1Z);
      float3 dirXT = normalize(to_float3(vXT.x*1.6f, -vXT.y, -1.5f));

      float4 pXT = raymarch(orgXT, dirXT, iTime);
      float glowXT = pXT.w;

      //float4 colXT = _mix(to_float4(0.7f,0.4f,0.1f,1.0f), to_float4(0.1f,0.5f,1.0f,1.0f), pXT.y*(-0.02f)+0.4f);
      float4 colXT = _mix(Color1, Color2, pXT.y*(-0.02f)+0.4f);

      fragColor = _mix(to_float4_aw(col,1.0f), colXT, _powf(glowXT*2.0f,4.0f));

      //fragColor = to_float4_aw(col,1.0f);
      
      if(AlphaHeart)
        fragColor.w = d-r >= 0.0f + Debug ? HeartColor.w : Color1.w;
    }
    
    SetFragmentShaderComputedColor(fragColor);
}