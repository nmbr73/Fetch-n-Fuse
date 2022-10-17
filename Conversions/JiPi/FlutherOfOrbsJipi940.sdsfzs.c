
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/894a09f482fb9b2822c093630fc37f0ce6cfec02b652e4e341323e4b6e4a4543.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * -0.40f;// + 0.5f;  // * -01.50f;//(MarchingCubes)  - 0.15f; (GlassDuck)
}

#define refract_f3 _refract_f3


// **************************************************************************
// DEFINITIONS

#define PI 3.14159f
#define TWO_PI 6.28318f
#define ONE_OVER_PI 0.3183099f
#define ONE_OVER_TWO_PI 0.159154f
#define PI_OVER_TWO 1.570796f
    
#define EPSILON 0.0001f
#define BIG_FLOAT 1000000.0f

// **************************************************************************
// OPTIMIZATION DEFS

// Comment in the quality of your choice based on graphics card performance

//#define HIGH_QUALITY
#define MID_QUALITY
//#define LOW_QUALITY

#ifdef HIGH_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 64
#define NUM_POLAR_MARCH_STEPS 128
#define FOG_EXTENT 1400.0f
#endif

#ifdef MID_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 48
#define NUM_POLAR_MARCH_STEPS 64
#define FOG_EXTENT 1000.0f
#endif 

#ifdef LOW_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 32
#define NUM_POLAR_MARCH_STEPS 32
#define FOG_EXTENT 800.0f
#endif 

// always have audio samples as multiple of 3
#define NUM_AUDIO_SAMPLE_TRIPLES 3

// **************************************************************************
  #define CELL_BOX_HALF_SIZE_DEFAULT 75.0f
  #define ORB_OUTER_RADIUS_DEFAULT 50.0f
  #define ORB_CENTER_RADIUS_DEFAULT 10.0f

// **************************************************************************
// MATH UTILITIES
// **************************************************************************

// XXX: To get around a case where a number very close to zero can result in 
// erratic behavior with sign, we assume a positive sign when a value is 
// close to 0.
__DEVICE__ float zeroTolerantSign(float value)
{
    // DEBRANCHED
    // Equivalent to:
    // if (_fabs(value) > EPSILON) { 
    //    s = sign(value); 
    // }
    return _mix(1.0f, sign_f(value), step(EPSILON, _fabs(value)));
}

// convert a 3d point to two orb coordinates. First coordinate is latitudinal
// angle (angle from the plane going through x+z) Second coordinate is azimuth
// (rotation around the y axis)

// Range of outputs := ([PI/2, -PI/2], [-PI, PI])
__DEVICE__ float2 cartesianToPolar( float3 p ) 
{    
    return to_float2(PI/2.0f - _acosf(p.y / length(p)), _atan2f(p.z, p.x));
}

// Convert a polar coordinate (x is latitudinal angle, y is azimuthal angle)
// results in a 3-float vector with y being the up axis.

// Range of outputs := ([-1.0f,-1.0f,-1.] -> [1.,1.0f,1.])
__DEVICE__ float3 polarToCartesian( float2 angles )
{
    float cosLat = _cosf(angles.x);
    float sinLat = _sinf(angles.x);
    
    float cosAzimuth = _cosf(angles.y);
    float sinAzimuth = _sinf(angles.y);
    
    return to_float3(cosAzimuth * cosLat,
                     sinLat,
                     sinAzimuth * cosLat);
}

// Rotate the input point around the y-axis by the angle given as a 
// _cosf(angle) and _sinf(angle) argument.  There are many times where 
// I want to reuse the same angle on different points, so why do the 
// heavy trig twice.
// Range of outputs := ([-1.0f,-1.0f,-1.] -> [1.,1.0f,1.])
__DEVICE__ float3 rotateAroundYAxis( float3 point, float cosangle, float sinangle )
{
    return to_float3(point.x * cosangle  + point.z * sinangle,
                     point.y,
                     point.x * -sinangle + point.z * cosangle);
}

// Rotate the input point around the x-axis by the angle given as a 
// _cosf(angle) and _sinf(angle) argument.  There are many times where 
// I want to reuse the same angle on different points, so why do the 
// heavy trig twice.
// Range of outputs := ([-1.0f,-1.0f,-1.] -> [1.,1.0f,1.])
__DEVICE__ float3 rotateAroundXAxis( float3 point, float cosangle, float sinangle )
{
    return to_float3(point.x,
                     point.y * cosangle - point.z * sinangle,
                     point.y * sinangle + point.z * cosangle);
}

// Returns the floor and ceiling of the given float
__DEVICE__ float2 floorAndCeil( float _x ) 
{
    return to_float2(_floor(_x), _ceil(_x));
}

// Returns the floor and ceiling of each component in respective
// order (_floor(p.x), _ceil(p.x), _floor(p.y), _ceil(p.y)) 
__DEVICE__ float4 floorAndCeil2( float2 p ) 
{
    return to_float4_f2f2(floorAndCeil(p.x), 
                          floorAndCeil(p.y));
}

// Returns 2 floats, the first is the rounded value of float.  The second
// is the direction in which the rounding took place.  So if rounding 
// occurred towards a larger number 1 is returned, otherwise -1 is returned. 
__DEVICE__ float2 round_and_dir( float _x ) 
{
    return to_float2(_floor(_x+0.5f), sign_f(fract(_x)-0.5f));
}

// Returns 4 floats, 
// the first is the rounded value of p.x
// the second is the direction in which the rounding took place for p.x
// So if rounding occurred towards a greater number 1 is returned, 
// otherwise -1 is returned
// the third is the rounded value of p.y.  
// the fourth is the direction in which the rounding took place for p.y
__DEVICE__ float4 round_and_dir2( float2 p ) 
{
    return to_float4_f2f2(round_and_dir(p.x), round_and_dir(p.y));
}

// **************************************************************************
// NOISE/FRACTAL FUNCTIONS
// **************************************************************************

// Periodic saw tooth function that repeats with a period of 
// 4 and ranges from [-1, 1].  
// The function starts out at 0 for x=0,
//  raises to 1 for x=1,
//  drops to 0 for x=2,
//  continues to -1 for x=3,
//  and then rises back to 0 for x=4
// to complete the period

__DEVICE__ float sawtooth( float _x )
{
    float xmod = mod_f(_x+3.0f, 4.0f);
    return _fabs(xmod-2.0f) - 1.0f;
}

// **************************************************************************
// INTERSECT UTILITIES
// **************************************************************************

// intersection for a sphere with a ray. In the case where a ray hits, there 
// is gauranteed to be a tmin and tmax return value even if one of the hits
// is "behind" the ray origin. If both hits are behind the origin, no hit
// takes place.

// Returns a float3 where:
//  result.x = 1.0f or 0.0f to indicate if a hit took place
//  result.y = tmin
//  result.z = tmax

__DEVICE__ float3 intersectSphere(float3 rayOrigin,                 
                                  float3 rayDir, 
                                  float radius,
                                  float3 sphereCenter)
{

    // Calculate the ray origin in object space of the sphere
    float3 ospaceRayOrigin = rayOrigin - sphereCenter;

    // We don't consider an intersection if the ray is inside the sphere

    // DEBRANCH
    // Equivalent to:
    // if (dot(ospaceRayOrigin, ospaceRayOrigin) < radius*radius) {
    //     return to_float3_s(0.0f);
    // }
    
    float a = dot(rayDir, rayDir);
    float b = 2.0f*dot(ospaceRayOrigin, rayDir);
    float c = dot(ospaceRayOrigin, ospaceRayOrigin) - radius*radius;
    float discr = b*b - 4.0f*a*c; // discriminant

    float tmin = 0.0f;
    float tmax = 0.0f;

    // DEBRANCH
    // Equivalent to:
    // if (discr > 0.0f) {
    //     ...
    // }

    float isdiscrgtZero = step(0.0f, discr);

    // Real root of disc, so intersection
    // avoid taking square root of negative discriminant.
    float sdisc = _sqrtf(_fabs(discr));
    tmin = (-b - sdisc)/(2.0f * a);
    tmax = (-b + sdisc)/(2.0f * a); 

    float hit = isdiscrgtZero * _fmaxf( step(0.0f, tmin), step(0.0f, tmax) );

    return to_float3(hit, tmin, tmax);
}

