
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


// Noise simplex 2D by iq - https://www.shadertoy.com/view/Msf3WH

__DEVICE__ float2 hash( float2 p )
{
  p = to_float2( dot(p,to_float2(127.1f,311.7f)), dot(p,to_float2(269.5f,183.3f)) );
  return -1.0f + 2.0f*fract_f2(sin_f2(p)*43758.5453123f);
}

__DEVICE__ float noise( in float2 p )
{
  const float K1 = 0.366025404f; // (_sqrtf(3)-1)/2;
  const float K2 = 0.211324865f; // (3-_sqrtf(3))/6;

  float2  i = _floor( p + (p.x+p.y)*K1 );
  float2  a = p - i + (i.x+i.y)*K2;
  float m = step(a.y,a.x); 
  float2  o = to_float2(m,1.0f-m);
  float2  b = a - o + K2;
  float2  c = a - 1.0f + 2.0f*K2;
  float3  h = _fmaxf( 0.5f-to_float3(dot(a,a), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );
  float3  n = h*h*h*h*to_float3( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));
  return dot( n, to_float3_s(70.0f) );
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// modified from SmoothLife by davidar - https://www.shadertoy.com/view/Msy3RD



#define G(z) exp_f3(-0.5f*(z)*(z))

__KERNEL__ void InstamazeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendZ_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(BlendW_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER3(BlendS_Thr, -10.0f, 10.0f, 0.0f);
    CONNECT_BUTTON0(Modus, 1, Z,  W, Clear, NN, NN);
  
    fragCoord+=0.5f;

    const int R = 10;             // space resolution = kernel radius
    const float dt = 0.15f;       // time step

    //const mat4 beta = to_mat4(0.25f,1,0,0, 1,0.75f,0.75f,0, 1,0,0,0, 0,0,0,0);   // kernel ring heights
    
    //const mat3 beta = to_mat3(0.25,1,0, 1,0.75,0.75, 1,0,0);   // kernel ring heights
    
    const float beta[3][3] = {{0.25,1,0}, {1,0.75,0.75}, {1,0,0}};

    
    const float betaLen[3] = {2,3,1};    // kernel ring number
    const float3 mu = to_float3(0.16f, 0.22f, 0.28f);       // growth center
    const float3 sigma = to_float3(0.025f, 0.042f, 0.025f);   // growth width
    const float3 eta = to_float3_s(1);     // growth strength

    const float rho = 0.5f;       // kernel center
    const float omega = 0.15f;    // kernel width


    int2 pos = to_int2_cfloat(fragCoord);

    float3 sum = to_float3_s(0), total = to_float3_s(0);
    for (int _x = -R; _x <= R; _x++) for (int _y = -R; _y <= R; _y++) {
        float r = length(to_float2(_x,_y)/(float)(R));
        if (r > 1.0f) continue;
        //float val = texelFetch(iChannel0, pos + to_int2(x,y), 0).x;
        float val = texture(iChannel0, (make_float2(pos + to_int2(_x,_y))+0.5f)/iResolution).x;
        float height[3];
        for (int i = 0; i < 3; i++)
            height[i] = beta[i][(int)(r * betaLen[i])];
        float3 weight = to_float3(height[0],height[1],height[2]) * G((fract_f3(r * to_float3(betaLen[0],betaLen[1],betaLen[2])) - rho) / omega);
          
        sum += val * weight;
        total += weight;
    }
    float3 avg = sum / total;

    //float r = texelFetch(iChannel0, pos, 0).x;
    float r = texture(iChannel0, (make_float2(pos)+0.5f)/iResolution).x;
    r = _mix(r, dot(G((avg - mu) / sigma), eta), dt);
    r = clamp(r, 0.0f, 1.0f);

    if (iFrame < 2 || Reset)
        r = noise(fragCoord/(float)(R) + iTime*100.0f);
    if (iMouse.z > 0.0f) {
        float d = length((fragCoord - swi2(iMouse,x,y)) / swi2(iResolution,x,x));
        if (d <= 5.0f*(float)(R)/iResolution.x)
          r = noise(fragCoord/(float)(R) + iTime*100.0f);
    }
    
    
        //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,fragCoord/iResolution);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          r = _mix(r,(tex.x)*BlendZ_Thr, Blend1);

        if ((int)Modus&4)
          r = _mix(r, noise(fragCoord/(float)(R) + iTime*100.0f)*BlendW_Thr, Blend1);
/*
        if ((int)Modus&8)
        {  
          //swi2S(Q,x,y, _mix(swi2(Q,x,y), U, Blenda));
          //if (U.x>0.1f && U.x<0.9f && U.y > 0.1f && U.y < 0.9f)
          float2 uv = U/R;  
          if (uv.x>0.01f && uv.x<0.99f && uv.y > 0.01f && uv.y < 0.99f)
             //Q.x=U.x, Q.y=U.y; //Q.z=tex.x; Q
             Q = to_float4(_floor(U.x/10.0f+0.5f)*10.0f,_floor(U.y/10.0f+0.5f)*10.0f,0,0);
        }
*/        
      }
    }
    
    if ((int)Modus&8) //Clear
      r = 0.32f;

    fragColor = to_float4_s(r);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void InstamazeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  CONNECT_SLIDER9(Alpha, 0.0f, 1.0f, 1.0f);
  
  CONNECT_CHECKBOX1(TexturImage, 0);
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  

  float2 uv = fragCoord / iResolution;
  fragColor = to_float4_s(1.0f) - _tex2DVecN(iChannel0,uv.x,uv.y,15);
  
          //Textureblending
    if (Blend1 > 0.0f && TexturImage)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,fragCoord/iResolution);

      if (tex.w > 0.0f)
      {  
         fragColor = _mix(fragColor,tex,Blend1);
      }
    }
  
  
  fragColor.w = Alpha;

  SetFragmentShaderComputedColor(fragColor);
}