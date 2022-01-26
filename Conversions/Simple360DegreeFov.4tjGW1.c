// ACHTUNG:
//
//    Den brauche ich nur zum Debuggen - muss nicht
//    auf OpneCL und/oder Cuda laufen. Sobald das ganze funktioniert
//    schmeiss ich's eh wieder weg :-)


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0


//Licence: Public domain. Attribution/credit is a nice gesture,
//but not required
//define if you want to visualize the cubemap faces
//#define SHOW_FACE_GROUPS
//define if you want to see the normal as colors
//#define SHOW_NORMALS


#define PI 3.1415926f //const float PI = 3.1415926f;

__DEVICE__ mat3 rotX(float theta){
    float s = _sinf(theta);
    float c = _cosf(theta);

  mat3 m =
        mat3( 1, 0,  0,
              0, c,  -s,
              0, s,  c);
    return m;
}

__DEVICE__ mat3 rotY(float theta){
    float s = _sinf(theta);
    float c = _cosf(theta);

  mat3 m =
        mat3( c, 0, -s,
              0, 1,  0,
              s, 0,  c);
    return m;
}

__DEVICE__ mat3 rotZ(float theta){
    float s = _sinf(theta);
    float c = _cosf(theta);

  mat3 m =
        mat3( c, -s, 0,
              s, c,  0,
              0, 0,  1);
    return m;
}

__DEVICE__ float deg2rad(float deg){
    return deg*PI / 180.0f;
}


__KERNEL__ void Simple360DegreeFovFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

//Try changing these for a different FOV.
//For example 180 deg horisontal and 90 degree vertical
//float FOVX = 360.0f; //Max 360 deg
//float FOVY = 180.0f; //Max 180 deg

  CONNECT_SLIDER0(POSX,-1.0f,1.0f,0.0f);
  CONNECT_SLIDER1(POSY,-1.0f,1.0f,0.0f);
  CONNECT_SLIDER2(FOVX,0.0f,360.0f,360.0f);
  CONNECT_SLIDER3(FOVY,0.0f,360.0f,360.0f);


float4 faceColors[6];

    faceColors[0] = to_float4(1.0f, 0.0f, 0.0f, 1.0f); //left
  faceColors[1] = to_float4(0.0f, 1.0f, 0.0f, 1.0f); //right
  faceColors[2] = to_float4(0.0f, 0.0f, 1.0f, 1.0f); //bottom
    faceColors[3] = to_float4(1.0f, 1.0f, 0.0f, 1.0f); //top
    faceColors[4] = to_float4(1.0f, 0.0f, 1.0f, 1.0f); //front
    faceColors[5] = to_float4(0.0f, 1.0f, 1.0f, 1.0f); //back

  float2 interp = fragCoord / iResolution;
    //Mouse coordinates in [-1, 1] range
    //float2 mp = swi2(iMouse,x,y) / iResolution * to_float2_s(2.0f) + to_float2_s(1.0f);
    float2 mp=to_float2(POSX,POSY);

    interp.y = 1.0f - interp.y;
    mp.y = 1.0f - mp.y;

    //360 degrees around the x-axis, 180 degrees on the y-axis
    //The frustum can be split into several parts:
    //The very top is +y, the north pole, and the very bottom
    //is -y, i.e the south pole.
    //The middle consists of +z, -z, +x and -x, where -z
    //is the center of the frustum, and the left and right
    //frustum edges show +z, i.e what is behind you

    float fovX = deg2rad(FOVX);
    float fovY = deg2rad(FOVY);
    float hOffset = (2.0f*PI - fovX)*0.5f;
    float vOffset = (PI - fovY)*0.5f;
    float hAngle = hOffset + interp.x * fovX;
    float vAngle = vOffset + interp.y * fovY;
    float3 n;
    n.x = _sinf(vAngle) * _sinf(hAngle);
    n.y = _cosf(vAngle);
    n.z = _sinf(vAngle) * _cosf(hAngle);

    //Normal pitch-yaw camera controlled with the mouse
    n = rotY(mp.x * 2.0f * PI) * rotX(mp.y * 2.0f * PI) * n;

    float4 color;

    #ifdef SHOW_NORMALS
    color.w = 1.0f;
    n = normalize(n);
    swi3(color,x,y,z) = ((n + to_float3_s(1.0f)) * to_float3_s(0.5f));
    #else
    color = decube_f3(iChannel0,n);
    #ifdef SHOW_FACE_GROUPS
      float ax = _fabs(n.x);
      float ay = _fabs(n.y);
      float az = _fabs(n.z);
      if(ax > ay && ax > az){
        //x-major
            color *= (n.x < 0.0f ? faceColors[0] : faceColors[1]);
        } else if(ay > ax && ay > az){
            //y-major
            color *= (n.y < 0.0f ? faceColors[2] : faceColors[3]);
        } else {
            //z-major
            color *= (n.z < 0.0f ? faceColors[4] : faceColors[5]);
        }
    #endif
    #endif
  fragColor = color;


  SetFragmentShaderComputedColor(fragColor);
}