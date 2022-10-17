
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Abstract 1' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float iTime)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);
float bbbbbbbbbbbbbbbb;
      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y*_cosf(iTime)),Blend);
          Q = _mix(Q, to_float4(_sinf(iTime*15.0f)*3.0f,Par.x,_cosf(iTime*10.0f)*3.5f+5.5f, Par.y), Blend);
  
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,0.0f,(tex.y+MulOff.y)*MulOff.x,1.0f),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix(swi2(Q,x,y),  swi2(tex,x,y)*Par, Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}






__DEVICE__ float4 tf(int2 p, int i, int j, float2 R, __TEXTURE2D__ iChannel0){
    //return texelFetch(iChannel0,p+to_int2(i,j),0);
    return texture(iChannel0,(make_float2(p+to_int2(i,j))+0.5f)/R);
}
__DEVICE__ float4 state(int2 p, float2 R, __TEXTURE2D__ iChannel0){
    //vec4 colNow = to_float4(0,0,0,0);
    float4 r = tf(p,0,0,R,iChannel0);
    float4 colNow;
    float d0 = 0.05f;
    int i0 = int(r.x * d0 + 1.0f)-1;
    int i1 = i0 + 1;
    int j0 = int(r.y*d0 + 1.0f)-1;
    int j1 = j0 + 1;
    float s1 = r.x*d0 - (float)(i0);
    float s0 = 1.0f-s1;
    float t1 = r.y*d0 - (float)(j0);
    float t0 = 1.0f - t1;
    colNow = t0 *(s0 * tf(p,i0,j0,R,iChannel0) + s1 * tf(p,i1,j0,R,iChannel0)) +
             t1 *(s0 * tf(p,i0,j1,R,iChannel0) + s1 * tf(p,i1,j1,R,iChannel0));
    
        
    //for(int i = 0; i < 2; i++){
    //  for(int j = 0; j < 2; j++){
    //    colNow += tf(p, tx + i, ty + j,R,iChannel0) * 0.1f * _fabs((r.x*0.1f - float(tx - i)) - 1.0f) * _fabs((r.y*0.1f- float(ty - j)) - 1.0f);
    // }
    // }
    
        
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            //colNow += texelFetch(iChannel0, p + to_int2(i,j),0).rgba;
            //colNow += texture(iChannel0, (to_float2( p + to_int2(i,j))+0.5f)/R).rgba;
               
            //int tx = _floor()
            if(_fabs(i) + _fabs(j) < 3){
                float4 l = tf(p,i,j,R,iChannel0);
                float infl = -(float)(i) * l.x + -(float)(j) * l.y;
                colNow += infl * l * -0.005f;
                float d = (l.z - r.z)*0.1f;
                colNow += to_float4(d*(float)(i),d*(float)(j),-d*0.02f,0);
            }
            
        }
    }
    if(r.z > 0.0f){
        colNow+= to_float4(0,-0.5f,-0.05f,0);

    }
    colNow+= to_float4(_sinf(colNow.y*60.0f)*0.15f,0,0,0);
        
    if(p.x < 1 || p.y < 1 || p.x > (int)(iResolution.x) - 2 || p.y > (int)(iResolution.y) -2){
        return to_float4(0,0,0.3f,0);
    }
    return clamp(colNow*1.0f,-10.0f,10.0f);
    
}


__KERNEL__ void BurningCubeJipi360Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  CONNECT_CHECKBOX0(Reset, 0);  
  CONNECT_CHECKBOX1(Textur, 0);  

  //Blending
  CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);  

    fragCoord+=0.5f;
    // Amount, XV, YV, Tempurature;
    fragColor = to_float4(0.1f,0.1f,1.0f,1.0f);
    
    float4 col = state(to_int2_cfloat(fragCoord),R,iChannel0);
    
    float2 uv = fragCoord/iResolution;
    if(iFrame < 5 || Reset){
      col = _tex2DVecN(iChannel1,uv.x,uv.y,15)*2.0f-1.0f;
    }
    
    if(Textur) col+= _tex2DVecN(iChannel1,uv.x,uv.y,15);
    else       col+= _tex2DVecN(iChannel2,uv.x,uv.y,15);
    
    
    if(length((swi2(iMouse,x,y))-fragCoord)<3.0f){
        col = to_float4(_sinf(iTime*15.0f)*3.0f,-2.9f,_cosf(iTime*10.0f)*3.5f+5.5f,0);
    }
    fragColor = col;
    
    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iTime);
    
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------