// Reference: http://geomalgorithms.com/a05-_intersect-1.html. Does an
// intersection test against a plane that is assumed to be double sided and
// passes through the origin and has the specified normal.

// Returns a float2 where:
//   result.x = 1.0f or 0.0f if there is a hit
//   result.y = t such that origin + t*dir = hit point
__DEVICE__ float2 intersectDSPlane(float3 origin,
                      float3 dir,
                      float3 planeNormal,
                      float3 planeOffset)
{
    float dirDotN = dot(dir, planeNormal);
    // if the ray direction is parallel to the plane, let's just treat the 
    // ray as intersecting *really* far off, which will get culled as a
    // possible intersection.

    float denom = zeroTolerantSign(dirDotN) * _fmaxf(_fabs(dirDotN), EPSILON);
    float t = _fminf(BIG_FLOAT, -dot(planeNormal, (origin - planeOffset)) / denom);    
    return to_float2(step(EPSILON, t), t);

}

// Reference: http://geomalgorithms.com/a05-_intersect-1.html Does an
// intersection test against a plane that is assumed to  be single sided and
// passes through a planeOffset and has the specified normal.

// References: 
// http://www.geometrictools.com/Documentation/IntersectionLineCone.pdf
// http://www.geometrictools.com/LibMathematics/Intersection/Intersection.html
//
// Does an intersection with an infinite cone centered at the origin and 
// oriented along the positive y-axis extending to infinite.  Returns the 
// minimum positive value of t if there is an intersection.  

// This function works by taking in the latitudinal parameters from -PI/2 to
// PI/2 which correspond to the latitudinal of the x-z plane.  The reference
// provided assumes the angle of the cone is determined based on the cone angle
// from the y-axis.  So instead of using a cosine of the angle as described in
// the reference, we use a sine of the angle and also consider whether the
// angle is positive or negative to determine which side of the pooled cone
// we are selecting.
//
// If the angle of the z-x plane is near zero, this function just does a an
// intersection against the x-y plane to handle the full range of possible
// input.  This function clamps any angles between -PI/2 and PI/2.

// Trying to DEBRANCH this code results in webgl behaving funny in MacOS
// XXX: Any help fixing this is appreciated

// Returns a float2 where:
//   result.x = 1.0f or 0.0f if there is a hit
//   result.y = t such that origin + t*dir = hit point

__DEVICE__ float2 intersectSimpleCone(float3 origin,
                         float3 dir,
                         float coneAngle)
{
    
    // for convenience, if coneAngle ~= 0.0f, then intersect
    // with the x-z plane.      
    float axisDir = zeroTolerantSign(coneAngle);
    float clampedConeAngle = clamp(_fabs(coneAngle), 0.0f, PI/2.0f);    
    
    float t = 0.0f;

    if (clampedConeAngle < EPSILON) {
        t = -origin.y / dir.y;
        return to_float2(step(0.0f, t), t);
    }
    
    // If coneAngle is 0, assume the cone is infinitely thin and no
    // intersection can take place
    if (clampedConeAngle < EPSILON) {
        return to_float2_s(0.0f);
    }
    
    float sinAngleSqr = _sinf(clampedConeAngle);
    sinAngleSqr *= sinAngleSqr;

    // Quadric to solve in order to find values of t such that 
    // origin + direction * t intersects with the pooled cone.
    // 
    // c2 * t^2 + 2*c1*t + c0 = 0
    //
    // This is a little math trick to get rid of the constants in the 
    // classic equation t = ( -b +- _sqrtf(b^2 - 4ac) ) / 2a
    // by making b = 2*c1, you can see how this helps divide out the constants
    // t = ( -2 * c1 +- _sqrtf( 4 * c1^2 - 4 * c2c0) / 2 * c2
    // see how the constants drop out so that
    // t = ( -c1 +- _sqrtf( c1^2 - c2 * c0)) / c2

    // Lots of short cuts in reference intersection code due to the fact that
    // the cone is at the origin and oriented along positive y axis.
    // 
    // A := cone aligned axis = to_float3(0.0f, 1.0f, 0.0f)
    // E := origin - coneOrigin = origin
    // AdD := dot(A, dir) = dir.y
    // AdE := dot(A, E) = origin.y
    // DdE := dot(dir, E) = dot(dir, origin)
    // EdE := dot(E, E) = dot(origin, origin)
    
    float DdO = dot(dir, origin);
    float OdO = dot(origin, origin);
    
    float c2 = dir.y * dir.y - sinAngleSqr;
    float c1 = dir.y * origin.y - sinAngleSqr * DdO;
    float c0 = origin.y * origin.y - sinAngleSqr * OdO;

    float discr = c1*c1 - c2*c0;
    float hit = 0.0f;

    // if c2 is near zero, then we know the line is tangent to the cone one one
    // side of the pool.  Need to check if the cone is tangent to the negative
    // side of the cone since that could still intersect with quadric in one
    // place.
    if (_fabs(c2) > EPSILON) {
    
        if (discr < 0.0f) {
            
            return to_float2_s(0.0f);
            
        } else {
    
            // Real root of disc, so intersection may have 2 solutions, fine the 
            // nearest one.
            float sdisc = _sqrtf(discr);
            float t0 = (-c1 - sdisc)/c2;
            float t1 = (-c1 + sdisc)/c2;
    
            // a simplification we can make since we know the cone is aligned
            // along the y-axis and therefore we only need to see how t affects
            // the y component.
            float intersectPt0y = origin.y + dir.y * t0;
            float intersectPt1y = origin.y + dir.y * t1;    
            
            // If the intersectPts y value is greater than 0.0f we know we've
            // intersected along the positive y-axis since the cone is aligned
            // along the y-axis
    
            // If the closest intersection point is also a valid intersection
            // have that be the winning value of t.
            
            if ((t0 >= 0.0f) &&
                (axisDir * intersectPt0y > 0.0f)) {
                t = t0;
                hit = 1.0f;
            }
            
            if ((t1 >= 0.0f) &&
                ((t1 < t0) || (hit < 0.5f)) &&
                (axisDir * intersectPt1y > 0.0f)) {
                t = t1;     
                hit = 1.0f;
            }
    
        } 

    } else if (_fabs(c1) > EPSILON) {
        // This is the code to handle the case where there  is a ray that is on
        // the pooled cone and intersects at the very tip of the cone at the
        // origin.
        float t0 = -0.5f * c0 / c1;
        
        float intersectPty = origin.y + dir.y * t0;
        if (( t0 >= 0.0f) && (axisDir * intersectPty > 0.0f)) {
            t = t0;
            hit = 1.0f;
        }       
    }
    
    return to_float2(hit, t);

}

// Returns the vector that is the shortest path from the bounded line segment
// u1->u2 to the line represented by the line passing through v1 and v2.  
//
// result.x is the distance of the shortest path between the line segment and
// the unbounded line
//
// result.y = is the value of t along the line segment of ba [0,1] that 
// represents the 3d point (p) of the shortest vector between the two line 
// segments that rests on the vector between u1 anb u2 such that 
//    p = u1 + (u2-u1) * t
//
// result.z = is the value of t along the line passing through v1 and v2 such
// that q represents the closest point on the line to the line segment u2<-u1:
//    q = v1 + (v2-v1) * t
// t is unbounded in this case but is parameterized such that t=0 at v1 and t=1
// at v2.

