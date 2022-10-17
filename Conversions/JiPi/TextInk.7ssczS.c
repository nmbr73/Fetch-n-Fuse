
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define PI 3.1415926f
#define R iResolution
#define A(U) _tex2DVecN(iChannel0, (U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3, (U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4, (U).x/R.x,(U).y/R.y,15)


__DEVICE__ float ln_f2 (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float ln_f3 (float3 p, float3 a, float3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
__DEVICE__ float hash (float2 p) // Dave H
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Font 1' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel3



__DEVICE__ float4 _char(float2 p, int c, __TEXTURE2D__ iChannel1) {
    if (p.x < 0.0f || p.x > 1.0f || p.y < 0.0f|| p.y > 1.0f) return to_float4(0,0,0,1e5);
    return texture(iChannel1, p/16.0f + fract_f2(to_float2(c, 15-c/16)/16.0f));
}


#define P(c) T.x-=0.5f; tV += _char(T,64+c,iChannel1).x*mouseDown;

__KERNEL__ void TextInkFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Dots, 0);
    CONNECT_CHECKBOX1(DotsOver, 0);
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 0.5f);
    U+=0.5f;

    //the two Qs here are responsibile for the dye pooling downwards
    float visc = 0.05f; //viscosity or thickness or how runny it is
    
    float2 dir = to_float2(_sinf(U.y/R.y*10.0f+iTime*5.0f) ,_sinf(U.x/R.x*10.0f+iTime*5.0f)+1.0f);
    dir = to_float2(0.0f,1.0f);
    //dir = to_float2(hash(to_float2(iTime,U.x*10.0f)),hash(to_float2(U.y*10.0f,iTime))) - 0.5f;
    //dir *= 2.0f;
    Q = A(U+to_float2(0,0));
    float xmove = _sinf((U.x/R.x-0.5f)*10.0f + iTime/1.0f)/30.0f;
    //xmove = 0.0f;
    Q = A(U+to_float2(xmove*1000.0f,visc*Q.w*0.0f));
    
    
    Q = A(U+dir);
    Q = A(U+to_float2_s(visc*Q.w)*dir);
    //Q = A(U+to_float2(0,0.0f));
    //Q.w is set to 1 when color is added
    float4 q = to_float4_s(0);
    for (int _x = -1; _x<= 1; _x++)
    for (int _y = -1; _y<= 1; _y++)
    if (_x!=0||_y!=0)
    {
        
        float2 u = to_float2(_x,_y); //corner diff
        float4 a = A(U+u); //corner sample
        float h = hash(U+0.5f*u); //corner random
        float m = (length(swi3(a,x,y,z))); //length of color of corner sample
        m = _fminf(m,1.0f); //cap length at 1
        //m = _powf(m,1.2f)+visc;
        //q += _powf(h,6.0f)*(a-Q)/2.0f;
        
        if (!isnan(_powf(h,6.0f)*(a.x-Q.x)/2.0f))
          q += _powf(h,6.0f)*(a-Q)/2.0f;
    }
    Q += 0.125f*q;
    
    
    float2 uv = U/R;
    uv.x *= R.x/R.y;
    
    if(iMouse.z == 0.0f) iMouse.z -= 0.01f;
    
    float mouseDown = step(0.0f,iMouse.z);
    
    float FontSize = 32.0f;
    float2 position = to_float2_s(0.5f)*to_float2(R.x/R.y,1.0f) - to_float2(FontSize/64.0f*3.0f/2.0f,FontSize/64.0f/2.0f);
    float2 T = ( uv - position)*64.0f/FontSize;
    
    float tV = _char(T,64 + 16,iChannel1).x*mouseDown;
    
    P(1);P(9);P(14);P(20);
    //tV += char(T - to_float2(0.5f,0.0f),64 + 2).x*mouseDown;
    //tV += tV2;
    
    float3 textCol = 1.0f-to_float3(0.3f,0.9f,0.5f);
    
    //float textBool = step(0.5f,tV);
    float textBool = step(0.4f,tV);
    
    //Q.w = _mix(Q.w,1.0f,textBool);
    //swi3(Q,x,y,z) = _mix(swi3(Q,x,y,z),textCol,textBool);
    
    Q = to_float4_aw(_mix(swi3(Q,x,y,z),textCol,textBool), _mix(Q.w,1.0f,textBool));
    
    
    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        /*
        tex = tex*2.0 - 1.0f;
        if ((int)Modus & 2) Q.x = _mix(Q.x,tex.x,Blend1);
        if ((int)Modus & 4) Q.y = _mix(Q.y,tex.y,Blend1);
        if ((int)Modus & 8) Q.z = _mix(Q.z,tex.z,Blend1);
        if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
        if ((int)Modus & 32) Q = to_float4(0.0f,0.0f,1.0f,1.0f);
        */
        Q = _mix(Q,tex,Blend1);
      }  
    } 

    Q += Color-0.5f;
    
    //vec4 d = D(U);
    float2 c = to_float2(hash(to_float2(iTime/450.0f,460.0f)*1000.0f)*R.x,hash(to_float2(iTime/100.0f,0.0f)*2000.0f)*R.y);
    float4 d = to_float4_f2f2(c,c);
    
    // Bunte Punkte
    if (ln_f2(U,swi2(d,x,y),swi2(d,z,w))<0.025f*R.y && Dots){
        if (DotsOver == 0) Q = 0.5f+0.5f*sin_f4(iTime+to_float4(3.0f+ iTime/100.0f,0.0f+ iTime/1131.0f,1.0f,4)); // Unter der Schrift
        else               Q = 0.5f+0.5f*sin_f4(iTime+to_float4(0,5.0f/4.0f,5.0f/8.0f,4));                     // Über der sSchrift
        Q *= 0.2f;
        Q.w = 1.0f;
    }
    if (iFrame < 1) Q=to_float4_s(0.0f);//Q = 1.0f-B(U); //Q=B(U);//

