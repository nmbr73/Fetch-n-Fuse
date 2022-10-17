
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// some constants
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.


#define KERNEL 29 // kernel for pressure solver: 29, 11, 7 or 3!
#define RUNGE_KUTTA 4 // order. 4, 2 (midpoint) or 1 (=Euler)


__DEVICE__ bool IsObstacle(float2 mouse, float2 pos, bool use_airfoil, float kRadius, float FoilCamber, float FoilSize, float FoilAlpha) {
  
  // airfoil characteristics
  //const float FoilCamber = 0.3f;   // higher = more bent
  //const float FoilSize = 120.0f;
  //const float FoilAlpha = 25.0f;   // angle of attack in degrees

  //const float kRadius = 12.0f;  // ball radius  
  
  if (length(mouse) < 0.01f) mouse = to_float2(100.0f, 180.0f);
  pos = mouse - pos;

  if (!use_airfoil) {    // array of balls:
    //return (length(to_float2(pos.x, fract(_fabs(pos.y) / 50.0f) * 50.0f)) < kRadius);
    //return (length(pos) < kRadius); // Ball
    
    if (FoilCamber > FoilSize ) return true;
    return false;
    
  }
  // Airfoil profile following more or less NACA formulae
  // see: https://en.wikipedia.org/wiki/NACA_airfoil#Equation_for_a_symmetrical_4-digit_NACA_airfoil
  float alpha = 3.1415f * (180.0f + FoilAlpha) / 180.0f;  // angle of attack
  mat2 M = to_mat2(_cosf(alpha)/ FoilSize, _sinf(alpha)/ FoilSize, -_sinf(alpha)/ FoilSize, _cosf(alpha)/ FoilSize) ;
  pos = mul_mat2_f2(M , pos);
  float x = pos.x, x2 = x * x;
  if (x < 0.0f || x > 1.0f) return false;
  // mor or le
  // mean camber line
  float xm = x * (1.0f - x) * FoilCamber;
  // thickness
  float th = 0.2969f * _sqrtf(x) - 0.1260f * x - 0.3516f * x2 + 0.2843f * x * x2 - 0.1036f * x2 * x2;
  float y = pos.y - xm;
  return _fabs(y) < th;
}



