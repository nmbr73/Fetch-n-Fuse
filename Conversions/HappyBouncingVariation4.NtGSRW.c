
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

__DEVICE__ inline float2 f2_multi_mat2( float2 A, mat2 B )  
  {  
	float2 C;  
	C.x = A.x * B.r0.x + A.y * B.r0.y;  
	C.y = A.x * B.r1.x + A.y * B.r1.y;  
	return C;  
  }
  
__DEVICE__ float3 cos_f3(float3 i) {float3 r; r.x = _cosf(i.x); r.y = _cosf(i.y); r.z = _cosf(i.z); return r;}


#define fill(sdf) (smoothstep(0.001f, 0.0f, sdf))
#define repeat(p,r) (mod_f(p,r)-r/2.0f)
#define ss(a,b,t) (smoothstep(a,b,t))
#define clp(t) (clamp(t,0.0f,1.0f))

// add shape to layer
__DEVICE__ void add (in float sdf, in float3 col, inout float *sdfLayers, inout float3 *colLayers)
{
  
    *colLayers = _mix(*colLayers, col, fill(sdf));
    *sdfLayers = _fminf(sdf, *sdfLayers);
}

// add shape to frame
__DEVICE__ void add (in float4 shape, inout float4 *frame)
{
    if (shape.w < 0.0f) *frame = to_float4_aw(swixyz(shape),(*frame).w);
}

// soft drop shadow from shape 
__DEVICE__ float shadow (float sdf)
{
    return clamp(sdf+0.9f,0.0f,1.0f);
}

// Inigo Quilez
// https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
__DEVICE__ float sdArc( in float2 p, in float ta, in float tb, in float ra, float rb )
{
    p.y -= ra; // offset y
    float2 sca = to_float2(_sinf(ta),_cosf(ta));
    float2 scb = to_float2(_sinf(tb),_cosf(tb));
    p = f2_multi_mat2(p,to_mat2(sca.x,sca.y,-sca.y,sca.x));
    p.x = _fabs(p.x);
    float k = (scb.y*p.x>scb.x*p.y) ? dot(p,scb) : length(p);
    return _sqrtf( dot(p,p) + ra*ra - 2.0f*ra*k ) - rb;
}
__DEVICE__ float smin( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); }
    
