
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Cubemap: Uffizi Gallery' to iChannel0
// Connect 'Cubemap: Uffizi Gallery Blurred' to iChannel1


// Reprojection II - @P_Malin

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.






//#define CubeTexture_RGB(M,X,Y,Z)  _tex2DVecN(M,X,Y,Z).rgb
//#define CubeTexture_R(M,X,Y,Z)    _tex2DVecN(M,X,Y,Z).r

// https://learnopengl.com/Getting-started/Textures
// https://learnopengl.com/Advanced-OpenGL/Cubemaps
// https://cgvr.cs.uni-bremen.de/teaching/cg_literatur/Cube_map_tutorial/cube_map.html
// https://www.khronos.org/opengl/wiki/Texture

// https://www.khronos.org/opengl/wiki/Cubemap_Texture
// https://www.shadertoy.com/view/tdjXDt <--
// https://www.shadertoy.com/view/tlyXzG <--
// https://www.shadertoy.com/view/WsGcRm
// https://www.shadertoy.com/view/wtV3W1
// https://www.shadertoy.com/view/WtlSD4
// https://www.shadertoy.com/view/3l2SDR

// Let's say that +Z is forward, and +Y is up, so +X to the right. Given that orientation, the texture coordinates of the 6 faces of the cube look like this:
//
// +Z face (directly in front): The U coordinate goes to the right, with the V coordinate going down.
// +X face (to our right): The U coordinate is going behind the viewer, with the V coordinate going down.
// -Z face (directly behind): The U coordinate goes to the left (relative to us facing forwards), with the V coordinate going down.
// -X face (to our left): The U coordinate is going forward, with the V coordinate going down.
// +Y face (above): The U coordinate goes to the right, with the V coordinate going forward.
// -Y face (bottom): The U coordinate goes to the right, with the V coordinate going backward.


__DEVICE__ float4 cube_texture_pixel(__TEXTURE2D__ crossmap, float ux, float uy, float uz)
{

  float4 rv=to_float4(0.0f,1.0f,0.0f,1.0f);

  float x;
  float y;

//  if (ux==-1.0f) { // -X, Face 1
  if (ux<0.0f) { // -X, Face 1

    x = (uz+1.0f)/8.0f;
    y = (uy-1.0f)/6.0f+2.0f/3.0f;

  return to_float4(1.0f,1.0f,0.0f,1.0f);

//  } else if (uz==1.0f) { // +Z, Face 4
  } else if (uz>=0.0f) { // +Z, Face 4

    x = (ux+1.0f)/8.0f+0.25;
    y = (uy-1.0f)/6.0f+2.0f/3.0f;

  return to_float4(0.0f,1.0f,1.0f,1.0f);

//  } else if (ux==1.0f) { // +X, Face 0
  } else if (ux>=0.0f) { // +X, Face 0

    // Spiegelverkehrt!!!
    x = -(((uz+1.0f)/2.0f+2.0f)/4.0f);
    y = (uy-1.0f)/6.0f+1.0f/3.0f + 1.0f/3.0f;

  return to_float4(1.0f,0.0f,1.0f,1.0f);

//  } else if (uz==-1.0f) { // -Z, Face 5
  } else if (uz<0.0f) { // -Z, Face 5

    // Spiegelverkehrt!!!
    x = -(((ux+1.0f)/2.0f+3.0f)/4.0f);
    y = (uy-1.0f)/6.0f+1.0f/3.0f + 1.0f/3.0f;

//  } else if (uy==-1.0f) { // -Y, Face 3
  } else if (uy<0.0f) { // -Y, Face 3

    x = (ux+1.0f)/8.0f+0.25f;
    y = (uz+1.0f)/6.0f;

//  } else if (uy==1.0f) { // +Y, Face 2
  } else if (uy>=0.0f) { // +Y, Face 2

    x = (ux+1.0f)/8.0f+0.25f;
    y = -((uz+1.0f)/6.0f+1.0f);

  } else
  {
    return rv;
  }

  // Wir kommen nie hier rein?!?
  // ... ich glaube, es ist zu spaet :-/
  return to_float4(1.0f,0.0f,0.0f,1.0f);

  rv = _tex2DVecN(crossmap,x,y,15);

  return rv;
}


