
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/boris-brejcha/schaltzentrale-boris-brejcha-joker-remake-free' to iChannel0


/** 
    License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
   
    NanoKontrol2 Korg Midi interface
    04/26/22 | byt3_m3chanic

    Made mostly just cause I had a day to play, but also a pretty 
    good demo of how to access the midi texture in KodeLife
    (though here on Shadertoy I'm just pumping the sound to controll)
    
    https://twitter.com/byt3m3chanic/status/1518968742314754049

    to use in KodeLife - replace the sampleFreq calls with midiCoord 

    int2 midiCoord(int offset){
        int x = offset % 32;
        int y = offset / 32;
        return to_int2(x,y);
    }
    
    float md1 = texelFetch(midi1, midiCoord(3 * 127 + i), 0).x;
*/

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define M iMouse
#define T iTime

#define PI2 6.28318530718f
#define PI  3.14159265359f



__DEVICE__ mat2 rot(float g) {return to_mat2(_cosf(g), _sinf(g),-_sinf(g), _cosf(g));}
__DEVICE__ float hash21( float2 p ) {return fract(_sinf(dot(p,to_float2(23.43f,84.21f)))*4832.3234f);}

__DEVICE__ float sampleFreq(float freq, __TEXTURE2D__ iChannel0) {
    return texture(iChannel0, to_float2(freq, 0.1f)).x;
}

__DEVICE__ float box(in float2 p, in float2 b){
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(_fmaxf(d.x,d.y),0.0f);
}

//float px = 0.0f;

__DEVICE__ float3 nanoBody( float3 C, float2 p, float px, float3 tcolor,float3 nanoCol, float3 panelCol[3]) {
    float2 uv=p-to_float2_s(0);
    
    //body
    float d = box(uv,to_float2(0.725f,0.2f))-0.015f;
    d=smoothstep(px,-px,d);
    
    //inset
    float i = box(uv,to_float2(0.715f,0.1875f))-0.015f;
    i=smoothstep(px,-px,i);
    
    //shadow
    float sd = box(uv+to_float2(0,0.01f),to_float2(0.71f,0.19f));
    sd=smoothstep(0.045f-px,-px,_fabs(sd)-0.01f);
    float hs = hash21(uv*_sinf(uv.x));
    
    //C= _mix(C,to_float3_s(0.0f),sd);               //Panelumrandung
    //C= _mix(C,to_float3_s(0.015f)-(hs*0.01f),d);   //Panelfarbe
    //C= _mix(C,to_float3_s(0.035f)-(hs*0.01f),clamp(_fminf(uv.y+0.45f,i),0.0f,1.0f));
    C= _mix(C,panelCol[0],sd);               //Panelumrandung
    C= _mix(C,panelCol[1]-(hs*0.01f),d);   //Panelfarbe
    C= _mix(C,panelCol[2]-(hs*0.01f),clamp(_fminf(uv.y+0.45f,i),0.0f,1.0f));
    
    // button fame boxes
    float r = box(uv+to_float2(0.55f,0.15f),to_float2(0.13f,0.0275f));
    r=smoothstep(px,-px,_fabs(r)-0.00075f);
    //C= _mix(C,to_float3_s(0.1f),r);
    C= _mix(C,nanoCol,r);
    
    r = box(uv+to_float2(0.55f,0.05f),to_float2(0.13f,0.021f));
    r=smoothstep(px,-px,_fabs(r)-0.00075f);
    //C= _mix(C,to_float3_s(0.1f),r);
    C= _mix(C,nanoCol,r);
    
    // power light
    float l = box(uv+to_float2(0.7f,-0.175f),to_float2(0.0125f,0.002f))-0.0025f;
    float sl=smoothstep(0.02f-px,-px,l);
    l=smoothstep(px,-px,l);
    C= _mix(C,tcolor*0.2f,sl);
    C= _mix(C,tcolor,l);
    return C;
}

