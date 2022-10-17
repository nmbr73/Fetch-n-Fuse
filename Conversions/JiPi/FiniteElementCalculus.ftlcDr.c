
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/c3a071ecf273428bc72fc72b2dd972671de8da420a2d4f917b75d20e1c24b34c.ogv' to iChannel0
// Connect Image 'Texture: Nyancat' to iChannel2
// Connect Image '/presets/webcam.png' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


#define dx     1.0f 
#define dy     1.0f 
#define dz     1.0f 
#define PosX   to_float2( 1.0f,  0.0f) //  1,  0,  0
#define NegX   to_float2(-1.0f,  0.0f) // -1,  0,  0
#define PosY   to_float2( 0.0f,  1.0f) //  0,  1,  0
#define NegY   to_float2( 0.0f, -1.0f) //  0, -1,  0
#define PosZ   to_float2( 0.0f,  0.0f) //  0,  0,  1
#define NegZ   to_float2( 0.0f,  0.0f) //  0,  0, -1
#define Zero   to_float2( 0.0f,  0.0f) //  0,  0,  0



__DEVICE__ float3 v(float2 ij, float2 offset, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    
    // Pick a combo of the following.
    const bool USE_WEBCAM   = false;
    const bool USE_NYANCAT  = true;
    const bool USE_VIDEO    = true;
    const bool USE_BUFFER_A = false;
    
    
    float2 uv = (ij + offset) / iResolution;
    float3 col = to_float3_s(0.0f);
    float count = 0.0f;

    if (USE_VIDEO) {
        col += swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
        count++;
    }
        
    if (USE_WEBCAM) {
        col += swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
        count++;
    }
        
    if (USE_NYANCAT) {
        float2 uv2 = to_float2(-0.15f + uv.x + 0.4f * _sinf(iTime), uv.y + 0.1f * _cosf(iTime));
        // Add the nyan cat sprite: https://www.shadertoy.com/view/lsX3Rr
        float2 uvNyan = (uv2  - to_float2(0.25f, 0.15f)) / (to_float2(0.7f,0.5f) - to_float2(0.5f, 0.15f));
        uvNyan = clamp(uvNyan, 0.0f, 1.0f);
        float ofx = _floor(mod_f(iTime*15.0f, 6.0f));
        float ww = 40.0f/256.0f;
        uvNyan = to_float2(clamp(uvNyan.x*ww + ofx*ww, 0.0f, 1.0f ), uvNyan.y);
        float4 texel = _tex2DVecN(iChannel2,uvNyan.x,uvNyan.y,15);
        if (texel.w > 0.5f) {
            col = swi3(texel,x,y,z);
            count = 1.0f;
        }
    }
    
    if (USE_BUFFER_A) {
        col += swi3(_tex2DVecN(iChannel3,uv.x,uv.y,15),x,y,z);
        count++;
    }

    return col/count;
}

__DEVICE__ float f(float2 ij, float2 offset, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    float3 col = v(ij, offset,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime);
    return (col.x + col.y + col.z) / 3.0f;
}