__DEVICE__ float3 CubeTexture_RGB(__TEXTURE2D__ cross, float x, float y, float z)
{
  float4 pixel=cube_texture_pixel(cross,x,y,z);
  float3 rgb=to_float3(pixel.x,pixel.y,pixel.z);
  return rgb;
}

__DEVICE__ float CubeTexture_R(__TEXTURE2D__ cross, float x, float y, float z)
{
  float4 pixel=cube_texture_pixel(cross,x,y,z);
  return pixel.x;
}








#define FORCE_SHADOW
#define ENABLE_REFLECTION

struct C_Ray
{
    float3 vOrigin;
    float3 vDir;
};

#define kMaxDist 1000.0
#define kEpsilon 0.0001

__DEVICE__ void TraceSlab(const in C_Ray ray, const in float3 vMin, const in float3 vMax, const in float3 vNormal, inout float fNear, inout float fFar)
{
  float3 vMinOffset = vMin - ray.vOrigin;
  float3 vMaxOffset = vMax - ray.vOrigin;

  // Project offset and dir
  float fMinOffset = dot(vMinOffset, vNormal);
  float fMaxOffset = dot(vMaxOffset, vNormal);

  float fDir = dot(ray.vDir, vNormal);

  if(_fabs(fDir) < kEpsilon)
  {
    // ray parallel to slab

    //if origin is not between slabs return false;
    if((fMinOffset > 0.0f) || (fMaxOffset < 0.0f))
    {
      fNear = kMaxDist;
      fFar = -kMaxDist;
    }

    // else this slab does not influence the result
  }
  else
  {
    // ray is not parallel to slab, calculate intersections

    float t0 = (fMinOffset) / fDir;
    float t1 = (fMaxOffset) / fDir;

    float fIntersectNear = _fminf(t0, t1);
    float fIntersectFar = _fmaxf(t0, t1);

    fNear = _fmaxf(fNear, fIntersectNear); // track largest near
    fFar = _fminf(fFar, fIntersectFar); // track smallest far
  }
}

__DEVICE__ float TraceBox( const in C_Ray ray, const in float3 vCorner1, const in float3 vCorner2 )
{
  float3 vMin = _fminf(vCorner1, vCorner2);
  float3 vMax = _fmaxf(vCorner1, vCorner2);

  float fNear = -kMaxDist;
  float fFar = kMaxDist;

  TraceSlab(ray, vMin, vMax, to_float3(1.0f, 0.0f, 0.0f), fNear, fFar);
  TraceSlab(ray, vMin, vMax, to_float3(0.0f, 1.0f, 0.0f), fNear, fFar);
  TraceSlab(ray, vMin, vMax, to_float3(0.0f, 0.0f, 1.0f), fNear, fFar);

  if(fNear > fFar)
  {
    return kMaxDist;
  }

  if(fFar < 0.0f)
  {
    return kMaxDist;
  }

  return fNear;
}

__DEVICE__ float3 Project( float3 a, float3 b )
{
  return a - b * dot(a, b);
}

__DEVICE__ float TraceCylinder( const in C_Ray ray, float3 vPos, float3 vDir, float fRadius, float fLength )
{
  float3 vOffset = vPos - ray.vOrigin;

  float3 vProjOffset = Project(vOffset, vDir);
  float3 vProjDir = Project(ray.vDir, vDir);
  float fProjScale = length(vProjDir);
  vProjDir /= fProjScale;

  // intersect circle in projected space

  float fTClosest = dot(vProjOffset, vProjDir);

  float3 vClosest = vProjDir * fTClosest;
  float fDistClosest = length(vClosest - vProjOffset);
  if(fDistClosest > fRadius)
  {
    return kMaxDist;
  }
  float fHalfChordLength = _sqrtf(fRadius * fRadius - fDistClosest * fDistClosest);
  float fTIntersectMin = (fTClosest - fHalfChordLength) / fProjScale;
  float fTIntersectMax = (fTClosest + fHalfChordLength) / fProjScale;

  // cap cylinder ends
  TraceSlab(ray, vPos, vPos + vDir * fLength, vDir, fTIntersectMin, fTIntersectMax);

  if(fTIntersectMin > fTIntersectMax)
  {
    return kMaxDist;
  }

  if(fTIntersectMin < 0.0f)
  {
    return kMaxDist;
  }

  return fTIntersectMin;
}