__DEVICE__ float SegmentDistance(float2 p, float2 p1, float2 p2) {
  float2 dir = p2 - p1;
  float d2 = dot(dir, dir);
  float d3 = dot(dir, p - p1) / d2;
  float frac = clamp(d3, 0.0f, 1.0f);
  return length(p - p1 - frac * dir);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


// Advects U->U* using the previous velocity field in BufD.
// bufA will contains advected {vx, vy, color1/2}
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

__DEVICE__ float4 Advect(float2 p, float2 iResolution, __TEXTURE2D__ iChannel0, float dt) { // using 4-order Runge-Kutta

  float2 norm = 1.0f / iResolution;
  p *= norm;
  float2 H = dt * norm;
  float2 k1 = H * swi2(_tex2DVecN(iChannel0,p.x,p.y,15),x,y);
#if (RUNGE_KUTTA == 4)
  float2 k2 = H * swi2(texture(iChannel0, p - 0.5f * k1),x,y);
  float2 k3 = H * swi2(texture(iChannel0, p - 0.5f * k2),x,y);
  float2 k4 = H * swi2(texture(iChannel0, p -       k3),x,y);
  float2 dp = (0.5f * (k1 + k4) + k2 + k3) / 3.0f;
#elif (RUNGE_KUTTA == 2)
  float2 k2 = H * swi2(texture(iChannel0, p - 0.5f * k1),x,y);
  float2 dp = k2;
#else  // RUNGE_KUTTA == 1
  float2 dp = k1;
#endif
return texture(iChannel0, p - dp);
}

__KERNEL__ void FluidflowsimulationJipi164Fuse__Buffer_A(float4 fragColor, float2 pos, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(use_airfoil,1);
  CONNECT_SLIDER0(kRadius, 0.0f, 30.0f, 12.0f);
  CONNECT_SLIDER1(FoilCamber, -10.0f, 1.0f, 0.3f);
  CONNECT_SLIDER2(FoilSize, 0.0f, 300.0f, 120.0f);
  CONNECT_SLIDER3(FoilAlpha, -10.0f, 50.0f, 25.0f);

  pos+=0.5f;
  
  //const bool use_airfoil = true;    // balls or airfoil
  const float dt = 1.0f;     // step
  const float Vo = 1.5f;     // initial / typical velocity

  float4 src = Advect(pos, iResolution, iChannel0, dt);  // advect backward
  float2 v = swi2(src,x,y);   // velocity
  float2 c = swi2(src,z,w);   // color
  // force some boundary conditions

// initial field
  if (iFrame <= 1 || Reset){
    v = to_float2(Vo, 0.0f);
    c.x = step(_sinf(242.223f * _sinf(pos.x * 320.231f + pos.y * 13.92f)), -0.4f);
    c.y = 0.0f;
  }
  // in/out flow from left to right
  if (pos.x < 2.0f || pos.x >= iResolution.x - 2.0f) {
    v = to_float2(Vo, 0.0f);
  }
  // rough canal
  if (pos.y < 2.0f || pos.y >= iResolution.y - 2.0f) {
    v = to_float2(0.0f, 0.0f);
  }
  if (pos.x < 5.0f) {  // some tracer injection
    c.x = 1.0f - step(_cosf(pos.y * 0.3f), 0.2f);
  }
  
  if ( !use_airfoil )
  {
    FoilCamber = texture( iChannel1, pos/iResolution).w; 
  }
  
  if (IsObstacle(swi2(iMouse,x,y), swi2(pos,x,y), use_airfoil, kRadius, FoilCamber, FoilSize, FoilAlpha)) {
    v = to_float2(0.0f, 0.0f);
    c.x = 0.0f;
    c.y = step(_sinf(pos.y / 2.0f), 0.9f);  // color tracers from obstacle
  }
  fragColor = to_float4_f2f2(v, c);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Computes D = K.∇.U*   [with K=-dt/(2.rho.dx)]
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

__DEVICE__ float Divergence(float2 p, float2 iResolution, __TEXTURE2D__ iChannel0) {
  float2 norm = 1.0f / iResolution;
  float2 P = p * norm;
  float2 dx = to_float2(norm.x,     0.0f);
  float2 dy = to_float2(    0.0f, norm.y);  
  float dv_dx = texture(iChannel0, P + dx).x
              - texture(iChannel0, P - dx).x;
  float dv_dy = texture(iChannel0, P + dy).y
              - texture(iChannel0, P - dy).y;
  return -0.5f * (dv_dx + dv_dy);
}

__KERNEL__ void FluidflowsimulationJipi164Fuse__Buffer_B(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel0)
{
  pos+=0.5f;

  fragColor = to_float4(0.0f, 0.0f, 0.0f, Divergence(pos, iResolution, iChannel0));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// Compute pressure for Laplacian equation ∇²P = ∇.U*
//
// The Jacobi method would iterate on the array
//   p_i,j = 1/4 * (div_i,j + p_i+2,j + p_i-2,j + p_i,j+2 + p_i,j-2)
// But since we work in-place in BufferC, this is more a Gauss-Seidel method!
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

#define DIV(X, Y) texture(iChannel0, (pos + to_float2(X, Y)) * norm).w
#define P(X, Y)   texture(iChannel1, (pos + to_float2(X, Y)) * norm).w  // re-use buf C

__KERNEL__ void FluidflowsimulationJipi164Fuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  pos+=0.5f;

  float2 norm = 1.0f / iResolution;
#if (KERNEL == 29)
  float div = 0.0f;
  div +=            1.0f * DIV(  0,-14);
  div +=           14.0f * DIV( -1,-13);
  div +=            4.0f * DIV(  0,-13);
  div +=           14.0f * DIV(  1,-13);
  div +=           91.0f * DIV( -2,-12);
  div +=           52.0f * DIV( -1,-12);
  div +=          212.0f * DIV(  0,-12);
  div +=           52.0f * DIV(  1,-12);
  div +=           91.0f * DIV(  2,-12);
  div +=          364.0f * DIV( -3,-11);
  div +=          312.0f * DIV( -2,-11);
  div +=         1466.0f * DIV( -1,-11);
  div +=          740.0f * DIV(  0,-11);
  div +=         1466.0f * DIV(  1,-11);
  div +=          312.0f * DIV(  2,-11);
  div +=          364.0f * DIV(  3,-11);
  div +=         1001.0f * DIV( -4,-10);
  div +=         1144.0f * DIV( -3,-10);
  div +=         6152.0f * DIV( -2,-10);
  div +=         4760.0f * DIV( -1,-10);
  div +=        10841.0f * DIV(  0,-10);
  div +=         4760.0f * DIV(  1,-10);
  div +=         6152.0f * DIV(  2,-10);
  div +=         1144.0f * DIV(  3,-10);
  div +=         1001.0f * DIV(  4,-10);
  div +=         2002.0f * DIV( -5, -9);
  div +=         2860.0f * DIV( -4, -9);
  div +=        17534.0f * DIV( -3, -9);
  div +=        18392.0f * DIV( -2, -9);
  div +=        48356.0f * DIV( -1, -9);
  div +=        33104.0f * DIV(  0, -9);
  div +=        48356.0f * DIV(  1, -9);
  div +=        18392.0f * DIV(  2, -9);
  div +=        17534.0f * DIV(  3, -9);
  div +=         2860.0f * DIV(  4, -9);
  div +=         2002.0f * DIV(  5, -9);
  div +=         3003.0f * DIV( -6, -8);
  div +=         5148.0f * DIV( -5, -8);
  div +=        35948.0f * DIV( -4, -8);
  div +=        47740.0f * DIV( -3, -8);
  div +=       144851.0f * DIV( -2, -8);
  div +=       137168.0f * DIV( -1, -8);
  div +=       231888.0f * DIV(  0, -8);
  div +=       137168.0f * DIV(  1, -8);
  div +=       144851.0f * DIV(  2, -8);
  div +=        47740.0f * DIV(  3, -8);
  div +=        35948.0f * DIV(  4, -8);
  div +=         5148.0f * DIV(  5, -8);
  div +=         3003.0f * DIV(  6, -8);
  div +=         3432.0f * DIV( -7, -7);
  div +=         6864.0f * DIV( -6, -7);
  div +=        54714.0f * DIV( -5, -7);
  div +=        88044.0f * DIV( -4, -7);
  div +=       307942.0f * DIV( -3, -7);
  div +=       376104.0f * DIV( -2, -7);
  div +=       744652.0f * DIV( -1, -7);
  div +=       620112.0f * DIV(  0, -7);
  div +=       744652.0f * DIV(  1, -7);
  div +=       376104.0f * DIV(  2, -7);
  div +=       307942.0f * DIV(  3, -7);
  div +=        88044.0f * DIV(  4, -7);
  div +=        54714.0f * DIV(  5, -7);
  div +=         6864.0f * DIV(  6, -7);
  div +=         3432.0f * DIV(  7, -7);
  div +=         3003.0f * DIV( -8, -6);
  div +=         6864.0f * DIV( -7, -6);
  div +=        62832.0f * DIV( -6, -6);
  div +=       118800.0f * DIV( -5, -6);
  div +=       479097.0f * DIV( -4, -6);
  div +=       719880.0f * DIV( -3, -6);
  div +=      1673336.0f * DIV( -2, -6);
  div +=      1845224.0f * DIV( -1, -6);
  div +=      2622481.0f * DIV(  0, -6);
  div +=      1845224.0f * DIV(  1, -6);
  div +=      1673336.0f * DIV(  2, -6);
  div +=       719880.0f * DIV(  3, -6);
  div +=       479097.0f * DIV(  4, -6);
  div +=       118800.0f * DIV(  5, -6);
  div +=        62832.0f * DIV(  6, -6);
  div +=         6864.0f * DIV(  7, -6);
  div +=         3003.0f * DIV(  8, -6);
  div +=         2002.0f * DIV( -9, -5);
  div +=         5148.0f * DIV( -8, -5);
  div +=        54714.0f * DIV( -7, -5);
  div +=       118800.0f * DIV( -6, -5);
  div +=       554232.0f * DIV( -5, -5);
  div +=       989664.0f * DIV( -4, -5);
  div +=      2696420.0f * DIV( -3, -5);
  div +=      3752136.0f * DIV( -2, -5);
  div +=      6439522.0f * DIV( -1, -5);
  div +=      6179364.0f * DIV(  0, -5);
  div +=      6439522.0f * DIV(  1, -5);
  div +=      3752136.0f * DIV(  2, -5);
  div +=      2696420.0f * DIV(  3, -5);
  div +=       989664.0f * DIV(  4, -5);
  div +=       554232.0f * DIV(  5, -5);
  div +=       118800.0f * DIV(  6, -5);
  div +=        54714.0f * DIV(  7, -5);
  div +=         5148.0f * DIV(  8, -5);
  div +=         2002.0f * DIV(  9, -5);
  div +=         1001.0f * DIV(-10, -4);
  div +=         2860.0f * DIV( -9, -4);
  div +=        35948.0f * DIV( -8, -4);
  div +=        88044.0f * DIV( -7, -4);
  div +=       479097.0f * DIV( -6, -4);
  div +=       989664.0f * DIV( -5, -4);
  div +=      3156832.0f * DIV( -4, -4);
  div +=      5324000.0f * DIV( -3, -4);
  div +=     11031091.0f * DIV( -2, -4);
  div +=     13981364.0f * DIV( -1, -4);
  div +=     18233940.0f * DIV(  0, -4);
  div +=     13981364.0f * DIV(  1, -4);
  div +=     11031091.0f * DIV(  2, -4);
  div +=      5324000.0f * DIV(  3, -4);
  div +=      3156832.0f * DIV(  4, -4);
  div +=       989664.0f * DIV(  5, -4);
  div +=       479097.0f * DIV(  6, -4);
  div +=        88044.0f * DIV(  7, -4);
  div +=        35948.0f * DIV(  8, -4);
  div +=         2860.0f * DIV(  9, -4);
  div +=         1001.0f * DIV( 10, -4);
  div +=          364.0f * DIV(-11, -3);
  div +=         1144.0f * DIV(-10, -3);
  div +=        17534.0f * DIV( -9, -3);
  div +=        47740.0f * DIV( -8, -3);
  div +=       307942.0f * DIV( -7, -3);
  div +=       719880.0f * DIV( -6, -3);
  div +=      2696420.0f * DIV( -5, -3);
  div +=      5324000.0f * DIV( -4, -3);
  div +=     13195432.0f * DIV( -3, -3);
  div +=     21066864.0f * DIV( -2, -3);
  div +=     35250918.0f * DIV( -1, -3);
  div +=     38793668.0f * DIV(  0, -3);
  div +=     35250918.0f * DIV(  1, -3);
  div +=     21066864.0f * DIV(  2, -3);
  div +=     13195432.0f * DIV(  3, -3);
  div +=      5324000.0f * DIV(  4, -3);
  div +=      2696420.0f * DIV(  5, -3);
  div +=       719880.0f * DIV(  6, -3);
  div +=       307942.0f * DIV(  7, -3);
  div +=        47740.0f * DIV(  8, -3);
  div +=        17534.0f * DIV(  9, -3);
  div +=         1144.0f * DIV( 10, -3);
  div +=          364.0f * DIV( 11, -3);
  div +=           91.0f * DIV(-12, -2);
  div +=          312.0f * DIV(-11, -2);
  div +=         6152.0f * DIV(-10, -2);
  div +=        18392.0f * DIV( -9, -2);
  div +=       144851.0f * DIV( -8, -2);
  div +=       376104.0f * DIV( -7, -2);
  div +=      1673336.0f * DIV( -6, -2);
  div +=      3752136.0f * DIV( -5, -2);
  div +=     11031091.0f * DIV( -4, -2);
  div +=     21066864.0f * DIV( -3, -2);
  div +=     44114320.0f * DIV( -2, -2);
  div +=     67161776.0f * DIV( -1, -2);
  div +=     91488921.0f * DIV(  0, -2);
  div +=     67161776.0f * DIV(  1, -2);
  div +=     44114320.0f * DIV(  2, -2);
  div +=     21066864.0f * DIV(  3, -2);
  div +=     11031091.0f * DIV(  4, -2);
  div +=      3752136.0f * DIV(  5, -2);
  div +=      1673336.0f * DIV(  6, -2);
  div +=       376104.0f * DIV(  7, -2);
  div +=       144851.0f * DIV(  8, -2);
  div +=        18392.0f * DIV(  9, -2);
  div +=         6152.0f * DIV( 10, -2);
  div +=          312.0f * DIV( 11, -2);
  div +=           91.0f * DIV( 12, -2);
  div +=           14.0f * DIV(-13, -1);
  div +=           52.0f * DIV(-12, -1);
  div +=         1466.0f * DIV(-11, -1);
  div +=         4760.0f * DIV(-10, -1);
  div +=        48356.0f * DIV( -9, -1);
  div +=       137168.0f * DIV( -8, -1);
  div +=       744652.0f * DIV( -7, -1);
  div +=      1845224.0f * DIV( -6, -1);
  div +=      6439522.0f * DIV( -5, -1);
  div +=     13981364.0f * DIV( -4, -1);
  div +=     35250918.0f * DIV( -3, -1);
  div +=     67161776.0f * DIV( -2, -1);
  div +=    130000120.0f * DIV( -1, -1);
  div +=    192838464.0f * DIV(  0, -1);
  div +=    130000120.0f * DIV(  1, -1);
  div +=     67161776.0f * DIV(  2, -1);
  div +=     35250918.0f * DIV(  3, -1);
  div +=     13981364.0f * DIV(  4, -1);
  div +=      6439522.0f * DIV(  5, -1);
  div +=      1845224.0f * DIV(  6, -1);
  div +=       744652.0f * DIV(  7, -1);
  div +=       137168.0f * DIV(  8, -1);
  div +=        48356.0f * DIV(  9, -1);
  div +=         4760.0f * DIV( 10, -1);
  div +=         1466.0f * DIV( 11, -1);
  div +=           52.0f * DIV( 12, -1);
  div +=           14.0f * DIV( 13, -1);
  div +=            1.0f * DIV(-14,  0);
  div +=            4.0f * DIV(-13,  0);
  div +=          212.0f * DIV(-12,  0);
  div +=          740.0f * DIV(-11,  0);
  div +=        10841.0f * DIV(-10,  0);
  div +=        33104.0f * DIV( -9,  0);
  div +=       231888.0f * DIV( -8,  0);
  div +=       620112.0f * DIV( -7,  0);
  div +=      2622481.0f * DIV( -6,  0);
  div +=      6179364.0f * DIV( -5,  0);
  div +=     18233940.0f * DIV( -4,  0);
  div +=     38793668.0f * DIV( -3,  0);
  div +=     91488921.0f * DIV( -2,  0);
  div +=    192838464.0f * DIV( -1,  0);
  div +=    461273920.0f * DIV(  0,  0);
  div +=    192838464.0f * DIV(  1,  0);
  div +=     91488921.0f * DIV(  2,  0);
  div +=     38793668.0f * DIV(  3,  0);
  div +=     18233940.0f * DIV(  4,  0);
  div +=      6179364.0f * DIV(  5,  0);
  div +=      2622481.0f * DIV(  6,  0);
  div +=       620112.0f * DIV(  7,  0);
  div +=       231888.0f * DIV(  8,  0);
  div +=        33104.0f * DIV(  9,  0);
  div +=        10841.0f * DIV( 10,  0);
  div +=          740.0f * DIV( 11,  0);
  div +=          212.0f * DIV( 12,  0);
  div +=            4.0f * DIV( 13,  0);
  div +=            1.0f * DIV( 14,  0);
  div +=           14.0f * DIV(-13,  1);
  div +=           52.0f * DIV(-12,  1);
  div +=         1466.0f * DIV(-11,  1);
  div +=         4760.0f * DIV(-10,  1);
  div +=        48356.0f * DIV( -9,  1);
  div +=       137168.0f * DIV( -8,  1);
  div +=       744652.0f * DIV( -7,  1);
  div +=      1845224.0f * DIV( -6,  1);
  div +=      6439522.0f * DIV( -5,  1);
  div +=     13981364.0f * DIV( -4,  1);
  div +=     35250918.0f * DIV( -3,  1);
  div +=     67161776.0f * DIV( -2,  1);
  div +=    130000120.0f * DIV( -1,  1);
  div +=    192838464.0f * DIV(  0,  1);
  div +=    130000120.0f * DIV(  1,  1);
  div +=     67161776.0f * DIV(  2,  1);
  div +=     35250918.0f * DIV(  3,  1);
  div +=     13981364.0f * DIV(  4,  1);
  div +=      6439522.0f * DIV(  5,  1);
  div +=      1845224.0f * DIV(  6,  1);
  div +=       744652.0f * DIV(  7,  1);
  div +=       137168.0f * DIV(  8,  1);
  div +=        48356.0f * DIV(  9,  1);
  div +=         4760.0f * DIV( 10,  1);
  div +=         1466.0f * DIV( 11,  1);
  div +=           52.0f * DIV( 12,  1);
  div +=           14.0f * DIV( 13,  1);
  div +=           91.0f * DIV(-12,  2);
  div +=          312.0f * DIV(-11,  2);
  div +=         6152.0f * DIV(-10,  2);
  div +=        18392.0f * DIV( -9,  2);
  div +=       144851.0f * DIV( -8,  2);
  div +=       376104.0f * DIV( -7,  2);
  div +=      1673336.0f * DIV( -6,  2);
  div +=      3752136.0f * DIV( -5,  2);
  div +=     11031091.0f * DIV( -4,  2);
  div +=     21066864.0f * DIV( -3,  2);
  div +=     44114320.0f * DIV( -2,  2);
  div +=     67161776.0f * DIV( -1,  2);
  div +=     91488921.0f * DIV(  0,  2);
  div +=     67161776.0f * DIV(  1,  2);
  div +=     44114320.0f * DIV(  2,  2);
  div +=     21066864.0f * DIV(  3,  2);
  div +=     11031091.0f * DIV(  4,  2);
  div +=      3752136.0f * DIV(  5,  2);
  div +=      1673336.0f * DIV(  6,  2);
  div +=       376104.0f * DIV(  7,  2);
  div +=       144851.0f * DIV(  8,  2);
  div +=        18392.0f * DIV(  9,  2);
  div +=         6152.0f * DIV( 10,  2);
  div +=          312.0f * DIV( 11,  2);
  div +=           91.0f * DIV( 12,  2);
  div +=          364.0f * DIV(-11,  3);
  div +=         1144.0f * DIV(-10,  3);
  div +=        17534.0f * DIV( -9,  3);
  div +=        47740.0f * DIV( -8,  3);
  div +=       307942.0f * DIV( -7,  3);
  div +=       719880.0f * DIV( -6,  3);
  div +=      2696420.0f * DIV( -5,  3);
  div +=      5324000.0f * DIV( -4,  3);
  div +=     13195432.0f * DIV( -3,  3);
  div +=     21066864.0f * DIV( -2,  3);
  div +=     35250918.0f * DIV( -1,  3);
  div +=     38793668.0f * DIV(  0,  3);
  div +=     35250918.0f * DIV(  1,  3);
  div +=     21066864.0f * DIV(  2,  3);
  div +=     13195432.0f * DIV(  3,  3);
  div +=      5324000.0f * DIV(  4,  3);
  div +=      2696420.0f * DIV(  5,  3);
  div +=       719880.0f * DIV(  6,  3);
  div +=       307942.0f * DIV(  7,  3);
  div +=        47740.0f * DIV(  8,  3);
  div +=        17534.0f * DIV(  9,  3);
  div +=         1144.0f * DIV( 10,  3);
  div +=          364.0f * DIV( 11,  3);
  div +=         1001.0f * DIV(-10,  4);
  div +=         2860.0f * DIV( -9,  4);
  div +=        35948.0f * DIV( -8,  4);
  div +=        88044.0f * DIV( -7,  4);
  div +=       479097.0f * DIV( -6,  4);
  div +=       989664.0f * DIV( -5,  4);
  div +=      3156832.0f * DIV( -4,  4);
  div +=      5324000.0f * DIV( -3,  4);
  div +=     11031091.0f * DIV( -2,  4);
  div +=     13981364.0f * DIV( -1,  4);
  div +=     18233940.0f * DIV(  0,  4);
  div +=     13981364.0f * DIV(  1,  4);
  div +=     11031091.0f * DIV(  2,  4);
  div +=      5324000.0f * DIV(  3,  4);
  div +=      3156832.0f * DIV(  4,  4);
  div +=       989664.0f * DIV(  5,  4);
  div +=       479097.0f * DIV(  6,  4);
  div +=        88044.0f * DIV(  7,  4);
  div +=        35948.0f * DIV(  8,  4);
  div +=         2860.0f * DIV(  9,  4);
  div +=         1001.0f * DIV( 10,  4);
  div +=         2002.0f * DIV( -9,  5);
  div +=         5148.0f * DIV( -8,  5);
  div +=        54714.0f * DIV( -7,  5);
  div +=       118800.0f * DIV( -6,  5);
  div +=       554232.0f * DIV( -5,  5);
  div +=       989664.0f * DIV( -4,  5);
  div +=      2696420.0f * DIV( -3,  5);
  div +=      3752136.0f * DIV( -2,  5);
  div +=      6439522.0f * DIV( -1,  5);
  div +=      6179364.0f * DIV(  0,  5);
  div +=      6439522.0f * DIV(  1,  5);
  div +=      3752136.0f * DIV(  2,  5);
  div +=      2696420.0f * DIV(  3,  5);
  div +=       989664.0f * DIV(  4,  5);
  div +=       554232.0f * DIV(  5,  5);
  div +=       118800.0f * DIV(  6,  5);
  div +=        54714.0f * DIV(  7,  5);
  div +=         5148.0f * DIV(  8,  5);
  div +=         2002.0f * DIV(  9,  5);
  div +=         3003.0f * DIV( -8,  6);
  div +=         6864.0f * DIV( -7,  6);
  div +=        62832.0f * DIV( -6,  6);
  div +=       118800.0f * DIV( -5,  6);
  div +=       479097.0f * DIV( -4,  6);
  div +=       719880.0f * DIV( -3,  6);
  div +=      1673336.0f * DIV( -2,  6);
  div +=      1845224.0f * DIV( -1,  6);
  div +=      2622481.0f * DIV(  0,  6);
  div +=      1845224.0f * DIV(  1,  6);
  div +=      1673336.0f * DIV(  2,  6);
  div +=       719880.0f * DIV(  3,  6);
  div +=       479097.0f * DIV(  4,  6);
  div +=       118800.0f * DIV(  5,  6);
  div +=        62832.0f * DIV(  6,  6);
  div +=         6864.0f * DIV(  7,  6);
  div +=         3003.0f * DIV(  8,  6);
  div +=         3432.0f * DIV( -7,  7);
  div +=         6864.0f * DIV( -6,  7);
  div +=        54714.0f * DIV( -5,  7);
  div +=        88044.0f * DIV( -4,  7);
  div +=       307942.0f * DIV( -3,  7);
  div +=       376104.0f * DIV( -2,  7);
  div +=       744652.0f * DIV( -1,  7);
  div +=       620112.0f * DIV(  0,  7);
  div +=       744652.0f * DIV(  1,  7);
  div +=       376104.0f * DIV(  2,  7);
  div +=       307942.0f * DIV(  3,  7);
  div +=        88044.0f * DIV(  4,  7);
  div +=        54714.0f * DIV(  5,  7);
  div +=         6864.0f * DIV(  6,  7);
  div +=         3432.0f * DIV(  7,  7);
  div +=         3003.0f * DIV( -6,  8);
  div +=         5148.0f * DIV( -5,  8);
  div +=        35948.0f * DIV( -4,  8);
  div +=        47740.0f * DIV( -3,  8);
  div +=       144851.0f * DIV( -2,  8);
  div +=       137168.0f * DIV( -1,  8);
  div +=       231888.0f * DIV(  0,  8);
  div +=       137168.0f * DIV(  1,  8);
  div +=       144851.0f * DIV(  2,  8);
  div +=        47740.0f * DIV(  3,  8);
  div +=        35948.0f * DIV(  4,  8);
  div +=         5148.0f * DIV(  5,  8);
  div +=         3003.0f * DIV(  6,  8);
  div +=         2002.0f * DIV( -5,  9);
  div +=         2860.0f * DIV( -4,  9);
  div +=        17534.0f * DIV( -3,  9);
  div +=        18392.0f * DIV( -2,  9);
  div +=        48356.0f * DIV( -1,  9);
  div +=        33104.0f * DIV(  0,  9);
  div +=        48356.0f * DIV(  1,  9);
  div +=        18392.0f * DIV(  2,  9);
  div +=        17534.0f * DIV(  3,  9);
  div +=         2860.0f * DIV(  4,  9);
  div +=         2002.0f * DIV(  5,  9);
  div +=         1001.0f * DIV( -4, 10);
  div +=         1144.0f * DIV( -3, 10);
  div +=         6152.0f * DIV( -2, 10);
  div +=         4760.0f * DIV( -1, 10);
  div +=        10841.0f * DIV(  0, 10);
  div +=         4760.0f * DIV(  1, 10);
  div +=         6152.0f * DIV(  2, 10);
  div +=         1144.0f * DIV(  3, 10);
  div +=         1001.0f * DIV(  4, 10);
  div +=          364.0f * DIV( -3, 11);
  div +=          312.0f * DIV( -2, 11);
  div +=         1466.0f * DIV( -1, 11);
  div +=          740.0f * DIV(  0, 11);
  div +=         1466.0f * DIV(  1, 11);
  div +=          312.0f * DIV(  2, 11);
  div +=          364.0f * DIV(  3, 11);
  div +=           91.0f * DIV( -2, 12);
  div +=           52.0f * DIV( -1, 12);
  div +=          212.0f * DIV(  0, 12);
  div +=           52.0f * DIV(  1, 12);
  div +=           91.0f * DIV(  2, 12);
  div +=           14.0f * DIV( -1, 13);
  div +=            4.0f * DIV(  0, 13);
  div +=           14.0f * DIV(  1, 13);
  div +=            1.0f * DIV(  0, 14);
  div /= 1073741824.0f;
  float p = 0.0f;
  p +=            1.0f * P(  0,-15);
  p +=           15.0f * P( -1,-14);
  p +=           15.0f * P(  1,-14);
  p +=          105.0f * P( -2,-13);
  p +=          225.0f * P(  0,-13);
  p +=          105.0f * P(  2,-13);
  p +=          455.0f * P( -3,-12);
  p +=         1575.0f * P( -1,-12);
  p +=         1575.0f * P(  1,-12);
  p +=          455.0f * P(  3,-12);
  p +=         1365.0f * P( -4,-11);
  p +=         6825.0f * P( -2,-11);
  p +=        11025.0f * P(  0,-11);
  p +=         6825.0f * P(  2,-11);
  p +=         1365.0f * P(  4,-11);
  p +=         3003.0f * P( -5,-10);
  p +=        20475.0f * P( -3,-10);
  p +=        47775.0f * P( -1,-10);
  p +=        47775.0f * P(  1,-10);
  p +=        20475.0f * P(  3,-10);
  p +=         3003.0f * P(  5,-10);
  p +=         5005.0f * P( -6, -9);
  p +=        45045.0f * P( -4, -9);
  p +=       143325.0f * P( -2, -9);
  p +=       207025.0f * P(  0, -9);
  p +=       143325.0f * P(  2, -9);
  p +=        45045.0f * P(  4, -9);
  p +=         5005.0f * P(  6, -9);
  p +=         6435.0f * P( -7, -8);
  p +=        75075.0f * P( -5, -8);
  p +=       315315.0f * P( -3, -8);
  p +=       621075.0f * P( -1, -8);
  p +=       621075.0f * P(  1, -8);
  p +=       315315.0f * P(  3, -8);
  p +=        75075.0f * P(  5, -8);
  p +=         6435.0f * P(  7, -8);
  p +=         6435.0f * P( -8, -7);
  p +=        96525.0f * P( -6, -7);
  p +=       525525.0f * P( -4, -7);
  p +=      1366365.0f * P( -2, -7);
  p +=      1863225.0f * P(  0, -7);
  p +=      1366365.0f * P(  2, -7);
  p +=       525525.0f * P(  4, -7);
  p +=        96525.0f * P(  6, -7);
  p +=         6435.0f * P(  8, -7);
  p +=         5005.0f * P( -9, -6);
  p +=        96525.0f * P( -7, -6);
  p +=       675675.0f * P( -5, -6);
  p +=      2277275.0f * P( -3, -6);
  p +=      4099095.0f * P( -1, -6);
  p +=      4099095.0f * P(  1, -6);
  p +=      2277275.0f * P(  3, -6);
  p +=       675675.0f * P(  5, -6);
  p +=        96525.0f * P(  7, -6);
  p +=         5005.0f * P(  9, -6);
  p +=         3003.0f * P(-10, -5);
  p +=        75075.0f * P( -8, -5);
  p +=       675675.0f * P( -6, -5);
  p +=      2927925.0f * P( -4, -5);
  p +=      6831825.0f * P( -2, -5);
  p +=      9018009.0f * P(  0, -5);
  p +=      6831825.0f * P(  2, -5);
  p +=      2927925.0f * P(  4, -5);
  p +=       675675.0f * P(  6, -5);
  p +=        75075.0f * P(  8, -5);
  p +=         3003.0f * P( 10, -5);
  p +=         1365.0f * P(-11, -4);
  p +=        45045.0f * P( -9, -4);
  p +=       525525.0f * P( -7, -4);
  p +=      2927925.0f * P( -5, -4);
  p +=      8783775.0f * P( -3, -4);
  p +=     15030015.0f * P( -1, -4);
  p +=     15030015.0f * P(  1, -4);
  p +=      8783775.0f * P(  3, -4);
  p +=      2927925.0f * P(  5, -4);
  p +=       525525.0f * P(  7, -4);
  p +=        45045.0f * P(  9, -4);
  p +=         1365.0f * P( 11, -4);
  p +=          455.0f * P(-12, -3);
  p +=        20475.0f * P(-10, -3);
  p +=       315315.0f * P( -8, -3);
  p +=      2277275.0f * P( -6, -3);
  p +=      8783775.0f * P( -4, -3);
  p +=     19324305.0f * P( -2, -3);
  p +=     25050025.0f * P(  0, -3);
  p +=     19324305.0f * P(  2, -3);
  p +=      8783775.0f * P(  4, -3);
  p +=      2277275.0f * P(  6, -3);
  p +=       315315.0f * P(  8, -3);
  p +=        20475.0f * P( 10, -3);
  p +=          455.0f * P( 12, -3);
  p +=          105.0f * P(-13, -2);
  p +=         6825.0f * P(-11, -2);
  p +=       143325.0f * P( -9, -2);
  p +=      1366365.0f * P( -7, -2);
  p +=      6831825.0f * P( -5, -2);
  p +=     19324305.0f * P( -3, -2);
  p +=     32207175.0f * P( -1, -2);
  p +=     32207175.0f * P(  1, -2);
  p +=     19324305.0f * P(  3, -2);
  p +=      6831825.0f * P(  5, -2);
  p +=      1366365.0f * P(  7, -2);
  p +=       143325.0f * P(  9, -2);
  p +=         6825.0f * P( 11, -2);
  p +=          105.0f * P( 13, -2);
  p +=           15.0f * P(-14, -1);
  p +=         1575.0f * P(-12, -1);
  p +=        47775.0f * P(-10, -1);
  p +=       621075.0f * P( -8, -1);
  p +=      4099095.0f * P( -6, -1);
  p +=     15030015.0f * P( -4, -1);
  p +=     32207175.0f * P( -2, -1);
  p +=     41409225.0f * P(  0, -1);
  p +=     32207175.0f * P(  2, -1);
  p +=     15030015.0f * P(  4, -1);
  p +=      4099095.0f * P(  6, -1);
  p +=       621075.0f * P(  8, -1);
  p +=        47775.0f * P( 10, -1);
  p +=         1575.0f * P( 12, -1);
  p +=           15.0f * P( 14, -1);
  p +=            1.0f * P(-15,  0);
  p +=          225.0f * P(-13,  0);
  p +=        11025.0f * P(-11,  0);
  p +=       207025.0f * P( -9,  0);
  p +=      1863225.0f * P( -7,  0);
  p +=      9018009.0f * P( -5,  0);
  p +=     25050025.0f * P( -3,  0);
  p +=     41409225.0f * P( -1,  0);
  p +=     41409225.0f * P(  1,  0);
  p +=     25050025.0f * P(  3,  0);
  p +=      9018009.0f * P(  5,  0);
  p +=      1863225.0f * P(  7,  0);
  p +=       207025.0f * P(  9,  0);
  p +=        11025.0f * P( 11,  0);
  p +=          225.0f * P( 13,  0);
  p +=            1.0f * P( 15,  0);
  p +=           15.0f * P(-14,  1);
  p +=         1575.0f * P(-12,  1);
  p +=        47775.0f * P(-10,  1);
  p +=       621075.0f * P( -8,  1);
  p +=      4099095.0f * P( -6,  1);
  p +=     15030015.0f * P( -4,  1);
  p +=     32207175.0f * P( -2,  1);
  p +=     41409225.0f * P(  0,  1);
  p +=     32207175.0f * P(  2,  1);
  p +=     15030015.0f * P(  4,  1);
  p +=      4099095.0f * P(  6,  1);
  p +=       621075.0f * P(  8,  1);
  p +=        47775.0f * P( 10,  1);
  p +=         1575.0f * P( 12,  1);
  p +=           15.0f * P( 14,  1);
  p +=          105.0f * P(-13,  2);
  p +=         6825.0f * P(-11,  2);
  p +=       143325.0f * P( -9,  2);
  p +=      1366365.0f * P( -7,  2);
  p +=      6831825.0f * P( -5,  2);
  p +=     19324305.0f * P( -3,  2);
  p +=     32207175.0f * P( -1,  2);
  p +=     32207175.0f * P(  1,  2);
  p +=     19324305.0f * P(  3,  2);
  p +=      6831825.0f * P(  5,  2);
  p +=      1366365.0f * P(  7,  2);
  p +=       143325.0f * P(  9,  2);
  p +=         6825.0f * P( 11,  2);
  p +=          105.0f * P( 13,  2);
  p +=          455.0f * P(-12,  3);
  p +=        20475.0f * P(-10,  3);
  p +=       315315.0f * P( -8,  3);
  p +=      2277275.0f * P( -6,  3);
  p +=      8783775.0f * P( -4,  3);
  p +=     19324305.0f * P( -2,  3);
  p +=     25050025.0f * P(  0,  3);
  p +=     19324305.0f * P(  2,  3);
  p +=      8783775.0f * P(  4,  3);
  p +=      2277275.0f * P(  6,  3);
  p +=       315315.0f * P(  8,  3);
  p +=        20475.0f * P( 10,  3);
  p +=          455.0f * P( 12,  3);
  p +=         1365.0f * P(-11,  4);
  p +=        45045.0f * P( -9,  4);
  p +=       525525.0f * P( -7,  4);
  p +=      2927925.0f * P( -5,  4);
  p +=      8783775.0f * P( -3,  4);
  p +=     15030015.0f * P( -1,  4);
  p +=     15030015.0f * P(  1,  4);
  p +=      8783775.0f * P(  3,  4);
  p +=      2927925.0f * P(  5,  4);
  p +=       525525.0f * P(  7,  4);
  p +=        45045.0f * P(  9,  4);
  p +=         1365.0f * P( 11,  4);
  p +=         3003.0f * P(-10,  5);
  p +=        75075.0f * P( -8,  5);
  p +=       675675.0f * P( -6,  5);
  p +=      2927925.0f * P( -4,  5);
  p +=      6831825.0f * P( -2,  5);
  p +=      9018009.0f * P(  0,  5);
  p +=      6831825.0f * P(  2,  5);
  p +=      2927925.0f * P(  4,  5);
  p +=       675675.0f * P(  6,  5);
  p +=        75075.0f * P(  8,  5);
  p +=         3003.0f * P( 10,  5);
  p +=         5005.0f * P( -9,  6);
  p +=        96525.0f * P( -7,  6);
  p +=       675675.0f * P( -5,  6);
  p +=      2277275.0f * P( -3,  6);
  p +=      4099095.0f * P( -1,  6);
  p +=      4099095.0f * P(  1,  6);
  p +=      2277275.0f * P(  3,  6);
  p +=       675675.0f * P(  5,  6);
  p +=        96525.0f * P(  7,  6);
  p +=         5005.0f * P(  9,  6);
  p +=         6435.0f * P( -8,  7);
  p +=        96525.0f * P( -6,  7);
  p +=       525525.0f * P( -4,  7);
  p +=      1366365.0f * P( -2,  7);
  p +=      1863225.0f * P(  0,  7);
  p +=      1366365.0f * P(  2,  7);
  p +=       525525.0f * P(  4,  7);
  p +=        96525.0f * P(  6,  7);
  p +=         6435.0f * P(  8,  7);
  p +=         6435.0f * P( -7,  8);
  p +=        75075.0f * P( -5,  8);
  p +=       315315.0f * P( -3,  8);
  p +=       621075.0f * P( -1,  8);
  p +=       621075.0f * P(  1,  8);
  p +=       315315.0f * P(  3,  8);
  p +=        75075.0f * P(  5,  8);
  p +=         6435.0f * P(  7,  8);
  p +=         5005.0f * P( -6,  9);
  p +=        45045.0f * P( -4,  9);
  p +=       143325.0f * P( -2,  9);
  p +=       207025.0f * P(  0,  9);
  p +=       143325.0f * P(  2,  9);
  p +=        45045.0f * P(  4,  9);
  p +=         5005.0f * P(  6,  9);
  p +=         3003.0f * P( -5, 10);
  p +=        20475.0f * P( -3, 10);
  p +=        47775.0f * P( -1, 10);
  p +=        47775.0f * P(  1, 10);
  p +=        20475.0f * P(  3, 10);
  p +=         3003.0f * P(  5, 10);
  p +=         1365.0f * P( -4, 11);
  p +=         6825.0f * P( -2, 11);
  p +=        11025.0f * P(  0, 11);
  p +=         6825.0f * P(  2, 11);
  p +=         1365.0f * P(  4, 11);
  p +=          455.0f * P( -3, 12);
  p +=         1575.0f * P( -1, 12);
  p +=         1575.0f * P(  1, 12);
  p +=          455.0f * P(  3, 12);
  p +=          105.0f * P( -2, 13);
  p +=          225.0f * P(  0, 13);
  p +=          105.0f * P(  2, 13);
  p +=           15.0f * P( -1, 14);
  p +=           15.0f * P(  1, 14);
  p +=            1.0f * P(  0, 15);
  p /= 1073741824.0f;
#elif (KERNEL == 11)
  // This versions uses 11x11 kernel only
  float div = 0.0f;
  div +=            1.0f * DIV(  0, -5);
  div +=            5.0f * DIV( -1, -4);
  div +=            4.0f * DIV(  0, -4);
  div +=            5.0f * DIV(  1, -4);
  div +=           10.0f * DIV( -2, -3);
  div +=           16.0f * DIV( -1, -3);
  div +=           41.0f * DIV(  0, -3);
  div +=           16.0f * DIV(  1, -3);
  div +=           10.0f * DIV(  2, -3);
  div +=           10.0f * DIV( -3, -2);
  div +=           24.0f * DIV( -2, -2);
  div +=           98.0f * DIV( -1, -2);
  div +=          128.0f * DIV(  0, -2);
  div +=           98.0f * DIV(  1, -2);
  div +=           24.0f * DIV(  2, -2);
  div +=           10.0f * DIV(  3, -2);
  div +=            5.0f * DIV( -4, -1);
  div +=           16.0f * DIV( -3, -1);
  div +=           98.0f * DIV( -2, -1);
  div +=          224.0f * DIV( -1, -1);
  div +=          500.0f * DIV(  0, -1);
  div +=          224.0f * DIV(  1, -1);
  div +=           98.0f * DIV(  2, -1);
  div +=           16.0f * DIV(  3, -1);
  div +=            5.0f * DIV(  4, -1);
  div +=            1.0f * DIV( -5,  0);
  div +=            4.0f * DIV( -4,  0);
  div +=           41.0f * DIV( -3,  0);
  div +=          128.0f * DIV( -2,  0);
  div +=          500.0f * DIV( -1,  0);
  div +=         1424.0f * DIV(  0,  0);
  div +=          500.0f * DIV(  1,  0);
  div +=          128.0f * DIV(  2,  0);
  div +=           41.0f * DIV(  3,  0);
  div +=            4.0f * DIV(  4,  0);
  div +=            1.0f * DIV(  5,  0);
  div +=            5.0f * DIV( -4,  1);
  div +=           16.0f * DIV( -3,  1);
  div +=           98.0f * DIV( -2,  1);
  div +=          224.0f * DIV( -1,  1);
  div +=          500.0f * DIV(  0,  1);
  div +=          224.0f * DIV(  1,  1);
  div +=           98.0f * DIV(  2,  1);
  div +=           16.0f * DIV(  3,  1);
  div +=            5.0f * DIV(  4,  1);
  div +=           10.0f * DIV( -3,  2);
  div +=           24.0f * DIV( -2,  2);
  div +=           98.0f * DIV( -1,  2);
  div +=          128.0f * DIV(  0,  2);
  div +=           98.0f * DIV(  1,  2);
  div +=           24.0f * DIV(  2,  2);
  div +=           10.0f * DIV(  3,  2);
  div +=           10.0f * DIV( -2,  3);
  div +=           16.0f * DIV( -1,  3);
  div +=           41.0f * DIV(  0,  3);
  div +=           16.0f * DIV(  1,  3);
  div +=           10.0f * DIV(  2,  3);
  div +=            5.0f * DIV( -1,  4);
  div +=            4.0f * DIV(  0,  4);
  div +=            5.0f * DIV(  1,  4);
  div +=            1.0f * DIV(  0,  5);
  div /= 4096.0f;
  float p = 0.0f;
  p +=            1.0f * P(  0, -6);
  p +=            6.0f * P( -1, -5);
  p +=            6.0f * P(  1, -5);
  p +=           15.0f * P( -2, -4);
  p +=           36.0f * P(  0, -4);
  p +=           15.0f * P(  2, -4);
  p +=           20.0f * P( -3, -3);
  p +=           90.0f * P( -1, -3);
  p +=           90.0f * P(  1, -3);
  p +=           20.0f * P(  3, -3);
  p +=           15.0f * P( -4, -2);
  p +=          120.0f * P( -2, -2);
  p +=          225.0f * P(  0, -2);
  p +=          120.0f * P(  2, -2);
  p +=           15.0f * P(  4, -2);
  p +=            6.0f * P( -5, -1);
  p +=           90.0f * P( -3, -1);
  p +=          300.0f * P( -1, -1);
  p +=          300.0f * P(  1, -1);
  p +=           90.0f * P(  3, -1);
  p +=            6.0f * P(  5, -1);
  p +=            1.0f * P( -6,  0);
  p +=           36.0f * P( -4,  0);
  p +=          225.0f * P( -2,  0);
  p +=          400.0f * P(  0,  0);
  p +=          225.0f * P(  2,  0);
  p +=           36.0f * P(  4,  0);
  p +=            1.0f * P(  6,  0);
  p +=            6.0f * P( -5,  1);
  p +=           90.0f * P( -3,  1);
  p +=          300.0f * P( -1,  1);
  p +=          300.0f * P(  1,  1);
  p +=           90.0f * P(  3,  1);
  p +=            6.0f * P(  5,  1);
  p +=           15.0f * P( -4,  2);
  p +=          120.0f * P( -2,  2);
  p +=          225.0f * P(  0,  2);
  p +=          120.0f * P(  2,  2);
  p +=           15.0f * P(  4,  2);
  p +=           20.0f * P( -3,  3);
  p +=           90.0f * P( -1,  3);
  p +=           90.0f * P(  1,  3);
  p +=           20.0f * P(  3,  3);
  p +=           15.0f * P( -2,  4);
  p +=           36.0f * P(  0,  4);
  p +=           15.0f * P(  2,  4);
  p +=            6.0f * P( -1,  5);
  p +=            6.0f * P(  1,  5);
  p +=            1.0f * P(  0,  6);
  p /= 4096.0f;
#elif (KERNEL == 7)  // eventually a 7x7 version
  float div = 0.0f;
  div +=            1.0f * DIV(  0, -3);
  div +=            3.0f * DIV( -1, -2);
  div +=            4.0f * DIV(  0, -2);
  div +=            3.0f * DIV(  1, -2);
  div +=            3.0f * DIV( -2, -1);
  div +=            8.0f * DIV( -1, -1);
  div +=           25.0f * DIV(  0, -1);
  div +=            8.0f * DIV(  1, -1);
  div +=            3.0f * DIV(  2, -1);
  div +=            1.0f * DIV( -3,  0);
  div +=            4.0f * DIV( -2,  0);
  div +=           25.0f * DIV( -1,  0);
  div +=           80.0f * DIV(  0,  0);
  div +=           25.0f * DIV(  1,  0);
  div +=            4.0f * DIV(  2,  0);
  div +=            1.0f * DIV(  3,  0);
  div +=            3.0f * DIV( -2,  1);
  div +=            8.0f * DIV( -1,  1);
  div +=           25.0f * DIV(  0,  1);
  div +=            8.0f * DIV(  1,  1);
  div +=            3.0f * DIV(  2,  1);
  div +=            3.0f * DIV( -1,  2);
  div +=            4.0f * DIV(  0,  2);
  div +=            3.0f * DIV(  1,  2);
  div +=            1.0f * DIV(  0,  3);
  div /= 256.0f;
  float p = 0.0f;
  p +=            1.0f * P(  0, -4);
  p +=            4.0f * P( -1, -3);
  p +=            4.0f * P(  1, -3);
  p +=            6.0f * P( -2, -2);
  p +=           16.0f * P(  0, -2);
  p +=            6.0f * P(  2, -2);
  p +=            4.0f * P( -3, -1);
  p +=           24.0f * P( -1, -1);
  p +=           24.0f * P(  1, -1);
  p +=            4.0f * P(  3, -1);
  p +=            1.0f * P( -4,  0);
  p +=           16.0f * P( -2,  0);
  p +=           36.0f * P(  0,  0);
  p +=           16.0f * P(  2,  0);
  p +=            1.0f * P(  4,  0);
  p +=            4.0f * P( -3,  1);
  p +=           24.0f * P( -1,  1);
  p +=           24.0f * P(  1,  1);
  p +=            4.0f * P(  3,  1);
  p +=            6.0f * P( -2,  2);
  p +=           16.0f * P(  0,  2);
  p +=            6.0f * P(  2,  2);
  p +=            4.0f * P( -1,  3);
  p +=            4.0f * P(  1,  3);
  p +=            1.0f * P(  0,  4);
  p /= 256.0f;
#elif (KERNEL == 3)
  float div = 0.0f;
  div +=            1.0f * DIV(  0, -1);
  div +=            1.0f * DIV( -1,  0);
  div +=            4.0f * DIV(  0,  0);
  div +=            1.0f * DIV(  1,  0);
  div +=            1.0f * DIV(  0,  1);
  div /= 16.0f;
  float p = 0.0f;
  p +=            1.0f * P(  0, -2);
  p +=            2.0f * P( -1, -1);
  p +=            2.0f * P(  1, -1);
  p +=            1.0f * P( -2,  0);
  p +=            4.0f * P(  0,  0);
  p +=            1.0f * P(  2,  0);
  p +=            2.0f * P( -1,  1);
  p +=            2.0f * P(  1,  1);
  p +=            1.0f * P(  0,  2);
  p /= 16.0f;
#endif
  fragColor = to_float4(0.0f, 0.0f, 0.0f, div + p);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// Compute pressure gradient and subtracts to velocity
// bufD = bufA - ∇(bufC) - nu.∇²(bufA) = new velocity
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

__KERNEL__ void FluidflowsimulationJipi164Fuse__Buffer_D(float4 fragColor, float2 p, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  p+=0.5f;

  const float nu = 0.0001f;  // viscosity

  float2 norm = 1.0f / iResolution;
  p *= norm;
  float2 dx = to_float2(norm.x,     0.0f);
  float2 dy = to_float2(    0.0f, norm.y);
  // compute grad(p) and subtract to velocity field
  float gx = texture(iChannel0, p + dx).w
           - texture(iChannel0, p - dx).w;
  float gy = texture(iChannel0, p + dy).w
           - texture(iChannel0, p - dy).w;
  float4 v = _tex2DVecN(iChannel1,p.x,p.y,15);
  //swi2(v,x,y) -= 0.5f * to_float2(gx, gy);
  v.x -= 0.5f * gx;
  v.y -= 0.5f * gy;
float DDDDDDDDDDDDDDDDDDDDD;  
  // diffusion
  float2 laplacian = 4.0f * swi2(v,x,y) 
                    - (swi2(texture(iChannel1, p + dx),x,y) + swi2(texture(iChannel1, p - dx),x,y),
                       swi2(texture(iChannel1, p - dy),x,y) + swi2(texture(iChannel1, p - dy),x,y));
  
  //swi2(v,x,y) += nu * laplacian;
  v.x += nu * laplacian.x;
  v.y += nu * laplacian.y;
  
  //swi2(v,z,w) = _tex2DVecN(iChannel1,p.x,p.y,15).zw;
  v.z = _tex2DVecN(iChannel1,p.x,p.y,15).z;
  v.w = _tex2DVecN(iChannel1,p.x,p.y,15).w;
  
  fragColor = v;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


//   Viscous incompressible fluid solver.
//
// Buffer A/D contains {vx, vy, color1, color2}
// Buffer A is the advected fields U*
// re-used Buffer C contains the last solved pressure
// Buffer B contains the divergence ∇.U*
//
// Press '1' to show velocity field, 
//       '2' for the pressure,
//       '3' for the divergence
// Otherwise the tracers are shown.
//
//  There's a 'use_airfoil' bool in common buffer to switch to
//  an airfoil profile.
//
// The solver in 'Buffer C' has 15 iterations of the Jacobi steps,
// There's a #define to switch to 5 steps only (or even less!).
// Works ~ok visually. But low number of steps is struggling to cancel the divergence.
//
// Heavily inspired by Robert's Schuetze (@trirop)'s shader:
//   https://www.shadertoy.com/view/MdSczK
// and follow-up by @ultraviolet:
//   https://www.shadertoy.com/view/4lScRG
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.


#define R iResolution

__KERNEL__ void FluidflowsimulationJipi164Fuse(float4 fragColor, float2 pos, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  //CONNECT_BUTTON0(Modus,TYP,BTN1,BTN2,BTN3,BTN4,BTN5);
  CONNECT_INTSLIDER0(show_what, 0, 3, 0);
  CONNECT_CHECKBOX1(use_airfoil,1);

  CONNECT_SLIDER0(kRadius, 0.0f, 30.0f, 12.0f);
  CONNECT_SLIDER1(FoilCamber, -10.0f, 1.0f, 0.3f);
  CONNECT_SLIDER2(FoilSize, 0.0f, 300.0f, 120.0f);
  CONNECT_SLIDER3(FoilAlpha, -10.0f, 50.0f, 25.0f);

  pos+=0.5f;
  
  // 0 = tracers, 1 = velocity, 2 = pressure, 3 = divergence
  //int show_what = 0;
  
  //const bool use_airfoil = true;    // balls or airfoil
  
  // For velocity field display:
  const float kCell = 40.0f;
  const float kVelocityUnit = 20.0f;

  
  //if (texelFetch(iChannel3, to_int2(49, 0),0 ).x > 0.5f) show_what = 1;
  //if (texelFetch(iChannel3, to_int2(50, 0),0 ).x > 0.5f) show_what = 2;
  //if (texelFetch(iChannel3, to_int2(51, 0),0 ).x > 0.5f) show_what = 3;
  //if (texture(iChannel3, (make_float2(to_int2(49, 0))+0.5f)/R ).x > 0.5f) show_what = 1;
  //if (texture(iChannel3, (make_float2(to_int2(50, 0))+0.5f)/R ).x > 0.5f) show_what = 2;
  //if (texture(iChannel3, (make_float2(to_int2(51, 0))+0.5f)/R ).x > 0.5f) show_what = 3;

#ifdef Org
  if (IsObstacle(swi2(iMouse,x,y), pos, use_airfoil, kRadius, FoilCamber, FoilSize, FoilAlpha)) {      // draw the obstacle
    fragColor = to_float4(0.3f, 0.7f, 0.3f, 1.0f);
    //SetFragmentShaderComputedColor(fragColor);
    //return;
  }
#endif  
  

  if (show_what == 2) {  // pressure
    float4 t = texture(iChannel1, pos / iResolution);
    fragColor = to_float4_aw(fract_f3(swi3(t,w,w,w) * 0.5f), 1.0f);
  } else if (show_what == 1) {  // visualize velocity field
    float2 p0 = _floor(pos / kCell) * kCell;
    float2 p1 = p0 + to_float2_s(0.5f * kCell);
    float2 p2 = p1 + kVelocityUnit * swi2(texture(iChannel0, p1 / iResolution),x,y);
    float d = 1.0f - smoothstep(0.0f, 2.0f, SegmentDistance(pos, p1, p2));
    fragColor = to_float4(d, d, d, 1.0f);
  } else if (show_what == 3) {  // show divergence
    float4 t = texture(iChannel2, pos / iResolution);
    fragColor = to_float4_aw(0.5f - 2.3f * swi3(t,w,w,w), 1.0f);
  } else if (show_what == 0) {  // show tracers
    float4 t = texture(iChannel0, pos / iResolution);
    fragColor = to_float4_aw(_mix(to_float3(0.7f, 0.8f, 0.9f) * t.z, to_float3(1.0f, 1.0f, 0.0f), t.w), 1.0f);
  }


  float4 tex = texture( iChannel3, pos/iResolution); 

  if ( !use_airfoil )
    {
       FoilCamber = tex.w;
    }

  if (IsObstacle(swi2(iMouse,x,y), pos, use_airfoil, kRadius, FoilCamber, FoilSize, FoilAlpha)) {      // draw the obstacle
    fragColor = to_float4(0.3f, 0.7f, 0.3f, 1.0f);
    
    if ( !use_airfoil )
    {
      fragColor = tex; 
    }
    
    //SetFragmentShaderComputedColor(fragColor);
    //return;
  }


  SetFragmentShaderComputedColor(fragColor);
}