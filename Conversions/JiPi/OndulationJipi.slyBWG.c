
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0



#define PI 3.1415f

__DEVICE__ mat2 rotate2d(float angle)
{
   return to_mat2(_cosf(angle),-_sinf(angle),_sinf(angle),_cosf(angle));   
}

__DEVICE__ float2 toPolar(float2 uv)
{
    float distance = length(uv);
    float angle = _atan2f(uv.y,uv.x);
    return to_float2(angle/PI*2.0f,distance);
}


__KERNEL__ void OndulationJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(ShowCentral, 0);

    CONNECT_POINT0(PosXY, 0.0f, 0.0f );
    //CONNECT_POINT1(Pos2XY, 0.0f, 0.0f );
    CONNECT_SLIDER0(SizeP, -1.0f, 50.0f, 25.0f);
    CONNECT_SLIDER1(NBR_WAVE, -1.0f, 1000.0f, 100.0f);
    CONNECT_SLIDER2(HEIGHT_WAVE, -1.0f, 1000.0f, 100.0f);
    //CONNECT_SLIDER3(Radius, -1.0f, 10.0f, 2.0f);


    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    float ratio = iResolution.y/iResolution.x;
    
    //uv.x = (uv.x-0.25f) / ratio;
    //uv = toPolar(uv-0.5f);

    float _iTime = -iTime; 

    float2 varyingUV = uv+PosXY-0.5f;

    varyingUV.x/=ratio;

    // Time varying pixel color
    uv.y += (_sinf((toPolar(varyingUV)).y*NBR_WAVE+ _iTime*20.0f)/HEIGHT_WAVE);
    uv.x += (_cosf((toPolar(varyingUV)).y*NBR_WAVE+ _iTime*20.0f)/HEIGHT_WAVE);
    uv.y+=0.01f; //zoomer sur l'image afin de masquer la répétition
    uv.y*=0.97f;
    
    //si on joue sur les valeur x et y d'uv on peu crée d'autre type de déformation soit vertical soit horizontal ou les deux
    
    //uv += PosXY; 
    
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);


    //Markierung des Zentralpunktes
    float2 mouse = iResolution-(PosXY+0.5)*iResolution;//swi2(iMouse,x,y); //PosXY+0.5f;//

    float dist = distance_f2(mouse, fragCoord);
    if ( ShowCentral && dist < SizeP ) {
      // Drawing a circle around the mouse with a solid color
      // Blue if mouse button is not pressed, orange if it is pressed
      col = to_float3(1.0f, 1.0f, 1.0f);// + to_float3(1.0f, 0.5f, -1.0f) * (float)(mouse_hold);
    }

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}