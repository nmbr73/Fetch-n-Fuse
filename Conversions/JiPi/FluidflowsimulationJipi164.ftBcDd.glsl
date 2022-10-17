

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.


#define R iResolution.xy

void mainImage(out vec4 fragColor, in vec2 pos) {
  //if (texelFetch(iChannel3, ivec2(49, 0),0 ).x > 0.5) show_what = 1;
  //if (texelFetch(iChannel3, ivec2(50, 0),0 ).x > 0.5) show_what = 2;
  //if (texelFetch(iChannel3, ivec2(51, 0),0 ).x > 0.5) show_what = 3;
  if (texture(iChannel3, (vec2(ivec2(49, 0))+0.5)/R ).x > 0.5) show_what = 1;
  if (texture(iChannel3, (vec2(ivec2(50, 0))+0.5)/R ).x > 0.5) show_what = 2;
  if (texture(iChannel3, (vec2(ivec2(51, 0))+0.5)/R ).x > 0.5) show_what = 3;

  if (IsObstacle(iMouse.xy, pos)) {      // draw the obstacle
    fragColor = vec4(0.3, 0.7, 0.3, 1.0);
    return;
  }

  if (show_what == 2) {  // pressure
    vec4 t = texture(iChannel1, pos / iResolution.xy);
    fragColor = vec4(fract(t.www * 0.5), 1.0);
  } else if (show_what == 1) {  // visualize velocity field
    vec2 p0 = floor(pos / kCell) * kCell;
    vec2 p1 = p0 + vec2(0.5 * kCell);
    vec2 p2 = p1 + kVelocityUnit * texture(iChannel0, p1 / iResolution.xy).xy;
    float d = 1. - smoothstep(0., 2.0, SegmentDistance(pos, p1, p2));
    fragColor = vec4(d, d, d, 1.0);
  } else if (show_what == 3) {  // show divergence
    vec4 t = texture(iChannel2, pos / iResolution.xy);
    fragColor = vec4(0.5 - 2.3 * t.www, 1.0);
  } else if (show_what == 0) {  // show tracers
    vec4 t = texture(iChannel0, pos / iResolution.xy);
    fragColor = vec4(mix(vec3(0.7, 0.8, 0.9) * t.z, vec3(1., 1., 0.), t.w), 1.0);
  }
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Advects U->U* using the previous velocity field in BufD.
// bufA will contains advected {vx, vy, color1/2}
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

vec4 Advect(vec2 p) { // using 4-order Runge-Kutta
  vec2 norm = 1. / iResolution.xy;
  p *= norm;
  vec2 H = dt * norm;
  vec2 k1 = H * texture(iChannel0, p           ).xy;
#if (RUNGE_KUTTA == 4)
  vec2 k2 = H * texture(iChannel0, p - 0.5 * k1).xy;
  vec2 k3 = H * texture(iChannel0, p - 0.5 * k2).xy;
  vec2 k4 = H * texture(iChannel0, p -       k3).xy;
  vec2 dp = (0.5 * (k1 + k4) + k2 + k3) / 3.0;
#elif (RUNGE_KUTTA == 2)
  vec2 k2 = H * texture(iChannel0, p - 0.5 * k1).xy;
  vec2 dp = k2;
#else  // RUNGE_KUTTA == 1
  vec2 dp = k1;
#endif
return texture(iChannel0, p - dp);
}

void mainImage(out vec4 fragColor, in vec2 pos) {
  vec4 src = Advect(pos);  // advect backward
  vec2 v = src.xy;   // velocity
  vec2 c = src.zw;   // color
  // force some boundary conditions

// initial field
  if (iFrame <= 1){
    v = vec2(Vo, 0.0);
    c.x = step(sin(242.223 * sin(pos.x * 320.231 + pos.y * 13.92)), -0.4);
    c.y = 0.;
  }
  // in/out flow from left to right
  if (pos.x < 2. || pos.x >= iResolution.x - 2.) {
    v = vec2(Vo, 0.0);
  }
  // rough canal
  if (pos.y < 2. || pos.y >= iResolution.y - 2.) {
    v = vec2(0.0, 0.0);
  }
  if (pos.x < 5.) {  // some tracer injection
    c.x = 1. - step(cos(pos.y * 0.3), .2);
  }
  if (IsObstacle(iMouse.xy, pos.xy)) {
    v = vec2(0., .0);
    c.x = 0.;
    c.y = step(sin(pos.y / 2.), .9);  // color tracers from obstacle
  }
  fragColor = vec4(v, c);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Computes D = K.∇.U*   [with K=-dt/(2.rho.dx)]
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float Divergence(vec2 p) {
  vec2 norm = 1. / iResolution.xy;
  vec2 P = p * norm;
  vec2 dx = vec2(norm.x,     0.);
  vec2 dy = vec2(    0., norm.y);  
  float dv_dx = texture(iChannel0, P + dx).x
              - texture(iChannel0, P - dx).x;
  float dv_dy = texture(iChannel0, P + dy).y
              - texture(iChannel0, P - dy).y;
  return -0.5 * (dv_dx + dv_dy);
}

void mainImage(out vec4 fragColor, in vec2 pos) {
  fragColor = vec4(0., 0., 0., Divergence(pos));
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Compute pressure for Laplacian equation ∇²P = ∇.U*
//
// The Jacobi method would iterate on the array
//   p_i,j = 1/4 * (div_i,j + p_i+2,j + p_i-2,j + p_i,j+2 + p_i,j-2)
// But since we work in-place in BufferC, this is more a Gauss-Seidel method!
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define DIV(X, Y) texture(iChannel0, (pos + vec2(X, Y)) * norm).w
#define P(X, Y)   texture(iChannel1, (pos + vec2(X, Y)) * norm).w  // re-use buf C

void mainImage(out vec4 fragColor, in vec2 pos) {
  vec2 norm = 1. / iResolution.xy;
#if (KERNEL == 29)
  float div = 0.;
  div +=            1. * DIV(  0,-14);
  div +=           14. * DIV( -1,-13);
  div +=            4. * DIV(  0,-13);
  div +=           14. * DIV(  1,-13);
  div +=           91. * DIV( -2,-12);
  div +=           52. * DIV( -1,-12);
  div +=          212. * DIV(  0,-12);
  div +=           52. * DIV(  1,-12);
  div +=           91. * DIV(  2,-12);
  div +=          364. * DIV( -3,-11);
  div +=          312. * DIV( -2,-11);
  div +=         1466. * DIV( -1,-11);
  div +=          740. * DIV(  0,-11);
  div +=         1466. * DIV(  1,-11);
  div +=          312. * DIV(  2,-11);
  div +=          364. * DIV(  3,-11);
  div +=         1001. * DIV( -4,-10);
  div +=         1144. * DIV( -3,-10);
  div +=         6152. * DIV( -2,-10);
  div +=         4760. * DIV( -1,-10);
  div +=        10841. * DIV(  0,-10);
  div +=         4760. * DIV(  1,-10);
  div +=         6152. * DIV(  2,-10);
  div +=         1144. * DIV(  3,-10);
  div +=         1001. * DIV(  4,-10);
  div +=         2002. * DIV( -5, -9);
  div +=         2860. * DIV( -4, -9);
  div +=        17534. * DIV( -3, -9);
  div +=        18392. * DIV( -2, -9);
  div +=        48356. * DIV( -1, -9);
  div +=        33104. * DIV(  0, -9);
  div +=        48356. * DIV(  1, -9);
  div +=        18392. * DIV(  2, -9);
  div +=        17534. * DIV(  3, -9);
  div +=         2860. * DIV(  4, -9);
  div +=         2002. * DIV(  5, -9);
  div +=         3003. * DIV( -6, -8);
  div +=         5148. * DIV( -5, -8);
  div +=        35948. * DIV( -4, -8);
  div +=        47740. * DIV( -3, -8);
  div +=       144851. * DIV( -2, -8);
  div +=       137168. * DIV( -1, -8);
  div +=       231888. * DIV(  0, -8);
  div +=       137168. * DIV(  1, -8);
  div +=       144851. * DIV(  2, -8);
  div +=        47740. * DIV(  3, -8);
  div +=        35948. * DIV(  4, -8);
  div +=         5148. * DIV(  5, -8);
  div +=         3003. * DIV(  6, -8);
  div +=         3432. * DIV( -7, -7);
  div +=         6864. * DIV( -6, -7);
  div +=        54714. * DIV( -5, -7);
  div +=        88044. * DIV( -4, -7);
  div +=       307942. * DIV( -3, -7);
  div +=       376104. * DIV( -2, -7);
  div +=       744652. * DIV( -1, -7);
  div +=       620112. * DIV(  0, -7);
  div +=       744652. * DIV(  1, -7);
  div +=       376104. * DIV(  2, -7);
  div +=       307942. * DIV(  3, -7);
  div +=        88044. * DIV(  4, -7);
  div +=        54714. * DIV(  5, -7);
  div +=         6864. * DIV(  6, -7);
  div +=         3432. * DIV(  7, -7);
  div +=         3003. * DIV( -8, -6);
  div +=         6864. * DIV( -7, -6);
  div +=        62832. * DIV( -6, -6);
  div +=       118800. * DIV( -5, -6);
  div +=       479097. * DIV( -4, -6);
  div +=       719880. * DIV( -3, -6);
  div +=      1673336. * DIV( -2, -6);
  div +=      1845224. * DIV( -1, -6);
  div +=      2622481. * DIV(  0, -6);
  div +=      1845224. * DIV(  1, -6);
  div +=      1673336. * DIV(  2, -6);
  div +=       719880. * DIV(  3, -6);
  div +=       479097. * DIV(  4, -6);
  div +=       118800. * DIV(  5, -6);
  div +=        62832. * DIV(  6, -6);
  div +=         6864. * DIV(  7, -6);
  div +=         3003. * DIV(  8, -6);
  div +=         2002. * DIV( -9, -5);
  div +=         5148. * DIV( -8, -5);
  div +=        54714. * DIV( -7, -5);
  div +=       118800. * DIV( -6, -5);
  div +=       554232. * DIV( -5, -5);
  div +=       989664. * DIV( -4, -5);
  div +=      2696420. * DIV( -3, -5);
  div +=      3752136. * DIV( -2, -5);
  div +=      6439522. * DIV( -1, -5);
  div +=      6179364. * DIV(  0, -5);
  div +=      6439522. * DIV(  1, -5);
  div +=      3752136. * DIV(  2, -5);
  div +=      2696420. * DIV(  3, -5);
  div +=       989664. * DIV(  4, -5);
  div +=       554232. * DIV(  5, -5);
  div +=       118800. * DIV(  6, -5);
  div +=        54714. * DIV(  7, -5);
  div +=         5148. * DIV(  8, -5);
  div +=         2002. * DIV(  9, -5);
  div +=         1001. * DIV(-10, -4);
  div +=         2860. * DIV( -9, -4);
  div +=        35948. * DIV( -8, -4);
  div +=        88044. * DIV( -7, -4);
  div +=       479097. * DIV( -6, -4);
  div +=       989664. * DIV( -5, -4);
  div +=      3156832. * DIV( -4, -4);
  div +=      5324000. * DIV( -3, -4);
  div +=     11031091. * DIV( -2, -4);
  div +=     13981364. * DIV( -1, -4);
  div +=     18233940. * DIV(  0, -4);
  div +=     13981364. * DIV(  1, -4);
  div +=     11031091. * DIV(  2, -4);
  div +=      5324000. * DIV(  3, -4);
  div +=      3156832. * DIV(  4, -4);
  div +=       989664. * DIV(  5, -4);
  div +=       479097. * DIV(  6, -4);
  div +=        88044. * DIV(  7, -4);
  div +=        35948. * DIV(  8, -4);
  div +=         2860. * DIV(  9, -4);
  div +=         1001. * DIV( 10, -4);
  div +=          364. * DIV(-11, -3);
  div +=         1144. * DIV(-10, -3);
  div +=        17534. * DIV( -9, -3);
  div +=        47740. * DIV( -8, -3);
  div +=       307942. * DIV( -7, -3);
  div +=       719880. * DIV( -6, -3);
  div +=      2696420. * DIV( -5, -3);
  div +=      5324000. * DIV( -4, -3);
  div +=     13195432. * DIV( -3, -3);
  div +=     21066864. * DIV( -2, -3);
  div +=     35250918. * DIV( -1, -3);
  div +=     38793668. * DIV(  0, -3);
  div +=     35250918. * DIV(  1, -3);
  div +=     21066864. * DIV(  2, -3);
  div +=     13195432. * DIV(  3, -3);
  div +=      5324000. * DIV(  4, -3);
  div +=      2696420. * DIV(  5, -3);
  div +=       719880. * DIV(  6, -3);
  div +=       307942. * DIV(  7, -3);
  div +=        47740. * DIV(  8, -3);
  div +=        17534. * DIV(  9, -3);
  div +=         1144. * DIV( 10, -3);
  div +=          364. * DIV( 11, -3);
  div +=           91. * DIV(-12, -2);
  div +=          312. * DIV(-11, -2);
  div +=         6152. * DIV(-10, -2);
  div +=        18392. * DIV( -9, -2);
  div +=       144851. * DIV( -8, -2);
  div +=       376104. * DIV( -7, -2);
  div +=      1673336. * DIV( -6, -2);
  div +=      3752136. * DIV( -5, -2);
  div +=     11031091. * DIV( -4, -2);
  div +=     21066864. * DIV( -3, -2);
  div +=     44114320. * DIV( -2, -2);
  div +=     67161776. * DIV( -1, -2);
  div +=     91488921. * DIV(  0, -2);
  div +=     67161776. * DIV(  1, -2);
  div +=     44114320. * DIV(  2, -2);
  div +=     21066864. * DIV(  3, -2);
  div +=     11031091. * DIV(  4, -2);
  div +=      3752136. * DIV(  5, -2);
  div +=      1673336. * DIV(  6, -2);
  div +=       376104. * DIV(  7, -2);
  div +=       144851. * DIV(  8, -2);
  div +=        18392. * DIV(  9, -2);
  div +=         6152. * DIV( 10, -2);
  div +=          312. * DIV( 11, -2);
  div +=           91. * DIV( 12, -2);
  div +=           14. * DIV(-13, -1);
  div +=           52. * DIV(-12, -1);
  div +=         1466. * DIV(-11, -1);
  div +=         4760. * DIV(-10, -1);
  div +=        48356. * DIV( -9, -1);
  div +=       137168. * DIV( -8, -1);
  div +=       744652. * DIV( -7, -1);
  div +=      1845224. * DIV( -6, -1);
  div +=      6439522. * DIV( -5, -1);
  div +=     13981364. * DIV( -4, -1);
  div +=     35250918. * DIV( -3, -1);
  div +=     67161776. * DIV( -2, -1);
  div +=    130000120. * DIV( -1, -1);
  div +=    192838464. * DIV(  0, -1);
  div +=    130000120. * DIV(  1, -1);
  div +=     67161776. * DIV(  2, -1);
  div +=     35250918. * DIV(  3, -1);
  div +=     13981364. * DIV(  4, -1);
  div +=      6439522. * DIV(  5, -1);
  div +=      1845224. * DIV(  6, -1);
  div +=       744652. * DIV(  7, -1);
  div +=       137168. * DIV(  8, -1);
  div +=        48356. * DIV(  9, -1);
  div +=         4760. * DIV( 10, -1);
  div +=         1466. * DIV( 11, -1);
  div +=           52. * DIV( 12, -1);
  div +=           14. * DIV( 13, -1);
  div +=            1. * DIV(-14,  0);
  div +=            4. * DIV(-13,  0);
  div +=          212. * DIV(-12,  0);
  div +=          740. * DIV(-11,  0);
  div +=        10841. * DIV(-10,  0);
  div +=        33104. * DIV( -9,  0);
  div +=       231888. * DIV( -8,  0);
  div +=       620112. * DIV( -7,  0);
  div +=      2622481. * DIV( -6,  0);
  div +=      6179364. * DIV( -5,  0);
  div +=     18233940. * DIV( -4,  0);
  div +=     38793668. * DIV( -3,  0);
  div +=     91488921. * DIV( -2,  0);
  div +=    192838464. * DIV( -1,  0);
  div +=    461273920. * DIV(  0,  0);
  div +=    192838464. * DIV(  1,  0);
  div +=     91488921. * DIV(  2,  0);
  div +=     38793668. * DIV(  3,  0);
  div +=     18233940. * DIV(  4,  0);
  div +=      6179364. * DIV(  5,  0);
  div +=      2622481. * DIV(  6,  0);
  div +=       620112. * DIV(  7,  0);
  div +=       231888. * DIV(  8,  0);
  div +=        33104. * DIV(  9,  0);
  div +=        10841. * DIV( 10,  0);
  div +=          740. * DIV( 11,  0);
  div +=          212. * DIV( 12,  0);
  div +=            4. * DIV( 13,  0);
  div +=            1. * DIV( 14,  0);
  div +=           14. * DIV(-13,  1);
  div +=           52. * DIV(-12,  1);
  div +=         1466. * DIV(-11,  1);
  div +=         4760. * DIV(-10,  1);
  div +=        48356. * DIV( -9,  1);
  div +=       137168. * DIV( -8,  1);
  div +=       744652. * DIV( -7,  1);
  div +=      1845224. * DIV( -6,  1);
  div +=      6439522. * DIV( -5,  1);
  div +=     13981364. * DIV( -4,  1);
  div +=     35250918. * DIV( -3,  1);
  div +=     67161776. * DIV( -2,  1);
  div +=    130000120. * DIV( -1,  1);
  div +=    192838464. * DIV(  0,  1);
  div +=    130000120. * DIV(  1,  1);
  div +=     67161776. * DIV(  2,  1);
  div +=     35250918. * DIV(  3,  1);
  div +=     13981364. * DIV(  4,  1);
  div +=      6439522. * DIV(  5,  1);
  div +=      1845224. * DIV(  6,  1);
  div +=       744652. * DIV(  7,  1);
  div +=       137168. * DIV(  8,  1);
  div +=        48356. * DIV(  9,  1);
  div +=         4760. * DIV( 10,  1);
  div +=         1466. * DIV( 11,  1);
  div +=           52. * DIV( 12,  1);
  div +=           14. * DIV( 13,  1);
  div +=           91. * DIV(-12,  2);
  div +=          312. * DIV(-11,  2);
  div +=         6152. * DIV(-10,  2);
  div +=        18392. * DIV( -9,  2);
  div +=       144851. * DIV( -8,  2);
  div +=       376104. * DIV( -7,  2);
  div +=      1673336. * DIV( -6,  2);
  div +=      3752136. * DIV( -5,  2);
  div +=     11031091. * DIV( -4,  2);
  div +=     21066864. * DIV( -3,  2);
  div +=     44114320. * DIV( -2,  2);
  div +=     67161776. * DIV( -1,  2);
  div +=     91488921. * DIV(  0,  2);
  div +=     67161776. * DIV(  1,  2);
  div +=     44114320. * DIV(  2,  2);
  div +=     21066864. * DIV(  3,  2);
  div +=     11031091. * DIV(  4,  2);
  div +=      3752136. * DIV(  5,  2);
  div +=      1673336. * DIV(  6,  2);
  div +=       376104. * DIV(  7,  2);
  div +=       144851. * DIV(  8,  2);
  div +=        18392. * DIV(  9,  2);
  div +=         6152. * DIV( 10,  2);
  div +=          312. * DIV( 11,  2);
  div +=           91. * DIV( 12,  2);
  div +=          364. * DIV(-11,  3);
  div +=         1144. * DIV(-10,  3);
  div +=        17534. * DIV( -9,  3);
  div +=        47740. * DIV( -8,  3);
  div +=       307942. * DIV( -7,  3);
  div +=       719880. * DIV( -6,  3);
  div +=      2696420. * DIV( -5,  3);
  div +=      5324000. * DIV( -4,  3);
  div +=     13195432. * DIV( -3,  3);
  div +=     21066864. * DIV( -2,  3);
  div +=     35250918. * DIV( -1,  3);
  div +=     38793668. * DIV(  0,  3);
  div +=     35250918. * DIV(  1,  3);
  div +=     21066864. * DIV(  2,  3);
  div +=     13195432. * DIV(  3,  3);
  div +=      5324000. * DIV(  4,  3);
  div +=      2696420. * DIV(  5,  3);
  div +=       719880. * DIV(  6,  3);
  div +=       307942. * DIV(  7,  3);
  div +=        47740. * DIV(  8,  3);
  div +=        17534. * DIV(  9,  3);
  div +=         1144. * DIV( 10,  3);
  div +=          364. * DIV( 11,  3);
  div +=         1001. * DIV(-10,  4);
  div +=         2860. * DIV( -9,  4);
  div +=        35948. * DIV( -8,  4);
  div +=        88044. * DIV( -7,  4);
  div +=       479097. * DIV( -6,  4);
  div +=       989664. * DIV( -5,  4);
  div +=      3156832. * DIV( -4,  4);
  div +=      5324000. * DIV( -3,  4);
  div +=     11031091. * DIV( -2,  4);
  div +=     13981364. * DIV( -1,  4);
  div +=     18233940. * DIV(  0,  4);
  div +=     13981364. * DIV(  1,  4);
  div +=     11031091. * DIV(  2,  4);
  div +=      5324000. * DIV(  3,  4);
  div +=      3156832. * DIV(  4,  4);
  div +=       989664. * DIV(  5,  4);
  div +=       479097. * DIV(  6,  4);
  div +=        88044. * DIV(  7,  4);
  div +=        35948. * DIV(  8,  4);
  div +=         2860. * DIV(  9,  4);
  div +=         1001. * DIV( 10,  4);
  div +=         2002. * DIV( -9,  5);
  div +=         5148. * DIV( -8,  5);
  div +=        54714. * DIV( -7,  5);
  div +=       118800. * DIV( -6,  5);
  div +=       554232. * DIV( -5,  5);
  div +=       989664. * DIV( -4,  5);
  div +=      2696420. * DIV( -3,  5);
  div +=      3752136. * DIV( -2,  5);
  div +=      6439522. * DIV( -1,  5);
  div +=      6179364. * DIV(  0,  5);
  div +=      6439522. * DIV(  1,  5);
  div +=      3752136. * DIV(  2,  5);
  div +=      2696420. * DIV(  3,  5);
  div +=       989664. * DIV(  4,  5);
  div +=       554232. * DIV(  5,  5);
  div +=       118800. * DIV(  6,  5);
  div +=        54714. * DIV(  7,  5);
  div +=         5148. * DIV(  8,  5);
  div +=         2002. * DIV(  9,  5);
  div +=         3003. * DIV( -8,  6);
  div +=         6864. * DIV( -7,  6);
  div +=        62832. * DIV( -6,  6);
  div +=       118800. * DIV( -5,  6);
  div +=       479097. * DIV( -4,  6);
  div +=       719880. * DIV( -3,  6);
  div +=      1673336. * DIV( -2,  6);
  div +=      1845224. * DIV( -1,  6);
  div +=      2622481. * DIV(  0,  6);
  div +=      1845224. * DIV(  1,  6);
  div +=      1673336. * DIV(  2,  6);
  div +=       719880. * DIV(  3,  6);
  div +=       479097. * DIV(  4,  6);
  div +=       118800. * DIV(  5,  6);
  div +=        62832. * DIV(  6,  6);
  div +=         6864. * DIV(  7,  6);
  div +=         3003. * DIV(  8,  6);
  div +=         3432. * DIV( -7,  7);
  div +=         6864. * DIV( -6,  7);
  div +=        54714. * DIV( -5,  7);
  div +=        88044. * DIV( -4,  7);
  div +=       307942. * DIV( -3,  7);
  div +=       376104. * DIV( -2,  7);
  div +=       744652. * DIV( -1,  7);
  div +=       620112. * DIV(  0,  7);
  div +=       744652. * DIV(  1,  7);
  div +=       376104. * DIV(  2,  7);
  div +=       307942. * DIV(  3,  7);
  div +=        88044. * DIV(  4,  7);
  div +=        54714. * DIV(  5,  7);
  div +=         6864. * DIV(  6,  7);
  div +=         3432. * DIV(  7,  7);
  div +=         3003. * DIV( -6,  8);
  div +=         5148. * DIV( -5,  8);
  div +=        35948. * DIV( -4,  8);
  div +=        47740. * DIV( -3,  8);
  div +=       144851. * DIV( -2,  8);
  div +=       137168. * DIV( -1,  8);
  div +=       231888. * DIV(  0,  8);
  div +=       137168. * DIV(  1,  8);
  div +=       144851. * DIV(  2,  8);
  div +=        47740. * DIV(  3,  8);
  div +=        35948. * DIV(  4,  8);
  div +=         5148. * DIV(  5,  8);
  div +=         3003. * DIV(  6,  8);
  div +=         2002. * DIV( -5,  9);
  div +=         2860. * DIV( -4,  9);
  div +=        17534. * DIV( -3,  9);
  div +=        18392. * DIV( -2,  9);
  div +=        48356. * DIV( -1,  9);
  div +=        33104. * DIV(  0,  9);
  div +=        48356. * DIV(  1,  9);
  div +=        18392. * DIV(  2,  9);
  div +=        17534. * DIV(  3,  9);
  div +=         2860. * DIV(  4,  9);
  div +=         2002. * DIV(  5,  9);
  div +=         1001. * DIV( -4, 10);
  div +=         1144. * DIV( -3, 10);
  div +=         6152. * DIV( -2, 10);
  div +=         4760. * DIV( -1, 10);
  div +=        10841. * DIV(  0, 10);
  div +=         4760. * DIV(  1, 10);
  div +=         6152. * DIV(  2, 10);
  div +=         1144. * DIV(  3, 10);
  div +=         1001. * DIV(  4, 10);
  div +=          364. * DIV( -3, 11);
  div +=          312. * DIV( -2, 11);
  div +=         1466. * DIV( -1, 11);
  div +=          740. * DIV(  0, 11);
  div +=         1466. * DIV(  1, 11);
  div +=          312. * DIV(  2, 11);
  div +=          364. * DIV(  3, 11);
  div +=           91. * DIV( -2, 12);
  div +=           52. * DIV( -1, 12);
  div +=          212. * DIV(  0, 12);
  div +=           52. * DIV(  1, 12);
  div +=           91. * DIV(  2, 12);
  div +=           14. * DIV( -1, 13);
  div +=            4. * DIV(  0, 13);
  div +=           14. * DIV(  1, 13);
  div +=            1. * DIV(  0, 14);
  div /= 1073741824.;
  float p = 0.;
  p +=            1. * P(  0,-15);
  p +=           15. * P( -1,-14);
  p +=           15. * P(  1,-14);
  p +=          105. * P( -2,-13);
  p +=          225. * P(  0,-13);
  p +=          105. * P(  2,-13);
  p +=          455. * P( -3,-12);
  p +=         1575. * P( -1,-12);
  p +=         1575. * P(  1,-12);
  p +=          455. * P(  3,-12);
  p +=         1365. * P( -4,-11);
  p +=         6825. * P( -2,-11);
  p +=        11025. * P(  0,-11);
  p +=         6825. * P(  2,-11);
  p +=         1365. * P(  4,-11);
  p +=         3003. * P( -5,-10);
  p +=        20475. * P( -3,-10);
  p +=        47775. * P( -1,-10);
  p +=        47775. * P(  1,-10);
  p +=        20475. * P(  3,-10);
  p +=         3003. * P(  5,-10);
  p +=         5005. * P( -6, -9);
  p +=        45045. * P( -4, -9);
  p +=       143325. * P( -2, -9);
  p +=       207025. * P(  0, -9);
  p +=       143325. * P(  2, -9);
  p +=        45045. * P(  4, -9);
  p +=         5005. * P(  6, -9);
  p +=         6435. * P( -7, -8);
  p +=        75075. * P( -5, -8);
  p +=       315315. * P( -3, -8);
  p +=       621075. * P( -1, -8);
  p +=       621075. * P(  1, -8);
  p +=       315315. * P(  3, -8);
  p +=        75075. * P(  5, -8);
  p +=         6435. * P(  7, -8);
  p +=         6435. * P( -8, -7);
  p +=        96525. * P( -6, -7);
  p +=       525525. * P( -4, -7);
  p +=      1366365. * P( -2, -7);
  p +=      1863225. * P(  0, -7);
  p +=      1366365. * P(  2, -7);
  p +=       525525. * P(  4, -7);
  p +=        96525. * P(  6, -7);
  p +=         6435. * P(  8, -7);
  p +=         5005. * P( -9, -6);
  p +=        96525. * P( -7, -6);
  p +=       675675. * P( -5, -6);
  p +=      2277275. * P( -3, -6);
  p +=      4099095. * P( -1, -6);
  p +=      4099095. * P(  1, -6);
  p +=      2277275. * P(  3, -6);
  p +=       675675. * P(  5, -6);
  p +=        96525. * P(  7, -6);
  p +=         5005. * P(  9, -6);
  p +=         3003. * P(-10, -5);
  p +=        75075. * P( -8, -5);
  p +=       675675. * P( -6, -5);
  p +=      2927925. * P( -4, -5);
  p +=      6831825. * P( -2, -5);
  p +=      9018009. * P(  0, -5);
  p +=      6831825. * P(  2, -5);
  p +=      2927925. * P(  4, -5);
  p +=       675675. * P(  6, -5);
  p +=        75075. * P(  8, -5);
  p +=         3003. * P( 10, -5);
  p +=         1365. * P(-11, -4);
  p +=        45045. * P( -9, -4);
  p +=       525525. * P( -7, -4);
  p +=      2927925. * P( -5, -4);
  p +=      8783775. * P( -3, -4);
  p +=     15030015. * P( -1, -4);
  p +=     15030015. * P(  1, -4);
  p +=      8783775. * P(  3, -4);
  p +=      2927925. * P(  5, -4);
  p +=       525525. * P(  7, -4);
  p +=        45045. * P(  9, -4);
  p +=         1365. * P( 11, -4);
  p +=          455. * P(-12, -3);
  p +=        20475. * P(-10, -3);
  p +=       315315. * P( -8, -3);
  p +=      2277275. * P( -6, -3);
  p +=      8783775. * P( -4, -3);
  p +=     19324305. * P( -2, -3);
  p +=     25050025. * P(  0, -3);
  p +=     19324305. * P(  2, -3);
  p +=      8783775. * P(  4, -3);
  p +=      2277275. * P(  6, -3);
  p +=       315315. * P(  8, -3);
  p +=        20475. * P( 10, -3);
  p +=          455. * P( 12, -3);
  p +=          105. * P(-13, -2);
  p +=         6825. * P(-11, -2);
  p +=       143325. * P( -9, -2);
  p +=      1366365. * P( -7, -2);
  p +=      6831825. * P( -5, -2);
  p +=     19324305. * P( -3, -2);
  p +=     32207175. * P( -1, -2);
  p +=     32207175. * P(  1, -2);
  p +=     19324305. * P(  3, -2);
  p +=      6831825. * P(  5, -2);
  p +=      1366365. * P(  7, -2);
  p +=       143325. * P(  9, -2);
  p +=         6825. * P( 11, -2);
  p +=          105. * P( 13, -2);
  p +=           15. * P(-14, -1);
  p +=         1575. * P(-12, -1);
  p +=        47775. * P(-10, -1);
  p +=       621075. * P( -8, -1);
  p +=      4099095. * P( -6, -1);
  p +=     15030015. * P( -4, -1);
  p +=     32207175. * P( -2, -1);
  p +=     41409225. * P(  0, -1);
  p +=     32207175. * P(  2, -1);
  p +=     15030015. * P(  4, -1);
  p +=      4099095. * P(  6, -1);
  p +=       621075. * P(  8, -1);
  p +=        47775. * P( 10, -1);
  p +=         1575. * P( 12, -1);
  p +=           15. * P( 14, -1);
  p +=            1. * P(-15,  0);
  p +=          225. * P(-13,  0);
  p +=        11025. * P(-11,  0);
  p +=       207025. * P( -9,  0);
  p +=      1863225. * P( -7,  0);
  p +=      9018009. * P( -5,  0);
  p +=     25050025. * P( -3,  0);
  p +=     41409225. * P( -1,  0);
  p +=     41409225. * P(  1,  0);
  p +=     25050025. * P(  3,  0);
  p +=      9018009. * P(  5,  0);
  p +=      1863225. * P(  7,  0);
  p +=       207025. * P(  9,  0);
  p +=        11025. * P( 11,  0);
  p +=          225. * P( 13,  0);
  p +=            1. * P( 15,  0);
  p +=           15. * P(-14,  1);
  p +=         1575. * P(-12,  1);
  p +=        47775. * P(-10,  1);
  p +=       621075. * P( -8,  1);
  p +=      4099095. * P( -6,  1);
  p +=     15030015. * P( -4,  1);
  p +=     32207175. * P( -2,  1);
  p +=     41409225. * P(  0,  1);
  p +=     32207175. * P(  2,  1);
  p +=     15030015. * P(  4,  1);
  p +=      4099095. * P(  6,  1);
  p +=       621075. * P(  8,  1);
  p +=        47775. * P( 10,  1);
  p +=         1575. * P( 12,  1);
  p +=           15. * P( 14,  1);
  p +=          105. * P(-13,  2);
  p +=         6825. * P(-11,  2);
  p +=       143325. * P( -9,  2);
  p +=      1366365. * P( -7,  2);
  p +=      6831825. * P( -5,  2);
  p +=     19324305. * P( -3,  2);
  p +=     32207175. * P( -1,  2);
  p +=     32207175. * P(  1,  2);
  p +=     19324305. * P(  3,  2);
  p +=      6831825. * P(  5,  2);
  p +=      1366365. * P(  7,  2);
  p +=       143325. * P(  9,  2);
  p +=         6825. * P( 11,  2);
  p +=          105. * P( 13,  2);
  p +=          455. * P(-12,  3);
  p +=        20475. * P(-10,  3);
  p +=       315315. * P( -8,  3);
  p +=      2277275. * P( -6,  3);
  p +=      8783775. * P( -4,  3);
  p +=     19324305. * P( -2,  3);
  p +=     25050025. * P(  0,  3);
  p +=     19324305. * P(  2,  3);
  p +=      8783775. * P(  4,  3);
  p +=      2277275. * P(  6,  3);
  p +=       315315. * P(  8,  3);
  p +=        20475. * P( 10,  3);
  p +=          455. * P( 12,  3);
  p +=         1365. * P(-11,  4);
  p +=        45045. * P( -9,  4);
  p +=       525525. * P( -7,  4);
  p +=      2927925. * P( -5,  4);
  p +=      8783775. * P( -3,  4);
  p +=     15030015. * P( -1,  4);
  p +=     15030015. * P(  1,  4);
  p +=      8783775. * P(  3,  4);
  p +=      2927925. * P(  5,  4);
  p +=       525525. * P(  7,  4);
  p +=        45045. * P(  9,  4);
  p +=         1365. * P( 11,  4);
  p +=         3003. * P(-10,  5);
  p +=        75075. * P( -8,  5);
  p +=       675675. * P( -6,  5);
  p +=      2927925. * P( -4,  5);
  p +=      6831825. * P( -2,  5);
  p +=      9018009. * P(  0,  5);
  p +=      6831825. * P(  2,  5);
  p +=      2927925. * P(  4,  5);
  p +=       675675. * P(  6,  5);
  p +=        75075. * P(  8,  5);
  p +=         3003. * P( 10,  5);
  p +=         5005. * P( -9,  6);
  p +=        96525. * P( -7,  6);
  p +=       675675. * P( -5,  6);
  p +=      2277275. * P( -3,  6);
  p +=      4099095. * P( -1,  6);
  p +=      4099095. * P(  1,  6);
  p +=      2277275. * P(  3,  6);
  p +=       675675. * P(  5,  6);
  p +=        96525. * P(  7,  6);
  p +=         5005. * P(  9,  6);
  p +=         6435. * P( -8,  7);
  p +=        96525. * P( -6,  7);
  p +=       525525. * P( -4,  7);
  p +=      1366365. * P( -2,  7);
  p +=      1863225. * P(  0,  7);
  p +=      1366365. * P(  2,  7);
  p +=       525525. * P(  4,  7);
  p +=        96525. * P(  6,  7);
  p +=         6435. * P(  8,  7);
  p +=         6435. * P( -7,  8);
  p +=        75075. * P( -5,  8);
  p +=       315315. * P( -3,  8);
  p +=       621075. * P( -1,  8);
  p +=       621075. * P(  1,  8);
  p +=       315315. * P(  3,  8);
  p +=        75075. * P(  5,  8);
  p +=         6435. * P(  7,  8);
  p +=         5005. * P( -6,  9);
  p +=        45045. * P( -4,  9);
  p +=       143325. * P( -2,  9);
  p +=       207025. * P(  0,  9);
  p +=       143325. * P(  2,  9);
  p +=        45045. * P(  4,  9);
  p +=         5005. * P(  6,  9);
  p +=         3003. * P( -5, 10);
  p +=        20475. * P( -3, 10);
  p +=        47775. * P( -1, 10);
  p +=        47775. * P(  1, 10);
  p +=        20475. * P(  3, 10);
  p +=         3003. * P(  5, 10);
  p +=         1365. * P( -4, 11);
  p +=         6825. * P( -2, 11);
  p +=        11025. * P(  0, 11);
  p +=         6825. * P(  2, 11);
  p +=         1365. * P(  4, 11);
  p +=          455. * P( -3, 12);
  p +=         1575. * P( -1, 12);
  p +=         1575. * P(  1, 12);
  p +=          455. * P(  3, 12);
  p +=          105. * P( -2, 13);
  p +=          225. * P(  0, 13);
  p +=          105. * P(  2, 13);
  p +=           15. * P( -1, 14);
  p +=           15. * P(  1, 14);
  p +=            1. * P(  0, 15);
  p /= 1073741824.;
#elif (KERNEL == 11)
  // This versions uses 11x11 kernel only
  float div = 0.;
  div +=            1. * DIV(  0, -5);
  div +=            5. * DIV( -1, -4);
  div +=            4. * DIV(  0, -4);
  div +=            5. * DIV(  1, -4);
  div +=           10. * DIV( -2, -3);
  div +=           16. * DIV( -1, -3);
  div +=           41. * DIV(  0, -3);
  div +=           16. * DIV(  1, -3);
  div +=           10. * DIV(  2, -3);
  div +=           10. * DIV( -3, -2);
  div +=           24. * DIV( -2, -2);
  div +=           98. * DIV( -1, -2);
  div +=          128. * DIV(  0, -2);
  div +=           98. * DIV(  1, -2);
  div +=           24. * DIV(  2, -2);
  div +=           10. * DIV(  3, -2);
  div +=            5. * DIV( -4, -1);
  div +=           16. * DIV( -3, -1);
  div +=           98. * DIV( -2, -1);
  div +=          224. * DIV( -1, -1);
  div +=          500. * DIV(  0, -1);
  div +=          224. * DIV(  1, -1);
  div +=           98. * DIV(  2, -1);
  div +=           16. * DIV(  3, -1);
  div +=            5. * DIV(  4, -1);
  div +=            1. * DIV( -5,  0);
  div +=            4. * DIV( -4,  0);
  div +=           41. * DIV( -3,  0);
  div +=          128. * DIV( -2,  0);
  div +=          500. * DIV( -1,  0);
  div +=         1424. * DIV(  0,  0);
  div +=          500. * DIV(  1,  0);
  div +=          128. * DIV(  2,  0);
  div +=           41. * DIV(  3,  0);
  div +=            4. * DIV(  4,  0);
  div +=            1. * DIV(  5,  0);
  div +=            5. * DIV( -4,  1);
  div +=           16. * DIV( -3,  1);
  div +=           98. * DIV( -2,  1);
  div +=          224. * DIV( -1,  1);
  div +=          500. * DIV(  0,  1);
  div +=          224. * DIV(  1,  1);
  div +=           98. * DIV(  2,  1);
  div +=           16. * DIV(  3,  1);
  div +=            5. * DIV(  4,  1);
  div +=           10. * DIV( -3,  2);
  div +=           24. * DIV( -2,  2);
  div +=           98. * DIV( -1,  2);
  div +=          128. * DIV(  0,  2);
  div +=           98. * DIV(  1,  2);
  div +=           24. * DIV(  2,  2);
  div +=           10. * DIV(  3,  2);
  div +=           10. * DIV( -2,  3);
  div +=           16. * DIV( -1,  3);
  div +=           41. * DIV(  0,  3);
  div +=           16. * DIV(  1,  3);
  div +=           10. * DIV(  2,  3);
  div +=            5. * DIV( -1,  4);
  div +=            4. * DIV(  0,  4);
  div +=            5. * DIV(  1,  4);
  div +=            1. * DIV(  0,  5);
  div /= 4096.;
  float p = 0.;
  p +=            1. * P(  0, -6);
  p +=            6. * P( -1, -5);
  p +=            6. * P(  1, -5);
  p +=           15. * P( -2, -4);
  p +=           36. * P(  0, -4);
  p +=           15. * P(  2, -4);
  p +=           20. * P( -3, -3);
  p +=           90. * P( -1, -3);
  p +=           90. * P(  1, -3);
  p +=           20. * P(  3, -3);
  p +=           15. * P( -4, -2);
  p +=          120. * P( -2, -2);
  p +=          225. * P(  0, -2);
  p +=          120. * P(  2, -2);
  p +=           15. * P(  4, -2);
  p +=            6. * P( -5, -1);
  p +=           90. * P( -3, -1);
  p +=          300. * P( -1, -1);
  p +=          300. * P(  1, -1);
  p +=           90. * P(  3, -1);
  p +=            6. * P(  5, -1);
  p +=            1. * P( -6,  0);
  p +=           36. * P( -4,  0);
  p +=          225. * P( -2,  0);
  p +=          400. * P(  0,  0);
  p +=          225. * P(  2,  0);
  p +=           36. * P(  4,  0);
  p +=            1. * P(  6,  0);
  p +=            6. * P( -5,  1);
  p +=           90. * P( -3,  1);
  p +=          300. * P( -1,  1);
  p +=          300. * P(  1,  1);
  p +=           90. * P(  3,  1);
  p +=            6. * P(  5,  1);
  p +=           15. * P( -4,  2);
  p +=          120. * P( -2,  2);
  p +=          225. * P(  0,  2);
  p +=          120. * P(  2,  2);
  p +=           15. * P(  4,  2);
  p +=           20. * P( -3,  3);
  p +=           90. * P( -1,  3);
  p +=           90. * P(  1,  3);
  p +=           20. * P(  3,  3);
  p +=           15. * P( -2,  4);
  p +=           36. * P(  0,  4);
  p +=           15. * P(  2,  4);
  p +=            6. * P( -1,  5);
  p +=            6. * P(  1,  5);
  p +=            1. * P(  0,  6);
  p /= 4096.;
#elif (KERNEL == 7)  // eventually a 7x7 version
  float div = 0.;
  div +=            1. * DIV(  0, -3);
  div +=            3. * DIV( -1, -2);
  div +=            4. * DIV(  0, -2);
  div +=            3. * DIV(  1, -2);
  div +=            3. * DIV( -2, -1);
  div +=            8. * DIV( -1, -1);
  div +=           25. * DIV(  0, -1);
  div +=            8. * DIV(  1, -1);
  div +=            3. * DIV(  2, -1);
  div +=            1. * DIV( -3,  0);
  div +=            4. * DIV( -2,  0);
  div +=           25. * DIV( -1,  0);
  div +=           80. * DIV(  0,  0);
  div +=           25. * DIV(  1,  0);
  div +=            4. * DIV(  2,  0);
  div +=            1. * DIV(  3,  0);
  div +=            3. * DIV( -2,  1);
  div +=            8. * DIV( -1,  1);
  div +=           25. * DIV(  0,  1);
  div +=            8. * DIV(  1,  1);
  div +=            3. * DIV(  2,  1);
  div +=            3. * DIV( -1,  2);
  div +=            4. * DIV(  0,  2);
  div +=            3. * DIV(  1,  2);
  div +=            1. * DIV(  0,  3);
  div /= 256.;
  float p = 0.;
  p +=            1. * P(  0, -4);
  p +=            4. * P( -1, -3);
  p +=            4. * P(  1, -3);
  p +=            6. * P( -2, -2);
  p +=           16. * P(  0, -2);
  p +=            6. * P(  2, -2);
  p +=            4. * P( -3, -1);
  p +=           24. * P( -1, -1);
  p +=           24. * P(  1, -1);
  p +=            4. * P(  3, -1);
  p +=            1. * P( -4,  0);
  p +=           16. * P( -2,  0);
  p +=           36. * P(  0,  0);
  p +=           16. * P(  2,  0);
  p +=            1. * P(  4,  0);
  p +=            4. * P( -3,  1);
  p +=           24. * P( -1,  1);
  p +=           24. * P(  1,  1);
  p +=            4. * P(  3,  1);
  p +=            6. * P( -2,  2);
  p +=           16. * P(  0,  2);
  p +=            6. * P(  2,  2);
  p +=            4. * P( -1,  3);
  p +=            4. * P(  1,  3);
  p +=            1. * P(  0,  4);
  p /= 256.;
#elif (KERNEL == 3)
  float div = 0.;
  div +=            1. * DIV(  0, -1);
  div +=            1. * DIV( -1,  0);
  div +=            4. * DIV(  0,  0);
  div +=            1. * DIV(  1,  0);
  div +=            1. * DIV(  0,  1);
  div /= 16.;
  float p = 0.;
  p +=            1. * P(  0, -2);
  p +=            2. * P( -1, -1);
  p +=            2. * P(  1, -1);
  p +=            1. * P( -2,  0);
  p +=            4. * P(  0,  0);
  p +=            1. * P(  2,  0);
  p +=            2. * P( -1,  1);
  p +=            2. * P(  1,  1);
  p +=            1. * P(  0,  2);
  p /= 16.;
#endif
  fragColor = vec4(0., 0., 0., div + p);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Compute pressure gradient and subtracts to velocity
// bufD = bufA - ∇(bufC) - nu.∇²(bufA) = new velocity
//
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

void mainImage(out vec4 fragColor, in vec2 p) {
  vec2 norm = 1. / iResolution.xy;
  p *= norm;
  vec2 dx = vec2(norm.x,     0.);
  vec2 dy = vec2(    0., norm.y);
  // compute grad(p) and subtract to velocity field
  float gx = texture(iChannel0, p + dx).w
           - texture(iChannel0, p - dx).w;
  float gy = texture(iChannel0, p + dy).w
           - texture(iChannel0, p - dy).w;
  vec4 v = texture(iChannel1, p);
  v.xy -= 0.5 * vec2(gx, gy);
  // diffusion
  vec2 laplacian = 4. * v.xy - (
     texture(iChannel1, p + dx).xy + texture(iChannel1, p - dx).xy,
     texture(iChannel1, p - dy).xy + texture(iChannel1, p - dy).xy);
  v.xy += nu * laplacian;
  v.zw = texture(iChannel1, p).zw;
  fragColor = v;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// some constants
// Created by Pascal Massimino [skal] (2022)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

const float dt = 1.0;     // step
const float Vo = 1.5;     // initial / typical velocity
const float nu = 0.0001;  // viscosity

const bool use_airfoil = true;    // balls or airfoil

#define KERNEL 29 // kernel for pressure solver: 29, 11, 7 or 3!
#define RUNGE_KUTTA 4 // order. 4, 2 (midpoint) or 1 (=Euler)

// 0 = tracers, 1 = velocity, 2 = pressure, 3 = divergence
int show_what = 0;

// airfoil characteristics
const float FoilCamber = 0.3;   // higher = more bent
const float FoilSize = 120.;
const float FoilAlpha = 25.;   // angle of attack in degrees

const float kRadius = 12.;  // ball radius

bool IsObstacle(vec2 mouse, vec2 pos) {
  if (length(mouse) < 0.01) mouse = vec2(100., 180.);
  pos = mouse - pos;

  if (!use_airfoil) {    // array of balls:
    return (length(vec2(pos.x, fract(abs(pos.y) / 50.) * 50.)) < kRadius);
  }
  // Airfoil profile following more or less NACA formulae
  // see: https://en.wikipedia.org/wiki/NACA_airfoil#Equation_for_a_symmetrical_4-digit_NACA_airfoil
  float alpha = 3.1415 * (180. + FoilAlpha) / 180.;  // angle of attack
  mat2 M = mat2(cos(alpha), sin(alpha), -sin(alpha), cos(alpha)) / FoilSize;
  pos = M * pos;
  float x = pos.x, x2 = x * x;
  if (x < 0. || x > 1.) return false;
  // mor or le
  // mean camber line
  float xm = x * (1. - x) * FoilCamber;
  // thickness
  float th = 0.2969 * sqrt(x) - 0.1260 * x - 0.3516 * x2 + 0.2843 * x * x2 - 0.1036 * x2 * x2;
  float y = pos.y - xm;
  return abs(y) < th;
}

// For velocity field display:
const float kCell = 40.;
const float kVelocityUnit = 20.;

float SegmentDistance(vec2 p, vec2 p1, vec2 p2) {
  vec2 dir = p2 - p1;
  float d2 = dot(dir, dir);
  float d3 = dot(dir, p - p1) / d2;
  float frac = clamp(d3, 0.0, 1.0);
  return length(p - p1 - frac * dir);
}