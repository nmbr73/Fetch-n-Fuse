// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          swi2S(Q,z,w, _mix(swi2(Q,z,w),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


#define NUM_PARTICLES 400
#define speed 0.003f
#define accel 0.0005f
#define epsilon 0.5f
#define invert 100000.0f
#define gravity 5000.0f
#define perpendicular 5000000.0f

__DEVICE__ float hash(float2 p) {
    float h = dot(p,to_float2(127.1f,311.7f)); 
    return fract(_sinf(h)*43758.5453123f);
}

__DEVICE__ float2 pm(float2 uv) {
    return mod_f2(mod_f2(uv, 1.0f) + 1.0f, 1.0f);
}

__DEVICE__ int getType(float2 uv, float2 R) {
    float pos = uv.x;
    float catSize = 0.25f;
    
    if (float(NUM_PARTICLES) < iResolution.x) {
        catSize = (float(NUM_PARTICLES) / iResolution.x) / 4.0f;
    }    
    
    if (pos < catSize) {
        return 0;    
    } else if (pos >= catSize && pos < catSize * 2.0f) {
        return 1;
    } else if (pos >= catSize * 2.0f && pos < catSize * 3.0f) {
        return 2;   
    } else {
        return 3;   
    }
}

__DEVICE__ float getRule(int type0, int type1, float2 R, __TEXTURE2D__ iChannel2) {
    float2 stepX = to_float2((float)(type1) / iResolution.x, 0.0f);
    float2 stepY = to_float2(0.0f, (float)(type0) / iResolution.y);
    float2 halfStep = 0.5f / iResolution;
    
    float2 uv = stepX + stepY + halfStep;
    return _tex2DVecN(iChannel2,uv.x,uv.y,15).x;
}