__DEVICE__ float3 segmentToLineDistance( float3 u1, 
                            float3 u2, 
                            float3 v1, 
                            float3 v2 )
{
    float3 u = u2 - u1;
    float3 v = v2 - v1;
    float3 w = u1 - v1;
    
    // For the maths:
    // http://geomalgorithms.com/a07-_distance.html#dist3D_Segment_to_Segment
    float a = dot(  u, u );
    float b = dot(  u, v ); 
    float c = dot(  v, v );   
    float d = dot(  u, w );
    float e = dot(  v, w ); 
    
    // just a way of calculating two equations with one operation.
    // th.x is the value of t along ba
    // th.y is the value of t along wv 

    // when a*c - b*b is near 0 (the lines are parallel), we will just assume
    // a close line is the distance between u1 and v1
    float denom = (a * c - b * b);

    // DEBRANCHED
    // Equivalent to:
    // float2 th = (_fabs(denom) < EPSILON ? to_float2_s(0.0f) : 
    //                                  to_float2( b*e - c*d, a*e - b*d ) / denom);

    float clampedDenom = sign_f(denom) * _fmaxf(EPSILON, _fabs(denom));
    float2 th = _mix( to_float2( b*e - c*d, a*e - b*d ) / clampedDenom, 
                      to_float2_s(0.0f),
                      step(_fabs(denom), EPSILON));

    // In the case where the line to line comparison has p be a point that lives
    // off of the bounded segment u2<-u1, just fine the closest path between u1
    // and u2 and pick the shortest

    float ifthxltZero = step(th.x, 0.0f);
    float ifthxgt1 = step(1.0f, th.x);

    // DEBRANCHED
    // Equivalent to:
    // if (th.x < 0.0f) {
    //     th.x = 0.0f;
    //     th.y = dot(v, u1-v1) / c; // v . (u1<-v1) / v . v
    // } else if (th.x > 1.0f) {
    //     th.x = 1.0f;
    //     th.y = dot(v, u2-v1) / c; // v . (u2<-v1) / v . v
    // }
    
    th.x = clamp(th.x, 0.0f, 1.0f);
    th.y = _mix(th.y, dot(v, u1-v1) / c, ifthxltZero);
    th.y = _mix(th.y, dot(v, u2-v1) / c, ifthxgt1);
    
    // p is the nearest clamped point on the line segment u1->u2
    float3 p = u1     + u  * th.x;
    // q is the nearest unbounded point on the line segment v1->v2
    float3 q = v1     + v  * th.y;
    
    return to_float3(length(p-q), th.x, th.y);
}


// Returns the vector that is the shortest path from the 3D point to the line
// segment as well as the parameter t that represents the length along the line
// segment that p is closest to.

// Returned result is:

// swi3(result,x,y,z) := vector of the path from p to q where q is defined as the point
// on the line segment that is closest to p.
// result.w   := the t parameter such that a + (b-a) * t = q 

__DEVICE__ float4 segmentToPointDistance( float3 a, 
                             float3 b, 
                             float3 p)
{
    
    float3 ba = b - a;    
    float t = dot(ba, (p - a)) / _fmaxf(EPSILON, dot(ba, ba));
    t = clamp(t, 0.0f, 1.0f);
    float4 result = to_float4_aw(ba * t + a - p, t);
    return result;
}

// Returns the vector that is the shortest path from the 3D point to the  line
// that passes through a and b as well as the parameter t that represents the
// length along the line that p is closest to.

// Returned result is:

// swi3(result,x,y,z) := vector of the path from p to q where q is defined as the point
// on the line segment that is closest to p.
// result.w   := the t parameter such that a + (b-a) * t = q 

__DEVICE__ float4 lineToPointDistance( float3 a, 
                          float3 b, 
                          float3 p)
{
    
    float3 ba = b - a;    
    float t = dot(ba, (p - a)) / dot(ba, ba);
    float4 result = to_float4_aw(ba * t + a - p, t);
    return result;
}

// **************************************************************************
// SHADING UTILITIES
// **************************************************************************

// Approximating a dialectric fresnel effect by using the schlick approximation
// http://en.wikipedia.org/wiki/Schlick's_approximation. Returns a float3 in case
// I want to approximate a different index of reflection for each channel to
// get a chromatic effect.
__DEVICE__ float3 fresnel(float3 I, float3 N, float eta)
{
    // assume that the surrounding environment is air on both sides of the 
    // dialectric
    float ro = (1.0f - eta) / (1.0f + eta);
    ro *= ro;
    
    float fterm = _powf(1.0f - _fmaxf(dot(-I, N), 0.0f), 5.0f);  
    return to_float3_s(ro + ( 1.0f - ro ) * fterm); 
}

// **************************************************************************
// TRACE UTILITIES
// **************************************************************************

// This ray structure is used to march the scene and accumulate all final
// results.

struct SceneRayStruct {
    float3 origin;        // origin of the ray - updates at refraction, 
                        // reflection boundaries
    float3 dir;           // direction of the ray - updates at refraction, 
                        // reflection boundaries

    float marchDist;    // current march distance of the ray - updated during
                        // tracing
    float marchMaxDist; // max march distance of the ray - does not update

    float3 cellCoords; // current box being marched through - updated during
                        // marching

    float4 orbHitPoint;   // xyz is the point on the orb that hit point took
                        // place.  The w value is 0 or 1 depending on if there
                        // was in fact a hit on an orb.

    float4 color;         // accumulated color and opaqueness of the ray - updated
                        // during tracing

    float4 debugColor;    // an alternative color that is updated during
                        // traversal if it has a non-zero opaqueness, then it
                        // will be returned to the frame buffer as the current
                        // frag color. Alpha is ignored when displaying to the 
                        // frag color.
};

// This ray structure is used to march through a space that represents the
// interior of the orbs and is chopped up into cells whose boundaries are
// aligned with the azimuthal and latitudinal angles of the sphere.

// Looking at a cross section of the orb from the azimuthal direction (from
// top down). You can see how in this case of a sphere divided into 8
// subdomains, the cell coordinates wrap at the back of the sphere.

//
//        . * ' * .
//      *   8 | 1   *
//    *  \    |    /  *
//   /  7  \  |  /  2  \
//  *________\|/________*
//  *        /|\        *
//   \  6  /  |  \  3  /
//    *  /    |    \  *
//      *   5 | 4   *
//        ' * . * ' 
//
//        V Front V


// Looking at a cross section of the orb from the side split down the center.
// You can see in this example of the latitudinal cells having 4 subdomains
// (or cells), the latitudinal cells are divided by cones originating from the
// sphere center.

//
//        . * ' * .
//      * \   4   / *
//    *    \     /    *
//   /  3   \   /   3  \
//  *________\ /________*
//  *        / \        *
//   \  2   /   \   2  /
//    *    /     \    *
//      * /   1   \ *
//        ' * . * '
//
//        V Bottom V

struct OrbRayStruct {
    float3 origin;                // origin of the ray to march through the
                                  // orb space - does not update
    
    float3 dir;                   // direction of the ray to march through the
                                  // orb space - does not update

    float marchDist;              // current march distance of the ray through
                                  // the sphere - updated during tracing

    float marchNextDist;          // represents the next extent of the current
                                  // cell so we can create a line segment on
                                  // the ray that represents the part of the
                                  // line that is clipped by the orb cell
                                  // boundaries.  We then use this line
                                  // segment to perform a distance test  of
                                  // the spark

    float marchMaxDist;           // max march distance of the ray - does not 
                                  // update represents the other side the sphere
 
    int azimuthalOrbCellMarchDir; // when marching through the orb cells, we
                                  // keep track of which way the ray is going
                                  // in the azimuthal direction, so we only
                                  // need to test against one side of the
                                  // orb cell - a plane that lies on the
                                  // y-axis and is rotated  based on the
                                  // current cell.  This avoids precision
                                  // issues calculated at beginning of
                                  // traversal and used through out the
                                  // iterations.

