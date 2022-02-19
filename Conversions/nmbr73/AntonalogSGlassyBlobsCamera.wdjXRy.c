
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel1


////////////////////////////////////////////////////////////////////////////////
//
// "Antonalog's Glassy Blobs with camera" - I took Antonalog's Glassy Blobs
// shader (https://www.shadertoy.com/view/lslGRS) and added trackball- or
// arcball-like camera control to it.
//
// Copyright 2019 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#define DO_BLOBS(DO) float4 b; b=to_float4(-0.38f + 0.25f*_sinf(iTime+0.00f), -0.60f + 0.25f*_cosf(iTime+0.00f), -0.67f, 0.17f); DO; b=to_float4(-0.33f + 0.25f*_sinf(iTime+1.00f), -0.59f + 0.25f*_cosf(iTime+1.00f), 0.02f, 0.19f); DO; b=to_float4(-0.33f + 0.25f*_sinf(iTime+2.00f), -0.42f + 0.25f*_cosf(iTime+2.00f), 0.48f, 0.12f); DO; b=to_float4(-0.50f + 0.25f*_sinf(iTime+3.00f), -0.18f + 0.25f*_cosf(iTime+3.00f), -0.30f, 0.15f); DO; b=to_float4(-0.57f + 0.25f*_sinf(iTime+4.00f), 0.09f + 0.25f*_cosf(iTime+4.00f), 0.14f, 0.16f); DO; b=to_float4(-0.58f + 0.25f*_sinf(iTime+5.00f), -0.13f + 0.25f*_cosf(iTime+5.00f), 0.58f, 0.12f); DO; b=to_float4(-0.48f + 0.25f*_sinf(iTime+6.00f), 0.67f + 0.25f*_cosf(iTime+6.00f), -0.66f, 0.13f); DO; b=to_float4(-0.37f + 0.25f*_sinf(iTime+7.00f), 0.43f + 0.25f*_cosf(iTime+7.00f), -0.16f, 0.18f); DO; b=to_float4(-0.49f + 0.25f*_sinf(iTime+8.00f), 0.41f + 0.25f*_cosf(iTime+8.00f), 0.62f, 0.16f); DO; b=to_float4(0.19f + 0.25f*_sinf(iTime+9.00f), -0.64f + 0.25f*_cosf(iTime+9.00f), -0.47f, 0.18f); DO; b=to_float4(0.19f + 0.25f*_sinf(iTime+10.00f), -0.43f + 0.25f*_cosf(iTime+10.00f), -0.04f, 0.13f); DO; b=to_float4(-0.01f + 0.25f*_sinf(iTime+11.00f), -0.40f + 0.25f*_cosf(iTime+11.00f), 0.39f, 0.11f); DO; b=to_float4(-0.12f + 0.25f*_sinf(iTime+12.00f), -0.06f + 0.25f*_cosf(iTime+12.00f), -0.70f, 0.12f); DO; b=to_float4(0.08f + 0.25f*_sinf(iTime+13.00f), 0.18f + 0.25f*_cosf(iTime+13.00f), 0.07f, 0.15f); DO; b=to_float4(-0.15f + 0.25f*_sinf(iTime+14.00f), -0.12f + 0.25f*_cosf(iTime+14.00f), 0.51f, 0.19f); DO; b=to_float4(0.09f + 0.25f*_sinf(iTime+15.00f), 0.57f + 0.25f*_cosf(iTime+15.00f), -0.48f, 0.10f); DO; b=to_float4(0.12f + 0.25f*_sinf(iTime+16.00f), 0.64f + 0.25f*_cosf(iTime+16.00f), 0.19f, 0.14f); DO; b=to_float4(-0.11f + 0.25f*_sinf(iTime+17.00f), 0.67f + 0.25f*_cosf(iTime+17.00f), 0.42f, 0.20f); DO; b=to_float4(0.55f + 0.25f*_sinf(iTime+18.00f), -0.69f + 0.25f*_cosf(iTime+18.00f), -0.35f, 0.18f); DO; b=to_float4(0.33f + 0.25f*_sinf(iTime+19.00f), -0.49f + 0.25f*_cosf(iTime+19.00f), -0.03f, 0.17f); DO; b=to_float4(0.35f + 0.25f*_sinf(iTime+20.00f), -0.66f + 0.25f*_cosf(iTime+20.00f), 0.55f, 0.15f); DO; b=to_float4(0.51f + 0.25f*_sinf(iTime+21.00f), -0.12f + 0.25f*_cosf(iTime+21.00f), -0.66f, 0.14f); DO; b=to_float4(0.48f + 0.25f*_sinf(iTime+22.00f), -0.08f + 0.25f*_cosf(iTime+22.00f), -0.12f, 0.11f); DO; b=to_float4(0.50f + 0.25f*_sinf(iTime+23.00f), 0.15f + 0.25f*_cosf(iTime+23.00f), 0.60f, 0.16f); DO; b=to_float4(0.59f + 0.25f*_sinf(iTime+24.00f), 0.43f + 0.25f*_cosf(iTime+24.00f), -0.52f, 0.11f); DO; b=to_float4(0.50f + 0.25f*_sinf(iTime+25.00f), 0.66f + 0.25f*_cosf(iTime+25.00f), 0.15f, 0.18f); DO; b=to_float4(0.35f + 0.25f*_sinf(iTime+26.00f), 0.44f + 0.25f*_cosf(iTime+26.00f), 0.37f, 0.14f); DO; 

