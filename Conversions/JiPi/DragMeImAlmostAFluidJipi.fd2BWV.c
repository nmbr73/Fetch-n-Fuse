
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

// FLUID PART

//float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}

__DEVICE__ float4 t (float2 v, int a, int b, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,fract_f2((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,fract_f2(v/ur));}

__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}


__KERNEL__ void DragMeImAlmostAFluidJipiFuse__Buffer_A(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5f*ur);
        // make a small right pointing velocity in the middle
        Co = to_float4(0.1f*_expf(-0.01f*q*q),0,0,2.0f*_expf(-0.01f*q*q));
    } else {
        // start where the pixel is and make a box around it
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= swi2(t(v,ur,iChannel0),x,y);
            A -= swi2(t(A,ur,iChannel0),x,y);
            B -= swi2(t(B,ur,iChannel0),x,y);
            C -= swi2(t(C,ur,iChannel0),x,y);
            D -= swi2(t(D,ur,iChannel0),x,y);
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        float4 me = t(v,0,0,ur,iChannel0);
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        //average the neighbors to allow values to blend
        float4 ne = 0.25f*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
        me = _mix(t(v,ur,iChannel0),ne,to_float4(0.06f,0.06f,1.0f,0.0f));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.0f);
    
        // PRESSURE GRADIENT
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        // add the pressure gradient to the velocity
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        // MOUSE MOVEMENT
        float4 mouse = texture(iChannel1,to_float2_s(0.5f));
        float q = ln(U,swi2(mouse,x,y),swi2(mouse,z,w));
        float2 m = swi2(mouse,x,y)-swi2(mouse,z,w);
        float l = length(m);
        if (l>0.0f) m = _fminf(l,10.0f)*m/l;
        // add a line from the mouse to the velocity field and add some color
        swi3S(me,x,y,w, swi3(me,x,y,w) + 0.03f*_expf(-6e-2*q*q*q)*to_float3_aw(m,20.0f));
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -0.6f, 0.6f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// MOUSE
__KERNEL__ void DragMeImAlmostAFluidJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f; 
    
    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


// FLUID PART
#ifdef XXX
float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,fract((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,fract(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif
__KERNEL__ void DragMeImAlmostAFluidJipiFuse__Buffer_C(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5f*ur);
        // make a small right pointing velocity in the middle
        Co = to_float4(0.1f*_expf(-0.01f*q*q),0,0,2.0f*_expf(-0.01f*q*q));
    } else {
        // start where the pixel is and make a box around it
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= swi2(t(v,ur,iChannel0),x,y);
            A -= swi2(t(A,ur,iChannel0),x,y);
            B -= swi2(t(B,ur,iChannel0),x,y);
            C -= swi2(t(C,ur,iChannel0),x,y);
            D -= swi2(t(D,ur,iChannel0),x,y);
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        float4 me = t(v,0,0,ur,iChannel0);
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        //average the neighbors to allow values to blend
        float4 ne = 0.25f*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
        me = _mix(t(v,ur,iChannel0),ne,to_float4(0.06f,0.06f,1.0f,0.0f));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.0f);
    
        // PRESSURE GRADIENT
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        // add the pressure gradient to the velocity
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        // MOUSE MOVEMENT
        float4 mouse = texture(iChannel1,to_float2_s(0.5f));
        float q = ln(U,swi2(mouse,x,y),swi2(mouse,z,w));
        float2 m = swi2(mouse,x,y)-swi2(mouse,z,w);
        float l = length(m);
        if (l>0.0f) m = _fminf(l,10.0f)*m/l;
        // add a line from the mouse to the velocity field and add some color
        swi3S(me,x,y,w, swi3(me,x,y,w) + 0.03f*_expf(-6e-2*q*q*q)*to_float3_aw(m,20.0f));
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -0.6f, 0.6f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// FLUID PART
#ifdef XXX
float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,fract((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,fract(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif

__KERNEL__ void DragMeImAlmostAFluidJipiFuse__Buffer_D(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 U = uu+0.5f;
    float2  ur = iResolution;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5f*ur);
        // make a small right pointing velocity in the middle
        Co = to_float4(0.1f*_expf(-0.01f*q*q),0,0,2.0f*_expf(-0.01f*q*q));
    } else {
        // start where the pixel is and make a box around it
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= swi2(t(v,ur,iChannel0),x,y);
            A -= swi2(t(A,ur,iChannel0),x,y);
            B -= swi2(t(B,ur,iChannel0),x,y);
            C -= swi2(t(C,ur,iChannel0),x,y);
            D -= swi2(t(D,ur,iChannel0),x,y);
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        float4 me = t(v,0,0,ur,iChannel0);
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        //average the neighbors to allow values to blend
        float4 ne = 0.25f*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
        me = _mix(t(v,ur,iChannel0),ne,to_float4(0.06f,0.06f,1.0f,0.0f));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.0f);
    
        // PRESSURE GRADIENT
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        // add the pressure gradient to the velocity
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        // MOUSE MOVEMENT
        float4 mouse = texture(iChannel1,to_float2_s(0.5f));
        float q = ln(U,swi2(mouse,x,y),swi2(mouse,z,w));
        float2 m = swi2(mouse,x,y)-swi2(mouse,z,w);
        float l = length(m);
        if (l>0.0f) m = _fminf(l,10.0f)*m/l;
        // add a line from the mouse to the velocity field and add some color
        swi3S(me,x,y,w, swi3(me,x,y,w) + 0.03f*_expf(-6e-2*q*q*q)*to_float3_aw(m,20.0f));
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -0.6f, 0.6f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*

  So a litte bit ago I made this:  
    https://www.shadertoy.com/view/lsVfRd

  but I forgiot the convective term...

  I didnt realize the velocity advects through itself
  kinda trippy tbh

  fluids are like multi-dimensional infinitesimal newton's cradles
  
  


*/
__KERNEL__ void DragMeImAlmostAFluidJipiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;

    float4 g = texture(iChannel0,U/iResolution);
    float2 d = to_float2(
      texture(iChannel0,(U+to_float2(1,0))/iResolution).w-texture(iChannel0,(U-to_float2(1,0))/iResolution).w,
      texture(iChannel0,(U+to_float2(0,1))/iResolution).w-texture(iChannel0,(U-to_float2(0,1))/iResolution).w
      );
    float3 n = normalize(to_float3_aw(d,0.1f));
    float a = _acosf(dot(n,normalize(to_float3_s(1))))/3.141593f;
    g.w = 2.0f*_sqrtf(g.w);
    float3 color = 1.3f*(0.5f+0.5f*sin_f3(abs_f3(swi3(g,x,y,z))*to_float3(20,20,1)+2.0f*g.w*to_float3(_sinf(g.w),_cosf(g.w*2.0f),3)))*(_fabs(a)*0.5f+0.5f);
    C = to_float4_aw( color*color*1.2 ,1);

  SetFragmentShaderComputedColor(C);
}