    float2 orbCellCoords;         // Keep track of the current orb cell
                                  // we're in: x is the latitudinal cell number
                                  // y is the azimuthal cell number

    float orbHash;                // Hash to this particular orb so we can use
                                  // it as the unique identifier to key into 
                                  // variation based signals.

    float4 color;                 // accumulated color and opaqueness of the ray
                                  // marching the orbs.

    float4 debugColor;            // an alternative color that is updated during
                                  // traversal if it has a non-zero opaqueness,
                                  // then it will be returned to the frame
                                  // buffer as the current frag color

};

// Returns the distance field to the cell center corresponding to the 
// cell coordinate plus the neighbor offset, which should only have 
// integer values in the range ([-1,-1,-1], [1,1,1])
//
// swi3(result,x,y,z) = p relative to the neighbor orb center
// result.w   = distance to orb exterior 
__DEVICE__ float4 currCellOrbDF( float3 modp, 
                    float3 cellCoords, 
                    float3 neighborOffset,
                    float g_cellBoxHalfSize,
                    float g_time,
                    float g_orbOuterRadius)
{

    float maxOffset = g_cellBoxHalfSize - ORB_OUTER_RADIUS_DEFAULT;
    float3 currCellCoords = cellCoords + neighborOffset;  

    // Add a subtle sinuisoidal wave to sorb center.
    float3 noiseSeed = to_float3(dot(to_float3(5.3f, 5.0f, 6.5f), currCellCoords),
                                 dot(to_float3(6.5f, 5.3f, 5.0f), currCellCoords),
                                 dot(to_float3(5.0f, 6.5f, 5.3f), currCellCoords));
    
    noiseSeed += 0.5f * to_float3(0.5f, 0.9f, 1.2f) * g_time;
                          
    float3 currOffset = to_float3_s(maxOffset) * cos_f3(noiseSeed);     

    float3 currSphCenter = 2.0f * g_cellBoxHalfSize * neighborOffset + currOffset;
    float3 sphereRelativeP = modp - currSphCenter;

    float dist = length(sphereRelativeP) - g_orbOuterRadius;    

    return to_float4_aw(sphereRelativeP, dist);   
}

// Returns the distance field to the nearest orb shell plus the position of
// p relative to the center of the orb.
//
// swi3(result,x,y,z) = p relative to orb center
// result.w   = distance to orb exterior 
__DEVICE__ float4 orbDF( float3 p, float3 rayDir, float3 cellCoords,
                    float g_cellBoxHalfSize,
                    float g_time,
                    float g_orbOuterRadius )
{

    // Look into this cell to check distance to that sphere and then check the
    // distance to the spheres for the next cells to see if they are closer,
    // we can guess the "next" cells by just looking at the sign of the ray
    // dir (so we don't check all 27 possible cells - 9x9x9 grid).  This
    // limits it down just to checking 8 possible cells.
    float3 modp = mod_f(p, 2.0f * g_cellBoxHalfSize) - g_cellBoxHalfSize;

    float4 nextCellOffset = to_float4_aw(sign_f3(rayDir), 0.0f);

    // 1.0f current cell
    float4 result = currCellOrbDF(modp, cellCoords, to_float3_s(0.0f), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
        
    // 2.0f neighbor in the x direction of the ray
    float4 neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,x,w,w), g_cellBoxHalfSize, g_time, g_orbOuterRadius);    
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 3.0f neighbor in the y direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,w,y,w), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 4.0f neighbor in the z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,w,w,z), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }
    
    // 5.0f neighbor in the x-y direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,x,y,w), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 6.0f neighbor in the y-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,w,y,z), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 7.0f neighbor in the x-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,x,w,z), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }
        
    // 8.0f neighbor in the x-y-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, swi3(nextCellOffset,x,y,z), g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    if (neighborResult.w < result.w) { result = neighborResult; }

    return result;
}

// Given a scene ray generated from a camera, trace the scene by first
// distance field marching.

__DEVICE__ float traceScene(inout SceneRayStruct *sceneRay,
                    float g_cellBoxHalfSize,
                    float g_time,
                    float g_orbOuterRadius)
{

    // --------------------------------------------------------------------
    // How deep into the "infinite" cooridor of orbs do we want to go.  Each
    // iteration does distance field marching which will converge on the 
    // nearest orb or continue to the next.

    float3 marchPoint = (*sceneRay).origin;
    float dist = BIG_FLOAT;
    
    for (int iterations = 0; 
         iterations < MAX_NUM_TRACE_ITERATIONS; 
         iterations += 1) {
        
        if (((*sceneRay).marchMaxDist < (*sceneRay).marchDist) ||
            (dist < 0.01f)) {
            continue;
        }  
                
        // used as a way to hash the current cell
        (*sceneRay).cellCoords = _floor(marchPoint /(2.0f * g_cellBoxHalfSize));

        // scene map
        float4 orbDFresult = orbDF( marchPoint, 
                                  (*sceneRay).dir,
                                  (*sceneRay).cellCoords, g_cellBoxHalfSize, g_time, g_orbOuterRadius );
        dist = orbDFresult.w;

        if (dist < 0.01f) {

            swi3S((*sceneRay).orbHitPoint,x,y,z, swi3(orbDFresult,x,y,z));
            (*sceneRay).orbHitPoint.w = 1.0f;
        }
        
        (*sceneRay).marchDist += dist;
        marchPoint += dist * (*sceneRay).dir; 
        
    }            

    return (*sceneRay).orbHitPoint.w;
}