// snippets
__DEVICE__ mat2 rot(float a) { float c=_cosf(a),s=_sinf(a); return to_mat2(c,-s,s,c); }
__DEVICE__ float circle (float2 p, float size)
{
    return length(p)-size;
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



// "happy bouncing v4"
// shader about boucing animation, space transformation, easing functions,
// funny shape and colorful vibes.
// by leon denise (2021-12-22)
// licensed under hippie love conspiracy

// using Inigo Quilez works:
// arc sdf from https://www.shadertoy.com/view/wl23RK
// color palette https://iquilezles.org/www/articles/palettes/palettes.htm

__DEVICE__ float globalSpeed = 0.5f;
#define pi 3.1415f

__DEVICE__ float2 animation(float2 p, float t)
{
    float ta = fract_f(t)*6.283f;
    float tt = t;
    
    p.y -= 0.15f; // sidebar
    p = f2_multi_mat2(p,rot(_sinf(ta)*0.2f*ss(0.0f,0.5f,p.y+0.2f))); // swing
    
    return p;
}

__DEVICE__ float4 buddy (float2 pp, float3 tint, float t, float ii)
{
    // result
    float scene = 100.0f;
    float3 col = to_float3_s(0);

    // variables
    float2 p, q;
    float shape, zhape;
    float ta = t*6.283f;
    float bodySize = 0.25f;
    float turn = _sinf(t*6.283f*0.5f-0.5f)*2.0f;
    
    // body
    p = animation(pp, t);
    float body = circle(p, bodySize);

    p = animation(pp, t-0.01f)+to_float2(0.0f,-1.0f)*bodySize;
    p.x = _fabs(p.x)-0.1f;
    zhape = circle(p*to_float2(0.7f,0.4f), 0.02f);
    body = smin(body, zhape, 0.04f);
    
    // mouth
    p = animation(pp, t + 0.02f)+to_float2(-0.2f*turn,0.5f)*bodySize;
    q = p;
    shape = smin(body, circle(p, 0.1f), 0.2f);
    add(shape, tint, &scene, &col);
    col *= clp(zhape*10.0f+0.5f)*-0.2f+1.0f;
    p *= 1.5f;
    p.x = _fabs(p.x)-0.05f;
    shape = sdArc(p+to_float2(0,0.1f), pi/-3.0f, 2.0f, 0.05f, 0.02f);
    add(shape, tint*0.6f, &scene, &col);
    
    // nose
    shape = circle(q-to_float2(0,0.02f), 0.04f);
    add(shape, tint*0.5f, &scene, &col);
    col *= shadow(shape);

    // eyes
    shape = 100.0f;
    p = animation(pp, t+0.02f);
    p = p - to_float2(0.2f*turn, 0.6f)*bodySize;
    p.x = _fabs(p.x)-0.04f;
    add(circle(p, 0.04f), to_float3_s(1)*ss(-0.2f,0.2f,p.y+0.1f), &shape, &col);
    add(circle(p+to_float2_s(0.01f), 0.02f), to_float3_s(0), &shape, &col);
    col *= shadow(shape);
    scene = _fminf(scene, shape);

    return to_float4_aw(col, scene);
}

__KERNEL__ void HappyBouncingVariation4Kernel( __CONSTANTREF__ Params*  params, __TEXTURE2D__ iChannel0, __TEXTURE2D_WRITE__ dst )
{
  PROLOGUE(color,pixel);


    float2 uv = pixel/iResolution;
    color = to_float4_s(0.25f)*step(uv.y,0.1f); // sidebar
    
    float4 shape;
    float2 pos = (pixel-to_float2(0.5f,0)*iResolution)/iResolution.y;
    float2 p, pp;
    
    // rolling buddy
    p = pos;
    p.y -= 0.1f;
    p.x -= fract_f(iTime*globalSpeed*0.5f+0.61f)*2.0f-1.0f;
    p = f2_multi_mat2(p,rot(pos.x*20.0f));
    shape = to_float4_aw(to_float3(0.976f,0.976f,0.424f)*ss(0.1f,0.0f,length(p)), circle(p,0.04f));
    add(shape, &color);
    p.y += 0.02f;
    add(to_float4_aw(to_float3_s(0), sdArc(p, pi/-2.0f, 1.0f, 0.02f, 0.005f)), &color);
    p.x = _fabs(p.x)-0.01f;
    p.y -= 0.03f;
    add(to_float4_aw(to_float3_s(0), circle(p, 0.006f)), &color);
    color = to_float4_aw(swixyz(color) * shadow(shape.w),color.w);
    
    // bouncing buddies
    const float instances = 5.0f;
    for (float i = 0.0f; i < instances; ++i)
    {
        float ii = i/(instances);
        float iy = i/(instances-1.0f);
        float t = (iTime*globalSpeed + iy);
        
        // distribute instances
        p = pos;
        p.x += (iy*2.0f-1.0f)*0.6f;
        p.y -= 0.15f;
        
        // scale
        p *= 2.0f;
        
        // jump animation
        float ta = fract_f(t)*6.283f;
        float tt = _sinf(clp(fract_f(t*0.5f)*3.0f)*3.14f);
        p += to_float2(0.0f,-_fabs(_sinf(ta))*2.0f)*tt*0.3f; // looping
        p.y *= 1.0f+0.1f*_sinf(ta*4.0f)*tt; // stretch
        
        float dist = length(animation(p,t));
        
        // color palette by Inigo Quilez
        float3 tint = 0.5f+0.5f*cos_f3(to_float3(0.0f,0.3f,0.6f)*6.28f + i*5.0f + dist*-2.0f);
        
        // glow
        color = to_float4_aw(swixyz(color) + tint * clp(-dist+0.4f)*2.0f, color.w);
        
        // add shape to frame
        add(buddy(p, tint, t, ii), &color);
    }

  color.w = params->a0;

  EPILOGUE(color);
}