__DEVICE__ float2 Q(float a, float b, float c)
{
  float d = b*b-4.0f*a*c;
  if (d < 0.0f) return to_float2(1e10,-1e10);
  d=_sqrtf(d);  
  float oo2a = 0.5f/a;
  float n = (-b-d)*oo2a;
  float x = (-b+d)*oo2a;
//  return to_float2( _fminf(n,x), _fmaxf(n,x) );
  return to_float2( n,x );
}

__DEVICE__ float2 SphereT(float3 P, float3 V, float3 A, float R)
{
  return Q(dot(V,V),2.0f*(dot(P,V)-(dot(A,V))),dot(A,A)+dot(P,P)-R*R-(2.0f*(dot(A,P))));
}

__DEVICE__ float2 NearestBlobBound(float3 P, float3 V, float r, float iTime)
{
  float2 t = to_float2(1e10,-1e10);
  float2 s;
  DO_BLOBS( s=SphereT(P,V,swi3(b,x,y,z),r*b.w); t.x=_fminf(t.x,s.x); t.y=_fmaxf(t.y,s.y) )
  return t;
}

__DEVICE__ float k = 10.0f;

__DEVICE__ float sdf(float3 x, float iTime)
{
  //http://www.johndcook.com/blog/2010/01/13/soft-maximum/
  float sum = 0.0f;
  
  DO_BLOBS( sum += _expf( k*(b.w - length(x-swi3(b,x,y,z))) ) )
  return _logf( sum ) / k;  
}

__DEVICE__ float3 BlobNor(float3 x, float iTime)
{
  float3 sum=to_float3(0.0f,0.0f,0.0f);

  float w;
  float3 n;
  float L;
  float3 v;
  DO_BLOBS( v=x-swi3(b,x,y,z); L=length(v); n=v*(1.0f/L); w = _expf(k*(b.w - L)); sum += w*n );
  return normalize( sum );  
  
}

/*
__DEVICE__ float3 ss_nor(float3 X)
{
  return normalize(cross(dFdx(X),dFdy(X)));
}
__DEVICE__ float3 ss_grad(float3 X)
{
  return cross(dFdx(X),dFdy(X));
}
*/
__DEVICE__ float shlick(float3 N, float3 V)
{
  float f = dot(-V,N);
  f = 1.0f-f;  
  float ff = f;
  f *= f;    //2
//  f *= f;    //4
//  f *= ff;  //5
  float r0 = 0.075f;
  f = r0 + (1.0f-r0)*f;
  return f;
}

__DEVICE__ float3 Transmittance(float3 color, float T)
{
  return -1.0f*log_f3(color)/T;
}

__DEVICE__ float3 Filter(float thick, float3 trans)
{
  float conc = 0.6f;
  return exp_f3( -trans * conc * thick );
}

// --- addition-start by MacSlow ----------
__DEVICE__ float3 cam (float2 uv, float3 ro, float3 aim, float zoom)
{
    float3 f = normalize (aim - ro);
    float3 wu = to_float3 (0.0f, 1.0f, 0.0f);
    float3 r = normalize (cross (wu, f));
    float3 u = normalize (cross (f, r));
    float3 c = ro + f*zoom;
    return normalize (c + r*uv.x + u*uv.y - ro);
}
// --- addition-end by MacSlow ----------