// Function used for ray marching the interior of the orb.  The sceneRay is 
// also passed in since it may have some information we want to key off of
// like the current orb cell coordinate.
//
// Returns the OrbRayStruct that was passed in with certain parameters
// updated.
__DEVICE__ void shadeOrbInterior(inout OrbRayStruct *orbRay, in SceneRayStruct sceneRay, 
                                float2 g_sparkRotationRate, float g_time, float2 g_numOrbCells, float g_orbCenterRadius, float g_orbOuterRadius, float g_sparkWidth, 
                                float g_beatRate, float g_audioFreqs[NUM_AUDIO_SAMPLE_TRIPLES], float g_audioResponse, float g_sparkColorIntensity, float3 g_sparkCoreColor, 
                                float3 g_sparkExtentColor )
{    
    
    // Before marching into the orb coordinates, first determine if the orbRay
    // is casting in the direction of the sphere where the "angle" is increasing
    // along the azimuthal direction, in which case we assume we are marching
    // along the azimuthal cells in a positive direction.

    float3 orbRayEnd = (*orbRay).origin + (*orbRay).dir * (*orbRay).marchMaxDist;
    float4 ltpresult = lineToPointDistance((*orbRay).origin, orbRayEnd, to_float3_s(0.0f));
    float3 ltp = swi3(ltpresult,x,y,z);
    float3 rdircrossltp = normalize(cross((*orbRay).dir, ltp));
        
    (*orbRay).azimuthalOrbCellMarchDir = (dot(rdircrossltp, 
                                          to_float3(0.0f, 1.0f, 0.0f)) > 0.0f) ? 1 : -1;  


    // ------------------------------------------------------------
    // Convert the cartesian point to orb space within the range
    // Latitudinal    := [-PI/2, PI/2]
    // Azimuth        := [-PI, PI]
    float2 orbRayOriginPt = cartesianToPolar((*orbRay).origin);                
    
    //
    // convert orb coordinates into number of spark domains       
    // Latitudinal    := [-PI/2, PI/2]
    // Azimuth        := [-PI, PI]
    // LatdDomains    := [-numLatdDomains/2.0f, numLatdDomains/2.] 
    // AzimuthDomains := [-numAzimDomains/2.0f, numAzimDomains/2.]
    // In order to avoid confusion        

    orbRayOriginPt -= g_sparkRotationRate * g_time;
    orbRayOriginPt *= g_numOrbCells * to_float2(1.0f/PI, 1.0f/TWO_PI);

    float4 rOrbRayOriginPtResults = round_and_dir2(orbRayOriginPt);
    float2 cellCoords = (swi2(rOrbRayOriginPtResults,x,z));                        
    cellCoords -= to_float2(0.5f,0.5f) * swi2(rOrbRayOriginPtResults,y,w);

    (*orbRay).orbCellCoords = cellCoords;

    // ------------------------------------------------------------
    // March through the orb cells a fixed number of steps.

    for ( int i = 0; i < NUM_POLAR_MARCH_STEPS; i++)
    {
        // If orbRay color has near opaque opacity or we've reached the extent of 
        // the orb, no need to continue marching.  Use continue instead of break
        // since that appears to be more optimal.
        if (_fmaxf(step((*orbRay).marchMaxDist, (*orbRay).marchDist),
                   step(0.95f, (*orbRay).color.w)) > 0.5f) {
            continue;
        }
        
        float3 marchPoint = (*orbRay).origin + (*orbRay).dir * (*orbRay).marchDist;
        
        // Convert the cartesian point to orb space within the range
        // Latitudinal    := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI]
        float2 orbMarchPt = cartesianToPolar(marchPoint);                
        
        // convert orb coordinates into number of cell domains       
        // From:
        // Latitudinal    := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI]
        // To:
        // LatdDomains    := [-numLatdDomains/2.0f, numLatdDomains/2.]        
        // AzimuthDomains := [-numAzimDomains/2.0f, numAzimDomains/2.]

        orbMarchPt -= g_sparkRotationRate * g_time;
        orbMarchPt *= g_numOrbCells * to_float2(ONE_OVER_PI, ONE_OVER_TWO_PI);

        float2 orbSparkStart = (*orbRay).orbCellCoords;     
        
        // convert back into orb coordinate range 
        // Latitudinal      := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI] 
        float2 remapOrbSparkStart = orbSparkStart * to_float2(PI, TWO_PI) / 
                                            g_numOrbCells;
                
        remapOrbSparkStart += g_sparkRotationRate * g_time;     
                
        // ------------------------------------------------------------------
        // Spark color contribution

        // create the properties of this spark
        float3 sparkDir = polarToCartesian(remapOrbSparkStart);
        float3 sparkOrigin = g_orbCenterRadius * sparkDir;
        float3 sparkEnd = g_orbOuterRadius * sparkDir;
                
        // test this marches closeness to the spark and use that to
        // drive spark presence.
        float3 sparkResult = segmentToLineDistance(sparkOrigin,
                                                 sparkEnd,
                                                 (*orbRay).origin,
                                                 orbRayEnd);

        float3 sparkColor = to_float3_s(0.0f);
        float sparkOpacity = 0.0f;        
        
        // As the orbs are traced farther into the distance, increase the glow extent
        // and alpha extent to account for filtering.

        float distanceLerp = smoothstep(100.0f, 600.0f, sceneRay.marchDist);
        float sparkGlowExtent = _fmaxf(g_sparkWidth, _mix(0.5f, 3.0f, distanceLerp));
        float sparkAlphaExtent = 1.2f * sparkGlowExtent;
        float sparkAttenuate = _fabs(sparkResult.x);

        // Use triples to compress the expensive operations by using their float3
        // versions.
        float3 audioAttenuateTriples = to_float3_s(0.0f);
        
        // Determine how the audio signals drive this spark.  Each audio sample is
        // "traveling" through the spark of the orbs from top down with different offsets.
        for (int i = 0; i < NUM_AUDIO_SAMPLE_TRIPLES; i += 1) {
            float3 indexes = 3.0f * to_float3((float)(i) + 0.0f, 
                                              (float)(i) + 1.0f,
                                              (float)(i) + 2.0f);

            float3 sparkPopTravelSeed = 10.0f * indexes + (*orbRay).orbHash + (*orbRay).orbCellCoords.x + g_beatRate * 3.0f * g_time;                                  
            sparkPopTravelSeed = smoothstep(to_float3_s(15.0f), 
                                            to_float3_s(0.0f), 
                                            mod_f3(sparkPopTravelSeed, 
                                            5.0f * 3.0f * (float)(NUM_AUDIO_SAMPLE_TRIPLES)));

            sparkPopTravelSeed = smoothstep(to_float3_s(0.0f), 
                                            to_float3_s(0.75f),  
                                            pow_f3(sparkPopTravelSeed, to_float3_s(1.5f)) );

            audioAttenuateTriples += g_audioFreqs[i] * sparkPopTravelSeed;
        }
        
        // Take the sum of all of the audio attnuate triples for this
        // spark.
        float audioAttenuate = dot(audioAttenuateTriples, to_float3_s(1.0f));

        audioAttenuate = _mix(1.0f, _fminf(1.0f, audioAttenuate), g_audioResponse);
    
        sparkAttenuate = _mix(1.2f * sparkAlphaExtent, sparkAttenuate, audioAttenuate);

        // The core presence has a longer throw then the spark itself, which helps it
        // pop against sparks in the bg.
        float sparkCorePresence = smoothstep(sparkGlowExtent, 0.0f, sparkAttenuate);
        float sparkAmt = smoothstep(sparkAlphaExtent, 0.0f, sparkAttenuate);        

        sparkColor = g_sparkColorIntensity * _mix(g_sparkCoreColor, 
                                                 g_sparkExtentColor, 
                                                 sparkResult.y);

        // Pump this spark based on the amount of audio passing through 
        // the spark.
        sparkColor = _mix(sparkColor, 
                         1.25f * sparkColor, 
                         g_audioResponse * audioAttenuate);

        sparkColor = (1.0f - (*orbRay).color.w) * sparkColor * sparkCorePresence;
        sparkOpacity = (1.0f - (*orbRay).color.w) * sparkAmt;        

        // ------------------------------------------------------------------
        // Compute the cell bound marching
        
        // Test the boundary walls of a polar cell represented by the floor or
        // ceil of the polarized march pt.  The boundary walls to test are
        // determined based on how we're marching through the orb cells.

        // Remember that x is the latitudinal angle so to find it's boundary,
        // we intersect with a cone.  y is the azimuth angle so we can more
        // simply intersect the plane that is perpendicular with the x-z plane
        // and rotated around the y-axis by the value of the angle.

        // Remember to remap the floors and ceils to   
        // Latitudinal      := [-PI/2, PI/2]
        // Azimuth          := [-PI, PI]
        float4 orbCellBounds = floorAndCeil2((*orbRay).orbCellCoords) *
                               swi4(to_float2(PI, TWO_PI),x,x,y,y) / swi4(g_numOrbCells,x,x,y,y);
        
        orbCellBounds += swi4(g_sparkRotationRate,x,x,y,y) * g_time;
        
        float nextRelativeDist = (*orbRay).marchMaxDist - (*orbRay).marchDist;
        float t = BIG_FLOAT;        
        float2 cellCoordIncr = to_float2_s(0.0f);

        // Intersect with the planes passing through the origin and aligned
        // with the y-axis.  The plane is a rotation around the y-axis based
        // on the  azimuthal boundaries.  Remember we know which direction the
        // ray is traveling so we only need to test one side of the cell.        
        float orbNextCellAzimAngle = ((*orbRay).azimuthalOrbCellMarchDir < 0 ? 
                                      orbCellBounds.z : orbCellBounds.w);

        float3 orbNextCellAzimBound = to_float3(-_sinf(orbNextCellAzimAngle), 
                                                0.0f, 
                                                _cosf(orbNextCellAzimAngle));

        float2 intersectResult = intersectDSPlane(marchPoint, (*orbRay).dir, 
                                                orbNextCellAzimBound, to_float3_s(0.0f) );

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5f) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = to_float2(0.0f, orbRay.azimuthalOrbCellMarchDir);            
        // }

        float isAzimPlaneHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = _mix(nextRelativeDist, intersectResult.y, isAzimPlaneHit);
        cellCoordIncr = _mix(cellCoordIncr, to_float2(0.0f, (*orbRay).azimuthalOrbCellMarchDir), isAzimPlaneHit);
        
        // XXX Future work: It would be nice if we only test one side of the
        // cell wall in the latitudinal direction based on the direction of the
        // orb ray as it marches through the cells, like what I'm doing with the
        // azimuthal direction. But due to some shader issues and the extra
        // complexity this adds to the  code, this seems like a more stable
        // approach.  If we do go down this road, you could determine which
        // direction the ray is travelling but you'll need to test if that ray
        // crosses the "dividing plane" since the "negative" direction will
        // become the "positive" direction at  that point.  Perhaps this
        // indicates I need to rethink how the orb cell coordinates are
        // defined.  I think solving this problem will get to the bottom of the
        // black speckling that happens.

        // Test the top of the current orb cell
        intersectResult = intersectSimpleCone(marchPoint, (*orbRay).dir, orbCellBounds.x);

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5f) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = to_float2(-1.0f, 0.0f);            
        // }
        
        float isTopConeHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = _mix(nextRelativeDist, intersectResult.y, isTopConeHit);
        cellCoordIncr = _mix(cellCoordIncr, to_float2(-1, 0.0f), isTopConeHit);
    

        // Test the bottom of the current orb cell
        intersectResult = intersectSimpleCone(marchPoint, (*orbRay).dir, orbCellBounds.y);

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5f) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = to_float2(-1.0f, 0.0f);            
        // }

        float isBottomConeHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = _mix(nextRelativeDist, intersectResult.y, isBottomConeHit);
        cellCoordIncr = _mix(cellCoordIncr, to_float2(1, 0.0f), isBottomConeHit);
        

        // ------------------------------------------------------------------
        // Update orbRay for next march step

        // We now know what cell we're going to march into next
        // XXX: There is a fudge factor on the EPSILON here to get around some
        // precision issues we're seeing with intersecting the simple cones. 
        // This probably indicates there is something flawed in the cell logic
        // traversal.
        (*orbRay).marchNextDist = _fminf((*orbRay).marchMaxDist, (*orbRay).marchDist + nextRelativeDist + 0.01f);
        (*orbRay).orbCellCoords += cellCoordIncr;

        // Make sure that y wraps (the azimuthal dimension) when you've reached
        // the extent of the number of orb cells
        (*orbRay).orbCellCoords.y = mod_f((*orbRay).orbCellCoords.y + g_numOrbCells.y/2.0f, 
                                          g_numOrbCells.y) - g_numOrbCells.y/2.0f;
        
        (*orbRay).marchDist = (*orbRay).marchNextDist;        
        
        // ------------------------------------------------------------------
        // Shade the center of the orb interior itself (only if this cell's
        // march has intersected with that center interior).  Avoid an if 
        // statement here by having the signal that drives the presence of 
        // this color come on only when the ray is intersecting that orb
        // interior.

        float3 marchExit = (*orbRay).origin + (*orbRay).dir * (*orbRay).marchNextDist;  

        float4 segToPtResult = segmentToPointDistance(marchPoint,
                                                    marchExit,
                                                    to_float3_s(0.0f));
        
        float distToCenter = length(swi3(segToPtResult,x,y,z));

        float centerOrbProximityMask = smoothstep(g_orbCenterRadius + 1.0f, 
                                                  g_orbCenterRadius, 
                                                  distToCenter);

        float sparkOriginGlowMaxDist = _mix(3.0f, 0.5f, 
                                           smoothstep(8.0f, 
                                                      128.0f, 
                                                      _fminf(g_numOrbCells.x, 
                                                             g_numOrbCells.y)));
        
        float sparkOriginGlow = _fminf(sparkOriginGlowMaxDist, _fabs(sparkOriginGlowMaxDist - 
                                       distance_f3(swi3(segToPtResult,x,y,z), sparkOrigin)));
        
        sparkOriginGlow = audioAttenuate * _powf(sparkOriginGlow, 3.5f);

        // Spark origin contribution, color a signal based on this particular marches
        // closest point to the orb origin and that distance to the actual spark 
        // origin.
        sparkColor += 0.06f * centerOrbProximityMask * sparkOriginGlow * g_sparkCoreColor;
        sparkOpacity += 0.06f * centerOrbProximityMask * sparkOriginGlow;

        float isMarchIntersectingCenterOrb = smoothstep(g_orbCenterRadius + 0.1f, 
                                                        g_orbCenterRadius, distToCenter);

        float centerOrbGlow = 0.4f * g_sparkColorIntensity * _fmaxf(0.0f, 
                    1.0f - dot(-1.0f*normalize(swi3(segToPtResult,x,y,z)), (*orbRay).dir));

        centerOrbGlow = _powf(centerOrbGlow, 0.6f);

        // Center orb glow (diffuse falloff) - independent of sparks
        sparkColor += _fmaxf(0.0f, (1.0f - sparkOpacity)) * g_sparkCoreColor * centerOrbGlow * isMarchIntersectingCenterOrb;
                
        swi3S((*orbRay).color,x,y,z, swi3((*orbRay).color,x,y,z) + (1.0f - (*orbRay).color.w) * sparkColor);
        (*orbRay).color.w += (1.0f - (*orbRay).color.w) * sparkOpacity;
                                
        // If we want to terminate march (because we intersected the center orb), 
        // then make sure to make the alpha for this orb ray 1.0f 
        (*orbRay).color.w += (1.0f - (*orbRay).color.w) * isMarchIntersectingCenterOrb;        

    }
    
}