__KERNEL__ void ParticleSystemDynamicsJipi350Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);    
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);    
    
    CONNECT_SLIDER5(Speed, -1.0f, 1.0f, 0.003f);
    
    fragCoord+=0.5f;
    
    float2 uv = fragCoord / iResolution;
    float4 is = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float2 pos = swi2(is,x,y);
    float2 vel = swi2(is,z,w);
    
    float2 accum = to_float2_s(0.0f);
    
    float2 c = fragCoord;
    float2 r = iResolution;
    float2 r_n = to_float2(0.0f, iResolution.y);
    float2 r_e = to_float2(iResolution.x, 0.0f);
    float2 stepX = to_float2(1.0f / iResolution.x, 0.0f);
    float2 stepY = to_float2(0.0f, 1.0f / iResolution.y);
    float2 halfStep = 0.5f / iResolution;
    
    float2 posS = pos * r;
    
    int myType = getType(uv,R);

    for (int i = 0; i < NUM_PARTICLES; i++) {
        float2 xpos = stepX * float(i);
        float2 uvN = pm(xpos) + stepY * _floor(xpos.x) + halfStep;
        float2 posN = r * swi2(_tex2DVecN(iChannel0,uvN.x,uvN.y,15),x,y);
        
        int thisType = getType(uvN,R);
        float rule = getRule(myType, thisType,R,iChannel2);
        
        float2 velN = swi2(_tex2DVecN(iChannel0,uvN.x,uvN.y,15),z,w);
        
        if (length(posN - posS) > epsilon) {  
        
            
          float rule = getRule(myType, thisType,R,iChannel2);
            
          float2 minV = to_float2(0.0f, 0.0f);
          float minD = 10000.0f;
          float d[5];
          d[0] = distance_f2(posN, posS);
          d[1] = distance_f2(posN + r_n, posS);
          d[2] = distance_f2(posN + r_e, posS);
          d[3] = distance_f2(posN - r_n, posS);
          d[4] = distance_f2(posN - r_e, posS);

            float2 v[5];
            v[0] = posN - posS;
            v[1] = posN + r_n - posS;
            v[2] = posN + r_e - posS;
            v[3] = posN - r_n - posS;
            v[4] = posN - r_e - posS;

            for (int i = 0; i < 5; i++) {
                if (d[i] < minD) {
                    minD = d[i];
                    minV = v[i];
                }
            }

            accum += gravity * rule * (normalize(velN) + normalize(minV)) / (minD * minD);
            accum -= invert * rule * normalize(minV) / (minD * minD * minD);
            accum += perpendicular * rule * normalize(velN) * length(cross(to_float3_aw(vel, 0.0f), to_float3_aw(velN, 0.0f))) / (minD * minD * minD);
        }
    }  
    
    float mouseScale = 1.0f + 0.2f / (distance_f2(posS, swi2(iMouse,x,y)) / iResolution.x);
    float2 tempVel = vel + accel * accum;
    float2 norm = normalize(tempVel);
    vel = length(tempVel) > 1.0f ? norm : tempVel;
    vel *= mouseScale;

    float4 Input = to_float4_f2f2(pos,vel);
    
    if (Blend1>0.0) 
    { 
      float4 Output = Blending(iChannel3, fragCoord/R, Input, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
      
      pos = swi2(Output,x,y);
      vel = swi2(Output,z,w);
    }


    // initialize with noise
    if(iFrame<10 || Reset) {
        fragColor = to_float4(hash(uv + 1.1f), hash(uv + 2.3f), hash(uv + 3.8f) - 0.5f, hash(uv + 4.2f) - 0.5f);
    } else {
        fragColor = to_float4_f2f2(pm(pos + Speed * vel), vel); //speed -> Speed
    }

    

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//#define NUM_PARTICLES 400
#define scale 0.5f

__DEVICE__ float4 getTypeB(float2 uv, float2 R) {
    float pos = uv.x;
    float catSize = 0.25f;
    
    if ((float)(NUM_PARTICLES) < iResolution.x) {
        catSize = ((float)(NUM_PARTICLES) / iResolution.x) / 4.0f;
    }   
    
    if (pos < catSize) {
        return to_float4(1.0f, 0.0f, 0.0f, 0.0f);    
    } else if (pos >= catSize && pos < catSize * 2.0f) {
        return to_float4(0.0f, 1.0f, 0.0f, 0.0f); 
    } else if (pos >= catSize * 2.0f && pos < catSize * 3.0f) {
        return to_float4(0.0f, 0.0f, 1.0f, 0.0f);   
    } else {
        return to_float4(0.0f, 0.0f, 0.0f, 1.0f);    
    }
}

__KERNEL__ void ParticleSystemDynamicsJipi350Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    
    float2 uv = fragCoord / iResolution;
    float2 stepX = to_float2(1.0f / iResolution.x, 0.0f);
    float2 stepY = to_float2(0.0f, 1.0f / iResolution.y);
    float2 halfStep = 0.5f / iResolution;
    
    float4 accum = to_float4_s(0.0f);
    
    float2 c = fragCoord;
    float2 r = iResolution;
    float2 r_n = to_float2(0.0f, iResolution.y);
    float2 r_e = to_float2(iResolution.x, 0.0f);
   

    for (int i = 0; i < NUM_PARTICLES; i++) {
        float2 xpos = stepX * (float)(i);
        float2 uvN = pm(xpos) + stepY * _floor(xpos.x) + halfStep;
        float2 pos = r * swi2(_tex2DVecN(iChannel0,uvN.x,uvN.y,15),x,y);

        float d = distance_f2(c, pos);
        float d_n = distance_f2(c, pos + r_n);
        float d_e = distance_f2(c, pos + r_e);
        float d_s = distance_f2(c, pos - r_n);
        float d_w = distance_f2(c, pos - r_e);
        float minWrap = _fminf(min(d_n, d_s), _fminf(d_e, d_w));
        float minDistance = _fminf(minWrap, d);
        accum += getTypeB(uvN,R) * scale / (minDistance);
    }
    fragColor = clamp(accum, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void ParticleSystemDynamicsJipi350Fuse__Buffer_C(float4 O, float2 U)
{
    U+=0.5f;  

    // define our rule table
    
    int x = (int)(U.x);
    int y = (int)(U.y);
    
    float4 one = to_float4_s(1.0f);
    float4 neg_three = to_float4_s(-3.0f);
    float4 neg = to_float4_s(-1.0f);
    
    /*    
          A  B  C  D
        A 1 -3  1  1
        B 1  1 -3  1
        C 1 -3  1  1
        D 1  1 -3  1
    */

    if (y == 0) {
        if (x == 1) {
            O = neg_three;   
        } else {
            O = one;
        }
    } else if (y == 1) {
        if (x == 2) {
            O = neg_three;   
        } else {
            O = one;
        }
    } else if (y == 2) {
        if (x == 1) {
            O = neg_three;   
        } else {
            O = one;
        }       
    } else {
        if (x == 2) {
            O = neg_three;   
        } else {
            O = one;
        } 
    }


    // Two alternative rules:
    
    /*
         1 -1 -1  1
        -1  1  1 -1
        -1 -1  1  1
         1  1 -1 -1

    if (y == 0) {
        if (x == 0 || x == 3) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 1) {
        if (x == 0 || x == 3) {
            O = neg;   
        } else {
            O = one;
        }
    } else if (y == 2) {
        if (x == 0 || x == 1) {
            O = neg;   
        } else {
            O = one;
        }       
    } else {
        if (x == 0 || x == 1) {
            O = one;   
        } else {
            O = neg;
        } 
    }
    */

    
    /* 
        -1  1 -1 -1
        -1 -1  1 -1
        -1 -1 -1  1
         1 -1 -1 -1


    if (y == 0) {
        if (x == 1) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 1) {
        if (x == 2) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 2) {
        if (x == 3) {
            O = one;   
        } else {
            O = neg;
        }       
    } else {
        if (x == 0) {
            O = one;   
        } else {
            O = neg;
        } 
    }
    */
    
  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void ParticleSystemDynamicsJipi350Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    fragCoord+=0.5f;  

    float2 uv = fragCoord / iResolution;
    float4 is = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    float l = length(is);    
    fragColor = ((0.5f + 0.5f * _sinf(20.0f * l))/l) *  to_float4_aw(swi3(is,x,y,z), 0.0f) + 0.5f * to_float4(is.w, is.w, 0.0f, 0.0f); 

   if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
   
   
   
   if (ApplyColor)
   {
     fragColor = fragColor + (Color-0.5f);
     fragColor.w = Color.w;
   }


  SetFragmentShaderComputedColor(fragColor);
}