__KERNEL__ void AntonalogSGlassyBlobsCameraFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel1)
{

  float3 P, V;

  // --- addition-start by MacSlow ----------
    float r = 2.5f;
    float azimuthAngle = ((iMouse.x / iResolution.x) * 2.0f - 1.0f) * 179.0f;
    float elevationAngle = ((iMouse.y / iResolution.y) * 2.0f - 1.0f) * 79.0f;
    float x = r*_cosf (radians (azimuthAngle));
    float y = r*_sinf (radians (elevationAngle));
    float z = r*_sinf (radians (azimuthAngle));
    P = to_float3(x, y, z);
    float3 aim = to_float3_s (0.0f);
    float2 uv = fragCoord/iResolution;
    float2 uvRaw = uv;
    uv = uv*2.0f - 1.0f;
    uv.x *= iResolution.x/iResolution.y;
    V = cam (uv, P, aim, 1.75f);
  // --- addition-end by MacSlow ----------
  
  float overb = 1.5f;
  
  //float3 bg = swi3(_tex2DVecN(iChannel1,V.x,V.y,15),x,y,z) * overb;
  float3 bg = swi3(decube_f3(iChannel1,V),x,y,z) * overb;
  
  float3 bg_V = V;
  float2 bound=NearestBlobBound(P, V, 2.0f,iTime);
  float t = bound.x;
  
  float3 trans=Transmittance(to_float3(0.3f,0.7f,0.1f), 1.0f);
  
  if (t < 1e10)
  {
    int steps=0;
    float d = -1e10;
    float old_d = -1e10;
    float3 X;

    float inside=0.0f;
    float last_surface_t=0.0f;
  
    float thick=0.0f;
    #define STEPS  64
    float t_step = (bound.y-bound.x)*(1.0f/(float)(STEPS));
    
    float3 c = to_float3(0.0f,0.0f,0.0f);
    
    float last_f=0.0f;
    
    float3 filter_col = to_float3_s(1.0f);
    
    float blocked=0.0f;
    
    for (int i=0; i<STEPS; i++)
    {      
      X = P+V*t;
      d = sdf(X,iTime);
    
      if (d * old_d < 0.0f)  //a crossing
      {
        inside = 1.0f - inside;
        float int_t = _mix(t-t_step,t,_fabs(old_d)/(_fabs(d)+_fabs(old_d)));
                //t-d; 
              //0.5f*(t-t_step-old_d + t-d);  
      
        float3 int_X = P+V*int_t;
        float3 N = BlobNor(int_X,iTime);        
  
        if (inside < 0.5f)  //just came out
        {
          float this_thick = (int_t - last_surface_t);
          filter_col *= Filter(this_thick,trans) * (1.0f-last_f);
          
          thick += this_thick;
          
        //  V = bg_V = normalize( refract(bg_V,N,0.995f) );
        }
        else  //just went in
        {    
          float f = shlick(N,V);
          last_f = f;
          
          float3 refV = reflect(V,N);
          //float3 ref = f*swi3(_tex2DVecN(iChannel1,refV.x,refV.y,15),x,y,z); 
          float3 ref = f*swi3(decube_f3(iChannel1,refV),x,y,z); 
          
          float2 blocker=NearestBlobBound(int_X, refV, 1.0f,iTime);
          if (blocker.x > 0.01f && blocker.x < 1e5) 
          {
            blocked = blocker.y-blocker.x;
            ref *= blocked;
          }
          
          c += ref * filter_col;

          V = bg_V = normalize( refract_f3(bg_V,N,0.995f) );

        }
        
        last_surface_t = int_t;
      }      
            
      //stop if grad is pointing away from view ray...
      //saves steps but introduces some nasty artifacts on some cards
  /*    float3 G=-ss_grad(X);
      if (dot(G,V) < 0.0f) 
      {
        break;
      }*/
      
    
    //  t -= d;    
      t += t_step;
      
      old_d = d;
      
      steps++;
    }
        
//    float S = float(steps)/64.0f; ///8.0f;
  //  fragColor = to_float4(S,S,S,1.0f);
        
  //  fragColor = to_float4_aw(to_float3(thick),1.0f);
    
  //  c += Filter(thick,trans)*bg;
    float3 ref_bg = swi3(decube_f3(iChannel1,bg_V),x,y,z)  * overb;
    c += filter_col * ref_bg;
  //  c += (1.0f-thick)*bg;

    fragColor = to_float4_aw(c,1.0f);
    
  //  fragColor = to_float4_aw(to_float3(last_f),1.0f);
  //  fragColor = to_float4_aw(to_float3(blocked),1.0f);
  }
  else
  {
    fragColor = to_float4_aw(bg,1.0f);
  }

  // --- addition-start by MacSlow ----------
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) * 1.0f - 0.7f*length (uvRaw*2.0f - 1.0f));
    fragColor = pow_f4(fragColor, to_float4_s(1.0f/2.2f));
    // --- addition-end by MacSlow ----------

  SetFragmentShaderComputedColor(fragColor);
}