__DEVICE__ float sdBoxFrame( float3 p, float3 b, float e )
{
         p = abs_f3(p  )-b;
  float3 q = abs_f3(p+e)-e;
  return _fminf(_fminf(
         length(_fmaxf(to_float3(p.x,q.y,q.z),to_float3_s(0.0f)))+_fminf(_fmaxf(p.x,_fmaxf(q.y,q.z)),0.0f),
         length(_fmaxf(to_float3(q.x,p.y,q.z),to_float3_s(0.0f)))+_fminf(_fmaxf(q.x,_fmaxf(p.y,q.z)),0.0f)),
         length(_fmaxf(to_float3(q.x,q.y,p.z),to_float3_s(0.0f)))+_fminf(_fmaxf(q.x,_fmaxf(q.y,p.z)),0.0f));
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}

__DEVICE__ float3 rotx(float3 p, float a){
    float s = _sinf(a);
    float c = _cosf(a);
    return(to_float3(p.x*c+p.y*s,-p.x*s+p.y*c,p.z));
}
__DEVICE__ float sdSph(float3 p, float r){
    return length(p) - r;
}
__DEVICE__ float4 getSDF(float3 p, float time){
    float3 pos = rotx(p,time);
    pos = rotx(swi3(pos,z,y,x),time*0.7f);
    return to_float4(0,-0.5f,1.0f,sdBoxFrame(pos,to_float3(0.1f,0.1f,0.1f),0.0015f));
}

__KERNEL__ void BurningCubeJipi360Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    fragCoord+=0.5f;
    float time = iTime;
    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.x;
     //vec2 pos = 0.25f*to_float2(_sinf(time),_cosf(time));
    //float d = length(uv-pos);
        
    float3 pos = to_float3(0.03f*_sinf(time*1.0f),_cosf(time*0.5f),-_sinf(time*0.5f));
    float3 dir = normalize(swi3(rotx(swi3(to_float3(uv.x,uv.y,-1.0f),z,x,y),time*0.5f),z,x,y));
    
    float4 col = to_float4(0,0,0,0);
    int ma = 100;
    float4 newcol;
    bool hit = false;
    for(int i = 0; i < ma; i ++){
        float4 oc = getSDF(pos,time);
        newcol = oc;
        float dis = oc.w;
        
        col += to_float4_s(0.01f/dis);
        if(dis < 0.0001f){
            i = ma;
            hit = true;
        }
        if(dis > 10.0f){
            i = ma;
        }
        pos += dir * dis;
    }
    if(hit){
        col = newcol;
    }else{
        col = to_float4(0,0,0,0);
    }
    
    //if(d<0.05f){
    //    col = to_float4(0,0,1.0f,0);
    //}
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1



__KERNEL__ void BurningCubeJipi360Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 uv = fragCoord/iResolution;
    float4 col;
    float4 col2;
    float4 raw = _tex2DVecN(iChannel0,uv.x,uv.y,15)*0.5f+0.5f;
    float4 raw2 = raw;


    raw2*=-1.0f;
    col2 = raw2*0.5f+0.5f;
    col2 = to_float4(col2.y*1.5f+_fabs(col2.z)*0.9f-0.9f,col2.y*0.5f+_fabs(col2.z)*0.3f-0.5f ,col2.y*0.3f+_fabs(col2.z)*0.3f-0.5f,0);

    if(raw.z < 0.9f){
        raw *= -1.0f;
    }
    col = raw*0.5f+0.5f;
    
    fragColor = _mix(col,col2, clamp(_sinf(iTime)+0.5f,0.0f,1.0f));
    //fragColor =  _tex2DVecN(iChannel1,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}