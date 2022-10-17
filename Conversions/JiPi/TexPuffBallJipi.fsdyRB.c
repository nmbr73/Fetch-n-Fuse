
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rock Tiles' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define r1 1.0f
#define gr1 0.5f
#define c1 to_float3_s(0.0f)
#define r2 1.0f
#define c2 to_float3(0.0f, 4.0f, 0.0f)

__DEVICE__ float sdfSphere(float3 p, float3 c, float radius){
    return length(p - c) - radius;
}

__DEVICE__ float sdfSphere2(float3 p, float3 c, float radius){
    return length(p - c) - 0.0f;
}

__DEVICE__ float hash3(float2 xy){
    xy = mod_f2(xy, 0.19f);
    float h = dot(swi3(xy,y,y,x), to_float3(0.013f, 27.15f, 2027.3f));
    h *= h;
    h *= fract(h);
    
    return fract(h);
}


__DEVICE__ float3 quaternionVectorRotation(float3 v, float4 q){
    float3 rowOne = to_float3(1.0f - (2.0f * ((q.z * q.z) + (q.w * q.w))), 2.0f* ((q.y * q.z) - (q.x * q.y)), 2.0f * ((q.x * q.z) + (q.y * q.w)));
    float3 rowTwo = to_float3(2.0f * ((q.y * q.z) + (q.x * q.w)), 1.0f - (2.0f * ((q.y * q.y) + (q.w * q.w))), 2.0f * ((q.y * q.z) + (q.x * q.w)));
    float3 rowThree = to_float3(2.0f * ((q.y * q.w) - (q.x * q.z)), 2.0f * ((q.x * q.y) + (q.z * q.w)), 1.0f - (2.0f * ((q.y * q.y) + (q.z * q.z))));
float zzzzzzzzzzzzzzzz;    
    mat3 r = to_mat3_f3(rowOne, rowTwo, rowThree);
    return mul_mat3_f3(r , v);
}

__DEVICE__ float color(float x, float a, float b){
    return ((b * x) * _expf(a * (x - 1.0f)));
}

__DEVICE__ float color2(float x, float a, float x0){
    return 1.0f / (_expf(_fabs(a * (x - x0))));
}



__DEVICE__ float4 quaternionMult(float4 a, float4 b){
  
    float3 tmp =  a.x*swi3(b,y,z,w) + b.x*swi3(a,y,z,w) + cross(swi3(a,y,z,w), swi3(b,y,z,w));
    return to_float4(a.x * b.x - dot(swi3(a,y,z,w), swi3(b,y,z,w)),tmp.x,tmp.y,tmp.z);
}


__DEVICE__ float3 generateReflectionVector(float3 normal, float3 inV){
    return inV - (2.0f * normal);
}


__DEVICE__ float acos2(float X, float Y){
    
    return _acosf(X / length(to_float2(X, Y))) * sign_f(Y);
}

__DEVICE__ float atan2(float x, float y){
    return _atan2f(y / x,1.0f) + (((1.0f - sign_f(x)) / 2.0f) * 3.14f);
}