__DEVICE__ float3 knob( float3 C, float2 p, float level, float px, float3 knobCol) {
    float2 uv = p-to_float2_s(0);
    //base
    uv=mul_f2_mat2(uv,rot(level*PI2));
    float k = length(uv)-0.037f;
    float dk = length(uv)-0.027f;
    dk=smoothstep(px,-px,_fabs(dk)-0.005f);
    k=smoothstep(px,-px,k);

    C= _mix(C,to_float3_s(0.001f),k);
    C= _mix(C,uv.x>0.0f?to_float3_s(0.16f):to_float3_s(0.05f),dk);
    
    //line
    float l = box(uv-to_float2(0,0.01f),to_float2(0.0015f,0.01f));
    l=smoothstep(px,-px,l);
    
    //C= _mix(C,to_float3_s(0.6f),l);
    C= _mix(C,knobCol,l);
    return C;
}

__DEVICE__ float3 slider(float3 C, float2 p, float level, float px, float3 sliderCol) {
    float2 uv=p-to_float2_s(0);
    
    //background
    float d = box(uv,to_float2(0.015f,0.125f))-0.015f;
    d=smoothstep(px,-px,d);
    C= _mix(C,uv.x<0.?to_float3_s(0.1f):to_float3_s(0.15f),d);
    
    //lines
    float l = box(uv,to_float2(0.015f,0.0001f));
    l = _fminf(box(to_float2(uv.x,_fabs(abs(uv.y)-0.075f)-0.025f),to_float2(0.015f,0.0001f)),l);
    l=smoothstep(px,-px,l);
    //C= _mix(C,uv.x>0.0f?to_float3_s(0.00f):to_float3_s(0.5f),l);
    C= _mix(C,uv.x>0.0f?to_float3_s(0.00f):sliderCol,l);
    
    //slider
    level = (level*0.75f)-0.15f;
    float b = box(uv-to_float2(0,level),to_float2(0.0125f,0.0175f))-0.0075f;
    b=smoothstep(px,-px,b);
    
    C= _mix(C,to_float3_s(0.01f),b);
    return C;
}

__DEVICE__ float3 button( float3 C, float2 p, float state, int type, float px, float3 lcolor) {
    float2 uv = p-to_float2_s(0);
    //base
    float b = box(uv,to_float2(0.0125f,type==2?.005:.0125))-0.005f;
    float sl=smoothstep(0.02f-px,-px,b);
    b=smoothstep(px,-px,b);
    
    if(state>0.001f) C= _mix(C,lcolor*0.2f,sl);
    C= _mix(C,state>0.001f?lcolor:to_float3_s(0.05f),b);
    return C;
}

__DEVICE__ float3 backdrop( float3 C, float2 p, float px, float3 tcolor, float iTime, float3 bkgCol, float angle, float2 dc, float dy, __TEXTURE2D__ iChannel0) {
    float2 uv = p-to_float2_s(0);
    //uv=mul_f2_mat2(uv,rot(0.78f));
    uv=mul_f2_mat2(uv,rot(angle));
    //float d = box(uv,to_float2(5.0f,0.4f));
    float d = box(uv,dc);
    float dcut=d;
    float sd=smoothstep(0.03f-px,-px,d);
    d=smoothstep(px,-px,d);
    C=_mix(C,to_float3_s(0.0001f),sd); // Questreifen schatten
    C=_mix(C,to_float3_s(0.001f),d);   // Oben/Untenfarbverlauf
    //C=_mix(C,to_float3(0.384f,0.510f,0.227f),clamp(_fminf((p.y+0.015f)*2.865f,d),0.0f,1.0f));
    C=_mix(C,bkgCol,clamp(_fminf((p.y+0.015f)*2.865f,d),0.0f,1.0f));

    
    float i = 0.03f;
    float2 nv=p-to_float2(1.0f-T*0.1f,-0.02f);
    float2 f = fract(nv*40.0f)-0.5f;
    float2 fid = _floor(nv*40.0f)+0.5f;
    fid.x=mod_f(fid.x,30.0f);
    float ht = sampleFreq(fid.x*0.015f, iChannel0)*0.85f; 
    
    float bd = box(f,to_float2_s(0.4f))-0.01f;
    bd=_fmaxf(bd,dcut);
    bd=smoothstep(px,-px,bd);

    float bx = box(f,to_float2_s(0.3f))-0.01f;
    bx=_fabs(bx)-0.12f;
    bx=_fmaxf(bx,dcut);
    bx=smoothstep(px,-px,bx);
    
    //draw dots
    if(fid.x<40.0f&& fid.y>0.0f) {
        float avg = (fid.y*0.095f)-0.5f; 
        if(ht>avg) C=_mix(C,to_float3_s(0.0001f),bx);                 // Wavefelderfarbe
        if ( ht>avg&&ht<avg+0.1f) C=_mix(C,to_float3_s(0.0001f),bd); // Wavefelderspitzenfarbe
    }
  
      
    //uv.y=_fabs(uv.y-0.38f)-0.02f;
    uv.y=_fabs(uv.y-dc.y-dy)-dy;
    //float nd = box(uv,to_float2(5.0f,0.0075f));
    float nd = box(uv,to_float2(dc.x,0.0075f));
    nd=smoothstep(px,-px,nd);
    C=_mix(C,tcolor,nd);
    return C;

}

