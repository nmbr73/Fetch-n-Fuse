
//__DEVICE__ float3 cos_f3(float3 i) {float3 r; r.x = _cosf(i.x); r.y = _cosf(i.y); r.z = _cosf(i.z); return r;}



// Inigo Quilez
// https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
__DEVICE__ float sdArc( in float2 p, in float ta, in float tb, in float ra, float rb )
{
    float2 sca = to_float2(_sinf(ta),_cosf(ta));
    float2 scb = to_float2(_sinf(tb),_cosf(tb));
    p = mul_f2_mat2(p,to_mat2(sca.x,sca.y,-sca.y,sca.x));
    p.x = _fabs(p.x);
    float k = (scb.y*p.x>scb.x*p.y) ? dot(p,scb) : length(p);
    return _sqrtf( dot(p,p) + ra*ra - 2.0f*ra*k ) - rb;
}

// snippets
#define fill(sdf) (smoothstep(0.001f, 0.0f, sdf))
#define repeat(p,r) (mod_f(p,r)-r/2.0f)

__DEVICE__ mat2 rot(float a) { float c=_cosf(a),s=_sinf(a); return to_mat2(c,-s,s,c); }
__DEVICE__ float circle (float2 p, float size)
{
    return length(p)-size;
}

// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}
__DEVICE__ float3 hash31(float p)
{
   float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
   p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
   return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}




// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// Fork of "happy bouncing variation 1" by leon. https://shadertoy.com/view/ftGXR1
// 2021-12-22 00:28:04

// Fork of "happy bouncing" by leon. https://shadertoy.com/view/flyXRh
// 2021-12-22 00:11:16

// "happy bouncing"
// shader about boucing animation, space transformation, easing functions,
// funny shape and colorful vibes.
// by leon denise (2021-12-21)
// licensed under hippie love conspiracy

// using Inigo Quilez works:
// arc sdf from https://www.shadertoy.com/view/wl23RK
// color palette https://iquilezles.org/www/articles/palettes/palettes.htm



// easing curves (not easy to tweak)
// affect timing of transformations;

__DEVICE__ float jump (float t, int modus)
{
    t = _fminf(1.0f, t*4.0f);
    t = _fabs(_sinf(t*3.1415f));
    return _powf(sin(t*3.14f/2.0f), (modus==2?0.5f:1.9f));
}

__DEVICE__ float walk (float t)
{
    t = _mix(_powf(t,0.5f), _powf(t, 2.0f), t);
    return (_cosf(t*3.1415f*2.0f));
}

__DEVICE__ float swing (float t, int modus)
{
    if(modus!=2)
    {
      if (modus==0) t = t*2.0f;
      t = _powf(t, 0.5f);
    }
    t = _sinf(t*3.14f*2.0f);
    return t;
}

__DEVICE__ float stretch (float t, int modus)
{
    float tt;
    if (modus==2)
      tt = _sinf(_powf(t, 2.0f)*10.0f);
    else
      tt = _cosf(_powf(t, 0.2f)*30.0f);
    if (modus==1)  tt = _sinf(_powf(t, 0.2f)*10.0f);

    return tt;
}

__DEVICE__ float bounce (float t, int modus)
{
  float tt;
  if (modus==2)
    tt = _cosf(_powf(t, 0.5f)*3.14f);
  else
    tt = _cosf(_powf(t, 0.2f)*(modus==1?6.38f:10.0f));
  return tt;
}

// list of transformation (fun to tweak)
__DEVICE__ float2 animation(float2 p, float t, float bodySize, int modus)
{
    t = fract(t);

    p.y -= bodySize-(modus==2?0.0f:0.5f);
    //sidebar
    if(modus==2)     p.y -= 0.1f;

    p.y -= jump(t,modus)*0.5f;

    if(modus==2) return p;

    if (modus==0) p.x += walk(t)*0.1f;

    p.x *= stretch(t, modus)*-0.2f+1.0f;

    // bounce stretch with collision
    float b = bounce(t,modus)*-0.2f;
    p.y *= b+1.0f;
    p.y += _fabs(b)*bodySize;

    return p;
}

