
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

#define R   (iResolution)
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Update the density of each particle

// iChannel0 = Buf A (density)
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define PARTICLE_RADIUS        2.5f
#define PARTICLE_DENSITY_REST  0.4f

#define CEIL(x) ((float) ((int)((x) + 0.9999f))) // To workaround a bug with Firefox on Windows...


__DEVICE__ void densityUpdate (in float2 offset, inout float *particleDensity, float2 particlePosition, inout float2 *particleIdCheck, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2) {

  // Get the position of the cell
  float2 cellPosition = _floor (particlePosition + offset) + 0.5f;

  // Get the particle ID
  float2 particleId = swi2(texture (iChannel2, cellPosition / iResolution),x,y);

  // Check whether there is a particle here
  if (offset.x == 0.0f && offset.y == 0.0f) {

    // This is the current particle
    *particleIdCheck = particleId;
  } else if (particleId.x > 0.0f) {

    // Get the position of this other particle
    float2 otherParticlePosition = swi2(texture (iChannel1, particleId / iResolution),z,w);

    // Check whether these 2 particles touch each other
    float dist = length (otherParticlePosition - particlePosition);
    if (dist < 2.0f * PARTICLE_RADIUS) {

      // Compute the density
      float compression = 1.0f - dist / (2.0f * PARTICLE_RADIUS);
      *particleDensity += compression * compression * compression;
    }
  }
}