__KERNEL__ void Nanokontrol2Fuse(float4 O, float2 F, float iTime, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0)
{
  
    // button color
//    const float3 lcolor = to_float3(0.949f,0.008f,0.290f);
    // power color
//    const float3 tcolor = to_float3(0.855f,0.969f,0.812f);

    CONNECT_COLOR0(LColor, 0.949f,0.008f,0.290f, 1.0f); 
    CONNECT_COLOR1(TColor, 0.855f,0.969f,0.812f, 1.0f);
    CONNECT_COLOR2(bkgCol, 0.384f,0.510f,0.227f, 1.0f); 
    CONNECT_COLOR3(sliderCol, 0.5f,0.5f,0.5f, 1.0f); 
    CONNECT_COLOR4(knobCol, 0.6f,0.6f,0.6f, 1.0f); 
    CONNECT_COLOR5(nanoCol, 0.1f,0.1f,0.1f, 1.0f);     
    CONNECT_COLOR6(Background, 0.5f,0.5f,0.5f, 1.0f);     
    CONNECT_COLOR7(PanelborderCol, 0.0f,0.0f,0.0f, 1.0f);
    CONNECT_COLOR8(Panel1Col, 0.015f,0.015f,0.015f, 1.0f);
    CONNECT_COLOR9(Panel2Col, 0.035f,0.035f,0.035f, 1.0f);
    
    CONNECT_SLIDER0(DY, -10.0f, 10.0f, 0.02);
    CONNECT_CHECKBOX0(Fun, 0);
    
    CONNECT_SLIDER1(angle, -10.0f, 10.0f, 0.78f);
    CONNECT_POINT0(dc, 5.0f, 0.4f);
    
    
    
    Background -=0.5f; 
    float3 PanelCol[3] = {swi3(PanelborderCol,x,y,z),swi3(Panel1Col,x,y,z),swi3(Panel2Col,x,y,z)};
  
    float3 lcolor = swi3(LColor,x,y,z);
    float3 tcolor = swi3(TColor,x,y,z);
  
    float2 uv = (2.0f* swi2(F,x,y)-swi2(R,x,y))/_fmaxf(R.x,R.y);
    float hs = hash21(uv*_sinf(uv.x));
    float3 C = to_float3_s(0.3f-(hs*0.05f)) + swi3(Background,x,y,z);

    float px=2.0f/R.x;
    
    C = backdrop(C, uv, px, tcolor, iTime, swi3(bkgCol,x,y,z),angle, dc, DY, iChannel0);

    // uncomment for fun
    if(Fun) //Panel wird von rechts nach links durch Bild bewegt
    {
      uv.x+=T*0.3f;
      uv.x=mod_f(uv.x+1.0f,2.0f)-1.0f;
    }
    
    C = nanoBody(C, uv+to_float2(0,0.25f),px,tcolor,swi3(nanoCol,x,y,z), PanelCol);
    // to use midi texture you loop over
    // the control values based on a 32x32
    // texture
    // https://hexler.net/kodelife/manual/parameters-built-in

    // sliders
    for(int i = 0; i<8;i++) {
        //float md1 = (texelFetch(midi1, midiCoord(3 * 127 + i), 0).x);
        float md1 = sampleFreq((float)(i)*0.042f,iChannel0)*0.35f; 
        float2 p = to_float2(0.25f,0.29f)-to_float2((float)(i)*0.13f,0);
        
        C = slider(C, uv+p, md1, px, swi3(sliderCol,x,y,z));
    }    

    // knobs
    float tk = 0.0f;
    for(int i = 16; i<24;i++) {
    //
        float md1 = sampleFreq((float)(i)*0.051f,iChannel0)*0.5f; 
        float2 p = to_float2(0.25f,0.0925f)-to_float2((float)(tk)*0.13f,0);
        C = knob(C,uv+p,md1,px, swi3(knobCol,x,y,z));
        
        tk++;
    }
    
    // solo buttons
    tk = 0.0f;
    for(int i = 32; i<40;i++) {
        float md1 = sampleFreq((float)(tk*1.25f)*0.02f,iChannel0)*0.45f; 
        if(md1<0.2f) md1 = 0.0f;
        float2 p = to_float2(0.3125f,0.2f)-to_float2((float)(tk)*0.13f,0);
        C = button(C,uv+p, md1, 0, px, lcolor);
        tk++;
    }  

    // mute buttons
    tk = 0.0f;
    for(int i = 48; i<56;i++) {
        float md1 = sampleFreq((float)(tk*1.5f)*0.1f,iChannel0)*0.45f; 
        if(md1<0.1f) md1 = 0.0f;
        float2 p = to_float2(0.3125f,0.25f)-to_float2((float)(tk)*0.13f,0);
        C = button(C,uv+p, md1, 0, px, lcolor);
        tk++;
    }  

    // record buttons
    tk = 0.0f;
    for(int i = 64; i<72;i++) {
        float md1 = sampleFreq((float)(i-35)*0.1f,iChannel0)*0.45f; 
        if(md1<0.1f) md1 = 0.0f;
        float2 p = to_float2(0.3125f,0.3f)-to_float2((float)(tk)*0.13f,0);
        C = button(C,uv+p, md1, 0, px, lcolor);
        tk++;
    }  
    
    // track buttons
    for(int i = 41; i<46;i++) {
        float md1 = sampleFreq((float)(i-41)*0.1f,iChannel0)*0.45f; 
        if(md1<0.25f) md1 = 0.0f;
        // track button midi jumps all over? why Korg?
        // fixing my brute force
        float fk = 0.0f;
        if(i==41){
          fk=10.0f;
        }else if(i==42){
          fk=9.0f;
        }else if(i==43){
          fk=7.0f;
        }else if(i==44){
          fk=8.0f;
        }else{
          fk=11.0f;
        }

        float2 p = to_float2(1.0f,0.4f)-to_float2((float)(fk)*0.05f,0);
        C = button(C,uv+p, md1, 0, px, lcolor);
        tk++;
    }
    
    // cycle button
    tk = 0.0f;
    float md1 = sampleFreq((float)(0.23434f)*0.1f,iChannel0)*0.45f; 
    if(md1<0.2f) md1 = 0.0f;
    C = button(C,uv+to_float2(0.65f,0.30f), md1, 2, px, lcolor);
    
    // marker buttons
        for(int i = 60; i<63;i++) {
        float md1 = sampleFreq((float)(i-35)*0.1f,iChannel0)*0.45f; 
        if(md1<0.1f) md1 = 0.0f;
        float2 p = to_float2(0.55f,0.30f)-to_float2((float)(tk)*0.05f,0);
        C = button(C,uv+p, md1, 2, px, lcolor);
        tk++;
    }  
    
    // track buttons
        for(int i = 58; i<60;i++) {
        float md1 = sampleFreq((float)(i-57)*0.078f,iChannel0)*0.45f; 
        if(md1<0.25f) md1 = 0.0f;
        float2 p = to_float2(0.8f,0.25f)-to_float2((float)(tk)*0.05f,0);
        C = button(C,uv+p, md1, 2, px, lcolor);
        tk++;
    }  

    if(hs<0.65f) C = clamp(C+(hs*0.005f),C,to_float3_s(1));
    C = pow_f3(C, to_float3_s(0.4545f));
    O = to_float4_aw(C,1.0f);


  SetFragmentShaderComputedColor(O);
}