__DEVICE__ float TraceFloor( const in C_Ray ray, const in float fHeight )
{
  if(ray.vOrigin.y < fHeight)
  {
    return 0.0f;
  }

  if(ray.vDir.y > 0.0f)
  {
    return kMaxDist;
  }

  float t = (fHeight - ray.vOrigin.y) / ray.vDir.y;

  return _fmaxf(t, 0.0f);
}

__DEVICE__ float TracePillar( const in C_Ray ray, in float3 vPos )
{
  vPos.y = -1.0f;
  float fRadius = 0.3f;
  float fDistance = TraceCylinder( ray, vPos, to_float3(0.0f, 1.0f, 0.0f), fRadius, 10.0f);
  float fBaseSize = 0.4f;

  float3 vBaseMin = to_float3(-fBaseSize, 0.0f, -fBaseSize);
  float3 vBaseMax = to_float3(fBaseSize, 0.8f, fBaseSize);
  fDistance = _fminf( fDistance, TraceBox( ray, vPos + vBaseMin, vPos + vBaseMax) );

  float fTopSize = 0.4f;
  float3 vTopMin = to_float3(-fTopSize, 5.6f, -fTopSize);
  float3 vTopMax = to_float3(fTopSize, 7.0f, fTopSize);
  fDistance = _fminf( fDistance, TraceBox( ray, vPos + vTopMin, vPos + vTopMax) );

  return fDistance;
}

__DEVICE__ float TraceColumn( const in C_Ray ray, float3 vPos )
{
  float3 vColumnMin = to_float3(-0.84f, -10.0f, -0.4f);
  float3 vColumnMax = - vColumnMin;
  return TraceBox( ray, vPos + vColumnMin, vPos + vColumnMax );
}


__DEVICE__ float TraceBuildingSide( const in C_Ray ray )
{
  const float fBuildingMin = -90.0f;
  const float fBuildingMax = 50.0f;


  float fDistance = kMaxDist;

  float fStepHeight = 0.14f;
  float fStepDepth = 0.2f;
  float fStepStart = 7.5f;
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, -1.5f + fStepHeight * 0.0f, fStepStart + fStepDepth * 0.0f), to_float3(fBuildingMax, -1.5f + fStepHeight * 1.0f, fStepStart + 20.0f) ));
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, -1.5f + fStepHeight * 1.0f, fStepStart + fStepDepth * 1.0f), to_float3(fBuildingMax, -1.5f + fStepHeight * 2.0f, fStepStart + 20.0f) ));
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, -1.5f + fStepHeight * 2.0f, fStepStart + fStepDepth * 2.0f), to_float3(fBuildingMax, -1.5f + fStepHeight * 3.0f, fStepStart + 20.0f) ));
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, -1.5f + fStepHeight * 3.0f, fStepStart + fStepDepth * 3.0f), to_float3(fBuildingMax, -1.5f + fStepHeight * 4.0f, fStepStart + 20.0f) ));

  float x = -2.0f;
  for(int i=0; i<5; i++)
  {
    float3 vBase = to_float3(x * 11.6f, 0.0f, 0.0f);
    x += 1.0f;

    fDistance = _fminf(fDistance, TraceColumn(ray, vBase + to_float3(0.0f, 0.0f, 8.5f)));


    fDistance = _fminf(fDistance, TracePillar(ray, vBase + to_float3(-4.1f, 0.0f, 8.5f)));
    fDistance = _fminf(fDistance, TracePillar(ray, vBase + to_float3(4.0f, 0.0f, 8.5f)));
  }


  float fBackWallDist = 9.5f;
  float fBuildingHeight = 100.0f;
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, -3.0f, fBackWallDist), to_float3(fBuildingMax, fBuildingHeight, fBackWallDist + 10.0f) ));

  float fBuildingTopDist = 8.1f;
  float fCeilingHeight = 4.7f;
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, fCeilingHeight, fBuildingTopDist), to_float3(fBuildingMax, fBuildingHeight, fBuildingTopDist + 10.0f) ));

  float fRoofDistance = 6.0f;
  float fRoofHeight = 21.0f;
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, fRoofHeight, fRoofDistance), to_float3(fBuildingMax, fRoofHeight + 0.2f, fRoofDistance + 10.0f) ));

  return fDistance;
}