//Q = to_float4(textBool,mouseDown,0.0f,1.0f);//textBool);
//Q = to_float4_s(textBool);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void TextInkFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0)
{
    U+=0.5f;

    Q = B(U);
    Q += 5e-4f*A(U);
    
    if (iFrame < 1) Q=to_float4_s(0.0f);//Q = 1.0f-B(U); //Q=C(U);//

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


//Mouse
__KERNEL__ void TextInkFuse__Buffer_C(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    U+=0.5f;
 
    float4 p = _tex2DVecN(iChannel0,U.x/iResolution.x,U.y/iResolution.y,15);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
    else C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else C = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void TextInkFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    //CONNECT_CHECKBOX0(Special, 0);
    CONNECT_CHECKBOX2(Invers, 0);
    CONNECT_SLIDER0(Alpha , 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(Convolve , -1.0f, 1.0f, 0.05f);
    CONNECT_BUTTON0(CModi, 0, NoX, NoY, NoZ, Btn4, Btn5 );
    CONNECT_BUTTON1(Modi, 0, Norm, Inv, Special, Btn4, Btn5 );
  
    U+=0.5f;
    
    float4 a = A(U), b = B(U);
    Q = a+b;
    float n = hash(U+to_float2(0,1));
    float e = hash(U+to_float2(1,0));
    float s = hash(U-to_float2(0,1));
    float w = hash(U-to_float2(1,0));
    float3 no = normalize(to_float3(e-s,n-s,1));
    
    
    CModi = CModi-1;
    float _no = no.y;
  
    if (CModi == 1) _no = no.x;
    if (CModi == 2) _no = no.z;
    if (CModi == 3) _no = no.y;
    
    
    Q = to_float4_s(0.9f+0.05f*no.y)-sqrt_f4(Q);
    
    Modi = Modi-1;
    if (Modi == 1) Q = (to_float4_s(0.9f)+Convolve*_no-sqrt_f4(a+b))*a.w;//*a.w;//a+b;// to_float4_s(0.0f);//a+b; 
    if (Modi == 2) Q = (-(Convolve*_no)+sqrt_f4(a+b))*a.w;//*a.w;//a+b;// to_float4_s(0.0f);//a+b; 
    
    
    Q.w = Alpha;
    
  SetFragmentShaderComputedColor(Q);    
}