
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

union matrix
{
  mat3  m;
  float a[3][3];
};


#define BUFFER_A_SAMPLER iChannel0

#define NUM_TILE_IDS 32.0f
#define DATA_BEGIN 0.0f
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// global data 

__KERNEL__ void FallingSandOstJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_INTSLIDER0(Material, 0, 9, 1);
    
    CONNECT_COLOR0(Sand,  0.8f, 0.6f, 0.2f, 1.0f);
    CONNECT_COLOR1(Stone, 0.6f, 0.6f, 0.6f, 1.0f);
    CONNECT_COLOR2(Oil,   0.5f, 0.3f, 0.1f, 1.0f);
    CONNECT_COLOR3(Steam, 0.9f, 0.9f, 0.9f, 1.0f);
    CONNECT_COLOR4(Magma, 0.8f, 0.1f, 0.0f, 1.0f);
    CONNECT_COLOR5(Water, 0.1f, 0.2f, 0.8f, 1.0f);
    CONNECT_COLOR6(Fire,  1.0f, 0.5f, 0.0f, 1.0f);
    CONNECT_COLOR7(Glass, 0.8f, 0.8f, 0.8f, 1.0f);
    CONNECT_COLOR8(Mud,   0.4f, 0.2f, 0.0f, 1.0f);
    
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float index = fragCoord.x-0.5f + (fragCoord.y-0.5f)*iResolution.y;
    
    if (index >= DATA_BEGIN && index < DATA_END) {
        //Default value:
        fragColor = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
        
        if (index >= DENSITY_BEGIN && index < DENSITY_END) {
            float id = index - DENSITY_BEGIN;
            if (id == 0.0f) //Air
                fragColor = to_float4_s(0.0f);
            else if (id == 1.0f) // Sand
                fragColor = to_float4_s(1.5f);
            else if (id == 2.0f) // Stone
                fragColor = to_float4_s(3.0f);
            else if (id == 3.0f) // Oil
                fragColor = to_float4_s(0.89f);
            else if (id == 4.0f) // Steam
                fragColor = to_float4_s(-0.1f);
            else if (id == 5.0f) // Magma
                fragColor = to_float4_s(2.5f);
            else if (id == 6.0f) // Water
                fragColor = to_float4_s(1.0f);
            else if (id == 7.0f) // Fire
                fragColor = to_float4_s(-1.0f);
            else if (id == 8.0f) // Glass
                fragColor = to_float4_s(1.5f);
            else if (id == 9.0f) // Mud
                fragColor = to_float4_s(1.4f);
        }
        else if (index >= COLOR_BEGIN && index < COLOR_END) {
            float id = index - COLOR_BEGIN;
            if (id == 0.0f) // Air
                fragColor = to_float4_s(0.0f);
            else if (id == 1.0f) // Sand
                fragColor = Sand;//to_float4(0.8f, 0.6f, 0.2f, 1.0f);
            else if (id == 2.0f) // Stone
                fragColor = Stone;//to_float4(0.6f, 0.6f, 0.6f, 1.0f);
            else if (id == 3.0f) // Oil
                fragColor = Oil;//to_float4(0.5f, 0.3f, 0.1f, 1.0f);
            else if (id == 4.0f) // Steam
                fragColor = Steam;//to_float4(0.9f, 0.9f, 0.9f, 1.0f);
            else if (id == 5.0f) // Magma
                fragColor = Magma;//to_float4(0.8f, 0.1f, 0.0f, 1.0f);
            else if (id == 6.0f) // Water
                fragColor = Water;//to_float4(0.1f, 0.2f, 0.8f, 1.0f);
            else if (id == 7.0f) // Fire
                fragColor = Fire;//to_float4(1.0f, 0.5f, 0.0f, 1.0f);
            else if (id == 8.0f) // Glass
                fragColor = Glass;//to_float4(0.8f, 0.8f, 0.8f, 1.0f);
            else if (id == 9.0f) // Mud
                fragColor = Mud;//to_float4(0.4f, 0.2f, 0.0f, 1.0f);
        }
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    else if (index == SELECTED_ELEMENT) {
        if (iFrame == 0 || Reset) {
            fragColor = to_float4_s(6.0f);
            
            SetFragmentShaderComputedColor(fragColor);
            return;
        }
   
        #ifdef ORG
        if (texture(iChannel1, to_float2(48.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(0.0f);
        else if (texture(iChannel1, to_float2(49.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(1.0f);
        else if (texture(iChannel1, to_float2(50.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(2.0f);
        else if (texture(iChannel1, to_float2(51.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(3.0f);
        else if (texture(iChannel1, to_float2(52.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(4.0f);
        else if (texture(iChannel1, to_float2(53.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(5.0f);
        else if (texture(iChannel1, to_float2(54.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(6.0f);
        else if (texture(iChannel1, to_float2(55.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(7.0f);
        else if (texture(iChannel1, to_float2(56.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(8.0f);
        else if (texture(iChannel1, to_float2(57.5f/256.0f, 0.0f)).x > 0.0f)
            fragColor = to_float4_s(9.0f);
        else
            fragColor = _tex2DVecN(BUFFER_A_SAMPLER,uv.x,uv.y,15);
        #endif
        
        fragColor = to_float4_s((float)Material);
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    fragColor = to_float4(uv.x*32.0f, uv.y*32.0f, 0.5f,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


//#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_D_SAMPLER iChannel1

/*
#define NUM_TILE_IDS 32.0f
#define DATA_BEGIN 0.0f
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END
*/
// Movement calculations.

__DEVICE__ float rand(float2 co)
{
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}  

__DEVICE__ float getDensity(float id, float2 R, __TEXTURE2D__ BUFFER_A_SAMPLER) {
    float index = id + DENSITY_BEGIN;
    float2 uv = to_float2(mod_f(index/iResolution.x , 1.0f), _floor(index/iResolution.x)/iResolution.y) + 0.5f/iResolution;
    float density = _tex2DVecN(BUFFER_A_SAMPLER,uv.x,uv.y,15).x;
    return density;
}

__DEVICE__ mat3 readMatID(float2 fragCoord, float2 R, __TEXTURE2D__ BUFFER_D_SAMPLER) {
    //mat3 matID = to_mat3_f(0.0f);
    matrix matID;
    matID.m = to_mat3_f(0.0f);
    
    for (int x = 0; x < 3; ++x) {
            matID.a[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,-1))/iResolution).x;
            matID.a[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,0))/iResolution).x;
            matID.a[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,+1))/iResolution).x;
    }
    return matID.m;
}

__DEVICE__ mat3 readMatDensity(float2 fragCoord, matrix matID, float2 R, __TEXTURE2D__ BUFFER_A_SAMPLER) {
    //mat3 matDensity = mat3(0.0f);
    matrix matDensity;
    matDensity.m  = to_mat3_f(0.0f);
    
    //matrix _matID;
    //_matID.m = matID;
    
    for (int x = 0; x < 3; ++x) {
        matDensity.a[x][0] = getDensity(matID.a[x][0],R,BUFFER_A_SAMPLER);
        matDensity.a[x][1] = getDensity(matID.a[x][1],R,BUFFER_A_SAMPLER);
        matDensity.a[x][2] = getDensity(matID.a[x][2],R,BUFFER_A_SAMPLER);
    }
    return matDensity.m;
    
}

__DEVICE__ float2 calcMove(matrix matID, matrix matDensity, float2 fragCoord, float iTime) {
    int hash = (int)(1024.0f*rand(to_float2(rand(fragCoord), iTime)));
    int hash_3 = (int)(mod_f((float)(hash), 3.0f)+0.5f);
    int hash_2 = (int)(mod_f((float)(hash), 4.0f)+0.5f);
    float density = matDensity.a[1][1];
    
    //if (density <= 0.0f && hash_2 != 1)
    //    return to_float2_s(0.0f);
 
    if (matDensity.a[1][0] < density-1.25f)
        return to_float2(0, -1);
    
    if (matDensity.a[0][2] < density-1.25f)
        return to_float2(-1, -1);
    
    if (matDensity.a[2][2] < density-1.25f)
        return to_float2(1, -1);
    
    
    if (hash_3 == 0 && matDensity.a[0][0] < density)
        return to_float2(-1, -1);
    
    if (hash_3 == 2 && matDensity.a[2][0] < density)
        return to_float2(+1, -1);
    
    if (matDensity.a[1][1] < density)
        return to_float2(0, -1);
        
    if (hash_3 == 0)
        return to_float2(+1, 0);
    
    if (hash_3 == 2)
        return to_float2(-1, 0);
    
    /*if (hash_9 == 3)
        return to_float2(-1, +0);
    
    if (hash_9 == 4)
        return to_float2(+0, +0);
    
    if (hash_9 == 5)
        return to_float2(+1, +0);
    
    if (hash_9 == 6)
        return to_float2(-1, +1);
    
    if (hash_9 == 7)
        return to_float2(+0, +1);
    
    if (hash_9 == 8)
        return to_float2(+1, +1);*/
        
    return to_float2(0,-1);
}

__KERNEL__ void FallingSandOstJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    
    matrix matID;
    matID.m = readMatID(fragCoord, R, BUFFER_D_SAMPLER);
    matrix matDensity;
    matDensity.m = readMatDensity(fragCoord, matID,R, BUFFER_A_SAMPLER);
    //float[3][3] sand = {{to_float4(0.0f,to_float4(0.0f,to_float4(0.0},{to_float4(0.0f,to_float4(0.0f,to_float4(0.0},{to_float4(0.0f,to_float4(0.0f,to_float4(0.0}};
    
    float2 move = calcMove(matID, matDensity, fragCoord, iTime);
    
    fragColor = to_float4(move.x, move.y, 0.0f, 0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


//#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_B_SAMPLER iChannel1
#define BUFFER_D_SAMPLER iChannel2

/*
#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END
*/
// Pixel movements are paired.

/*
__DEVICE__ float getDensity(float id) {
    float index = id + DENSITY_BEGIN;
    float2 uv = to_float2(mod_f(index/iResolution.x , 1.0f), _floor(index/iResolution.x)/iResolution.y) + 0.5f/iResolution;
    float density = _tex2DVecN(BUFFER_A_SAMPLER,uv.x,uv.y,15).x;
    return density;
}

__DEVICE__ mat3 readMatID(float2 fragCoord) {
    mat3 matID = mat3(0.0f);
    
    for (int x = 0; x < 3; ++x) {
        matID[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,0-1))/iResolution).x;
        matID[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,1-1))/iResolution).x;
        matID[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,2-1))/iResolution).x;
    }
    return matID;
}

__DEVICE__ mat3 readMatDensity(float2 fragCoord, mat3 matID) {
    mat3 matDensity = mat3(0.0f);
    
    for (int x = 0; x < 3; ++x) {
        matDensity[x][0] = getDensity(matID[x][0]);
        matDensity[x][1] = getDensity(matID[x][1]);
        matDensity[x][2] = getDensity(matID[x][2]);
    }
    return matDensity;
}
*/

__DEVICE__ void readMatMove(float matMoveX[3][3], float matMoveY[3][3], float2 fragCoord,float2 R, __TEXTURE2D__ BUFFER_B_SAMPLER) {
    matrix matID;
    matID.m = to_mat3_f(0.0f);
    
    for (int x = 0; x < 3; ++x) {
        float2 v0 = swi2(texture(BUFFER_B_SAMPLER, (fragCoord+to_float2(x-1,0-1))/iResolution),x,y);
        float2 v1 = swi2(texture(BUFFER_B_SAMPLER, (fragCoord+to_float2(x-1,1-1))/iResolution),x,y);
        float2 v2 = swi2(texture(BUFFER_B_SAMPLER, (fragCoord+to_float2(x-1,2-1))/iResolution),x,y);
        matMoveX[x][0] = v0.x;
        matMoveY[x][0] = v0.y;
        matMoveX[x][1] = v1.x;
        matMoveY[x][1] = v1.y;
        matMoveX[x][2] = v2.x;
        matMoveY[x][2] = v2.y;
    }
}


__DEVICE__ float2 calcMovePaired(const float matID[3][3], const float matDensity[3][3], const float matMoveX[3][3], const float matMoveY[3][3], const float2 fragCoord) {
     float density = matDensity[1][1];
     float2 move = to_float2(matMoveX[1][1], matMoveY[1][1]);
     int ox = (int)(1.0f+move.x);
     int oy = (int)(1.0f+move.y);
    
  
    if (matMoveY[1][2] == -1.0f && matMoveX[1][2] == 0.0f && matDensity[1][2] > density)
        return to_float2(0.0f, 1.0f);
    
    if (matMoveY[0][2] == -1.0f && matMoveX[0][2] == 1.0f && matDensity[0][2] > density)
        return to_float2(-1.0f, 1.0f);
    
    if (matMoveY[2][2] == -1.0f && matMoveX[2][2] == -1.0f && matDensity[2][2] > density)
        return to_float2(1.0f, 1.0f);
    
    
    if (matMoveY[2][1] == 0.0f && matMoveX[2][1] == -1.0f)
        return to_float2(1.0f, 0.0f);
    
    if (matMoveY[0][1] == 0.0f && matMoveX[0][1] == 1.0f)
        return to_float2(-1.0f, 0.0f);
    
   
    /*if ((matDensity[0][0] <= density && ox == 0 && oy == 0 && matMoveX[0][0] == 1.0f && matMoveY[0][0] == 1.0f) ||
        (matDensity[1][0] <= density && ox == 1 && oy == 0 && matMoveX[1][0] == 0.0f && matMoveY[1][0] == 1.0f) ||
        (matDensity[2][0] <= density && ox == 2 && oy == 0 && matMoveX[2][0] == -1.0f && matMoveY[2][0] == 1.0f) ||
        (matDensity[0][1] <= density && ox == 0 && oy == 1 && matMoveX[0][1] == 1.0f && matMoveY[0][1] == 0.0f) ||
        (matDensity[1][1] <= density && ox == 1 && oy == 1 && matMoveX[1][1] == 0.0f && matMoveY[1][1] == 0.0f) ||
        (matDensity[2][1] <= density && ox == 2 && oy == 1 && matMoveX[2][1] == -1.0f && matMoveY[2][1] == 0.0f) ||
        (matDensity[0][2] <= density && ox == 0 && oy == 2 && matMoveX[0][2] == 1.0f && matMoveY[0][2] == -1.0f) ||
        (matDensity[1][2] <= density && ox == 1 && oy == 2 && matMoveX[1][2] == 0.0f && matMoveY[1][2] == -1.0f) ||
        (matDensity[2][2] <= density && ox == 2 && oy == 2 && matMoveX[2][2] == -1.0f && matMoveY[2][2] == -1.0f))
        return move;*/
    
    return move;
}


__KERNEL__ void FallingSandOstJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    
    matrix matID;
    matID.m = readMatID(fragCoord,R,BUFFER_D_SAMPLER);
    matrix matDensity;
    matDensity.m = readMatDensity(fragCoord, matID,R,BUFFER_A_SAMPLER);
    //float[3][3] sand = {{to_float4(0.0f,to_float4(0.0f,to_float4(0.0},{to_float4(0.0f,to_float4(0.0f,to_float4(0.0},{to_float4(0.0f,to_float4_aw(0.0f,to_float4(0.0}};
    matrix matMoveX; matMoveX.m = to_mat3_f(0);
    matrix matMoveY; matMoveY.m = to_mat3_f(0);
    
    readMatMove(matMoveX.a, matMoveY.a, fragCoord,R,BUFFER_B_SAMPLER);
        
    float2 move = calcMovePaired(matID.a, matDensity.a, matMoveX.a, matMoveY.a, fragCoord);
        
    fragColor = to_float4(move.x, move.y, 0.0f, 0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Abstract 1' to iChannel3
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel2


//#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_C_SAMPLER iChannel1
//#define BUFFER_D_SAMPLER iChannel2

/*
#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END
*/
// State buffer. States are updated here.
/*
__DEVICE__ float rand(float2 co)
{
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}  
*/

__DEVICE__ float getSelectedElement(float2 R, __TEXTURE2D__ BUFFER_A_SAMPLER) {
    float index = SELECTED_ELEMENT;
    float2 uv = to_float2(mod_f(index/iResolution.x , 1.0f), _floor(index/iResolution.x)/iResolution.y) + 0.5f/iResolution;
    float id = _tex2DVecN(BUFFER_A_SAMPLER,uv.x,uv.y,15).x;
    return id;
}
/*
__DEVICE__ mat3 readMatID(float2 fragCoord) {
    mat3 matID = mat3(0.0f);
    
    for (int x = 0; x < 3; ++x) {
        matID[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,0-1))/iResolution).x;
        matID[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,1-1))/iResolution).x;
        matID[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+to_float2(x-1,2-1))/iResolution).x;
    }
    return matID;
}
*/

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R, int Material)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
        {
          float id = _floor(tex.x*12.0f);
          if (id-0.5f > NUM_TILE_IDS)
              id = 5.0f;
          
          else if (id == 5.0f)
              id = 0.0f;
          else if (id == 5.0f)
              id == 0.0f;
          Q = _mix(Q,to_float4_s((id+MulOff.y)*MulOff.x),Blend);
        } 

        if ((int)Modus&4)
          
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
        if ((int)Modus&8)
          
          Q = _mix(Q,to_float4((tex.x+tex.y+tex.z+MulOff.y)*MulOff.x, (tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          
        if ((int)Modus&16) 
          //Material
          Q = _mix(Q,to_float4_s(Material),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void FallingSandOstJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(PenSize, 0.0f, 20.0f, 8.0f);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  All, AllID, Material, Special);
    CONNECT_POINT1(Par1, 0.0f, 0.0f);
    CONNECT_INTSLIDER1(BlendMaterial, 0, 9, 1);
    
    
    fragCoord+=0.5f;
   
    float2 uv = fragCoord / iResolution;
    
    if (iFrame <= 10 || Reset) {
      float id = _floor(_tex2DVecN(iChannel3,uv.x,uv.y,15).x*12.0f);
        if (id-0.5f > NUM_TILE_IDS)
            id = 5.0f;
        
        else if (id == 5.0f)
            id = 0.0f;
        else if (id == 5.0f)
            id == 0.0f;
        
        fragColor = to_float4(id,0.0f,0.0f,0.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
 
    float2 move = swi2(_tex2DVecN(BUFFER_C_SAMPLER,uv.x,uv.y,15),x,y);
    float2 otherMove = swi2(texture(BUFFER_C_SAMPLER, uv+move/iResolution),x,y);
    if (otherMove.x != -move.x || otherMove.y != -move.y)
        move = to_float2_s(0.0f);
    
    float oldID = _tex2DVecN(BUFFER_D_SAMPLER,uv.x,uv.y,15).x;
    float id = texture(BUFFER_D_SAMPLER, uv+move/iResolution).x;
    
    if (length(fragCoord - swi2(iMouse,x,y)) < PenSize && iMouse.z >= 1.0f)
        id = getSelectedElement(R,BUFFER_A_SAMPLER);
    
    int hash = (int)(1000.0f*rand(to_float2(rand(fragCoord), iTime)));
    
    bool hasFire = false;
    bool hasPlant = false;
    bool hasAir = false;
    bool hasWater = false;
    
    matrix matID;
    matID.m = readMatID(fragCoord,R,BUFFER_D_SAMPLER);
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            float id = matID.a[x][y];
            hasFire = hasFire || id == 5.0f || id == 7.0f;
            hasAir = hasAir || id == 0.0f;
            hasWater = hasWater || id == 6.0f;
            hasPlant = hasPlant;
        }
    }
    
    if (id == 4.0f && hash < 3) // Steam > water
        id = 6.0f;
    if (id == 6.0f && oldID == 5.0f) // water + magma > steam
        id = 4.0f;
    if (id == 5.0f && oldID == 6.0f) // magma + water > stone
        id = 2.0f;
    if (id == 1.0f && oldID == 6.0f) // sand + water > mud
        id = 9.0f;
    if (id == 6.0f && oldID == 1.0f) // water + sand > air
        id = 0.0f;
    if (id == 9.0f && oldID == 0.0f) // Mud + air > sand
        id = 1.0f;
    if (id == 0.0f && oldID == 9.0f) // air + mud > water
        id = 6.0f;
    if (id == 7.0f && hash < 10) // fire > air
        id = 0.0f;
    
    // Oil + fire + air > fire
    if (id == 3.0f && hasFire && hasAir && hash < 200) 
        id = 7.0f;
    
    // Sand + fire > glass
    if (id == 1.0f && hasFire) 
        id = 8.0f;
    
    // Glass + water > sand
    if (id == 8.0f && hasWater)
        id = 1.0f;
    
    // water + fire > steam
    if (id == 6.0f && hasFire && hash < 20)
        id = 4.0f;
    
    // magma + water > stone
    if (id == 5.0f && hasWater)
        id = 2.0f;
    
    fragColor = to_float4(id, 0.0f, 0.0f, 0.0f);
    
    if (Blend1>0.0) fragColor = Blending(iChannel3, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R, BlendMaterial);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel3
// Connect Image 'Previsualization: Buffer C' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel2

/*
#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_B_SAMPLER iChannel1
#define BUFFER_D_SAMPLER iChannel2

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END
*/
// Output image
/*
__DEVICE__ float rand(float2 co)
{
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}  
*/
__DEVICE__ float getSandID(float2 uv, float2 R, __TEXTURE2D__ BUFFER_D_SAMPLER) {
    float value = _tex2DVecN(BUFFER_D_SAMPLER,uv.x,uv.y,15).x;
    return mod_f(_floor(value), NUM_TILE_IDS);
}

__DEVICE__ float4 getSandColor(float sandID, float2 R, __TEXTURE2D__ BUFFER_A_SAMPLER) {
    float pixelIndex = (float)(sandID+COLOR_BEGIN)/iResolution.x;
    float2 uv = to_float2(mod_f(pixelIndex, 1.0f), pixelIndex/iResolution.y) + to_float2_s(0.5f)/iResolution;
    float4 color = _tex2DVecN(BUFFER_A_SAMPLER,uv.x,uv.y,15);
    return color;
}

__KERNEL__ void FallingSandOstJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
    
  float id = getSandID(uv,R,BUFFER_D_SAMPLER);
  float4 color = getSandColor(id,R,BUFFER_A_SAMPLER); 
    
  fragColor = color;

  SetFragmentShaderComputedColor(fragColor);
}