__DEVICE__ float TraceScene( const in C_Ray ray )
{
  const float fBuildingMin = -90.0f;
  const float fBuildingMax = 50.0f;


  float fDistance = kMaxDist;

  float fFloorHeight = -1.5f;
  fDistance = _fminf(fDistance, TraceFloor( ray, fFloorHeight ));

  // end of row
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMax, fFloorHeight, -100.0f), to_float3(fBuildingMax+1.0f, 100.0f, 100.0f) ));
  fDistance = _fminf(fDistance, TraceBox( ray, to_float3(fBuildingMin, fFloorHeight, -100.0f), to_float3(fBuildingMin-1.0f, 100.0f, 100.0f) ));

  fDistance = _fminf(fDistance, TraceBuildingSide( ray ));

  C_Ray ray2;
  ray2.vOrigin = ray.vOrigin * to_float3(1.0f, 1.0f, -1.0f);
  ray2.vDir = ray.vDir * to_float3(1.0f, 1.0f, -1.0f);
  ray2.vOrigin.z -= 0.3f;
  fDistance = _fminf(fDistance, TraceBuildingSide( ray2 ));

  return fDistance;
}


__DEVICE__ void GetCameraRay( const in float3 vPos, const in float3 vForwards, const in float3 vWorldUp, const in float2 px, out C_Ray* ray, float2 iResolution)
{
    float2 vUV = ( px / iResolution );
    float2 vViewCoord = vUV * 2.0f - 1.0f;

  vViewCoord.x *= iResolution.x / iResolution.y;
  vViewCoord.y *= -1.0f;

    ray->vOrigin = vPos;

    float3 vRight = normalize(cross(vWorldUp, vForwards));
    float3 vUp = cross(vRight, vForwards);

  vViewCoord *= 0.5f;

    ray->vDir = normalize( vRight * vViewCoord.x + vUp * vViewCoord.y + vForwards);
}

__DEVICE__ void GetCameraRayLookat( const in float3 vPos, const in float3 vCameraTarget, const in float2 px, out C_Ray* ray, float2 iResolution)
{
  float3 vForwards = normalize(vCameraTarget - vPos);
  float3 vUp = to_float3(0.0f, 1.0f, 0.0f);

  GetCameraRay(vPos, vForwards, vUp, px, ray,iResolution);
}