__DEVICE__ float4 sdEyes (float2 p, float t, float3 tint, float sens, float bodySize, int modus, float2 size, float divergence)
{
    float3 col = to_float3_s(0);
    float shape = 100.0f;

    // eyes positions
    p = animation(p, t,bodySize, modus);
    p = mul_f2_mat2(p,rot(swing(t,modus)*-0.5f));
    p -= to_float2(0.03f, bodySize+size.x*0.2f);
    p.x -= divergence*sens;

    // globe shape
    float eyes = circle(p, size.x);
    col = _mix(col, tint, fill(eyes));
    shape = _fminf(shape, eyes);

    // white eye shape
    eyes = circle(p, size.y);
    col = _mix(col, to_float3_s(1), fill(eyes));

    // black dot shape
    eyes = circle(p, 0.02f);
    col = _mix(col, to_float3_s(0), fill(eyes));

    return to_float4_aw(col, shape);
}




__KERNEL__ void happybouncingFuse(
  float4 color,
  float2 pixel,
  float2 iResolution,
  float iTime

 ){
    CONNECT_SLIDER0(variante,0.0f,2.0f,0.0f); // Name der 'float' Variable, Min, Max, und Default-Wert (Default wird hier nicht, aber spaeter in der Fuse verwendet)

    int modus = 0;    // Varianten 0:Original

    if (variante == 1.0f) modus = 1;
    if (variante == 2.0f) modus = 2;

    // global variable
    float bodySize = 0.2f;

    // shape eyes
    float2 size = to_float2(0.07f, 0.05f);
    float divergence = 0.06f;


    float2 uv = pixel/iResolution;

    color = to_float4(0,0,0,1);

    // ground
    //color.rgb += _mix(to_float3(0.945f,0.945f,0.792f), to_float3(0.820f,0.482f,0.694f), smoothstep(0.0f,0.2f,uv.y-0.2f));
    if(modus==2) color = to_float4_aw(swi3(color,x,y,z) + to_float3_s(0.25f)*step(uv.y,0.1f), color.w);

    // number of friends
    float buddies = 5.0f;

    if (modus == 1) buddies = 3.0f;
    if (modus == 2) buddies = 6.0f;

    for (float i = 0.0f; i < buddies; i+=1.0f)
    {
        // usefull to dissociate instances
        float ii = i/(buddies-(modus==2?0.0f:1.0f));
        // float iii = 1.0f-ii; unused!

        float iy = i/(buddies-1.0f); //Modus2


        // translate instances
        float2 pp;
        if (modus == 2)
        {
          pp = (pixel-to_float2(0.5f,0)*iResolution)/iResolution.y;
          pp.x += (iy*2.0f-1.0f)*0.5f;
          pp *= 2.0f;
        }
        else
        {
          pp = (pixel-0.5f*iResolution)/iResolution.y;
          pp.x += (ii*2.0f-1.0f)*0.4f;
        }

        if (modus == 1) pp.y -= 0.1f;

        // time
        float t = fract(iTime*0.5f + ii * 0.5f);

        // there will be sdf shapes
        float shape = 1000.0f;
        float2 p;

        // there will be layers
        float3 col = to_float3_s(0);


        if (modus == 2)
        {
          // color palette
          // Inigo Quilez (https://iquilezles.org/www/articles/palettes/palettes.htm)
          float3 tint = 0.5f+0.5f*cos_f3(to_float3(0.0f,0.3f,0.6f)*6.28f+i-length(animation(pp-to_float2(0,0.1f),t,bodySize,modus))*3.0f);

          // body shape
          p = animation(pp, t,bodySize,modus);
          p.x *= stretch(t,modus)*-0.2f+1.0f;
          float body = circle(p, bodySize);
          col += tint*fill(body);
          shape = _fminf(shape, body);
          float4 eyes = sdEyes(pp, t-0.03f, tint, -1.0f,bodySize,modus,size,divergence);
          col = _mix(col, swi3(eyes,x,y,z), step(eyes.w,0.0f));
          shape = _fminf(shape, eyes.w);
          eyes = sdEyes(pp, t-0.01f, tint, 1.0f,bodySize,modus,size,divergence);
          col = _mix(col, swi3(eyes,x,y,z), step(eyes.w,0.0f));
          shape = _fminf(shape, eyes.w);

          // smile animation
          float anim = _cosf(_powf(t, 2.0f)*6.28f)*0.5f+0.5f;

          // smile position
          p = animation(pp, t-0.01f,bodySize,modus);
          p -= bodySize*to_float2(0.1f, 0.6f-1.0f*anim);
          float2 q = p;

          // arc (fun to tweak)
          float smile = _mix(0.0f, 1.0f, anim);//+(0.5f+0.5f*_sinf(ii*12.0f+iTime*12.0f*ii));
          float thin = _mix(0.05f, 0.01f, anim);//+0.04f*(0.5f+0.5f*_sinf(ii*12.0f+iTime*22.0f*ii));
          float d = sdArc(p,-3.14/2.0f, smile, 0.1f, thin);

          // black line
          col = _mix(col, tint*(fract(q.y*5.0f)*0.7f+0.3f), fill(d));

          // add buddy to frame
          float ao = clamp(shape+0.9f,0.0f,1.0f);

          color = to_float4_aw(_mix(swi3(color,x,y,z) * ao, col, step(shape, 0.0f)),color.w);
        }
        else
        {
          // color palette
          // Inigo Quilez (https://iquilezles.org/www/articles/palettes/palettes.htm)
          float3 tint = 0.5f+0.5f*cos_f3(to_float3(0.0f,0.3f,0.6f)*6.28f+i-length(animation(pp,t,bodySize,modus))*3.0f);

          if (modus == 1) tint = 0.5f+0.5f*cos_f3(to_float3(0.0f,0.3f,0.6f)*6.28f+i-length(animation(pp-to_float2(0,0.1f),t,bodySize,modus))*3.0f);


          // body shape
          float body = circle(animation(pp, t, bodySize, modus), bodySize);
          col += tint*fill(body);
          shape = _fminf(shape, body);

          // eyes positions
          p = animation(pp, t+0.02f,bodySize, modus);
          p = mul_f2_mat2(p,rot(swing(t, modus)*-0.5f));
          p -= to_float2(0.03f, bodySize+size.x*0.2f);
          p.x = _fabs(p.x)-divergence;

          // globe shape
          float eyes = circle(p, size.x);
          shape = _fminf(shape, eyes);
          col = _mix(col, tint, fill(eyes));

          // white eye shape
          eyes = circle(p, size.y);
          col = _mix(col, to_float3_s(1), fill(eyes));
          shape = _fminf(shape, eyes);

          // black dot shape
          eyes = circle(p, 0.02f);
          col = _mix(col, to_float3_s(0), fill(eyes));

          // smile animation
          float anim = _cosf(_powf(t, 0.5f)*6.28f)*0.5f+0.5f;

          // smile position
          p = animation(pp, t-0.02f, bodySize, modus);
          p = mul_f2_mat2(p,rot(swing(t, modus)*- (modus==1?0.5f:0.9f)));
          p -= bodySize*to_float2(0.5f, 0.5f+anim*0.5f);
          if (modus==1)   p -= bodySize*to_float2(0.4f, 1.0f-1.5f*anim);



          // arc (fun to tweak)
          float smile = _mix(0.0f, 1.0f, anim);//+(0.5f+0.5f*_sinf(ii*12.0f+iTime*12.0f*ii));
          float thin = _mix(0.1f, 0.02f, anim);//+0.04f*(0.5f+0.5f*_sinf(ii*12.0f+iTime*22.0f*ii));
          float d = sdArc(p,-3.14/2., smile, 0.1f, thin);

          // mouth shape
          d = d-_mix(0.01f, 0.04f, anim);
          shape = _fminf(shape, d);
          col = _mix(col, tint*(modus==1?(1.0f-p.x):1.2f), fill(d));

          // black line
          col = _mix(col, tint*0.5f, fill(d+0.05f));

          // add color to frame
          color = to_float4_aw(_mix(swi3(color,x,y,z), col, step(shape, 0.0f)),color.w);
        }
    }

  SetFragmentShaderComputedColor(color);
}
