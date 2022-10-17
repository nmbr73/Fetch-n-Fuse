
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float2 Blending( __TEXTURE2D__ channel, float2 uv, float2 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float2(Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float2(MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}




__DEVICE__ float2 Hash22(float2 p) {
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}


__DEVICE__ float2 Hash32(float3 p) {
  float3 p3 = fract_f3((swi3(p,x,y,z)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}


__DEVICE__ float eval_pixel(float2 self, float2 neighbours[8])
{
    float2 offsets[8] = {
        to_float2(-1.0f, -1.0f),
        to_float2( 0.0f, -1.0f),
        to_float2( 1.0f, -1.0f),
        to_float2(-1.0f,  0.0f),
        to_float2( 1.0f,  0.0f),
        to_float2(-1.0f,  1.0f),
        to_float2( 0.0f,  1.0f),
        to_float2( 1.0f,  1.0f)
    };
    float distances[8];
    
    float error = 0.0f;
    
    float mean = 0.0f;
    for (int i = 0; i < 8; ++i) {
        mean += (distances[i] = length(neighbours[i] + offsets[i] - self));
    }
    
    mean /= 8.0f;
    float var = 0.0f;
    
    for (int i = 0; i < 8; ++i) {
        var += _powf(distances[i] - mean, 2.0f);
    }
    var /= 8.0f;
    
    float off_centre = length(self - to_float2(0.5f, 0.5f));
    
    return _powf(1.0f / (1e-5 + mean), 2.0f) + _powf(_fabs(var - 0.1f) * 64.0f, 3.0f) + _powf(off_centre * 1.0f, 6.0f) * 0.05f;
}


__KERNEL__ void SelfOrganishingJipi932Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;
float AAAAAAAAAAAAA;
    float2 mouse_offset = (swi2(iMouse,x,y) * 0.1f - fragCoord + to_float2(10.0f, 10.0f));
    
    float2 uv_self = fragCoord / iResolution;
    float2 uv_shift = to_float2(1.0f, 1.0f) / iResolution;

    float2 val_self = swi2(_tex2DVecN(iChannel0,uv_self.x,uv_self.y,15),x,y);
    float2 val_mom  = swi2(_tex2DVecN(iChannel0,uv_self.x,uv_self.y,15),z,w);
    
    float2 neighbours[8] = {
         swi2(texture(iChannel0, uv_self + to_float2(-uv_shift.x, -uv_shift.y)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2(       0.0f, -uv_shift.y)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2( uv_shift.x, -uv_shift.y)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2(-uv_shift.x,        0.0f)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2( uv_shift.x,        0.0f)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2(-uv_shift.x,  uv_shift.y)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2(       0.0f,  uv_shift.y)),x,y),
         swi2(texture(iChannel0, uv_self + to_float2( uv_shift.x,  uv_shift.y)),x,y)
         };
    
    float2 offset;
    float epsilon = 1e-3;
    offset.x = (eval_pixel(val_self + to_float2(epsilon, 0.0f), neighbours) - eval_pixel(val_self + to_float2(-epsilon, 0.0f), neighbours));
    offset.y = (eval_pixel(val_self + to_float2(0.0f, epsilon), neighbours) - eval_pixel(val_self + to_float2(0.0f, -epsilon), neighbours));
    
    float2 val_next = val_self + val_mom * 1e-1;
    
    if (iMouse.z > 0.0f)
        val_next += normalize(mouse_offset) / (1e-3 + length(mouse_offset) * 6.0f);
    
   if (Blend1>0.0) val_next = Blending(iChannel1, fragCoord/R, val_next, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
    
    
    if (length(offset) > 1e-3) {
        offset = normalize(offset);
        val_next -= offset * 5e-2 * _fmaxf(1.0f, 1.5f - iTime * 0.1f);
    }
    
    val_next = clamp(val_next, to_float2(0.0f, 0.0f), to_float2(1.0f, 1.0f));
    val_next += (Hash32(to_float3_aw(fragCoord * 10.0f, iTime)) - to_float2(0.5f, 0.5f)) * 1e-3;
        
    //swi2(fragColor,x,y) = val_next;
    fragColor.x = val_next.x;
    fragColor.y = val_next.y;
    
    //swi2(fragColor,z,w) = val_next - val_self + val_mom * 0.75f;
    fragColor.z = val_next.x - val_self.x + val_mom.x * 0.75f;
    fragColor.w = val_next.y - val_self.y + val_mom.y * 0.75f;





  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SelfOrganishingJipi932Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
  
    float2 uv;
    float2 loc_coor;
    uv = fragCoord * 0.1f + to_float2(10, 10);
    loc_coor = fract(uv);
    uv = _floor(uv) + to_float2(0.5f, 0.5f);
    uv /= iResolution;
    float2 uv_shift = to_float2(1.0f, 1.0f) / iResolution;
    
    float lit = 0.0f;
    
    for (int y = -3; y <= 3; ++y) {
        for (int x = -3; x <= 3; ++x) {
            float2 cell_uv = uv + uv_shift * to_float2(x, y);
            float2 val = swi2(_tex2DVecN(iChannel0,cell_uv.x,cell_uv.y,15),x,y);
            val += to_float2(x, y);
            lit += clamp(1.0f - length(val - loc_coor) * 5.0f, 0.0f, 1.0f);
        }
    }

    // Output to screen
    fragColor = to_float4(lit, lit, lit, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}