__DEVICE__ void GetCameraPosAndTarget( float fCameraIndex, out float3* vCameraPos, out float3* vCameraTarget )
{
  float fCameraCount = 6.0f;
  float fCameraIndexModCount = mod_f(fCameraIndex, fCameraCount);

  if(fCameraIndexModCount < 0.5f)
  {
    *vCameraPos = to_float3(0.0f, 0.0f, 0.0f);
    *vCameraTarget = to_float3(0.0f, 0.0f, 8.0f);
  }
  else if(fCameraIndexModCount < 1.5f)
  {
    *vCameraPos = to_float3(-3.0f, 0.0f, -5.0f);
    *vCameraTarget = to_float3(3.0f, -5.0f, 5.0f);
  }
  else if(fCameraIndexModCount < 2.5f)
  {
    *vCameraPos = to_float3(8.0f, 0.0f, 0.0f);
    *vCameraTarget = to_float3(-10.0f, 0.0f, 0.0f);
  }
  else if(fCameraIndexModCount < 3.5f)
  {
    *vCameraPos = to_float3(8.0f, 3.0f, -3.0f);
    *vCameraTarget = to_float3(-4.0f, -2.0f, 0.0f);
  }
  else if(fCameraIndexModCount < 4.5f)
  {
    *vCameraPos = to_float3(8.0f, 5.0f, 5.0f);
    *vCameraTarget = to_float3(-4.0f, 2.0f, -5.0f);
  }
  else
  {
    *vCameraPos = to_float3(-10.0f, 3.0f, 0.0f);
    *vCameraTarget = to_float3(4.0f, 4.5f, -5.0f);
  }
}

__DEVICE__ float3 BSpline( const in float3 a, const in float3 b, const in float3 c, const in float3 d, const in float t)
{
  // const mat4 mSplineBasis = to_mat4( -1.0f,  3.0f, -3.0f, 1.0f,
  //                      3.0f, -6.0f,  0.0f, 4.0f,
  //                     -3.0f,  3.0f,  3.0f, 1.0f,
  //                      1.0f,  0.0f,  0.0f, 0.0f) / 6.0f;

  const mat4 mSplineBasis = to_mat4( -1.0f/6.0f,  3.0f/6.0f, -3.0f/6.0f, 1.0f/6.0f,
                       3.0f/6.0f, -6.0f/6.0f,  0.0f/6.0f, 4.0f/6.0f,
                      -3.0f/6.0f,  3.0f/6.0f,  3.0f/6.0f, 1.0f/6.0f,
                       1.0f/6.0f,  0.0f/6.0f,  0.0f/6.0f, 0.0f/6.0f);

  float t2 = t * t;
  float4 T = to_float4(t2 * t, t2, t, 1.0f);

  float4 vCoeffsX = to_float4(a.x, b.x, c.x, d.x);
  float4 vCoeffsY = to_float4(a.y, b.y, c.y, d.y);
  float4 vCoeffsZ = to_float4(a.z, b.z, c.z, d.z);

  float4 vWeights = mul_f4_mat4(T , mSplineBasis);

  float3 vResult;

  vResult.x = dot(vWeights, vCoeffsX);
  vResult.y = dot(vWeights, vCoeffsY);
  vResult.z = dot(vWeights, vCoeffsZ);

  return vResult;
}

__DEVICE__ void GetCamera(out float3 *vCameraPos, out float3* vCameraTarget, float iTime)
{
  float fCameraGlobalTime = iTime * 0.5f;
  float fCameraTime = fract(fCameraGlobalTime);
  float fCameraIndex = _floor(fCameraGlobalTime);
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;
  float3 vCameraPosA;
  float3 vCameraTargetA;
  GetCameraPosAndTarget(fCameraIndex, &vCameraPosA, &vCameraTargetA);

  float3 vCameraPosB;
  float3 vCameraTargetB;
  GetCameraPosAndTarget(fCameraIndex + 1.0f, &vCameraPosB, &vCameraTargetB);

  float3 vCameraPosC;
  float3 vCameraTargetC;
  GetCameraPosAndTarget(fCameraIndex + 2.0f, &vCameraPosC, &vCameraTargetC);

  float3 vCameraPosD;
  float3 vCameraTargetD;
  GetCameraPosAndTarget(fCameraIndex + 3.0f, &vCameraPosD, &vCameraTargetD);

  *vCameraPos = BSpline(vCameraPosA, vCameraPosB, vCameraPosC, vCameraPosD, fCameraTime);
  *vCameraTarget = BSpline(vCameraTargetA, vCameraTargetB, vCameraTargetC, vCameraTargetD, fCameraTime);
}

