
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/giovanni-sollima/linvenzione-del-nero' to iChannel0


// Fork of "generative art deco 3" by morisil. https://shadertoy.com/view/mdl3WX
// 2022-10-28 00:47:55

// Fork of "generative art deco 2" by morisil. https://shadertoy.com/view/ftVBDz
// 2022-10-27 22:34:54

// Fork of "generative art deco" by morisil. https://shadertoy.com/view/7sKfDd
// 2022-09-28 11:25:15

// Copyright Kazimierz Pogoda, 2022 - https://xemantic.com/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// copyright statement borrowed from Inigo Quilez

// Music by Giovanni Sollima, L'invenzione del nero:
// https://soundcloud.com/giovanni-sollima/linvenzione-del-nero

// See also The Mathematics of Perception to check the ideas behind:
// https://www.shadertoy.com/view/7sVBzK



#define PI       3.14159265359f
#define TWO_PI   6.28318530718f

__DEVICE__ mat2 rotate2d(float _angle){
    return to_mat2(_cosf(_angle),-_sinf(_angle),
                   _sinf(_angle),_cosf(_angle));
}

__DEVICE__ float sdPolygon(in float angle, in float distance) {
  float segment = TWO_PI / 4.0f;
  return _cosf(_floor(0.5f + angle / segment) * segment - angle) * distance;
}

__DEVICE__ float getColorComponent(in float2 st, in float modScale, in float blur, float SHAPE_SIZE, float iTime) {
float zzzzzzzzzzzzzzz;    
    float2 modSt = mod_f2(st, 1.0f / modScale) * modScale * 2.0f - 1.0f;
    float dist = length(modSt);
    float angle = _atan2f(modSt.x, modSt.y) + _sinf(iTime * 0.08f) * 9.0f;
    //dist = sdPolygon(angle, dist);
    //dist += _sinf(angle * 3.0f + iTime * 0.21f) * 0.2f + _cosf(angle * 4.0f - iTime * 0.3f) * 0.1f;
    float shapeMap = smoothstep(SHAPE_SIZE + blur, SHAPE_SIZE - blur, _sinf(dist * 3.0f) * 0.5f + 0.5f);
    return shapeMap;
}

__KERNEL__ void GenerativeArtDeco4Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(SHAPE_SIZE, -1.0f, 10.0f, 0.618f);
  CONNECT_SLIDER1(CHROMATIC_ABBERATION, -1.0f, 1.0f, 0.01f);
  CONNECT_SLIDER2(ITERATIONS, -1.0f, 50.0f, 10.0f);
  CONNECT_SLIDER3(INITIAL_LUMA, -1.0f, 1.0f, 0.5f);
  CONNECT_SLIDER4(GRADING_INTENSITY, -1.0f, 1.0f, 0.4f);
  
  CONNECT_SLIDER5(BlurOff, -1.0f, 1.0f, 0.4f);
  CONNECT_SLIDER6(BlurFkt, -1.0f, 1.0f, 0.2f);
  
  CONNECT_SLIDER7(Zoom, -1.0f, 10.0f, 1.0f);
  
  CONNECT_POINT0(BlurLumaOff, 0.0f, 0.0f );
  
  CONNECT_CHECKBOX0(OrgPar, 1);
  
  if(OrgPar)
  {
     SHAPE_SIZE = 0.618f;
     CHROMATIC_ABBERATION = 0.01f;
     ITERATIONS = 10.0f;
     INITIAL_LUMA = 0.5f;
     GRADING_INTENSITY = 0.4f;
  }

    //float blur = 0.4f + _sinf(iTime * 0.52f) * 0.2f;
    float blur = BlurOff + _sinf(iTime * 0.52f) * BlurFkt;

    float2 st =
        (2.0f* fragCoord - iResolution)
        / _fminf(iResolution.x, iResolution.y);
        
    if(iMouse.z>0.0f) st+= 1.0f-(swi2(iMouse,x,y)/iResolution + 0.5f); 

    st*=Zoom;
    
    float2 origSt = st;
    st = mul_f2_mat2(st, rotate2d(_sinf(iTime * 0.14f) * 0.3f));
    st *= (_sinf(iTime * 0.15f) + 2.0f) * 0.3f;
    st *= _logf(length(st * 0.428f)) * 1.1f;

    float modScale = 1.0f;

    float3 color = (swi3(Color,x,y,z)-0.5f)*2.0f;//to_float3_s(0);
    float luma = INITIAL_LUMA;
    for (float i = 0.0f; i < ITERATIONS; i+=1.0f) {
        float2 center = st + to_float2(_sinf(iTime * 0.12f), _cosf(iTime * 0.13f));
        //center += _powf(length(center), 1.0f);
        float3 shapeColor = to_float3(
                      getColorComponent(center - st * CHROMATIC_ABBERATION, modScale, blur,SHAPE_SIZE,iTime),
                      getColorComponent(center, modScale, blur,SHAPE_SIZE,iTime),
                      getColorComponent(center + st * CHROMATIC_ABBERATION, modScale, blur,SHAPE_SIZE,iTime)        
                      ) * luma;
        st *= 1.1f + getColorComponent(center, modScale, 0.04f,SHAPE_SIZE,iTime) * 1.2f;
        st = mul_f2_mat2(st,rotate2d(_sinf(iTime  * 0.05f) * 1.33f));
        color += shapeColor;
        color = clamp(color, 0.0f, 1.0f);
//        if (color == to_float3_aw(1)) break;
        luma *= (0.6f+BlurLumaOff.x);
        blur *= (0.63f+BlurLumaOff.y);
    }
    
    float3 topGrading = to_float3(
                                   1.0f + _sinf(iTime * 1.13f * 0.3f) * GRADING_INTENSITY,
                                   1.0f + _sinf(iTime * 1.23f * 0.3f) * GRADING_INTENSITY,
                                   1.0f - _sinf(iTime * 1.33f * 0.3f) * GRADING_INTENSITY
                                 );
    float3 bottomGrading = to_float3(
                                   1.0f - _sinf(iTime * 1.43f * 0.3f) * GRADING_INTENSITY,
                                   1.0f - _sinf(iTime * 1.53f * 0.3f) * GRADING_INTENSITY,
                                   1.0f + _sinf(iTime * 1.63f * 0.3f) * GRADING_INTENSITY
                                 );
    float origDist = length(origSt);
    float3 colorGrading = _mix(topGrading, bottomGrading, origDist - 0.5f);
    fragColor = to_float4_aw(pow_f3(swi3(color,x,y,z), colorGrading), 1.0f);
    fragColor *= smoothstep(2.1f, 0.7f, origDist);

  SetFragmentShaderComputedColor(fragColor);
}