__DEVICE__ float3 sampleEnvironment(float3 normalizedDir, float g_bassBeat )
{

    float3 skyColor     = to_float3(0.55f, 0.45f, 0.58f);
    float3 horizonColor = to_float3(0.50f, 0.35f, 0.55f);

    float envAmp = 1.0f * (1.0f + 0.1f * g_bassBeat);
    return envAmp * _mix(horizonColor, skyColor, smoothstep(-0.5f, 1.0f, normalizedDir.y)); 
}

__DEVICE__ void simpleShadeOrb(inout SceneRayStruct *sceneRay)
{

    float3 hitPoint = swi3((*sceneRay).orbHitPoint,x,y,z);
    float3 rayDir   = (*sceneRay).dir;

    // We march the interior assuming the sphere is at the origin.        
    float3 hitNormal = normalize(hitPoint);

    float falloff = dot(hitNormal, -(*sceneRay).dir);
    swi3S((*sceneRay).color,x,y,z, to_float3_s(1.0f - falloff));
    (*sceneRay).color.w = 1.0f;
  
}

__DEVICE__ void shadeOrb(inout SceneRayStruct *sceneRay,
                         float2 g_sparkRotationRate, float g_time, float2 g_numOrbCells, float g_orbCenterRadius, float g_orbOuterRadius, float g_sparkWidth, 
                         float g_beatRate, float g_audioFreqs[NUM_AUDIO_SAMPLE_TRIPLES], float g_audioResponse, float g_sparkColorIntensity, float3 g_sparkCoreColor, 
                         float3 g_sparkExtentColor, float g_bassBeat,
                         float g_orbIOR, float g_orbSurfaceFacingKr, float g_orbSurfaceEdgeOnKr)
{

    // Create a unique hash for this particular cell. We're assuming
    // that the number of visible cells won't exceed 100x100x100.
    float orbHash = ((*sceneRay).cellCoords.x + 
                     100.0f * (*sceneRay).cellCoords.y + 
                     10000.0f * (*sceneRay).cellCoords.z);

    // --------------------------------------------------------------------
    // by rotating the hitPoint and the ray direction around the 
    // origin of the orb, we can add rotational variety from orb
    // to orb. 
    float3 hitPoint = swi3((*sceneRay).orbHitPoint,x,y,z);
    float3 rayDir   = (*sceneRay).dir;

    float rotateXAngle = 0.125f * g_time + orbHash;
    float cosRotateXAxis = _cosf(rotateXAngle);
    float sinRotateXAxis = _sinf(rotateXAngle);    
    
    hitPoint = rotateAroundXAxis(hitPoint, cosRotateXAxis, sinRotateXAxis);
    rayDir   = rotateAroundXAxis(rayDir, cosRotateXAxis, sinRotateXAxis);

    float rotateYAngle = 0.125f * g_time + orbHash;
    float cosRotateYAxis = _cosf(rotateYAngle);
    float sinRotateYAxis = _sinf(rotateYAngle);

    hitPoint = rotateAroundYAxis(hitPoint, cosRotateYAxis, sinRotateYAxis);
    rayDir   = rotateAroundYAxis(rayDir, cosRotateYAxis, sinRotateYAxis);
    
    // We march the interior assuming the sphere is at the origin.        
    float3 hitNormal = normalize(hitPoint);
    
    // --------------------------------------------------------------------
    // Environment map reflection on orb surface

    float3 reflectDir = reflect(rayDir, hitNormal);
    float3 reflColor = sampleEnvironment(reflectDir, g_bassBeat);
    
    float reflectRatio = fresnel(rayDir, hitNormal, 1.0f / g_orbIOR).x;
    float kr = _mix(g_orbSurfaceFacingKr, g_orbSurfaceEdgeOnKr, reflectRatio);
    swi3S((*sceneRay).color,x,y,z, swi3((*sceneRay).color,x,y,z) + kr * reflColor);
    
    (*sceneRay).color.w += (1.0f - (*sceneRay).color.w) * _fminf(1.0f, kr);//reflectRatio;

    // --------------------------------------------------------------------
    // Calculate the interior distance to march
     
    float3 refractDir = refract_f3(rayDir, hitNormal, 1.0f / g_orbIOR);    
        
    // Consider the interior sphere (the emitter) when ray marching
    float3 innerIntersectResult = intersectSphere(hitPoint, 
                                                refractDir, 
                                                g_orbCenterRadius,
                                                to_float3_s(0.0f));

    float interiorSphereHitDist = _mix(BIG_FLOAT, 
                                      innerIntersectResult.y, 
                                      innerIntersectResult.x);

    // --------------------------------------------------------------------
    // Orb Interior Shading     

    float3 orbIntersectResult = intersectSphere(hitPoint, 
                                              refractDir,
                                              g_orbOuterRadius,
                                              to_float3_s(0.0f));
    
    float sphereDepth = _fabs(orbIntersectResult.z - orbIntersectResult.y);    
    float rayExtent = _fminf(interiorSphereHitDist, sphereDepth);        

    OrbRayStruct orbRay = {hitPoint, // origin
                           refractDir, // dir
                           0.0f, // ray parameterized start
                           0.0f, // the next ray march step
                           rayExtent, // ray parameterized end
                           0, //  azimuthalOrbCellMarchDir
                           to_float2_s(0.0f), // orbCellCoords
                           orbHash, // unique float for this orb
                           to_float4_s(0.0f), // color
                           to_float4_s(0.0f)};  // debugColor 

    shadeOrbInterior(&orbRay, *sceneRay, g_sparkRotationRate, g_time, g_numOrbCells, g_orbCenterRadius, g_orbOuterRadius, g_sparkWidth, 
                                       g_beatRate, g_audioFreqs, g_audioResponse, g_sparkColorIntensity, g_sparkCoreColor, 
                                       g_sparkExtentColor); 

    // --------------------------------------------------------------------
    // Transfer the orb ray march results to the scene ray trace.

    swi3S((*sceneRay).color,x,y,z, swi3((*sceneRay).color,x,y,z) + (1.0f - (*sceneRay).color.w) * swi3(orbRay.color,x,y,z));
    (*sceneRay).color.w += (1.0f - (*sceneRay).color.w) * orbRay.color.w;

    (*sceneRay).debugColor += orbRay.debugColor;

}


