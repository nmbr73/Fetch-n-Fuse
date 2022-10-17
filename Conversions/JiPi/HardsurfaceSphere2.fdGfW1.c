
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define LOOP
//#define LOOP2

// skaplun https://www.shadertoy.com/view/7tf3Ws
__DEVICE__ float easeOutBack(float x, float t) {
    float c1 = t;
    float c3 = c1 + 1.0f;

    return 1.0f + c3 * _powf(x - 1.0f, 3.0f) + c1 * _powf(x - 1.0f, 2.0f);
}

__DEVICE__ float easeInOutBack(float x) {
    float c1 = 1.70158f;
    float c2 = c1 * 1.525f;

    return x < .5
      ? (_powf(2.0f * x, 2.0f) * ((c2 + 1.0f) * 2.0f * x - c2)) / 2.
      : (_powf(2.0f * x - 2.0f, 2.0f) * ((c2 + 1.0f) * (x * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

__DEVICE__ float easeSnap(float x) {
    x = _powf(x, 0.75f);
    x = easeInOutBack(x);
    return x;
}

__DEVICE__ float linearstep(float a, float b, float t) {
    return clamp((t - a) / (b - a), 0.0f, 1.0f);
}



__DEVICE__ float tFloor(float time, float timeOffset, float timeGap) {
    time += timeOffset;
    time -= timeGap / 3.0f;
    return _floor(time / timeGap);
}

__DEVICE__ float tFract(float time, float timeOffset, float timeGap) {
    time += timeOffset;
    time -= timeGap / 3.0f;
    return fract(time / timeGap) * timeGap * 0.5f;
}

__DEVICE__ float3 primaryAxis(float3 p) {
    float3 a = abs_f3(p);
    return (1.0f-step(swi3(a,x,y,z), swi3(a,y,z,x)))*step(swi3(a,z,x,y), swi3(a,x,y,z))*sign_f3(p);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel0



#define PI 3.14159265359f

__DEVICE__ mat3 scaleM(float s) {
    return to_mat3(
        s, 0, 0,
        0, s, 0,
        0, 0, 1
    );
}

__DEVICE__ mat3 rotM(float a) {
    return to_mat3(
        _cosf(a), _sinf(a), 0,
        -_sinf(a), _cosf(a), 0,
        0, 0, 1
    );
}

__DEVICE__ mat3 transM(float2 v) {
    return to_mat3(
        1, 0, v.x,
        0, 1, v.y,
        0, 0, 1
    );
}

__DEVICE__ float2 mul(float2 p, mat3 m) {
   return swi2((mul_f3_mat3(to_float3_aw(p, 1) , m)),x,y);
}

__DEVICE__ void calcAngleOffset(float tf, float ts, out float *angle, out float2 *offset, float timeOffset, float timeGap) {
    float time = tf + ts;
    *angle = time;
    *offset = to_float2(0, time);
    
    #ifndef LOOP
    #ifndef LOOP2
    return;
    #endif
    #endif
float zzzzzzzzzzzzzzzzzzzzzzzzzz;    
    float to = timeOffset / timeGap;

    *angle = 0.0f;
    if (mod_f(tf, 3.0f) == 2.0f) {
        *angle += mod_f(tf, 3.0f) + ts * -2.0f;
    } else{
        *angle += mod_f(tf, 3.0f) + ts;
    }
    *angle *= 1.0f;
    *angle += to;

    float3 ba = _fmaxf(to_float3_s(0), mod_f3(to_float3(tf + 2.0f, tf + 1.0f, tf), 3.0f) - 1.0f);
    float3 bb = _fmaxf(to_float3_s(0), mod_f3(to_float3(tf + 3.0f, tf + 2.0f, tf + 1.0f), 3.0f) - 1.0f);
    float3 bary = _mix(ba, bb, ts);
    *offset = bary.x * to_float2(0,0) + bary.y * to_float2(-1.0f,-0.3f) + bary.z * to_float2(0.3f,-0.333f);

    (*offset).y += to;
}

__DEVICE__ mat3 gridTransformation(out float *scale, float timeOffset, float timeGap, float gTime) {
    float tf = tFloor(gTime, timeOffset, timeGap);
    float ts = easeSnap(linearstep(0.3f, 0.8f, tFract(gTime, timeOffset, timeGap)));

    *scale = 2.5f;
        
    float angle;
    float2 offset;
    calcAngleOffset(tf, ts, &angle, &offset, timeOffset, timeGap);  

    mat3 m = scaleM(*scale);
    m = mul_mat3_mat3(m , rotM(PI * -0.08f * angle));
    m = mul_mat3_mat3(m , transM(offset * -0.78f));
    return m;
}

__DEVICE__ mat3 gridTransformation2(out float *scale, float timeOffset, float timeGap, float gTime) {
    float tf = tFloor(gTime, timeOffset, timeGap);
    float ts = easeOutBack(linearstep(0.6f, 0.9f, tFract(gTime, timeOffset, timeGap)), 1.70158f);
    
    *scale = 3.5f;
   
    float angle;
    float2 offset;
    calcAngleOffset(tf, ts, &angle, &offset, timeOffset, timeGap);  

    mat3 m = scaleM(*scale);
    m = mul_mat3_mat3(m , rotM(PI * -0.08f * angle * 0.5f));
    m = mul_mat3_mat3(m , rotM(PI * 2.0f/3.0f));
    m = mul_mat3_mat3(m , transM( swi2((offset * -0.78f * 0.5f),y,x)));
    return m;
}

__DEVICE__ float effectMask(float2 uv) {
    return _sinf(length(uv) * 12.0f + 1.0f) * 0.5f + 0.5f;
}

// --------------------------------------------------------
// Triangle Voronoi
// tdhooper https://www.shadertoy.com/view/ss3fW4
// iq https://shadertoy.com/view/ldl3W8
// --------------------------------------------------------

__DEVICE__ float vmax(float3 v) {
    return _fmaxf(v.x, _fmaxf(v.y, v.z));
}



__DEVICE__ float3 sdTriEdges(float2 p) {

  const float s3 = _sinf(PI / 3.0f);

    return to_float3(
        dot(p, to_float2(0,-1)),
        dot(p, to_float2(s3, 0.5f)),
        dot(p, to_float2(-s3, 0.5f))
    );
}

__DEVICE__ float sdTri(float2 p) {
    float3 t = sdTriEdges(p);
    return _fmaxf(t.x, _fmaxf(t.y, t.z));
}

__DEVICE__ float sdTri(float3 t) {
    return _fmaxf(t.x, _fmaxf(t.y, t.z));
}

__DEVICE__ float sdBorder(float3 tbRel, float2 pt1, float2 pt2) {
  
    const float s3 = _sinf(PI / 3.0f);
  
    float3 axis = primaryAxis(-tbRel);
    bool isEdge = axis.x + axis.y + axis.z < 0.0f;

    float2 gA = to_float2(0,-1);
    float2 gB = to_float2(s3, 0.5f);
    float2 gC = to_float2(-s3, 0.5f);
    
    float2 norA = gC * axis.x + gA * axis.y + gB * axis.z;
    float2 norB = gB * -axis.x + gC * -axis.y + gA * -axis.z;
    
    float2 dir = gA * axis.x + gB * axis.y + gC * axis.z;
    float2 corner = dir * dot(dir, pt1 - pt2) * 2.0f/3.0f;
        
    float2 ca, cb;
    float side;
    
    if (isEdge) {
        corner = pt2 + corner;
        ca = corner + _fmaxf(0.0f, dot(corner, -norB)) * norB;
        cb = corner + _fminf(0.0f, dot(corner, -norA)) * norA;
    } else {
        corner = pt1 - corner;
        ca = corner + _fmaxf(0.0f, dot(corner, -norA)) * norA;
        cb = corner + _fminf(0.0f, dot(corner, -norB)) * norB;
    }
    
    side = step(dot(corner, mul_f2_mat2(dir , to_mat2(0,-1,1,0))), 0.0f);
    corner = _mix(ca, cb, side);
    
    float d = length(corner);

    return d;
}

__DEVICE__ float2 hash2( float2 p, __TEXTURE2D__ iChannel0 )
{
  return swi2(texture( iChannel0, (p+0.5f)/256.0f),x,y);
}

__DEVICE__ float2 cellPoint(float2 n, float2 f, float2 cell, bool gaps, __TEXTURE2D__ iChannel0) {
    float2 coord = n + cell;
    float2 o = hash2( n + cell, iChannel0 );
    if (gaps && hash2(swi2(o,y,x) * 10.0f, iChannel0).y > 0.5f) {
        return to_float2_s(1e12);
    }
    #ifdef ANIMATE
        o = 0.5f + 0.5f*_sinf( time * PI * 2.0f + 6.2831f*o );
    #endif  
    float2 point = cell + o - f;
    return point;
}

__DEVICE__ float4 voronoi(float2 x, bool gaps, __TEXTURE2D__ iChannel0)
{
    float2 n = _floor(x);
    float2 f = fract_f2(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
    float2 closestCell, closestPoint;
float vvvvvvvvvvvvvvvvvvvvv;
    const int reach = 3;

    float closestDist = 8.0f;
    for( int j = -reach; j <= reach; j++ )
    for( int i = -reach; i <= reach; i++ )
    {
        float2 cell = to_float2(i, j);
        float2 point = cellPoint(n, f, cell, gaps, iChannel0);
        float dist = vmax(sdTriEdges(point));

        if( dist < closestDist )
        {
            closestDist = dist;
            closestPoint = point;
            closestCell = cell;
        }
    }


    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    closestDist = 8.0f;
    for( int j = -reach-1; j <= reach+1; j++ )
    for( int i = -reach-1; i <= reach+1; i++ )
    {
        float2 cell = closestCell + to_float2(i, j);
        float2 point = cellPoint(n, f, cell, gaps, iChannel0);

        float3 triEdges = sdTriEdges(closestPoint - point);
        float dist = vmax(triEdges);

        if( dist > 0.00001f ) {
            closestDist = _fminf(closestDist, sdBorder(triEdges, closestPoint, point));
        }
    }

    return to_float4(closestDist, closestCell.x + n.x, closestCell.y + n.y, 0.0f);
}

__DEVICE__ inline mat3 multi( float B, mat3 A)  
  {  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  } 

__DEVICE__ inline mat3 inverse( mat3 A)  
  {  
   mat3 R;  
   float result[3][3];  
   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
                    {A.r1.x, A.r1.y, A.r1.z},  
                    {A.r2.x, A.r2.y, A.r2.z}};  
     
   float det = a[0][0] * a[1][1] * a[2][2]  
             + a[0][1] * a[1][2] * a[2][0]  
             + a[0][2] * a[1][0] * a[2][1]  
             - a[2][0] * a[1][1] * a[0][2]  
             - a[2][1] * a[1][2] * a[0][0]  
             - a[2][2] * a[1][0] * a[0][1];  
   if( det != 0.0 )  
   {  
	   result[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];  
	   result[0][1] = a[2][1] * a[0][2] - a[2][2] * a[0][1];  
	   result[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];  
	   result[1][0] = a[2][0] * a[1][2] - a[1][0] * a[2][2];  
	   result[1][1] = a[0][0] * a[2][2] - a[2][0] * a[0][2];  
	   result[1][2] = a[1][0] * a[0][2] - a[0][0] * a[1][2];  
	   result[2][0] = a[1][0] * a[2][1] - a[2][0] * a[1][1];  
	   result[2][1] = a[2][0] * a[0][1] - a[0][0] * a[2][1];  
	   result[2][2] = a[0][0] * a[1][1] - a[1][0] * a[0][1];  
		 
	   R = to_mat3_f3(make_float3(result[0][0], result[0][1], result[0][2]),   
	                  make_float3(result[1][0], result[1][1], result[1][2]), 
                    make_float3(result[2][0], result[2][1], result[2][2]));  
	   return multi( 1.0f / det, R);  
   }  
   R = to_mat3_f3(make_float3(1.0f, 0.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), make_float3(0.0f, 0.0f, 1.0f));  
   return R;  
  } 


__KERNEL__ void HardsurfaceSphere2Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

#ifdef LOOP
float timeOffset = (92.0f + 100.0f) * (3.0f/4.0f);
float timeGap = 3.0f;
#else
#ifdef LOOP2
    float timeOffset = (92.0f + 100.0f);
    float timeGap = 4.0f;
#else
    float timeOffset = (92.0f + 100.0f);
    float timeGap = 4.0f;
#endif
#endif

float gTime;
float gDuration;
float gSpeed;


    gTime = iTime;
    gSpeed = 1.0f;
    gDuration = 14.0f;
    
    #ifdef LOOP
    gSpeed = 1.5f;
    gDuration = (3.0f * timeGap) / gSpeed; // 6
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    gTime += 0.25f;
    #endif
    
    #ifdef LOOP2
    gSpeed = 1.0f;
    gDuration = (3.0f * timeGap) / gSpeed; // 12
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    //gTime += 0.25f;
    #endif


    float2 uv = fragCoord / iResolution;
    uv *= to_float2(1,-1);
    uv.y /= 2.0f;
    
    if (uv.x > uv.y * -2.0f) {
        fragColor = to_float4_s(0);
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
float AAAAAAAAAAAAAAAAAAAAAA;
    float scl;
    mat3 m = gridTransformation(&scl, timeOffset, timeGap, gTime);

    float4 v = voronoi(mul(uv, m), false, iChannel0);
    float d = v.x / scl;
    float2 localPt = swi2(v,y,z);
    float2 worldPt = mul(localPt, inverse(m));
    float2 seed = hash2(localPt, iChannel0);
    float id = 0.0f;
    float tile = mod_f(localPt.x + localPt.y, 3.0f);

    if (tile == 0.0f)
    {
        m = gridTransformation2(&scl, timeOffset, timeGap, gTime);
        v = voronoi(mul(uv, m), false, iChannel0);
        d = _fminf(d, v.x / scl);
        localPt = swi2(v,y,z);
        worldPt = mul(localPt, inverse(m));
        seed = hash2(localPt + seed * 10.0f, iChannel0);
        id = 1.0f;
        tile = mod_f(localPt.x + localPt.y, 3.0f);
    }

    float mask = effectMask(worldPt);
    
    fragColor = to_float4(d, id, tile, mask);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//#define SHOW_DATA
//#define GREEN_BG

#if HW_PERFORMANCE==1
#define AA 2
#endif

#define PI 3.14159265359f
#define PHI 1.618033988749895f

// HG_SDF
__DEVICE__ float2 pR(in float2 p, float a) {
    p = _cosf(a)*p + _sinf(a)*to_float2(p.y, -p.x);
    return p;
}

// Rotate on axis
// blackle https://suricrasia.online/demoscene/functions/
__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
  return _mix(dot(ax,p)*ax, p, _cosf(ro))+_sinf(ro)*cross(ax,p);
}

__DEVICE__ float unlerp(float low, float high, float value) {
    return (value - low) / (high - low);
}


// --------------------------------------------------------
// Icosahedral domain mirroring
// knighty https://www.shadertoy.com/view/MsKGzw
// 
// Also get the face normal, and tangent planes used to
// calculate the uv coordinates later.
// --------------------------------------------------------

#define PI 3.14159265359f



__DEVICE__ float3 fold(in float3 p, int Type, float3 nc) {
  for(int i=0;i<Type;i++){
    swi2S(p,x,y, abs_f2(swi2(p,x,y)));
    p -= 2.0f * _fminf(0.0f, dot(p,nc)) * nc;
  }
  return p;
}

__DEVICE__ float vmin(float3 v) {
    return _fminf(v.x, _fminf(v.y, v.z));
}

//__DEVICE__ float vmax(float3 v) {
//    return _fmaxf(v.x, _fmaxf(v.y, v.z));
//}

__DEVICE__ float2 triTile(float2 p)
{ 
    float2 hx = mul_f2_mat2(p , to_mat2(1,-1.0f/1.73f, 0,2.0f/1.73f));
    float3 g = to_float3_aw(hx, 1.0f-hx.x-hx.y);
    float3 id = _floor(g);
    g = fract_f3(g); 

    if (length(g) > 1.0f) g = 1.0f - g;
    float3 axis = primaryAxis(g);
    float y = -(1.0f/3.0f - vmin(g));
    float x = (vmax((1.0f-swi3(axis,y,z,x)) * g) - vmax((1.0f-axis) * g)) * _cosf(1.0f / (PI / 3.0f));
    return to_float2(x,y);
}

// --------------------------------------------------------
// Modelling
// --------------------------------------------------------

struct Model {
    float d;
    float3 col;
    float3 emissive;
    int id;
    bool isBound;
};

__DEVICE__ float smin(float a, float b, float k){
    float f = clamp(0.5f + 0.5f * ((a - b) / k), 0.0f, 1.0f);
    return (1.0f - f) * a + f  * b - f * (1.0f - f) * k;
}

__DEVICE__ float smax(float a, float b, float k) {
    return -smin(-a, -b, k);
}

__DEVICE__ float cmin(float a, float b, float r) {
  return _fminf(min(a, b), (a - r + b)*_sqrtf(0.5f));
}

__DEVICE__ float cmax(float a, float b, float r) {
  return _fmaxf(_fmaxf(a, b), (a + r + b)*_sqrtf(0.5f));
}


__DEVICE__ float pReflect(inout float3 p, float3 planeNormal, float offset, float soft) {
    float t = dot(p, planeNormal)+offset;
    float tr = _sqrtf(t * t + soft);
    p = p + (-t + tr) * planeNormal;
    return sign_f(t);
}


__DEVICE__ Model map(float3 p, float gTime, float gDuration, float gSpeed, float timeOffset, float timeGap, __TEXTURE2D__ iChannel0, int Type, float3 nc, float3 pca, float3 pbc, float3 pab) {
    p.z*= -1.0f;
    p = erot(p, normalize(pca), fract(gTime / gDuration / gSpeed + 0.2f) * PI * 2.0f * (1.0f/3.0f));

    float r = 2.0f;

    float3 col = normalize(p) * 0.5f + 0.5f;
    
    float3 face, ab, atob;

    p = fold(p, Type, nc);
    
    pReflect(p, to_float3(1,0,0), 0.0f, 0.0001f);
    pReflect(p, to_float3(0,1,0), 0.0f, 0.0001f);
    pReflect(p, normalize(to_float3(-1,-1.6f,0.615f)), 0.0f, 0.0001f);
    
    face = pca;
    atob = pbc - pab;
    ab = pab;
float mmmmmmmmmmmmmmmmmmmmmmmmm;    
    float3 vv = normalize(face - ab);
    float3 uu = normalize(atob);
    float3 ww = face;
    mat3 m = to_mat3_f3(uu,vv,ww);
       
    float3 pp = p / dot(p, face);
    
    float2 uv = swi2((mul_f3_mat3(pp , m)),x,y);
    
    col = to_float3_aw(uv * to_float2(1,-1), 0);
     
    float d = length(p) - 2.0f;
    
    float4 data = texture(iChannel0, uv * to_float2(1,-1) * to_float2(1,2.0f));
    float border = data.x * r * 0.66f;
    float tile = data.z;
    int id = (int)(data.y);
    float mask = data.w;

    float o = _mix(-0.25f, 0.0f, _powf(tile/2.0f, 0.5f));
   
    float ito = ((linearstep(0.2f, 0.6f + o, tFract(gTime, timeOffset, timeGap)) - _powf(linearstep(0.8f + o, 0.9f + o, tFract(gTime, timeOffset, timeGap)), 4.0f)));
 
    if (tile == 1.0f) {
        ito *= -1.0f;
    }

    float inn = _mix(0.1f, 0.4f, tile/2.0f);
    
    ito *= 0.06f;
    r += ito;
       
    float ws = 1.0f;
    float w = _mix(0.025f, 0.1f, mod_f(tile + 1.0f, 3.0f) / 2.0f) * ws;

    
    float d0 = d;
    
    float2 p2 = to_float2(border, length(p) - r);
    d = p2.y;
    d = smin(d, dot(p2 - to_float2(0.005f,0), normalize(to_float2(-2.6f,1))), 0.005f);
    d = smax(d, p2.y - inn * 0.4f, 0.005f);

    if (id == 0) {
        float ii = _mix(0.25f, 0.04f, tile/3.0f);
        if (tile == 2.0f) {
            d = cmax(d, -(_fabs(border - 0.2f)), 0.01f);
        }
    }
    else
    {
       d = smax(d, dot(p2 - to_float2(0.1f, inn * 0.4f), normalize(to_float2(1,1))), 0.005f);
       d = cmin(d, _fmaxf(d - 0.01f, (_fabs(border - 0.1f))), 0.01f);
    }
    
    d = _fmaxf(d, -(border - w / 6.0f));
        
    border *= 1.5f;
    inn *= 1.5f;
    
    col = to_float3_s(0.6f);
    
    if (id == 1 && tile == 1.0f) {
        col *= 0.4f;
    }
    
    if (id == 1 && tile == 0.0f) {
        col *= 0.7f;
    }
    
    if (id == 1 && tile == 2.0f) {
        col *= 1.1f;
    }

    col = _mix(col, to_float3_s(0.15f), (1.0f-smoothstep(0.01f * ws*1.5f, 0.04f*ws*1.5f, _fabs(border * 6.0f - inn))));
    
    ito= _fmaxf(ito, 0.0f);
    col = _mix(col, to_float3_s(0), smoothstep(r + 0.025f - ito, r - 0.025f - ito, length(p)) * step(border, w + 0.01f));
        
    col *= clamp(border * 5.0f + 0.7f, 0.0f, 1.0f);
    
    
    float3 emissive = to_float3_s(0);
    if (tile == 1.0f) {
        float l = (1.0f - clamp(_fabs(border - 0.2f) * 100.0f, 0.0f, 1.0f)) * 2.0f;
        l += (1.0f - clamp(_fabs(border - 0.2f) * 15.0f, 0.0f, 1.0f)) * 0.1f;
        emissive += to_float3(0,0.8f,0.5f) * l;
        
        float lt = tFract(gTime - 0.4f, timeOffset, timeGap);
        float ramp = 1.0f - linearstep(0.025f, 0.07f, lt);
        ramp += linearstep(0.8f, 0.9f, lt);
        ramp *= _mix(1.0f, _sinf(lt * 150.0f) * 0.5f + 0.5f, linearstep(1.2f, 0.9f, lt));
        
        emissive *= ramp;
    }
    
    col *= 0.95f;
    
    if (id == 1) {
        col *= 0.4f;
    }
    
    Model ret = {d, col, emissive, id, false};
    return ret; //Model(d, col, emissive, id, false);
}

__DEVICE__ Model mapDebug(float3 p, float gTime, float gDuration, float gSpeed, float timeOffset, float timeGap, __TEXTURE2D__ iChannel0, int Type, float3 nc, float3 pca, float3 pbc, float3 pab) {
    Model m = map(p,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0, Type, nc, pca, pbc, pab);
    return m;
    
    float d = -p.z;
    m.d = _fmaxf(m.d, -d);
    d = _fabs(d) - 0.01f;
    if (d < m.d) {
float tttttttttttttttttttttt;      
        Model ret = {d, to_float3_s(0), to_float3_s(0), 9, false};
        return ret;  //Model(d, to_float3_s(0), to_float3_s(0), 9, false);
    }
    return m;
}


// --------------------------------------------------------
// Rendering
// --------------------------------------------------------

__DEVICE__ float vmul(float2 v) {
    return v.x * v.y;
}

// compile speed optim from IQ https://www.shadertoy.com/view/Xds3zN
__DEVICE__ float3 calcNormal(float3 pos, float gTime, float gDuration, float gSpeed, float timeOffset, float timeGap, __TEXTURE2D__ iChannel0, int Type, float3 nc, float3 pca, float3 pbc, float3 pab){
    float3 n = to_float3_s(0.0f);
    for( int i=0; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.0005f*e,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0,Type,nc,pca,pbc,pab).d;
    }
    return normalize(n);
}

__DEVICE__ mat3 calcLookAtMatrix(float3 ro, float3 ta, float3 up) {
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww,up));
    float3 vv = normalize(cross(uu,ww));
    return to_mat3_f3(uu, vv, ww);
}

// origin sphere intersection
// returns entry and exit distances from ray origin
__DEVICE__ float2 iSphere( in float3 ro, in float3 rd, float r )
{
  float3 oc = ro;
  float b = dot( oc, rd );
  float c = dot( oc, oc ) - r*r;
  float h = b*b - c;
  if( h<0.0f ) return to_float2_s(-1.0f);
  h = _sqrtf(h);
  return to_float2(-b-h, -b+h );
}

__DEVICE__ float _fwidth(float inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}

__DEVICE__ float3 render( float2 p, float gTime, float gDuration, float gSpeed, float timeOffset, float timeGap, __TEXTURE2D__ iChannel0, float4 iMouse, int Type, float3 nc, float3 pca, float3 pbc, float3 pab, float2 iResolution, float Par )
{

    float2 tuv = p;
    
    //tuv *= 0.333f;
    //pR(tuv, PI / 12.0f);
    float3 col = (to_float3_s(0.8f) - p.y * 0.33f) * 0.65f;
    //float w = fwidth(length(p)) / 2.0f;
    float w = _fwidth(length(p), iResolution, Par) / 2.0f;
float rrrrrrrrrrrrrrrrrrrrr;
    #ifndef LOOP
    float k = 1.0f / length(tuv) * 1.25f;
    tuv = to_float2(k, _atan2f(tuv.x/tuv.y, 1.0f) * 1.101f) * 1.0f;
    #ifdef LOOP2
    tuv += to_float2(0,1) * fract(gTime / gDuration / gSpeed + 0.2f) * 1.73f;
    #else
    tuv += to_float2(-0.25f,0.5f) * gTime * 0.25f * 0.5f;
    #endif
    tuv = triTile(tuv);
    float4 data = texture(iChannel0, tuv * to_float2(1,-1) * to_float2(1,2.0f) * 1.43f);
    
    float lp = length(p) - 0.05f;
    
    float ga = smoothstep(0.9f, 1.1f, lp);
    ga *= smoothstep(1.9f, 1.1f, lp);
    ga = 0.01f * ga - 0.005f;

    float gb = smoothstep(0.6f, 1.0f, lp);
    gb *= smoothstep(1.5f, 1.0f, lp);
    gb = 0.01f * gb - 0.005f;
    

    float g = smoothstep(ga + w, ga - w, _fabs(data.x));
    g += smoothstep(gb + w, gb - w, _fabs(data.x - 0.1f));
    
    col += to_float3(0,0.8f,0.5f) * g;

    //col = to_float3_s(0.03f);
    
    float e = unlerp(1.7f, 0.95f, length(p));
    e = _mix(0.2f, 0.00f, e);
    //e = 0.06f;
    
    //data.x /= k;

    float ms = smoothstep(1.8f, 0.9f, length(p));
    col += to_float3_s(1) * step(e, data.x) * 0.15f * ms;
    #else
    col = to_float3_s(0.5f);
    #endif
    
    #ifdef GREEN_BG
    col *= to_float3(0,0.8f,0.5f) * 1.8f;
    #endif
    
    //col = _mix(col, to_float3(1,0,0), clamp(unlerp(1.0f, 0.75f, length(p)), 0.0f, 1.0f));
    
    
    //return to_float3_aw(1) * step(data.x, 0.01f);
    
    float3 camPos = to_float3(0,0,9);
    
    float2 im = swi2(iMouse,x,y) / iResolution - 0.5f;
    
    if (iMouse.x <= 0.0f)
    {
        im = to_float2_s(0);
    }
    
    im += to_float2(0.66f,0.3f);
    
    swi2S(camPos,y,z, pR(swi2(camPos,y,z), (0.5f - im.y) * PI / 2.0f));
    swi2S(camPos,x,z, pR(swi2(camPos,x,z), (0.5f - im.x) * PI * 1.5f));
    
    mat3 camMat = calcLookAtMatrix(camPos, to_float3_s(0), to_float3(0,1,0));
    
    float focalLength = 3.0f;
    float3 rayDirection = normalize(mul_mat3_f3(camMat , to_float3_aw(swi2(p,x,y), focalLength)));
    
    float2 bound = iSphere(camPos, rayDirection, 2.3f);
    if (bound.x < 0.0f) {
      return col;
    }

    
    float3 rayPosition = camPos;
    float rayLength = 0.0f;
    Model model;
    float dist = 0.0f;
    bool bg = false;
    float closestPass = 1e12;

    for (int i = 0; i < 100; i++) {
        rayLength += dist * 0.9f;
        rayPosition = camPos + rayDirection * rayLength;
        model = mapDebug(rayPosition,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0,Type,nc,pca,pbc,pab);
        dist = model.d;
        
        if ( ! model.isBound) {
            closestPass = _fminf(closestPass, dist);
        }
        
        if (_fabs(dist) < 0.001f) {
          break;
        }
        
        if (rayLength > 15.0f) {
            bg = true;
            break;
        }
    }
    

    
    if ( ! bg) {
        col = model.col;
        float3 nor = calcNormal(rayPosition,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0,Type,nc,pca,pbc,pab);
        
        float3 lin = to_float3_s(0);
        
        float3 rd = rayDirection;
        float3 lig = normalize(to_float3(-0.1f,1,-0.1f));
        float3 hal = normalize(lig - rd );

        float dif, spe;

        dif = clamp(dot(lig, nor) * 0.75f + 0.4f, 0.0f, 1.0f);
        dif += _sqrtf(clamp( 0.5f+0.5f*nor.y, 0.0f, 1.0f )) * 0.5f;
        spe = _powf(clamp(dot(nor, normalize(lig - rd)), 0.0f, 1.0f), 100.0f);
        spe *= dif;
        spe *= 0.04f + 0.96f * _powf(clamp(1.0f - dot(hal, lig), 0.0f, 1.0f), 5.0f);
        lin += 1.3f * col * dif;
        lin += 6.0f * spe;
        
        lig = normalize(to_float3(0.5f,-1,0.5f));
        hal = normalize(lig - rd );
        
        dif = clamp(dot(lig, nor), 0.0f, 1.0f);        
        spe = _powf(clamp(dot(nor, normalize(lig - rd)), 0.0f, 1.0f), 100.0f);
        spe *= dif;
        spe *= 0.04f + 0.96f * _powf(clamp(1.0f - dot(hal, lig), 0.0f, 1.0f), 5.0f);       
        float m = clamp(dot(lig, normalize(rayPosition)) + 1.0f, 0.0f, 1.0f) * 1.5f;
        lin += 1.0f * col * dif * to_float3(0,0.8f,0.5f) * m;
        lin += 6.0f * spe * to_float3(0,0.8f,0.5f) * m;
        
        col = lin;
                
        col += model.emissive;
        
        if (model.id == 9) {
           col = to_float3(1,0.5f,0.5f) * fract(map(rayPosition,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0,Type,nc,pca,pbc,pab).d * 20.0f);
        }
    }
    else
    {
        col = _mix(col, to_float3_s(0.1f), smoothstep(0.015f + w*2.0f, 0.015f - w*2.0f, closestPass));
    }
    
    return col;
}

__KERNEL__ void HardsurfaceSphere2Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    CONNECT_SLIDER0(Par, -10.0f, 10.0f, 1.0f);
  
    fragCoord+=0.5f;

    #ifdef LOOP
float timeOffset = (92.0f + 100.0f) * (3.0f/4.0f);
float timeGap = 3.0f;
#else
#ifdef LOOP2
    float timeOffset = (92.0f + 100.0f);
    float timeGap = 4.0f;
#else
    float timeOffset = (92.0f + 100.0f);
    float timeGap = 4.0f;
#endif
#endif

float gTime;
float gDuration;
float gSpeed;


    gTime = iTime;
    gSpeed = 1.0f;
    gDuration = 14.0f;
    
    #ifdef LOOP
    gSpeed = 1.5f;
    gDuration = (3.0f * timeGap) / gSpeed; // 6
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    gTime += 0.25f;
    #endif
    
    #ifdef LOOP2
    gSpeed = 1.0f;
    gDuration = (3.0f * timeGap) / gSpeed; // 12
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    //gTime += 0.25f;
    #endif

    #ifdef SHOW_DATA
        float4 data = texture(iChannel0, fragCoord / iResolution);
        data.x = fract(data.x * 100.0f);
        fragColor = data;
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    #endif
    
    int Type=5;
    float3 nc;
    float3 pab;
    float3 pbc;
    float3 pca;


    float cospin=_cosf(PI/(float)(Type)), scospin=_sqrtf(0.75f-cospin*cospin);
    nc=to_float3(-0.5f,-cospin,scospin);
    pbc=to_float3(scospin,0.0f,0.5f);
    pca=to_float3(0.0f,scospin,cospin);
    pbc=normalize(pbc);
    pca=normalize(pca);
    pab=to_float3(0,0,1);
    pca *= 0.794654f;
    pab *= 0.850651f;

float IIIIIIIIIIIIIIIIIIIIIII;    
    float2 o = to_float2_s(0);
    float3 col = to_float3_s(0);

    // AA from iq https://www.shadertoy.com/view/3lsSzf
    #ifdef AA
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
      o = to_float2((float)(m),(float)(n)) / (float)(AA) - 0.5f;
      float d = 0.5f*vmul(sin_f2(mod_f2f2(fragCoord * to_float2(147,131), to_float2_s(PI * 2.0f))));
    #endif
    
      float2 p = (-iResolution + 2.0f * (fragCoord + o)) / iResolution.y;
      col += render(p,gTime,gDuration,gSpeed,timeOffset,timeGap,iChannel0, iMouse,Type,nc,pca,pbc,pab, iResolution, Par);
        
    #ifdef AA
    }
    col /= (float)(AA*AA);
    #endif
    
    // colour grading from tropical trevor's scripts
    // https://github.com/trevorvanhoof/ColorGrading
    float3 uGain = to_float3_s(0.0f);
    float3 uLift = to_float3_s(0.2f);
    float3 uOffset = to_float3_s(-0.225f);
    float3 uGamma = to_float3_s(0.3f);
    col = pow_f3(_fmaxf(to_float3_s(0.0f), col * (1.0f + uGain - uLift) + uLift + uOffset), _fmaxf(to_float3_s(0.0f), 1.0f - uGamma));
    col = pow_f3( col, to_float3_s(1.0f/2.2f) );
    
    fragColor = to_float4_aw(col, 0);

  SetFragmentShaderComputedColor(fragColor);
}