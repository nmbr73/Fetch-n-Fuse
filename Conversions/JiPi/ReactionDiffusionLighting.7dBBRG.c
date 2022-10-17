
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define TAU 6.28318530718f

//  1 out, 2 in...
__DEVICE__ float hash12(float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Starttexture' to iChannel1

#define D_A 1.0f
#define D_B 0.5f
#define F 0.055f
#define K 0.062f
#define T_STEP 0.5f


__DEVICE__ float4 laplacian(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0){

    const float weights[9] = {
        0.05f,0.2f,0.05f,
        0.2f, -1.0f, 0.2f,
        0.05f,0.2f,0.05f
        };
float zzzzzzzzzzzzzz;
    float4 weightedLevels = to_float4_s(0.0f);
    for(int i = 0; i < 9; i++){
        int dx = i%3-1;
        int dy = i/3-1;
        float2 uv = (fragCoord + to_float2(dx, dy))/iResolution;

        weightedLevels += weights[i] * _tex2DVecN(iChannel0,uv.x,uv.y,15);
    }
    
    return weightedLevels;
}


__KERNEL__ void ReactionDiffusionLightingFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Tex, 0);
    
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    //uv.x *= iResolution.x/iResolution.y;
float AAAAAAAAAAAAAAAA;
    float4 levels = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float a = levels.x;
    float b = levels.y;
   
    float4 weightedLevels = laplacian(fragCoord, iResolution, iChannel0);
    
    //https://www.karlsims.com/rd.html
    float aChange = (D_A * weightedLevels.x - a * b * b + F * (1.0f-a))*T_STEP; 
    float bChange = (D_B * weightedLevels.y + a * b * b - (F+K) *b)*T_STEP; 
    
    float newA = a + aChange;
    float newB = b + bChange;
    
    if(iFrame < 10 || Reset){
      
      if (Tex)
      {
        float4 Textur = _tex2DVecN(iChannel1,uv.x,uv.y,15);
        if (Textur.w > 0.0f)
        {
          newA = 0.5f+hash12(uv*100.0f)*0.8f;
          newB = 1.0f;//step(0.5f, hash12(_floor(uv*50.0f) + to_float2(10.0f, 16.23f)));
        }
        else
          newB = 0.0f;
      }
      else
      {
        newA = 0.5f+hash12(uv*100.0f)*0.8f;
        newB = step(0.5f, hash12(_floor(uv*50.0f) + to_float2(10.0f, 16.23f)));
      }
    }
   
    fragColor = to_float4(newA, newB, 1.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0





__DEVICE__ float3 palette( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3(TAU*(c*t+d) );
}

__DEVICE__ float3 palette1(float t){
    return palette(t, to_float3(0.8f, 0.5f, 0.4f), to_float3(0.2f, 0.4f, 0.2f), to_float3(2.0f, 1.0f, 1.0f), to_float3(0.00f, 0.25f, 0.25f));
}

__DEVICE__ float3 palette2(float t){
    return palette(t, to_float3(0.5f, 0.5f, 0.5f), to_float3(0.5f, 0.5f, 0.5f), to_float3(1.0f, 1.0f, 1.0f), to_float3(0.00f, 0.33f, 0.67f));
}

__DEVICE__ float3 palette3(float t){
    return palette(t, to_float3(0.5f, 0.5f, 0.5f), to_float3(0.5f, 0.5f, 0.5f), to_float3(2.0f, 1.0f, 0.0f), to_float3(0.50f, 0.20f, 0.25f));
}

__DEVICE__ float3 palette4(float t){
    return palette(t, to_float3(0.5f, 0.5f, 0.5f), to_float3(0.5f, 0.5f, 0.5f), to_float3(2.0f, 1.0f, 0.0f), to_float3(0.6627f, 0.9411f, 0.8196f));
}

__DEVICE__ float3 palette5(float t){
     return to_float3(1.0f, _powf(cos(t*TAU*1.0f), 2.0f)*0.8f+0.2f, _powf(cos(t*TAU*0.5f), 2.0f));
}

__DEVICE__ float3 palette6(float t){
    return to_float3_s(smoothstep(0.55f, 0.45f, t)*0.8f +0.06f);
}

__DEVICE__ float3 getColour(float depth, float2 uv, int Palette){
    
    switch(Palette)
    {
      case 1:
        return palette1(depth); break;
      case 2:
        return palette2(depth); break;
      case 3:
        return palette3(depth); break;
      case 4:
        return palette4(depth); break;
      case 5:
        return palette5(depth); break;
      case 6:
        return palette6(depth); break;
    }  
    return palette6(depth);
}

__DEVICE__ float getDepth(float2 uv, __TEXTURE2D__ iChannel0){
    float4 values = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float a = values.x;
    float b = values.y;
    float depth = a-b;
    return depth;
}

//https://www.shadertoy.com/view/XlGBW3
__DEVICE__ float3 getNormal(float2 uv, float2 iResolution, __TEXTURE2D__ iChannel0) {
    float3 p = to_float3_aw(uv, 0.0f);
    float2 e = to_float2(1.0f/iResolution.y, 0);//to_float2(0.0001f, 0);
    float d = getDepth(uv+swi2(e,x,y),iChannel0);

    float3 normal = d - to_float3(
                                  getDepth(uv-swi2(e,x,y),iChannel0),
                                  getDepth(uv-swi2(e,y,x),iChannel0),
                                  -1.
    );
    
    return normalize(normal);
}

__DEVICE__ float3 getLightDir(float2 uv, float4 iMouse, float2 iResolution){
    float3 lightPos = to_float3_aw(swi2(iMouse,x,y)/iResolution*1.0f, 0.2f);

    float3 fragPos = to_float3_aw(uv, 0.0f);
    float3 lightDir = to_float3_aw(swi2(lightPos,x,y)-to_float2(0.5f, 0.5f), 0.5f); 
    //vec3 lightDir = lightPos - fragPos;
    lightDir = normalize(lightDir);
    return lightDir;
}

//https://learnopengl.com/Lighting/Basic-Lighting
__DEVICE__ float3 getAmbient(float2 uv, float3 lightColor){
    float ambientStrength = 0.1f;
    float3 ambient = ambientStrength * lightColor;
    return ambient;
}

__DEVICE__ float3 getDiffuse(float2 uv, float4 iMouse, float2 iResolution, __TEXTURE2D__ iChannel0, float3 lightColor){
    
    float diff = dot(getNormal(uv, iResolution, iChannel0), getLightDir(uv, iMouse,iResolution)); 
    //diff = _fmaxf(diff, 0.0f);
    float3 diffuse = diff * lightColor;
    return diffuse;
}

__DEVICE__ float3 getSpecular(float2 uv, float4 iMouse, float2 iResolution, __TEXTURE2D__ iChannel0, float3 lightColor){

    float specularStrength = 10.0f;

    //vec3 viewPos = to_float3_aw(uv, 1.0f);
    //vec3 fragPos = to_float3_aw(uv, 0.0f);
    //vec3 viewDir = normalize(viewPos - FragPos);
    float3 viewDir = to_float3(0.0f, 0.0f, 1.0f);
    float3 reflectDir = reflect(-1.0f*getLightDir(uv, iMouse, iResolution), getNormal(uv, iResolution, iChannel0));  
    float spec = _powf(_fmaxf(dot(viewDir, reflectDir), 0.0f), 256.0f);
    float3 specular = specularStrength * spec * lightColor; 
    
    return specular;
}

__KERNEL__ void ReactionDiffusionLightingFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_COLOR0(LightColor, 1.0f, 1.0f, 1.0f, 1.0f);
    CONNECT_INTSLIDER0(Palette, 1, 6, 6);
    CONNECT_SLIDER0(Depth1, -1.0f, 2.0f, 0.8f);
    CONNECT_SLIDER1(Depth2, -1.0f, 2.0f, 0.7f);
    CONNECT_SLIDER2(Size, -1.0f, 2.0f, 0.4f);

    float3 lightColor = swi3(LightColor,x,y,z);//to_float3_s(1.0f);

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution*Size;//0.4f;
    //uv.x *= iResolution.x/iResolution.y;
   
    float depth = getDepth(uv, iChannel0);
    
    float3 normal = getNormal(uv, iResolution, iChannel0);
    
    //float3 cool = normal*depth*0.8f+depth*0.7f;
    float3 cool = normal*depth*Depth1+depth*Depth2;
    float3 c = to_float3_s(length(cool));
    
 
    float3 lightness = getDiffuse(uv, iMouse,iResolution, iChannel0, lightColor) + getAmbient(uv, lightColor)+ getSpecular(uv, iMouse,iResolution, iChannel0, lightColor);
    float3 colour = getColour(depth,uv, Palette);
    // Output to screen
    fragColor = to_float4_aw((colour*lightness), LightColor.w);

  SetFragmentShaderComputedColor(fragColor);
}