// **************************************************************************
// MAIN

__KERNEL__ void FlutherOfOrbsJipi940Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, float iChannelTime[], sampler2D iChannel0)
{
  
  CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
  
  CONNECT_CHECKBOX0(Manual, 0); 
  
  CONNECT_SLIDER1(Bass,   -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER2(Audio1, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER3(Audio2, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Audio3, -10.0f, 10.0f, 0.0f);
  
  // GLOBALS

  // the beat electronebulae is about 126 beats per minute, so we some timings
  // based events based on multiples of that beat rate.

  // - - - - - - - - - - - - -
  // Audio based signals
  float g_beatRate = 126.0f/60.0f;
  float g_time = 0.0f;
  float g_audioResponse = 1.0f;

  // audio signals
  float g_bassBeat = 0.0f;
  float g_audioFreqs[ NUM_AUDIO_SAMPLE_TRIPLES ];

  // - - - - - - - - - - - - -
  // Cell walls properties

  
  float g_cellBoxHalfSize = CELL_BOX_HALF_SIZE_DEFAULT;

  // - - - - - - - - - - - - -
  // Fog properties

  float3 g_fogColor = to_float3(0.23f, 0.12f, 0.12f);

  // - - - - - - - - - - - - -
  // Orb properties

  //   dimensions of the orb

  float g_orbOuterRadius = ORB_OUTER_RADIUS_DEFAULT; // defaults
  float g_orbCenterRadius = ORB_CENTER_RADIUS_DEFAULT; // defaults

  //   surface properties of the orb
  float g_orbSurfaceFacingKr = 0.1f;
  float g_orbSurfaceEdgeOnKr = 1.8f;
  float g_orbIOR = 1.33f;

  float2 g_numOrbCells = to_float2(24.0f, 128.0f); // defaults
  float2 g_numOrbsChangeRate = to_float2_s(0.0f);

  //   properties of the "spark" lines
  float g_sparkWidth = 1.2f;
  float g_sparkColorIntensity = 1.0f;
  float3 g_sparkCoreColor = to_float3(1.1f, 0.5f, 0.5f);
  float3 g_sparkExtentColor = to_float3(1.2f, 1.2f, 2.3f);

  float2 g_sparkRotationRate = to_float2_s(0.0f);

  // - - - - - - - - - - - - -
  // Camera properties

  float3 g_camOrigin = to_float3(0.0f, 0.0f, -120.0f);
  float3 g_camPointAt = to_float3( 0.0f, 0.0f, 0.0f );
  float3 g_camUpDir = to_float3( 0.0f, 1.0f, 0.0f );

  float2 g_camRotationRates = to_float2_s(0.0f);


    // ----------------------------------
    // Animate globals

    //g_time = iChannelTime[0];
    g_time = iTime;
    g_numOrbsChangeRate = to_float2(g_beatRate * 0.05f, g_beatRate * 0.0625f);
    g_sparkRotationRate = to_float2(0.0f, g_beatRate * 0.25f);
    g_camRotationRates = to_float2(0.125f * g_beatRate, -0.0625f * g_beatRate);
    
    // Change the number of polar cells along the azimuthal and latitudinal axis
    // over time using a sawtooth signal. 
    //g_numOrbCells.x = 16.0f + 4.0f * (2.0f * (sawtooth( g_numOrbsChangeRate.x * g_time + 1.0f)) +
    //                                2.0f);
    
    g_numOrbCells.y = 8.0f + 8.0f * _powf(2.0f, _floor(2.0f * sawtooth( g_numOrbsChangeRate.y * g_time + 4.0f) + 2.0f));
    
    // inflate and deflate the cell box size over time using a sine function. 
    g_cellBoxHalfSize = CELL_BOX_HALF_SIZE_DEFAULT * (1.0f + 0.5f * (0.5f * _sinf(0.25f * (g_beatRate * g_time / TWO_PI)) + 0.5f)); 

    
    // Ramp into the audio driving the spark signals.
    g_audioResponse = _mix(0.4f, 0.9f, smoothstep(0.0f, 5.0f, g_time));
    
    // Would be nice if I could find a way to ramp off of the beat better, but that
    // would require some history knowledge.
    g_bassBeat = g_audioResponse * smoothstep(0.9f, 0.95f, texture( iChannel0, to_float2( 0.0f, 0.0f ) ).x);

    // For each audio sample, sample the audio channel along the x-axis (increasing frequency
    // with each sample).  
    for (int i = 0; i < NUM_AUDIO_SAMPLE_TRIPLES; i += 1) {

        float audioOffset = (float)(i) * (0.95f/(float)(NUM_AUDIO_SAMPLE_TRIPLES*3));
        g_audioFreqs[i] = smoothstep(0.0f, 1.0f, texture( iChannel0, to_float2(0.0f + audioOffset, 0.0f)).x);
    }
    
    // In thumbnail mode, show at least some of the audio sparks
    //if (iResolution.y < 250.0f) 
    //    g_audioFreqs[0] = 1.0f;
    
    //Manual Audioparameter
    if(Manual)
    {
      g_bassBeat = Bass;
      g_audioFreqs[0] = Audio1;
      g_audioFreqs[1] = Audio2;
      g_audioFreqs[2] = Audio3;
    }
    
    
    // Thump the color intensity, and fog color based on the bass beat.
    g_sparkColorIntensity = _mix(1.0f, 1.3f, g_bassBeat);
    g_fogColor *= _mix(1.0f, 1.4f, g_bassBeat);

    // remap the mouse click ([-1, 1], [-1/ar, 1/ar])
    float2 click = swi2(iMouse,x,y) / swi2(iResolution,x,x);    
    click = 2.0f * click - 1.0f;   
    
    // camera position
    float3 lookAtOffset = to_float3(0.0f, 0.0f, -20.0f);
    
    float rotateXAngle    = 0.2f * g_time - PI * click.y;
    float cosRotateXAngle = _cosf(rotateXAngle);
    float sinRotateXAngle = _sinf(rotateXAngle);
    
    float rotateYAngle    = 0.2f * g_time + TWO_PI * click.x;
    float cosRotateYAngle = _cosf(rotateYAngle);
    float sinRotateYAngle = _sinf(rotateYAngle);

    // Rotate the lookAt position around the origin
    lookAtOffset = rotateAroundXAxis(lookAtOffset, cosRotateXAngle, sinRotateXAngle);
    lookAtOffset = rotateAroundYAxis(lookAtOffset, cosRotateYAngle, sinRotateYAngle);

    // Then add an offset on top of the lookAtOffset so that we "crane"
    // the camera 200 units away from the origin.  We can then slowly
    // rotate the crane around it's azimuth to explore the scene.
    g_camOrigin = to_float3(0.0f, 0.0f , -200.0f);
    g_camPointAt = g_camOrigin + lookAtOffset;

    // re-use variables
    rotateYAngle    = 1.0f * g_camRotationRates.y * g_time;
    cosRotateYAngle = _cosf(rotateYAngle);
    sinRotateYAngle = _sinf(rotateYAngle);

    g_camOrigin    = rotateAroundYAxis(g_camOrigin,  cosRotateYAngle, sinRotateYAngle );
    g_camPointAt   = rotateAroundYAxis(g_camPointAt, cosRotateYAngle, sinRotateYAngle );

    // ----------------------------------
    // Setup Camera

    // Shift p so it's in the range -1 to 1 in the x-axis and 1.0f/aspectRatio
    // to 1.0f/aspectRatio in the y-axis (a reminder aspectRatio := width /
    // height of screen)

    // I could simplify this to:
    // float2 p = fragCoord / swi2(iResolution,x,x); <- but that's a bit obtuse
    // to read.
    
    float2 p = fragCoord / iResolution;      
    float aspectRatio = iResolution.x / iResolution.y;
    p = 2.0f * p - 1.0f;
    p.y *= 1.0f/aspectRatio;
    
    // calculate the rayDirection that represents mapping the  image plane
    // towards the scene
    float3 cameraZDir = normalize( g_camPointAt - g_camOrigin );
    float3 cameraXDir = normalize( cross(cameraZDir, g_camUpDir) );

    // no need to normalize since we know cameraXDir and cameraZDir are orthogonal
    float3 cameraYDir = cross(cameraXDir, cameraZDir);
    
    // Make the constant in front of cameraZDir (camFocalLengthScalar) bigger
    // to tighten focus, smaller to widen focus.

    float2 uv = p*0.5f+0.5f;
    float vignet = _powf(10.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y),0.4f);
        
    // Add a bit of fisheye distortion based on vignet signal
    float camFocalLengthScalar = _mix(0.4f, 1.0f, vignet);
    float3 rayDir = normalize( p.x*cameraXDir + p.y*cameraYDir + camFocalLengthScalar * cameraZDir );    
        
    // Add a clipping plane bias so orbs don't get too close to camera
    g_camOrigin += 5.0f * rayDir;
    SceneRayStruct ray = {g_camOrigin, // origin
                          rayDir, // direction
                          0.0f, // current march depth
                          FOG_EXTENT, // max march depth
                          to_float3_s(0.0f), // cellCoords
                          to_float4_s(0.0f), // orbHitPoint
                          to_float4_s(0.0f), // color
                          to_float4_s(0.0f)}; // debug color
    
    // ----------------------------------
    // Trace Scene

    traceScene(&ray, g_cellBoxHalfSize, g_time, g_orbOuterRadius);
    

    // ----------------------------------
    // Shade Scene

    if ((ray.marchDist < FOG_EXTENT) && (ray.orbHitPoint.w > 0.5f)) {
        //simpleShadeOrb(&sceneRay);
        shadeOrb(&ray, g_sparkRotationRate, g_time, g_numOrbCells, g_orbCenterRadius, g_orbOuterRadius, g_sparkWidth, 
                       g_beatRate, g_audioFreqs, g_audioResponse, g_sparkColorIntensity, g_sparkCoreColor, 
                       g_sparkExtentColor, g_bassBeat, 
                       g_orbIOR, g_orbSurfaceFacingKr, g_orbSurfaceEdgeOnKr);        
    }

    // ----------------------------------
    // Harvest final color results

    float4 tracedColor = ray.color;
    
    // multiply the alpha onto the color.  If you want info on why I do this,
    // consult the source paper:
    // http://graphics.pixar.com/library/Compositing/paper.pdf
    //swi3(tracedColor,x,y,z) *= tracedColor.w;
    tracedColor.x *= tracedColor.w;
    tracedColor.y *= tracedColor.w;
    tracedColor.z *= tracedColor.w;

    // ----------------------------------
    // Fog

    swi3S(tracedColor,x,y,z, _mix(swi3(tracedColor,x,y,z), g_fogColor, 
                                  smoothstep(50.0f, FOG_EXTENT, ray.marchDist)));

    // ----------------------------------
    // Color grading

    // Increase contrast
    swi3S(tracedColor,x,y,z, pow_f3(swi3(tracedColor,x,y,z), to_float3_s(1.7f)));
    
    // vigneting
    tracedColor *= _mix(0.3f, 1.0f, vignet);

    fragColor = tracedColor;
    
    // Debug color's alpha is not multiplied during final output.  If it is 
    // non-zero, then the debugcolor is respected fully 
    swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z), swi3(ray.debugColor,x,y,z), 
                           step(EPSILON, ray.debugColor.w)));

    fragColor.w=Alpha;

  SetFragmentShaderComputedColor(fragColor);
}