__KERNEL__ void HotLiquidMetalJipi625Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(Reset, 0); 
  fragCoord+=0.5f;  

  float particleDensity;
  float2 particlePosition;
  float2 particleIdCheck;

  
  // Check for a reset
  bool reset = iFrame == 0 || Reset;

  // Define the density
  if (reset) {
    particleDensity = 1.0f;
  } else {

    // Get the particle data
    particlePosition = swi2(texture (iChannel1, fragCoord / iResolution),z,w);
    if (particlePosition.x > 0.0f) {
      particleDensity = 1.0f;

      // Check for nearby particles
      particleIdCheck = to_float2_s (-1.0f);
      const float collisionRadius = CEIL (PARTICLE_RADIUS * 2.0f);
      for (float i = -collisionRadius; i <= collisionRadius; ++i) {
        for (float j = -collisionRadius; j <= collisionRadius; ++j) {
          densityUpdate (to_float2 (i, j),&particleDensity,particlePosition,&particleIdCheck, R, iChannel1, iChannel2);
        }
      }

      // Make sure the particle is still tracked
      if (particleIdCheck.x != fragCoord.x || particleIdCheck.y != fragCoord.y) {

        // The particle is lost...
        particleDensity = 0.0f;
      }
    } else {

      // The particle is lost...
      particleDensity = 0.0f;
    }
  }

  // Compute the "density factor" to ease the computation of the pressure force
  float particleDensityFactor = (particleDensity - PARTICLE_DENSITY_REST) / (particleDensity * particleDensity);

  // Update the fragment
  fragColor = to_float4 (particleDensity, particleDensityFactor, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// Update the velocity and position of each particle

// iChannel0 = Buf A (density)
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define GRAVITY_NORM               100.0f
//#define PARTICLE_RADIUS        2.5f
#define PARTICLE_PRESSURE_FACTOR   4000.0f
#define PARTICLE_VISCOSITY_FACTOR  40.0f
#define PARTICLE_VELOCITY_MAX      1.5f
#define PARTICLE_SPAWN_VELOCITY    to_float2 (-10.0f, 0.0f)
#define PARTICLE_SPAWN_POSITION    iResolution * 0.9f
#define COLLIDER_RADIUS            0.75f
#define COLLIDER_SPRING_STIFFNESS  4000.0f
#define COLLIDER_SPRING_DAMPING    20.0f
#define TIME_STEP_MAX              0.02f
#define SQRT3                      1.732f

//#define CEIL(x) (float (int ((x) + 0.9999f))) // To workaround a bug with Firefox on Windows...
#define MAX(x,y) ((x) > (y) ? (x) : (y))


__DEVICE__ float2 rand (in float2 seed) {
  float2 n = seed * to_float2 (12.9898f, 78.233f);
  return fract_f2 (swi2(n,y,x) * fract_f2 (n));
}

__DEVICE__ float2 rand (in float seed) {
  float2 n = seed * to_float2 (12.9898f, 78.233f);
  return fract_f2 (swi2(n,y,x) * fract_f2 (n));
}

__DEVICE__ void accelerationUpdate (in float2 offset, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2,
                                    float particleDensity, float2 particlePosition, inout float2 *particleIdCheck, float particleDensityFactor,inout float2 *particleAcceleration, float2 particleVelocity  ) {

  // Get the position of the cell
  float2 cellPosition = _floor (particlePosition + offset) + 0.5f;

  // Get the particle ID and the collider
  float4 data = texture (iChannel2, cellPosition / iResolution);
  float2 particleId = swi2(data,x,y);
  float collider = data.w;

  // Check whether there is a particle here
  if (offset.x == 0.0f && offset.y == 0.0f) {

    // This is the current particle
    *particleIdCheck = particleId;
  } else if (particleId.x > 0.0f) {

    // Get the position of this other particle
    data = texture (iChannel1, particleId / iResolution);
    float2 otherParticlePosition = swi2(data,z,w);

    // Compute the distance between these 2 particles
    float2 direction = otherParticlePosition - particlePosition;
    float dist = length (direction);

    // Check whether these 2 particles touch each other
    if (dist < 2.0f * PARTICLE_RADIUS) {

      // Normalize the direction
      direction /= dist;
      dist /= 2.0f * PARTICLE_RADIUS;

      // Get the velocity and density of this other particle
      float2 otherParticleVelocity = swi2(data,x,y);
      data = texture (iChannel0, particleId / iResolution);
      float otherParticleDensity = data.x;
      float otherParticleDensityFactor = data.y;

      // Apply the pressure and viscosity forces (SPH)
      float compression = 1.0f - dist;
      float pressure = PARTICLE_PRESSURE_FACTOR * (particleDensityFactor + otherParticleDensityFactor);
      float viscosity = PARTICLE_VISCOSITY_FACTOR * _fmaxf (0.0f, dot (particleVelocity - otherParticleVelocity, direction)) / ((particleDensity + otherParticleDensity) * dist);
      *particleAcceleration -= direction * (pressure + viscosity) * 3.0f * compression * compression;
    }
  }

  // Collision with a collider?
  if (collider > 0.5f) {

    // Compute the signed distance between the center of the particle (circle) and the border of the collider (square)
    float2 direction = cellPosition - particlePosition;
    float2 distCollider = abs_f2 (direction) - COLLIDER_RADIUS;
    float dist = length (_fmaxf (distCollider, to_float2_s(0.0f))) + _fminf (max (distCollider.x, distCollider.y), 0.0f);

    // Check whether the particle touches the collider
    if (dist < PARTICLE_RADIUS) {

      // Normalize the direction
      direction = sign_f2 (direction) * (dist > 0.0f ? distCollider / dist : step (swi2(distCollider,y,x), distCollider));

      // Apply the collision force (spring)
      float compression = 1.0f - (dist + COLLIDER_RADIUS) / (PARTICLE_RADIUS + COLLIDER_RADIUS);
      *particleAcceleration -= direction * (compression * COLLIDER_SPRING_STIFFNESS + dot (particleVelocity, direction) * COLLIDER_SPRING_DAMPING);
    }
  }
}

__KERNEL__ void HotLiquidMetalJipi625Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  CONNECT_CHECKBOX0(Reset, 0); 
  fragCoord+=0.5f;

  float particleDensity;
  float particleDensityFactor;
  float2 particleAcceleration;
  float2 particleVelocity;
  float2 particlePosition;
  float2 particleIdCheck;

  // Check for a reset
  bool reset = iFrame == 0 || Reset;

  // Define the particle data
  if (reset) {

    // Define the particle spawning area
    float liquid =
      step (fragCoord.y, iResolution.y * 0.5f)
      * step (mod_f (fragCoord.x + SQRT3 * fragCoord.y, _ceil (2.0f * PARTICLE_RADIUS)), 1.0f)
      * step (mod_f (fragCoord.y, _ceil (SQRT3 * PARTICLE_RADIUS)), 1.0f);

    // Initialize the particle
    particleVelocity = to_float2_s (0.0f);
    particlePosition = liquid > 0.5f ? fragCoord + 0.01f * rand (fragCoord): to_float2_s(-1.0f);
  } else {

    // Get the particle data
    float4 data = texture (iChannel0, fragCoord / iResolution);
    particleDensity = data.x;
    if (particleDensity > 0.5f) {
      particleDensityFactor = data.y;
      data = texture (iChannel1, fragCoord / iResolution);
      particleVelocity = swi2(data,x,y);
      particlePosition = swi2(data,z,w);

      // Initialize the acceleration
      float gravityDirection = texture (iChannel3, to_float2 (1.5f, 0.5f) / iResolution).x;
      particleAcceleration = GRAVITY_NORM * to_float2 (_cosf (gravityDirection), _sinf (gravityDirection));

      // Check for collisions with nearby particles and colliders
      particleIdCheck = to_float2_s(-1.0f);
      const float collisionRadius = CEIL (PARTICLE_RADIUS + MAX (PARTICLE_RADIUS, COLLIDER_RADIUS));
      for (float i = -collisionRadius; i <= collisionRadius; ++i) {
        for (float j = -collisionRadius; j <= collisionRadius; ++j) {
          accelerationUpdate (to_float2 (i, j),R,iChannel0,iChannel1,iChannel2,
                              particleDensity,particlePosition,&particleIdCheck,particleDensityFactor,&particleAcceleration,particleVelocity );
        }
      }

      // Make sure the particle is still tracked
      if (particleIdCheck.x != fragCoord.x || particleIdCheck.y != fragCoord.y) {

        // The particle is lost...
        particlePosition = to_float2_s(-1.0f);
      } else {

        // Limit the time step
        float timeStep = _fminf (iTimeDelta, TIME_STEP_MAX);

        // Update the velocity of the particle
        particleVelocity += particleAcceleration * timeStep;

        // Limit the velocity (to avoid losing track of the particle)
        float dist = length (particleVelocity * timeStep);
        if (dist > PARTICLE_VELOCITY_MAX) {
          particleVelocity *= PARTICLE_VELOCITY_MAX / dist;
        }

        // Update the position of the particle
        particlePosition += particleVelocity * timeStep;
      }
    } else {

      // Check the particle ID
      float2 particleId = 0.5f + _floor (iResolution * rand (iTime));
      if (fragCoord.x == particleId.x && fragCoord.y == particleId.y) {

        // Spawn a new particle
        particleVelocity = PARTICLE_SPAWN_VELOCITY;
        particlePosition = _floor (PARTICLE_SPAWN_POSITION) + 0.5f;
      } else {

        // The particle is lost...
        particlePosition = to_float2_s (-1.0f);
      }
    }
  }

  // Update the fragment
  fragColor = to_float4_f2f2 (particleVelocity, particlePosition);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// Track the particles

// iChannel0 = [nothing]
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

//#define PARTICLE_VELOCITY_MAX    1.5
#define PARTICLE_SPAWN_POSITION  iResolution * 0.9f

//#define CEIL(x) (float (int ((x) + 0.9999f))) // To workaround a bug with Firefox on Windows...



__DEVICE__ float track (in float2 fragCoord, in float2 offset, bool reset, inout float2 *particleIdFound, inout float *particleVelocity,
                         float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2) {

  // Get the particle ID and collider
  float2 cellPosition = fragCoord + offset;
  float4 data;
  if (reset) {

    // Define the colliders
    float collider = step (0.5f * iResolution.x - 5.0f, _fabs (fragCoord.x - 0.5f * iResolution.x));
    collider += step (fragCoord.y, 5.0f);
    collider += step (_fabs (fragCoord.y - 0.8f * iResolution.y), 2.5f) * step (0.8f * iResolution.x, fragCoord.x);

    // Set the initial data
    data = to_float4 (cellPosition.x, cellPosition.y, 0.0f, collider);
  } else {

    // Get the exisiting data
    data = texture (iChannel2, cellPosition / iResolution);
  }
  float2 particleId = swi2(data,x,y);
  float collider = data.w;

  // Get the position of this particle
  if (particleId.x > 0.0f) {
    data = texture (iChannel1, particleId / iResolution);
    float2 particlePosition = swi2(data,z,w);

    // Check whether this particle is the one to track
    float2 delta = _floor (particlePosition - fragCoord + 0.5f);
    if (delta.x == 0.0f && delta.y == 0.0f) {

      // Take note of the particle ID and its velocity
      *particleIdFound = particleId;
      *particleVelocity = length (swi2(data,x,y));
    }
  }

  // Return the collider
  return collider;
}



__DEVICE__ float segDist (in float2 p, in float2 a, in float2 b) {
  p -= a;
  b -= a;
  return length (p - b * clamp (dot (p, b) / dot (b, b), 0.0f, 1.0f));
}

__KERNEL__ void HotLiquidMetalJipi625Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(Reset, 0); 
  fragCoord+=0.5f;


  // Initialization
  float2 particleIdFound = to_float2_s (-1.0f);
  float particleVelocity = -1.0f;
  float collider = 0.0f;

  // Check the player inputs
  float4 data = texture (iChannel3, to_float2_s (0.5f) / iResolution);
  bool reset = iFrame == 0 || data.w > 0.5f;

  // Check the current position
  float2 offset = to_float2_s (0.0f);
  collider = track (fragCoord, offset, reset, &particleIdFound, &particleVelocity,
                    R, iChannel1, iChannel2);

  // Allow to add colliders (removing particles)
  if (iMouse.z > 0.5f) {
    float dist;
    if (data.z < 0.5f) {
      dist = length (fragCoord - swi2(iMouse,x,y));
    } else {
      dist = segDist (fragCoord, swi2(data,x,y), swi2(iMouse,x,y));
    }
    if (dist < 3.0f) {
      collider = texture (iChannel3, to_float2 (1.5f, 0.5f) / iResolution).z;
    }
  }
  if (collider < 0.5f) {

    // Track the particle (spiral loop from the current position)
    float2 direction = to_float2 (1.0f, 0.0f);
    for (float n = 1.0f; n < (2.0f * CEIL (PARTICLE_VELOCITY_MAX) + 1.0f) * (2.0f * CEIL (PARTICLE_VELOCITY_MAX) + 1.0f); ++n) {
      if (particleIdFound.x > 0.0f) {
        break;
      }
      offset += direction;
      track (fragCoord, offset, reset, &particleIdFound, &particleVelocity,
             R, iChannel1, iChannel2);
      if (offset.x == offset.y || (offset.x < 0.0f && offset.x == -offset.y) || (offset.x > 0.0f && offset.x == 1.0f - offset.y)) {
        direction = to_float2 (-direction.y, direction.x);
      }
    }

    // Spawn a new particle?
    //if (particleIdFound.x < 0.0f && fragCoord == _floor (PARTICLE_SPAWN_POSITION) + 0.5f) {
      if (particleIdFound.x < 0.0f && fragCoord.x == _floor (iResolution.x * 0.9) + 0.5f && fragCoord.y == _floor (iResolution.y * 0.9) + 0.5f) {
      float2 particleId = 0.5f + _floor (iResolution * rand (iTime));
      float2 particlePosition = swi2(texture (iChannel1, particleId / iResolution),z,w);
      if (particlePosition.x == fragCoord.x && particlePosition.y == fragCoord.y) {
        particleIdFound = particleId;
      }
    }
  }

  // Update the fragment
  fragColor = to_float4 (particleIdFound.x,particleIdFound.y, particleVelocity, collider);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Preset: Keyboard' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// Handle player inputs

// iChannel0 = Keyboard
// iChannel1 = [nothing]
// iChannel2 = [nothing]
// iChannel3 = Buf D (inputs)

#define KEY_R    (to_float2 (82.5f, 0.5f) / 256.0f)
#define KEY_LEFT  (to_float2 (37.5f, 0.5f) / 256.0f)
#define KEY_RIGHT  (to_float2 (39.5f, 0.5f) / 256.0f)
#define KEY_DOWN  (to_float2 (40.5f, 0.5f) / 256.0f)
#define KEY_SPACE  (to_float2 (32.5f, 0.5f) / 256.0f)
#define PI      3.14159265359

__KERNEL__ void HotLiquidMetalJipi625Fuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  CONNECT_CHECKBOX0(Reset, 0); 
  
  CONNECT_BUTTON0(Modi, 0, Left,  Right, Down, Space, Special);
  
  fragCoord+=0.5f;

  float keyLeft = 0.0f, keyRight=0.0f, keyDown=0.0f, keySpace=0.0f;


  //Modi = Modi-1;
  if (Modi == 1) keyLeft  = 1.0f;
  if (Modi == 2) keyRight = 1.0f;
  if (Modi == 3) keyDown  = 1.0f;
  if (Modi == 4) keySpace = 1.0f;



  // Don't waste time
  if (fragCoord.x > 2.0f || fragCoord.y > 1.0f) {
    
    //discard;
    SetFragmentShaderComputedColor(fragColor);
    return;
  }

  // Get the status of the reset (R) key
  //float reset = _tex2DVecN (iChannel0,KEY_R.x,KEY_R.y,15).x;

  // Check what to do
  if (fragCoord.x < 1.0f) {

    // Update the fragment
    fragColor = to_float4_aw (swi3(iMouse,x,y,z), Reset);
  } else {

    // Set the direction of the gravity
    float gravityDirection;
    float gravityTimer;
    if (iFrame == 0 || Reset) {

      // Reset the gravity
      gravityDirection = -PI * 0.5f;
      gravityTimer = 0.0f;
    } else {

      // Get the current values
      float2 data = swi2(texture (iChannel3, fragCoord / iResolution),x,y);
      gravityDirection = data.x;
      gravityTimer = data.y;

      // Get the status of the left, right and down keys
      //float keyLeft = _tex2DVecN (iChannel0,KEY_LEFT.x,KEY_LEFT.y,15).x;
      //float keyRight = _tex2DVecN (iChannel0,KEY_RIGHT.x,KEY_RIGHT.y,15).x;
      //float keyDown = _tex2DVecN (iChannel0,KEY_DOWN.x,KEY_DOWN.y,15).x;
      
      if (keyLeft + keyRight + keyDown < 0.5f) {
        gravityTimer = _fmaxf (0.0f, gravityTimer - iTimeDelta * 5.0f);
      } else {
        if (keyLeft > 0.5f) {
          gravityDirection -= PI * 0.5f * iTimeDelta;
        } else if (keyRight > 0.5f) {
          gravityDirection += PI * 0.5f * iTimeDelta;
        } else if (gravityTimer == 0.0f) {
          gravityDirection += PI;
        }
        gravityTimer = 1.0f;
      }
    }

    // Get the status of the space key
//    float keySpace = _tex2DVecN (iChannel0,KEY_SPACE.x,KEY_SPACE.y,15).r;


    // Update the fragment
    fragColor = to_float4 (gravityDirection, gravityTimer, 1.0f - keySpace, Modi);
  }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Display particles and colliders

// iChannel0 = Background texture
// iChannel1 = [nothing]
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define PARTICLE_RADIUS_I           3.5f
#define PARTICLE_VELOCITY_FACTOR  0.02f

//#define CEIL(x) (float (int ((x) + 0.9999f))) // To workaround a bug with Firefox on Windows...



__DEVICE__ float3 particleColor (in float particleVelocity) {
  return _mix (to_float3 (0.8f, 0.2f, 0.2f), to_float3 (1.0f, 1.0f, 0.5f), particleVelocity * PARTICLE_VELOCITY_FACTOR);
}

__KERNEL__ void HotLiquidMetalJipi625Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  fragCoord+=0.5f;
  // Get the background texture
  float3 color = swi3(texture (iChannel0, fragCoord / iResolution),x,y,z);

  // Check whether there is a collider here
  float4 data = texture (iChannel2, fragCoord / iResolution);
  float collider = data.w;
  if (collider < 0.5f) {

    // Darken the background
    color *= 0.4f;

    // Check whether there is a particle here
    float particleVelocity = data.z;
    float weightTotal = step (0.0f, particleVelocity);
    float weightedVelocity = weightTotal * particleVelocity;

    // Check for nearby particles
    const float displayRadius = CEIL (PARTICLE_RADIUS_I);
    for (float i = -displayRadius; i <= displayRadius; ++i) {
      for (float j = -displayRadius; j <= displayRadius; ++j) {
        float2 offset = to_float2 (i, j);
        if (offset.x != 0.0f || offset.y != 0.0f) {
          particleVelocity = texture (iChannel2, (fragCoord + offset) / iResolution).z;
          if (particleVelocity >= 0.0f) {
            float weight = _fmaxf (0.0f, 1.0f - (length (offset) - 1.0f) / PARTICLE_RADIUS_I);
            weightTotal += weight;
            weightedVelocity += weight * particleVelocity;
          }
        }
      }
    }

    // Display the particle
    if (weightTotal > 0.0f) {
      color += particleColor (weightedVelocity / weightTotal) * _fminf (weightTotal * weightTotal, 1.0f);
    }
  }

  // Display the direction of the gravity
  data = texture (iChannel3, to_float2 (1.5f, 0.5f) / iResolution);
  float gravityTimer = data.y;
  if (gravityTimer > 0.0f) {
    float gravityDirection = data.x;
    float2 frag = fragCoord - 0.5f * iResolution;
    float2 direction = to_float2 (_cosf (gravityDirection), _sinf (gravityDirection));
    float2 pointA = 25.0f * direction;
    float2 pointB = 15.0f * direction;
    float2 offset = 10.0f * to_float2 (direction.y, -direction.x);
    float dist = segDist (frag, -pointA, pointA);
    dist = _fminf (dist, segDist (frag, pointA, pointB + offset));
    dist = _fminf (dist, segDist (frag, pointA, pointB - offset));
    color = _mix (color, to_float3_s(smoothstep (4.0f, 3.0f, dist)), gravityTimer * smoothstep (6.0f, 5.0f, dist));
  }

  // Set the fragment color
  fragColor = to_float4_aw (color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}