__KERNEL__ void TexPuffBallJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_SLIDER0(fov, -1.0f, 500.0f, 110.0f);
    CONNECT_SLIDER1(XYMul, -1.0f, 5.0f, 1.0f);
    CONNECT_SLIDER2(Vertical, -1.0f, 2.0f, 1.0f);
    CONNECT_SLIDER3(Horizontal, -1.0f, 2.0f, 1.0f);
    
    CONNECT_POINT0(XYOffset, 0.0f, 0.0f);

    float2 uv = fragCoord/iResolution;
    
    //get the mouse location for look direction unless mouse is not down
    float2 muv = iMouse.z > 0.0f ? swi2(iMouse,x,y) / iResolution : to_float2(0.5f, 0.5f);
            
    float4 col = to_float4_s(0.0f);
    float screenRatio = iResolution.y / iResolution.x;
    
    //Setting up the ray directions and other information about the point and camera
    //##############################################################################
    
    //camera direction angles phi (xy plane) and theta (xz plane)
    //float phi = radians(180.0f);
    //float theta = radians(105.0f);
    float phi   = -radians(360.0f * (1.0f - muv.x) * Horizontal);
    float theta = -radians(180.0f * (1.0f - muv.y) * Vertical);

    //get the camera direction as the basis for the rotation (each ray direction is a rotation of the camera direciton vector)
    //it is in quarternion form here so its a float4 instead of a float3
    float4 camD = to_float4(0.0f, _cosf(phi) * _sinf(theta), _sinf(phi) * _sinf(theta), _cosf(theta));
    
    
    float rad90 = radians(90.0f);
    
    //float fov = 110.0f;
    
    float xAng = radians(fov * (0.5f - uv.x));
    //replace "fov" with "(fov + (110.0f * _powf(0.5f - uv.x, 2.0f)))" below to add a counteractment to the fisheye lens effect
    //it basically counteracts the artifact with quaternions that happens when you rotate by a large angle on one axis then try to rotate on another axis perpendicular, it just rotates around it thus making the new direction lesser
    float yAng = radians(fov * screenRatio * (0.5f - uv.y));
    
    //get the axes that the quarternions should be based around (perpendicular to the camera plane or dv)
    float3 xRotAxis = to_float3(_cosf(phi) * _sinf(theta - rad90), _sinf(phi) * _sinf(theta - rad90), _cosf(theta - rad90));
    float3 yRotAxis = cross(xRotAxis, swi3(camD,y,z,w));//to_float3_aw(_cosf(phi - rad90) * _sinf(theta), _sinf(phi - rad90) * _sinf(theta), _cosf(theta));
    
    //get the quarternions of the ray direction rotations
    float IIIIIIIIIIIIIIII;
    float4 xQuat = to_float4(_cosf(xAng / 2.0f), xRotAxis.x * _sinf(xAng / 2.0f), xRotAxis.y * _sinf(xAng / 2.0f), xRotAxis.z * _sinf(xAng / 2.0f));
    float4 yQuat = to_float4(_cosf(yAng / 2.0f), yRotAxis.x * _sinf(yAng / 2.0f), yRotAxis.y * _sinf(yAng / 2.0f), yRotAxis.z * _sinf(yAng / 2.0f));
        
    //combine the rotations
    float4 compQuat = quaternionMult(yQuat, xQuat);
    
        //get the conjugate of the compQuart
    float4 conjComp = to_float4(compQuat.x,-compQuat.y,-compQuat.z,-compQuat.w);
    
    //ray direction
    float3 rayD = swi3(quaternionMult(quaternionMult(compQuat, camD), conjComp),y,z,w);
        
    //############################################
    float t = 0.0f;
    float d = 100.0f;
    float3 p = to_float3_s(0.0f);
    //float m = 0.0f;
    int steps = 300;
    
    float3 rayO = -1.0f*swi3(camD,y,z,w) * 2.0f;
    float t0 = distance_f3(rayO, c1) - r1;
    float minStep = (t0 + r1 + r1) / (float)(steps);
    
    float3 n = normalize(c1-rayO);
    float3 sunD = normalize(to_float3(_cosf(iTime + 1.7f), _sinf(iTime + 1.7f), 0.0f));
    
    float cValue, cv2, surface;
    float3 camN = normalize(rayO - c1);
    
    //trace 1
    for (int i = 0; i <= steps; i++){
        p = rayO + (rayD * (t));
        
        float3 surfHit = normalize(p - c1);
        float l = length(surfHit);
        
        float theta = acos2(surfHit.z, l);
        float2 angs = to_float2(atan2(surfHit.x, surfHit.y) + iTime, theta);
        
        angs.x *= screenRatio;
        //angs.y *= ray_mul;
        
        angs+=XYOffset;
        angs*=XYMul;
        
        float4 tex = texture(iChannel0, angs / 3.14f);

        d = distance_f3(p, c1);
        
        if(d - gr1 < length(tex) * 0.1f){
            col = tex;
            break;
        }
         
        t += minStep;
    }
    
    float shadow = dot(n, sunD);
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}