__DEVICE__ float3 SceneColor( C_Ray ray, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float iMouse_z  )
{
  float fHitDist = TraceScene(ray);
  float3 vHitPos = ray.vOrigin + ray.vDir * fHitDist;

  //float3 vResult = texture(iChannel0, swi3(vHitPos,x,y,z)).rgb;
  float3 vResult = CubeTexture_RGB(iChannel0, vHitPos.x, vHitPos.y, vHitPos.z);
  vResult = vResult * vResult;

  #ifdef FORCE_SHADOW
  if( _fabs(vHitPos.z) > 9.48f)
  {
    if( _fabs(vHitPos.x) < 20.0f)
    {
      float fIntensity = length(vResult);

      fIntensity = _fminf(fIntensity, 0.05f);

      vResult = normalize(vResult) * fIntensity;
    }
  }
  #endif

  #ifdef ENABLE_REFLECTION
  if(vHitPos.y < -1.4f)
  {
    float fDelta = -0.1f;
    //float vSampleDx = texture(iChannel0, swi3(vHitPos,x,y,z) + to_float3(fDelta, 0.0f, 0.0f)).r;
    float vSampleDx = CubeTexture_R(iChannel0, vHitPos.x+fDelta, vHitPos.y, vHitPos.z );
    vSampleDx = vSampleDx * vSampleDx;

    //float vSampleDy = texture(iChannel0, swi3(vHitPos,x,y,z) + to_float3(0.0f, 0.0f, fDelta)).r;
    float vSampleDy = CubeTexture_R(iChannel0, vHitPos.x, vHitPos.y, vHitPos.z+fDelta);
    vSampleDy = vSampleDy * vSampleDy;

    float3 vNormal = to_float3(vResult.x - vSampleDx, 2.0f, vResult.x - vSampleDy);
    vNormal = normalize(vNormal);

    float3 vReflect = reflect(ray.vDir, vNormal);

    float fDot = clamp(dot(-ray.vDir, vNormal), 0.0f, 1.0f);

    float r0 = 0.1f;
    float fSchlick =r0 + (1.0f - r0) * (_powf(1.0f - fDot, 5.0f));

    float3 vResult2 = CubeTexture_RGB(iChannel1,vReflect.x,vReflect.y,vReflect.z);
    vResult2 = vResult2 * vResult2;
    float shade = smoothstep(0.3f, 0.0f, vResult.x);
    vResult += shade * vResult2 * fSchlick * 5.0f;
  }
  #endif

  if(iMouse_z > 0.0f)
  {
    float3 vGrid =  step(to_float3_s(0.9f), fract(vHitPos + 0.01f));
    float fGrid = _fminf(dot(vGrid, to_float3_s(1.0f)), 1.0f);
    vResult = _mix(vResult, to_float3(0.0f, 0.0f, 1.0f), fGrid);
  }

  return sqrt_f3(vResult);
}

__KERNEL__ void ReprojectionIiFuse(float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    C_Ray ray;

  // float3 vResult = to_float3_s(0.0f);

  //float2 vMouse = swi2(iMouse,x,y) / iResolution;

  float3 vCameraPos;
  float3 vCameraTarget;

  GetCamera(&vCameraPos, &vCameraTarget, iTime);

  GetCameraRayLookat( vCameraPos, vCameraTarget, fragCoord, &ray, iResolution);

  //swi3(fragColor,x,y,z) = SceneColor( ray, iChannel0 );
  float3 sceneColor=SceneColor( ray, iChannel0, iChannel1, iMouse.z );



  SetFragmentShaderComputedColor(to_float4(sceneColor.x,sceneColor.y,sceneColor.z,1.0f));
}
/*
__DEVICE__ void mainVR( out float4 fragColor, in float2 fragCoord, in float3 fragRayOri, in float3 fragRayDir )
{
    C_Ray ray;

    ray.vOrigin = fragRayOri * 10.0f;
    ray.vOrigin.y += 3.0f;
    ray.vDir = fragRayDir;

    swi3(fragColor,x,y,z) = SceneColor( ray, iChannel0 );
    fragColor.w = 1.0f;


}
*/