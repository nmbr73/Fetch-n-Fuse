
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void MouseTrailJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;
   
    float2 uv = fragCoord/iResolution;
    float2 aspect = to_float2(iResolution.x/iResolution.y, 1.0f);
    float2 uvQuad = uv*aspect;
    
    float2 mP = uvQuad - swi2(iMouse,x,y)/iResolution*aspect;

    float d = 1.0f-length(mP);    
    
    float tex = texture(iChannel1, uv).x;
    
    d+=tex;
    
    float4 bufA = to_float4(texture(iChannel0, uv).x*0.999f,texture(iChannel0, uv).y*0.997f,texture(iChannel0, uv).z*0.998f,_tex2DVecN(iChannel0,uv.x,uv.y,15).w);
    float2 mPN = swi2(bufA,z,w);
    float2 vel = _fminf(_fmaxf(abs_f2(mPN - mP), to_float2_s(0.001f)), to_float2_s(0.05f));
    
    d = smoothstep(0.85f,1.3f,d+length(vel))/0.4f;
    float2 dot = to_float2_s(d);

    dot = _fmaxf(dot, swi2(bufA,x,y));
    
    float4 col = to_float4(dot.x, dot.y, mP.x, mP.y);
    
    if(iFrame == 0 || Reset) {
        col = to_float4(0.0f,0.0f, mP.x, mP.y);
    }
    
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MouseTrailJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);


    float2 uv = fragCoord/iResolution;
    float2 aspect = to_float2(iResolution.x/iResolution.y, 1.0f);
    float2 uvQuad = uv*aspect;
    
    float4 bufA = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float4 col = bufA;
    swi2S(col,x,y, swi2(col,x,y) * (swi2(uv,x,y)+(1.0f-swi2(uv,x,y))));
    
    if(iFrame == 0 || Reset) {
        col = to_float4(0.0f,0.0f,0.0f,1.0f);
    }
    
    fragColor = to_float4(col.x,col.y, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}