__DEVICE__ float3 gradient(float2 ij, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    float dfdx = 0.5f / dx * (f(ij, PosX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    float dfdy = 0.5f / dy * (f(ij, PosY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    float dfdz = 0.5f / dz * (f(ij, PosZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    return to_float3(dfdx, dfdy, dfdz);
}

__DEVICE__ float divergence(float2 ij, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    float dfdx = 0.5f / dx * (f(ij, PosX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    float dfdy = 0.5f / dy * (f(ij, PosY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    float dfdz = 0.5f / dz * (f(ij, PosZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) - f(ij, NegZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    return dfdx + dfdy + dfdz;
}

__DEVICE__ float3 curl(float2 ij, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    float dvzdy = 0.5f / dy * (v(ij, PosY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).z - v(ij, NegY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).z);
    float dvydz = 0.5f / dz * (v(ij, PosZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).y - v(ij, NegZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).y);
    float dvxdz = 0.5f / dz * (v(ij, PosZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).x - v(ij, NegZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).x);
    float dvzdx = 0.5f / dx * (v(ij, PosX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).z - v(ij, NegX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).z);
    float dvydx = 0.5f / dy * (v(ij, PosX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).y - v(ij, NegX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).y);
    float dvxdy = 0.5f / dx * (v(ij, PosY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).x - v(ij, NegY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime).x);
    return to_float3(dvzdy - dvydz, dvxdz - dvzdx, dvydx - dvxdy);
}

__DEVICE__ float laplacian(float2 ij, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float iTime) {
    return ( f(ij, PosX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) + f(ij, NegX,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime)
           + f(ij, PosY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) + f(ij, NegY,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime)
           + f(ij, PosZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime) + f(ij, NegZ,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime)
           - 6.0f * f(ij, Zero,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime)
           ) / (dx * dy);
}


__KERNEL__ void FiniteElementCalculusFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
        //CONNECT_CHECKBOX0(MouseChoice, 1); 
        CONNECT_INTSLIDER0(Choice, 0, 9, 0);


        const int ALL_FIELDS = 0;
        const int VECTOR_FIELD = 1;                   // Original RGB.
        const int SCALAR_FIELD = 2;                   // Average of RGB.
        const int GRADIENT_OF_SCALAR_FIELD = 3;       // Produces a vector field.
        const int DIVERGENCE_OF_VECTOR_FIELD = 4;     // Produces a scalar field.
        const int LAPLACIAN_OF_SCALAR_FIELD = 5;      // Produces a scalar field.
        const int LAPLACIAN_OF_SCALAR_FIELD_SIGN = 6; // Colors the Laplacian.
        const int CURL_OF_VECTOR_FIELD = 7;           // Produces a vector field.
        const int CURL_OF_VECTOR_FIELD_LENGTH = 8;    // Produces a scalar field.
        const int CURL_OF_VECTOR_FIELD_VORTICITY = 9; // Colors the Curl.
        const float NUM_CHOICES = 10.0f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    int choice = int(iMouse.x * NUM_CHOICES / iResolution.x);

    if (iMouse.z == 0)
      choice = Choice;

    if (choice == 0)
        choice = (int)(fragCoord.x * (NUM_CHOICES-1.0f) / iResolution.x) + 1;
    
    float intensity = 1.0f;
    float3 col;
    if (choice == VECTOR_FIELD) {
        col = v(fragCoord, to_float2(0, 0),R,iChannel0,iChannel1,iChannel2,iChannel3,iTime);
    } else if (choice == SCALAR_FIELD) {
        col = to_float3_s(f(fragCoord, to_float2(0, 0),R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
    } else if (choice == GRADIENT_OF_SCALAR_FIELD) {
        float3 G = gradient(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime);        
        col = G;
        // Boost the intensity.
        intensity = 6.0f;
    } else if (choice == DIVERGENCE_OF_VECTOR_FIELD) {
        col = to_float3_s(divergence(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
        // Boost the intensity
        intensity = 6.0f;
    } else if (choice == LAPLACIAN_OF_SCALAR_FIELD) {
        col = to_float3_s(laplacian(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
        
        // Boost the intensity
        intensity = 12.0f;
    } else if (choice == LAPLACIAN_OF_SCALAR_FIELD_SIGN) {
        float L = laplacian(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime);
        
        // Differentiate between negative, positive, and zero
        // Red is less than zero, Green is greater than zero, Blue is none.
        if (L < 0.0f)
            col = to_float3(-L, 0.0f, 0.0f);
        else if (L > 0.0f)
            col = to_float3(0.0f, L, 0.0f);
        else
            col = to_float3(0.0f, 0.0f, 0.0f);
        
        // Boost the intensity
        intensity = 12.0f;
    } else if (choice == CURL_OF_VECTOR_FIELD) {
        col = curl(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime);

        // Boost the intensity
        intensity = 12.0f;
    } else if (choice == CURL_OF_VECTOR_FIELD_LENGTH) {
        float C = length(curl(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime));
        col = to_float3_s(C);
        
        // Don't boost the intensity.
        intensity = 1.0f;
    } else if (choice == CURL_OF_VECTOR_FIELD_VORTICITY) {
        float C = dot(curl(fragCoord,R,iChannel0,iChannel1,iChannel2,iChannel3,iTime), to_float3(0.0f, 0.0f, 1.0f));
        
        // This is vorticity because we are using a 2D field.
        
        // Differentiate between negative, positive, and zero
        // Red is less than zero, Green is greater than zero, Blue is none.
        if (C < 0.0f)
            col = to_float3(-C, 0.0f, 0.0f);
        else if (C > 0.0f)
            col = to_float3(0.0f, C, 0.0f);
        else
            col = to_float3(0.0f, 0.0f, 0.0f);
        
        // Boost the intensity
        intensity = 6.0f;
    }
    
    if (intensity != 1.0f)
        col = intensity * col / (1.0f + intensity * col);

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}