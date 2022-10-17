

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Display particles and colliders

// iChannel0 = Background texture
// iChannel1 = [nothing]
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define PARTICLE_RADIUS				3.5
#define PARTICLE_VELOCITY_FACTOR	0.02

#define CEIL(x) (float (int ((x) + 0.9999))) // To workaround a bug with Firefox on Windows...

float segDist (in vec2 p, in vec2 a, in vec2 b) {
	p -= a;
	b -= a;
	return length (p - b * clamp (dot (p, b) / dot (b, b), 0.0, 1.0));
}

vec3 particleColor (in float particleVelocity) {
	return mix (vec3 (0.8, 0.2, 0.2), vec3 (1.0, 1.0, 0.5), particleVelocity * PARTICLE_VELOCITY_FACTOR);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord) {

	// Get the background texture
	vec3 color = texture (iChannel0, fragCoord / iResolution.xy).rgb;

	// Check whether there is a collider here
	vec4 data = texture (iChannel2, fragCoord / iResolution.xy);
	float collider = data.a;
	if (collider < 0.5) {

		// Darken the background
		color *= 0.4;

		// Check whether there is a particle here
		float particleVelocity = data.b;
		float weightTotal = step (0.0, particleVelocity);
		float weightedVelocity = weightTotal * particleVelocity;

		// Check for nearby particles
		const float displayRadius = CEIL (PARTICLE_RADIUS);
		for (float i = -displayRadius; i <= displayRadius; ++i) {
			for (float j = -displayRadius; j <= displayRadius; ++j) {
				vec2 offset = vec2 (i, j);
				if (offset != vec2 (0.0)) {
					particleVelocity = texture (iChannel2, (fragCoord + offset) / iResolution.xy).b;
					if (particleVelocity >= 0.0) {
						float weight = max (0.0, 1.0 - (length (offset) - 1.0) / PARTICLE_RADIUS);
						weightTotal += weight;
						weightedVelocity += weight * particleVelocity;
					}
				}
			}
		}

		// Display the particle
		if (weightTotal > 0.0) {
			color += particleColor (weightedVelocity / weightTotal) * min (weightTotal * weightTotal, 1.0);
		}
	}

	// Display the direction of the gravity
	data = texture (iChannel3, vec2 (1.5, 0.5) / iResolution.xy);
	float gravityTimer = data.g;
	if (gravityTimer > 0.0) {
		float gravityDirection = data.r;
		vec2 frag = fragCoord - 0.5 * iResolution.xy;
		vec2 direction = vec2 (cos (gravityDirection), sin (gravityDirection));
		vec2 pointA = 25.0 * direction;
		vec2 pointB = 15.0 * direction;
		vec2 offset = 10.0 * vec2 (direction.y, -direction.x);
		float dist = segDist (frag, -pointA, pointA);
		dist = min (dist, segDist (frag, pointA, pointB + offset));
		dist = min (dist, segDist (frag, pointA, pointB - offset));
		color = mix (color, vec3 (smoothstep (4.0, 3.0, dist)), gravityTimer * smoothstep (6.0, 5.0, dist));
	}

	// Set the fragment color
	fragColor = vec4 (color, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Update the density of each particle

// iChannel0 = Buf A (density)
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define PARTICLE_RADIUS			2.5
#define PARTICLE_DENSITY_REST	0.4

#define CEIL(x) (float (int ((x) + 0.9999))) // To workaround a bug with Firefox on Windows...

float particleDensity;
vec2 particlePosition;
vec2 particleIdCheck;

void densityUpdate (in vec2 offset) {

	// Get the position of the cell
	vec2 cellPosition = floor (particlePosition + offset) + 0.5;

	// Get the particle ID
	vec2 particleId = texture (iChannel2, cellPosition / iResolution.xy).rg;

	// Check whether there is a particle here
	if (offset == vec2 (0.0)) {

		// This is the current particle
		particleIdCheck = particleId;
	} else if (particleId.x > 0.0) {

		// Get the position of this other particle
		vec2 otherParticlePosition = texture (iChannel1, particleId / iResolution.xy).ba;

		// Check whether these 2 particles touch each other
		float dist = length (otherParticlePosition - particlePosition);
		if (dist < 2.0 * PARTICLE_RADIUS) {

			// Compute the density
			float compression = 1.0 - dist / (2.0 * PARTICLE_RADIUS);
			particleDensity += compression * compression * compression;
		}
	}
}

void mainImage (out vec4 fragColor, in vec2 fragCoord) {

	// Check for a reset
	bool reset = iFrame == 0 || texture (iChannel3, vec2 (0.5) / iResolution.xy).a > 0.5;

	// Define the density
	if (reset) {
		particleDensity = 1.0;
	} else {

		// Get the particle data
		particlePosition = texture (iChannel1, fragCoord / iResolution.xy).ba;
		if (particlePosition.x > 0.0) {
			particleDensity = 1.0;

			// Check for nearby particles
			particleIdCheck = vec2 (-1.0);
			const float collisionRadius = CEIL (PARTICLE_RADIUS * 2.0);
			for (float i = -collisionRadius; i <= collisionRadius; ++i) {
				for (float j = -collisionRadius; j <= collisionRadius; ++j) {
					densityUpdate (vec2 (i, j));
				}
			}

			// Make sure the particle is still tracked
			if (particleIdCheck != fragCoord) {

				// The particle is lost...
				particleDensity = 0.0;
			}
		} else {

			// The particle is lost...
			particleDensity = 0.0;
		}
	}

	// Compute the "density factor" to ease the computation of the pressure force
	float particleDensityFactor = (particleDensity - PARTICLE_DENSITY_REST) / (particleDensity * particleDensity);

	// Update the fragment
	fragColor = vec4 (particleDensity, particleDensityFactor, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Update the velocity and position of each particle

// iChannel0 = Buf A (density)
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define GRAVITY_NORM				100.0
#define PARTICLE_RADIUS				2.5
#define PARTICLE_PRESSURE_FACTOR	4000.0
#define PARTICLE_VISCOSITY_FACTOR	40.0
#define PARTICLE_VELOCITY_MAX		1.5
#define PARTICLE_SPAWN_VELOCITY		vec2 (-10.0, 0.0)
#define PARTICLE_SPAWN_POSITION		iResolution.xy * 0.9
#define COLLIDER_RADIUS				0.75
#define COLLIDER_SPRING_STIFFNESS	4000.0
#define COLLIDER_SPRING_DAMPING		20.0
#define TIME_STEP_MAX				0.02
#define SQRT3						1.732

#define CEIL(x) (float (int ((x) + 0.9999))) // To workaround a bug with Firefox on Windows...
#define MAX(x,y) ((x) > (y) ? (x) : (y))

float particleDensity;
float particleDensityFactor;
vec2 particleAcceleration;
vec2 particleVelocity;
vec2 particlePosition;
vec2 particleIdCheck;

vec2 rand (in vec2 seed) {
	vec2 n = seed * vec2 (12.9898, 78.233);
	return fract (n.yx * fract (n));
}

vec2 rand (in float seed) {
	vec2 n = seed * vec2 (12.9898, 78.233);
	return fract (n.yx * fract (n));
}

void accelerationUpdate (in vec2 offset) {

	// Get the position of the cell
	vec2 cellPosition = floor (particlePosition + offset) + 0.5;

	// Get the particle ID and the collider
	vec4 data = texture (iChannel2, cellPosition / iResolution.xy);
	vec2 particleId = data.rg;
	float collider = data.a;

	// Check whether there is a particle here
	if (offset == vec2 (0.0)) {

		// This is the current particle
		particleIdCheck = particleId;
	} else if (particleId.x > 0.0) {

		// Get the position of this other particle
		data = texture (iChannel1, particleId / iResolution.xy);
		vec2 otherParticlePosition = data.ba;

		// Compute the distance between these 2 particles
		vec2 direction = otherParticlePosition - particlePosition;
		float dist = length (direction);

		// Check whether these 2 particles touch each other
		if (dist < 2.0 * PARTICLE_RADIUS) {

			// Normalize the direction
			direction /= dist;
			dist /= 2.0 * PARTICLE_RADIUS;

			// Get the velocity and density of this other particle
			vec2 otherParticleVelocity = data.rg;
			data = texture (iChannel0, particleId / iResolution.xy);
			float otherParticleDensity = data.r;
			float otherParticleDensityFactor = data.g;

			// Apply the pressure and viscosity forces (SPH)
			float compression = 1.0 - dist;
			float pressure = PARTICLE_PRESSURE_FACTOR * (particleDensityFactor + otherParticleDensityFactor);
			float viscosity = PARTICLE_VISCOSITY_FACTOR * max (0.0, dot (particleVelocity - otherParticleVelocity, direction)) / ((particleDensity + otherParticleDensity) * dist);
			particleAcceleration -= direction * (pressure + viscosity) * 3.0 * compression * compression;
		}
	}

	// Collision with a collider?
	if (collider > 0.5) {

		// Compute the signed distance between the center of the particle (circle) and the border of the collider (square)
		vec2 direction = cellPosition - particlePosition;
		vec2 distCollider = abs (direction) - COLLIDER_RADIUS;
		float dist = length (max (distCollider, 0.0)) + min (max (distCollider.x, distCollider.y), 0.0);

		// Check whether the particle touches the collider
		if (dist < PARTICLE_RADIUS) {

			// Normalize the direction
			direction = sign (direction) * (dist > 0.0 ? distCollider / dist : step (distCollider.yx, distCollider));

			// Apply the collision force (spring)
			float compression = 1.0 - (dist + COLLIDER_RADIUS) / (PARTICLE_RADIUS + COLLIDER_RADIUS);
			particleAcceleration -= direction * (compression * COLLIDER_SPRING_STIFFNESS + dot (particleVelocity, direction) * COLLIDER_SPRING_DAMPING);
		}
	}
}

void mainImage (out vec4 fragColor, in vec2 fragCoord) {

	// Check for a reset
	bool reset = iFrame == 0 || texture (iChannel3, vec2 (0.5) / iResolution.xy).a > 0.5;

	// Define the particle data
	if (reset) {

		// Define the particle spawning area
		float liquid =
			step (fragCoord.y, iResolution.y * 0.5)
			* step (mod (fragCoord.x + SQRT3 * fragCoord.y, ceil (2.0 * PARTICLE_RADIUS)), 1.0)
			* step (mod (fragCoord.y, ceil (SQRT3 * PARTICLE_RADIUS)), 1.0);

		// Initialize the particle
		particleVelocity = vec2 (0.0);
		particlePosition = liquid > 0.5 ? fragCoord + 0.01 * rand (fragCoord): vec2 (-1.0);
	} else {

		// Get the particle data
		vec4 data = texture (iChannel0, fragCoord / iResolution.xy);
		particleDensity = data.r;
		if (particleDensity > 0.5) {
			particleDensityFactor = data.g;
			data = texture (iChannel1, fragCoord / iResolution.xy);
			particleVelocity = data.rg;
			particlePosition = data.ba;

			// Initialize the acceleration
			float gravityDirection = texture (iChannel3, vec2 (1.5, 0.5) / iResolution.xy).r;
			particleAcceleration = GRAVITY_NORM * vec2 (cos (gravityDirection), sin (gravityDirection));

			// Check for collisions with nearby particles and colliders
			particleIdCheck = vec2 (-1.0);
			const float collisionRadius = CEIL (PARTICLE_RADIUS + MAX (PARTICLE_RADIUS, COLLIDER_RADIUS));
			for (float i = -collisionRadius; i <= collisionRadius; ++i) {
				for (float j = -collisionRadius; j <= collisionRadius; ++j) {
					accelerationUpdate (vec2 (i, j));
				}
			}

			// Make sure the particle is still tracked
			if (particleIdCheck != fragCoord) {

				// The particle is lost...
				particlePosition = vec2 (-1.0);
			} else {

				// Limit the time step
				float timeStep = min (iTimeDelta, TIME_STEP_MAX);

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
			vec2 particleId = 0.5 + floor (iResolution.xy * rand (iTime));
			if (fragCoord == particleId) {

				// Spawn a new particle
				particleVelocity = PARTICLE_SPAWN_VELOCITY;
				particlePosition = floor (PARTICLE_SPAWN_POSITION) + 0.5;
			} else {

				// The particle is lost...
				particlePosition = vec2 (-1.0);
			}
		}
	}

	// Update the fragment
	fragColor = vec4 (particleVelocity, particlePosition);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Track the particles

// iChannel0 = [nothing]
// iChannel1 = Buf B (velocity & position)
// iChannel2 = Buf C (id & colliders)
// iChannel3 = Buf D (inputs)

#define PARTICLE_VELOCITY_MAX	1.5
#define PARTICLE_SPAWN_POSITION	iResolution.xy * 0.9

#define CEIL(x) (float (int ((x) + 0.9999))) // To workaround a bug with Firefox on Windows...

bool reset;
vec2 particleIdFound;
float particleVelocity;

float track (in vec2 fragCoord, in vec2 offset) {

	// Get the particle ID and collider
	vec2 cellPosition = fragCoord + offset;
	vec4 data;
	if (reset) {

		// Define the colliders
		float collider = step (0.5 * iResolution.x - 5.0, abs (fragCoord.x - 0.5 * iResolution.x));
		collider += step (fragCoord.y, 5.0);
		collider += step (abs (fragCoord.y - 0.8 * iResolution.y), 2.5) * step (0.8 * iResolution.x, fragCoord.x);

		// Set the initial data
		data = vec4 (cellPosition, 0.0, collider);
	} else {

		// Get the exisiting data
		data = texture (iChannel2, cellPosition / iResolution.xy);
	}
	vec2 particleId = data.rg;
	float collider = data.a;

	// Get the position of this particle
	if (particleId.x > 0.0) {
		data = texture (iChannel1, particleId / iResolution.xy);
		vec2 particlePosition = data.ba;

		// Check whether this particle is the one to track
		vec2 delta = floor (particlePosition - fragCoord + 0.5);
		if (delta == vec2 (0.0)) {

			// Take note of the particle ID and its velocity
			particleIdFound = particleId;
			particleVelocity = length (data.rg);
		}
	}

	// Return the collider
	return collider;
}

vec2 rand (in float seed) {
	vec2 n = seed * vec2 (12.9898, 78.233);
	return fract (n.yx * fract (n));
}

float segDist (in vec2 p, in vec2 a, in vec2 b) {
	p -= a;
	b -= a;
	return length (p - b * clamp (dot (p, b) / dot (b, b), 0.0, 1.0));
}

void mainImage (out vec4 fragColor, in vec2 fragCoord) {

	// Initialization
	particleIdFound = vec2 (-1.0);
	particleVelocity = -1.0;
	float collider = 0.0;

	// Check the player inputs
	vec4 data = texture (iChannel3, vec2 (0.5) / iResolution.xy);
	reset = iFrame == 0 || data.a > 0.5;

	// Check the current position
	vec2 offset = vec2 (0.0);
	collider = track (fragCoord, offset);

	// Allow to add colliders (removing particles)
	if (iMouse.z > 0.5) {
		float dist;
		if (data.b < 0.5) {
			dist = length (fragCoord - iMouse.xy);
		} else {
			dist = segDist (fragCoord, data.rg, iMouse.xy);
		}
		if (dist < 3.0) {
			collider = texture (iChannel3, vec2 (1.5, 0.5) / iResolution.xy).b;
		}
	}
	if (collider < 0.5) {

		// Track the particle (spiral loop from the current position)
		vec2 direction = vec2 (1.0, 0.0);
		for (float n = 1.0; n < (2.0 * CEIL (PARTICLE_VELOCITY_MAX) + 1.0) * (2.0 * CEIL (PARTICLE_VELOCITY_MAX) + 1.0); ++n) {
			if (particleIdFound.x > 0.0) {
				break;
			}
			offset += direction;
			track (fragCoord, offset);
			if (offset.x == offset.y || (offset.x < 0.0 && offset.x == -offset.y) || (offset.x > 0.0 && offset.x == 1.0 - offset.y)) {
				direction = vec2 (-direction.y, direction.x);
			}
		}

		// Spawn a new particle?
		if (particleIdFound.x < 0.0 && fragCoord == floor (PARTICLE_SPAWN_POSITION) + 0.5) {
			vec2 particleId = 0.5 + floor (iResolution.xy * rand (iTime));
			vec2 particlePosition = texture (iChannel1, particleId / iResolution.xy).ba;
			if (particlePosition == fragCoord) {
				particleIdFound = particleId;
			}
		}
	}

	// Update the fragment
	fragColor = vec4 (particleIdFound, particleVelocity, collider);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Handle player inputs

// iChannel0 = Keyboard
// iChannel1 = [nothing]
// iChannel2 = [nothing]
// iChannel3 = Buf D (inputs)

#define KEY_R		(vec2 (82.5, 0.5) / 256.0)
#define KEY_LEFT	(vec2 (37.5, 0.5) / 256.0)
#define KEY_RIGHT	(vec2 (39.5, 0.5) / 256.0)
#define KEY_DOWN	(vec2 (40.5, 0.5) / 256.0)
#define KEY_SPACE	(vec2 (32.5, 0.5) / 256.0)
#define PI			3.14159265359

void mainImage (out vec4 fragColor, in vec2 fragCoord) {

	// Don't waste time
	if (fragCoord.x > 2.0 || fragCoord.y > 1.0) {
		discard;
	}

	// Get the status of the reset (R) key
	float reset = texture (iChannel0, KEY_R).r;

	// Check what to do
	if (fragCoord.x < 1.0) {

		// Update the fragment
		fragColor = vec4 (iMouse.xyz, reset);
	} else {

		// Set the direction of the gravity
		float gravityDirection;
		float gravityTimer;
		if (iFrame == 0 || reset > 0.5) {

			// Reset the gravity
			gravityDirection = -PI * 0.5;
			gravityTimer = 0.0;
		} else {

			// Get the current values
			vec2 data = texture (iChannel3, fragCoord / iResolution.xy).rg;
			gravityDirection = data.r;
			gravityTimer = data.g;

			// Get the status of the left, right and down keys
			float keyLeft = texture (iChannel0, KEY_LEFT).r;
			float keyRight = texture (iChannel0, KEY_RIGHT).r;
			float keyDown = texture (iChannel0, KEY_DOWN).r;
			if (keyLeft + keyRight + keyDown < 0.5) {
				gravityTimer = max (0.0, gravityTimer - iTimeDelta * 5.0);
			} else {
				if (keyLeft > 0.5) {
					gravityDirection -= PI * 0.5 * iTimeDelta;
				} else if (keyRight > 0.5) {
					gravityDirection += PI * 0.5 * iTimeDelta;
				} else if (gravityTimer == 0.0) {
					gravityDirection += PI;
				}
				gravityTimer = 1.0;
			}
		}

		// Get the status of the space key
		float keySpace = texture (iChannel0, KEY_SPACE).r;

		// Update the fragment
		fragColor = vec4 (gravityDirection, gravityTimer, 1.0 - keySpace, 0.0);
	}
}