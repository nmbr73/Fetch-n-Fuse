
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Texture: Wood' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//==============================================================================================================================================================
//  Created by Alexander Foksha
//
//  Do with this or without this code whatever you wish at your own risk.
//  You have been warned !!
//==============================================================================================================================================================

#if 0

/* =========================== Binary icosahedral group =========================== */

const float4 BI[120] = vec4[]
{
    /* 4-vector with components {x, y, z, w} represents quaternion w + xi + yj + zk */

    /* 8 basic unit quaternions :: ±1, ±i, ±j, ±k */
    /* these eight elements form a subgroup S */
    /*               g0 = */ to_float4( 0.0f,  0.0f,  0.0f,  1.0f),      /* +1, order :: 1  */
    /*               g1 = */ to_float4( 0.0f,  0.0f,  0.0f, -1.0f),      /* -1, order :: 2  */
    /*               g2 = */ to_float4( 1.0f,  0.0f,  0.0f,  0.0f),      /* +i, order :: 4  */
    /*               g3 = */ to_float4(-1.0f,  0.0f,  0.0f,  0.0f),      /* -i, order :: 4  */
    /*               g4 = */ to_float4( 0.0f,  1.0f,  0.0f,  0.0f),      /* +j, order :: 4  */
    /*               g5 = */ to_float4( 0.0f, -1.0f,  0.0f,  0.0f),      /* -j, order :: 4  */
    /*               g6 = */ to_float4( 0.0f,  0.0f,  1.0f,  0.0f),      /* +k, order :: 4  */
    /*               g7 = */ to_float4( 0.0f,  0.0f, -1.0f,  0.0f),      /* -k, order :: 4  */

    /* 16 unit Hurwitz quaternions */
    /* these 16 elements together with first 8 form a subgroup H (Hurwitz subgroup) */
    /* as a union of right conjugacy classes H = { S * g0, S * g8, S * g16 } */
    /* as a point set they form vertices of the octaplex, self-dual 24-cell in spherical space */

    /*   g8 =   1 *  g8 = */ to_float4( 0.5f,  0.5f,  0.5f,  0.5f),      /* order :: 6  */
    /*   g9 =  -1 *  g8 = */ to_float4(-0.5f, -0.5f, -0.5f, -0.5f),      /* order :: 3  */
    /*  g10 =   i *  g8 = */ to_float4( 0.5f,  0.5f, -0.5f, -0.5f),      /* order :: 3  */
    /*  g11 =  -i *  g8 = */ to_float4(-0.5f, -0.5f,  0.5f,  0.5f),      /* order :: 6  */
    /*  g12 =   j *  g8 = */ to_float4(-0.5f,  0.5f,  0.5f, -0.5f),      /* order :: 3  */
    /*  g13 =  -j *  g8 = */ to_float4( 0.5f, -0.5f, -0.5f,  0.5f),      /* order :: 6  */
    /*  g14 =   k *  g8 = */ to_float4( 0.5f, -0.5f,  0.5f, -0.5f),      /* order :: 3  */
    /*  g15 =  -k *  g8 = */ to_float4(-0.5f,  0.5f, -0.5f,  0.5f),      /* order :: 6  */

    /*  g16 =   1 * g16 = */ to_float4( 0.5f,  0.5f,  0.5f, -0.5f),      /* order :: 3  */
    /*  g17 =  -1 * g16 = */ to_float4(-0.5f, -0.5f, -0.5f,  0.5f),      /* order :: 6  */
    /*  g18 =   i * g16 = */ to_float4(-0.5f,  0.5f, -0.5f, -0.5f),      /* order :: 3  */
    /*  g19 =  -i * g16 = */ to_float4( 0.5f, -0.5f,  0.5f,  0.5f),      /* order :: 6  */
    /*  g20 =   j * g16 = */ to_float4(-0.5f, -0.5f,  0.5f, -0.5f),      /* order :: 3  */
    /*  g21 =  -j * g16 = */ to_float4( 0.5f,  0.5f, -0.5f,  0.5f),      /* order :: 6  */
    /*  g22 =   k * g16 = */ to_float4( 0.5f, -0.5f, -0.5f, -0.5f),      /* order :: 3  */
    /*  g23 =  -k * g16 = */ to_float4(-0.5f,  0.5f,  0.5f,  0.5f),      /* order :: 6  */

    /* the whole group B is a union of 5 conjugacy classes w.r.t subgroup H
       B = { H * g0, H * g24, H * g48, H * g72, H * g96 } */

    /*  g24 =  g0 * g24 = */ to_float4( psi,  0.0f,  0.5f,  phi),      /* order :: 10 */
    /*  g25 =  g1 * g24 = */ to_float4(-psi,  0.0f, -0.5f, -phi),      /* order :: 5  */
    /*  g26 =  g2 * g24 = */ to_float4( phi, -0.5f,  0.0f, -psi),      /* order :: 5  */
    /*  g27 =  g3 * g24 = */ to_float4(-phi,  0.5f,  0.0f,  psi),      /* order :: 10 */
    /*  g28 =  g4 * g24 = */ to_float4( 0.5f,  phi, -psi,  0.0f),      /* order :: 4  */
    /*  g29 =  g5 * g24 = */ to_float4(-0.5f, -phi,  psi,  0.0f),      /* order :: 4  */
    /*  g30 =  g6 * g24 = */ to_float4( 0.0f,  psi,  phi, -0.5f),      /* order :: 3  */
    /*  g31 =  g7 * g24 = */ to_float4( 0.0f, -psi, -phi,  0.5f),      /* order :: 6  */
    /*  g32 =  g8 * g24 = */ to_float4( phi,  psi,  0.5f,  0.0f),      /* order :: 4  */
    /*  g33 =  g9 * g24 = */ to_float4(-phi, -psi, -0.5f,  0.0f),      /* order :: 4  */
    /*  g34 = g10 * g24 = */ to_float4( phi,  0.0f, -psi,  0.5f),      /* order :: 6  */
    /*  g35 = g11 * g24 = */ to_float4(-phi,  0.0f,  psi, -0.5f),      /* order :: 3  */
    /*  g36 = g12 * g24 = */ to_float4( psi, -0.5f,  phi,  0.0f),      /* order :: 4  */
    /*  g37 = g13 * g24 = */ to_float4(-psi,  0.5f, -phi,  0.0f),      /* order :: 4  */
    /*  g38 = g14 * g24 = */ to_float4( psi, -phi,  0.0f,  0.5f),      /* order :: 6  */
    /*  g39 = g15 * g24 = */ to_float4(-psi,  phi,  0.0f, -0.5f),      /* order :: 3  */
    /*  g40 = g16 * g24 = */ to_float4( 0.0f,  phi,  0.5f,  psi),      /* order :: 10 */
    /*  g41 = g17 * g24 = */ to_float4( 0.0f, -phi, -0.5f, -psi),      /* order :: 5  */
    /*  g42 = g18 * g24 = */ to_float4( 0.0f,  0.5f, -psi,  phi),      /* order :: 10 */
    /*  g43 = g19 * g24 = */ to_float4( 0.0f, -0.5f,  psi, -phi),      /* order :: 5  */
    /*  g44 = g20 * g24 = */ to_float4(-0.5f,  0.0f,  phi,  psi),      /* order :: 10 */
    /*  g45 = g21 * g24 = */ to_float4( 0.5f,  0.0f, -phi, -psi),      /* order :: 5  */
    /*  g46 = g22 * g24 = */ to_float4(-0.5f, -psi,  0.0f,  phi),      /* order :: 10 */
    /*  g47 = g23 * g24 = */ to_float4( 0.5f,  psi,  0.0f, -phi),      /* order :: 5  */

    /*  g48 =  g0 * g48 = */ to_float4(-psi,  0.0f,  0.5f,  phi),      /* order :: 10 */
    /*  g49 =  g1 * g48 = */ to_float4( psi,  0.0f, -0.5f, -phi),      /* order :: 5  */
    /*  g50 =  g2 * g48 = */ to_float4( phi, -0.5f,  0.0f,  psi),      /* order :: 10 */
    /*  g51 =  g3 * g48 = */ to_float4(-phi,  0.5f,  0.0f, -psi),      /* order :: 5  */
    /*  g52 =  g4 * g48 = */ to_float4( 0.5f,  phi,  psi,  0.0f),      /* order :: 4  */
    /*  g53 =  g5 * g48 = */ to_float4(-0.5f, -phi, -psi,  0.0f),      /* order :: 4  */
    /*  g54 =  g6 * g48 = */ to_float4( 0.0f, -psi,  phi, -0.5f),      /* order :: 3  */
    /*  g55 =  g7 * g48 = */ to_float4( 0.0f,  psi, -phi,  0.5f),      /* order :: 6  */
    /*  g56 =  g8 * g48 = */ to_float4( 0.5f,  0.0f,  phi,  psi),      /* order :: 10 */
    /*  g57 =  g9 * g48 = */ to_float4(-0.5f,  0.0f, -phi, -psi),      /* order :: 5  */
    /*  g58 = g10 * g48 = */ to_float4( 0.5f,  psi,  0.0f,  phi),      /* order :: 10 */
    /*  g59 = g11 * g48 = */ to_float4(-0.5f, -psi,  0.0f, -phi),      /* order :: 5  */
    /*  g60 = g12 * g48 = */ to_float4( 0.0f, -phi,  0.5f,  psi),      /* order :: 10 */
    /*  g61 = g13 * g48 = */ to_float4( 0.0f,  phi, -0.5f, -psi),      /* order :: 5  */
    /*  g62 = g14 * g48 = */ to_float4( 0.0f, -0.5f, -psi,  phi),      /* order :: 10 */
    /*  g63 = g15 * g48 = */ to_float4( 0.0f,  0.5f,  psi, -phi),      /* order :: 5  */
    /*  g64 = g16 * g48 = */ to_float4(-psi,  0.5f,  phi,  0.0f),      /* order :: 4  */
    /*  g65 = g17 * g48 = */ to_float4( psi, -0.5f, -phi,  0.0f),      /* order :: 4  */
    /*  g66 = g18 * g48 = */ to_float4(-psi,  phi,  0.0f,  0.5f),      /* order :: 6  */
    /*  g67 = g19 * g48 = */ to_float4( psi, -phi,  0.0f, -0.5f),      /* order :: 3  */
    /*  g68 = g20 * g48 = */ to_float4(-phi, -psi,  0.5f,  0.0f),      /* order :: 4  */
    /*  g69 = g21 * g48 = */ to_float4( phi,  psi, -0.5f,  0.0f),      /* order :: 4  */
    /*  g70 = g22 * g48 = */ to_float4(-phi,  0.0f, -psi,  0.5f),      /* order :: 6  */
    /*  g71 = g23 * g48 = */ to_float4( phi,  0.0f,  psi, -0.5f),      /* order :: 3  */

    /*  g72 =  g0 * g72 = */ to_float4( 0.0f,  0.5f,  psi,  phi),      /* order :: 10 */
    /*  g73 =  g1 * g72 = */ to_float4( 0.0f, -0.5f, -psi, -phi),      /* order :: 5  */
    /*  g74 =  g2 * g72 = */ to_float4( phi, -psi,  0.5f,  0.0f),      /* order :: 4  */
    /*  g75 =  g3 * g72 = */ to_float4(-phi,  psi, -0.5f,  0.0f),      /* order :: 4  */
    /*  g76 =  g4 * g72 = */ to_float4( psi,  phi,  0.0f, -0.5f),      /* order :: 3  */
    /*  g77 =  g5 * g72 = */ to_float4(-psi, -phi,  0.0f,  0.5f),      /* order :: 6  */
    /*  g78 =  g6 * g72 = */ to_float4(-0.5f,  0.0f,  phi, -psi),      /* order :: 5  */
    /*  g79 =  g7 * g72 = */ to_float4( 0.5f,  0.0f, -phi,  psi),      /* order :: 10 */
    /*  g80 =  g8 * g72 = */ to_float4( psi,  0.5f,  phi,  0.0f),      /* order :: 4  */
    /*  g81 =  g9 * g72 = */ to_float4(-psi, -0.5f, -phi,  0.0f),      /* order :: 4  */
    /*  g82 = g10 * g72 = */ to_float4( phi,  0.5f,  0.0f,  psi),      /* order :: 10 */
    /*  g83 = g11 * g72 = */ to_float4(-phi, -0.5f,  0.0f, -psi),      /* order :: 5  */
    /*  g84 = g12 * g72 = */ to_float4( 0.0f, -psi,  phi,  0.5f),      /* order :: 6  */
    /*  g85 = g13 * g72 = */ to_float4( 0.0f,  psi, -phi, -0.5f),      /* order :: 3  */
    /*  g86 = g14 * g72 = */ to_float4( 0.5f, -psi,  0.0f,  phi),      /* order :: 10 */
    /*  g87 = g15 * g72 = */ to_float4(-0.5f,  psi,  0.0f, -phi),      /* order :: 5  */
    /*  g88 = g16 * g72 = */ to_float4(-0.5f,  phi,  psi,  0.0f),      /* order :: 4  */
    /*  g89 = g17 * g72 = */ to_float4( 0.5f, -phi, -psi,  0.0f),      /* order :: 4  */
    /*  g90 = g18 * g72 = */ to_float4( 0.0f,  phi, -0.5f,  psi),      /* order :: 10 */
    /*  g91 = g19 * g72 = */ to_float4( 0.0f, -phi,  0.5f, -psi),      /* order :: 5  */
    /*  g92 = g20 * g72 = */ to_float4(-phi,  0.0f,  psi,  0.5f),      /* order :: 6  */
    /*  g93 = g21 * g72 = */ to_float4( phi,  0.0f, -psi, -0.5f),      /* order :: 3  */
    /*  g94 = g22 * g72 = */ to_float4(-psi,  0.0f, -0.5f,  phi),      /* order :: 10 */
    /*  g95 = g23 * g72 = */ to_float4( psi,  0.0f,  0.5f, -phi),      /* order :: 5  */

    /*  g96 =  g0 * g96 = */ to_float4( 0.0f, -0.5f,  psi,  phi),      /* order :: 10 */
    /*  g97 =  g1 * g96 = */ to_float4( 0.0f,  0.5f, -psi, -phi),      /* order :: 5  */
    /*  g98 =  g2 * g96 = */ to_float4( phi, -psi, -0.5f,  0.0f),      /* order :: 4  */
    /*  g99 =  g3 * g96 = */ to_float4(-phi,  psi,  0.5f,  0.0f),      /* order :: 4  */
    /* g100 =  g4 * g96 = */ to_float4( psi,  phi,  0.0f,  0.5f),      /* order :: 6  */
    /* g101 =  g5 * g96 = */ to_float4(-psi, -phi,  0.0f, -0.5f),      /* order :: 3  */
    /* g102 =  g6 * g96 = */ to_float4( 0.5f,  0.0f,  phi, -psi),      /* order :: 5  */
    /* g103 =  g7 * g96 = */ to_float4(-0.5f,  0.0f, -phi,  psi),      /* order :: 10 */
    /* g104 =  g8 * g96 = */ to_float4( phi,  0.0f,  psi,  0.5f),      /* order :: 6  */
    /* g105 =  g9 * g96 = */ to_float4(-phi,  0.0f, -psi, -0.5f),      /* order :: 3  */
    /* g106 = g10 * g96 = */ to_float4( psi,  0.0f, -0.5f,  phi),      /* order :: 10 */
    /* g107 = g11 * g96 = */ to_float4(-psi,  0.0f,  0.5f, -phi),      /* order :: 5  */
    /* g108 = g12 * g96 = */ to_float4( 0.5f, -phi,  psi,  0.0f),      /* order :: 4  */
    /* g109 = g13 * g96 = */ to_float4(-0.5f,  phi, -psi,  0.0f),      /* order :: 4  */
    /* g110 = g14 * g96 = */ to_float4( 0.0f, -phi, -0.5f,  psi),      /* order :: 10 */
    /* g111 = g15 * g96 = */ to_float4( 0.0f,  phi,  0.5f, -psi),      /* order :: 5  */
    /* g112 = g16 * g96 = */ to_float4( 0.0f,  psi,  phi,  0.5f),      /* order :: 6  */
    /* g113 = g17 * g96 = */ to_float4( 0.0f, -psi, -phi, -0.5f),      /* order :: 3  */
    /* g114 = g18 * g96 = */ to_float4(-0.5f,  psi,  0.0f,  phi),      /* order :: 10 */
    /* g115 = g19 * g96 = */ to_float4( 0.5f, -psi,  0.0f, -phi),      /* order :: 5  */
    /* g116 = g20 * g96 = */ to_float4(-psi, -0.5f,  phi,  0.0f),      /* order :: 4  */
    /* g117 = g21 * g96 = */ to_float4( psi,  0.5f, -phi,  0.0f),      /* order :: 4  */
    /* g118 = g22 * g96 = */ to_float4(-phi, -0.5f,  0.0f,  psi),      /* order :: 10 */
    /* g119 = g23 * g96 = */ to_float4( phi,  0.5f,  0.0f, -psi),      /* order :: 5  */
}

#endif

/* helper function :: exchanges the values if necessary so on return we have v0 <= v1 */
__DEVICE__ void order(inout float *v0, inout float *v1)
{
    float vm = _fmaxf(*v0, *v1);
    *v0 = _fminf(*v0, *v1);
    *v1 = vm;
}

/* helper function :: smoothed minimum */
__DEVICE__ float smin(float a, float b, float k)
{
    float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return _mix(b, a, h) - k * h * (1.0f - h);
}

#define phi  0.8090169943749474f  /* (_sqrtf(5) + 1) / 4 = psi + 0.5f */
#define psi  0.3090169943749474f  /* (_sqrtf(5) - 1) / 4 */

//#define pi      3.14159265358979324f
#define two_pi  6.28318530717958648f
#define gamma   2.2f

/* const float4 g24 = to_float4( psi,  0.0f,  0.5f,  phi); */
/* const float4 g48 = to_float4(-psi,  0.0f,  0.5f,  phi); */
/* const float4 g72 = to_float4( 0.0f,  0.5f,  psi,  phi); */
/* const float4 g96 = to_float4( 0.0f, -0.5f,  psi,  phi); */

/* right multiplications by basic quaternions i, j, k */
__DEVICE__ float4 mulr_i(float4 p) { return to_float4( p.w,  p.z, -p.y, -p.x); }
__DEVICE__ float4 mulr_j(float4 p) { return to_float4(-p.z,  p.w,  p.x, -p.y); }
__DEVICE__ float4 mulr_k(float4 p) { return to_float4( p.y, -p.x,  p.w, -p.z); }

/*             | vertices | edges |        2-faces |        3-cells  | */
/* SIMPLEX     |        5 |    10 |   10 triangles |   5 tetrahedra  | */
/* TESSERACT   |       16 |    32 |   24 squares   |   8 cubes       | */
/* ORTHOPLEX   |        8 |    24 |   32 triangles |  16 tetrahedra  | */
/* OCTAPLEX    |       24 |    96 |   96 triangles |  24 octahedra   | */
/* DODECAPLEX  |      600 |  1200 |  720 pentagons | 120 dodecahedra | */
/* TETRAPLEX   |      120 |   720 | 1200 triangles | 600 tetrahedra  | */

/* dot products are maximal when distances between points are minimal */

/*
   returns maximal dot product of p with the vertices of the octaplex
   which, as a point set, coincide with the Hurwitz subgroup of the
   binary icosahedral group

   24 points
*/
__DEVICE__ float4 octaplex_max_dp4(float4 p)
{
    /* (xi + yj + zk + w) * g8 =
       (xi + yj + zk + w) * (i + j + k + 1) / 2 =
        { +xi +yj +zk +w } +
        { +wi +zj -yk -x } +
        { -zi +wj +xk -y } +
        { +yi -xj +wk -z } =  p + s
    */
    float4 s = mulr_i(p) + mulr_j(p) + mulr_k(p);
    float4 p8 = 0.5f * (s + p);

    /* (xi + yj + zk + w) * g16 =
       (xi + yj + zk + w) * (i + j + k - 1) / 2 =
       -{ +xi +yj +zk +w }
        { +wi +zj -yk -x } +
        { -zi +wj +xk -y } +
        { +yi -xj +wk -z } = -p + s
    */
    float4 p16 = 0.5f * (s - p);

    /* taking absolute value of the dot product computes minimum
       distance to the pair of opposite points on the sphere */
    return _fmaxf(abs_f4(p), _fmaxf(abs_f4(p8), abs_f4(p16)));
}

/*
    returns pair of maximal dot products of p with the vertices of the octaplex
    these two maximal dot products always belong to different orthoplectic subsets
*/
__DEVICE__ float2 octaplex_max_dp2(float4 p)
{
    float4 s = mulr_i(p) + mulr_j(p) + mulr_k(p);
    float4 p8 = 0.5f * (s + p);
    float4 p16 = 0.5f * (s - p);

    float4 q0 = abs_f4(p);
    float4 q1 = abs_f4(p8);
    float4 q2 = abs_f4(p16);

    swi2S(q0,x,y, _fmaxf(swi2(q0,x,y), swi2(q0,z,w)));
    swi2S(q1,x,y, _fmaxf(swi2(q1,x,y), swi2(q1,z,w)));
    swi2S(q2,x,y, _fmaxf(swi2(q2,x,y), swi2(q2,z,w)));

    float s0 = _fmaxf(q0.x, q0.y);
    float s1 = _fmaxf(q1.x, q1.y);
    float s2 = _fmaxf(q2.x, q2.y);

    order(&s0, &s1);
    order(&s1, &s2);      /* s2 is maximal */

    s1 = _fmaxf(s0, s1);   /* s1 is second maximal */

    return to_float2(s1, s2);
}

__DEVICE__ float octaplex_max_dp1(float4 p)
{
    float4 s = mulr_i(p) + mulr_j(p) + mulr_k(p);
    float4 p8 = 0.5f * (s + p);
    float4 p16 = 0.5f * (s - p);

    float4 dp = _fmaxf(abs_f4(p), _fmaxf(abs_f4(p8), abs_f4(p16)));
    swi2S(dp,x,y, _fmaxf(swi2(dp,x,y), swi2(dp,z,w)));
    return _fmaxf(dp.x, dp.y);
}

/*
const float4 simplex[5] = vec4[]
(
    to_float4(  mu,  mu,  mu, -0.25f),
    to_float4(  mu, -mu, -mu, -0.25f),
    to_float4( -mu,  mu, -mu, -0.25f),
    to_float4( -mu, -mu,  mu, -0.25f),
    to_float4( 0.0f, 0.0f, 0.0f,   1.0f)
);
*/
#define mu  0.5590169943749474f  /* _sqrtf(5) / 4 */

/*
   returns maximal dot product of p with the vertices of the simplex,
   which does not fit into general scheme of working with binary icosahedral
   group and quaternions, so we just compute distances directly, also note that
   simplex is not symmetric about the origin, e.y if v is the vertex,
   then -v is not, so absolute values of the dot products should not be taken

   5 points
*/
__DEVICE__ float simplex_max_dp(float4 p)
{
    float qw = 0.25f * p.w;
    float gx = mu * p.x;
    float gy = mu * p.y;
    float gz = mu * p.z;

    /* four dot products */
    float4 dp = to_float4(
         gx +  gy +  gz - qw,
         gx + -gy + -gz - qw,
        -gx +  gy + -gz - qw,
        -gx + -gy +  gz - qw
    );

    swi2S(dp,x,y, _fmaxf(swi2(dp,x,y), swi2(dp,z,w)));

    /* the 5th dot product is simply p.w */
    return _fmaxf(max(dp.x, dp.y), p.w);
}

/* returns pair of maximal dot products of p with the vertices of the simplex */
__DEVICE__ float2 simplex_max_dp2(float4 p)
{
    float qw = 0.25f * p.w;
    float gx = mu * p.x;
    float gy = mu * p.y;
    float gz = mu * p.z;

    /* five dot products */
    float q0 =  gx +  gy +  gz - qw;
    float q1 =  gx + -gy + -gz - qw;
    float q2 = -gx +  gy + -gz - qw;
    float q3 = -gx + -gy +  gz - qw;
    float q4 = p.w;

    order(&q0, &q1);
    order(&q1, &q2);
    order(&q2, &q3);
    order(&q3, &q4);      /* q4 is now second maximal */

    q1 = _fmaxf(q0, q1);
    q3 = _fmaxf(q2, q3);
    q3 = _fmaxf(q1, q3);   /* q3 is now second maximal */

    return to_float2(q3, q4);
}

/*
   returns maximal dot product of p with the vertices of the orthoplex, which,
   as a point set, coincide with basic quaternions { ±1, ±i, ±j, ±k }

   8 points
*/
__DEVICE__ float orthoplex_max_dp(float4 p)
{
    float4 d = abs_f4(p);
    swi2S(d,x,y, _fmaxf(swi2(d,x,y), swi2(d,z,w)));
    return _fmaxf(d.x, d.y);
}

/* returns pair of maximal dot products of p with the vertices of the orthoplex */
__DEVICE__ float2 orthoplex_max_dp2(float4 p)
{
    float4 q = abs_f4(p);
    order(&q.x, &q.y);
    order(&q.y, &q.z);
    order(&q.z, &q.w);                /* q.w is now maximal */
    q.z = _fmaxf(q.z, _fmaxf(q.x, q.y));  /* q.z is second maximal */
    return swi2(q,z,w);
}

/*
   returns maximal dot product of p with the vertices of the tesseract, which,
   as a point set, coincide with half-integral Hurwitz quaternions

   16 points
*/
__DEVICE__ float tesseract_max_dp(float4 p)
{
    float4 s = mulr_i(p) + mulr_j(p) + mulr_k(p);
    float4 p8 = 0.5f * (s + p);
    float4 p16 = 0.5f * (s - p);
    float4 dp = _fmaxf(abs_f4(p8), abs_f4(p16));
    swi2S(dp,x,y, _fmaxf(swi2(dp,x,y), swi2(dp,z,w)));
    return _fmaxf(dp.x, dp.y);
}

/*
    returns pair of maximal dot products of p with the vertices of the tesseract
    these two maximal dot products always belong to different orthoplectic subsets
*/
__DEVICE__ float2 tesseract_max_dp2(float4 p)
{
    float4 s = mulr_i(p) + mulr_j(p) + mulr_k(p);
    float4 p8 = 0.5f * (s + p);
    float4 p16 = 0.5f * (s - p);
    float4 dp0 = abs_f4(p8);
    float4 dp1 = abs_f4(p16);
    swi2S(dp0,x,y, _fmaxf(swi2(dp0,x,y), swi2(dp0,z,w)));
    swi2S(dp1,x,y, _fmaxf(swi2(dp1,x,y), swi2(dp1,z,w)));

    float q0 = _fmaxf(dp0.x, dp0.y);
    float q1 = _fmaxf(dp1.x, dp1.y);
    return to_float2(_fminf(q0, q1), _fmaxf(q0, q1));
}

/*
   returns maximal dot product of p with the vertices of the tetraplex
   which, as a point set, coincide with the elements of the whole binary
   icosahedral group

   120 points
*/

__DEVICE__ float tetraplex_max_dp(float4 p)
{
    /* p * g24 =  psi * (p * i) + 0.5f * (p * k) + phi * p */
    /* p * g48 = -psi * (p * i) + 0.5f * (p * k) + phi * p */
    /* p * g72 =  0.5f * (p * j) + psi * (p * k) + phi * p */
    /* p * g96 = -0.5f * (p * j) + psi * (p * k) + phi * p */

    float4 psi_p = psi * p;
    float4 half_p = 0.5f * p;

    float4 psi_pi = mulr_i(psi_p);
    float4 half_pj = mulr_j(half_p);
    float4 half_pk = mulr_k(half_p);
    float4 psi_pk = mulr_k(psi_p);
    float4 phi_p = psi_p + half_p;

    float4 p24 = phi_p + half_pk + psi_pi;
    float4 p48 = phi_p + half_pk - psi_pi;
    float4 p72 = phi_p + psi_pk  + half_pj;
    float4 p96 = phi_p + psi_pk  - half_pj;

    float4 q0 = octaplex_max_dp4(p);
    float4 q1 = octaplex_max_dp4(p24);
    float4 q2 = octaplex_max_dp4(p48);
    float4 q3 = octaplex_max_dp4(p72);
    float4 q4 = octaplex_max_dp4(p96);

    float4 d = _fmaxf(q0, _fmaxf(q1, _fmaxf(q2, _fmaxf(q3, q4))));
    swi2S(d,x,y, _fmaxf(swi2(d,x,y), swi2(d,z,w)));
    return _fmaxf(d.x, d.y);
}

/*
    returns pair of maximal dot products of p with the vertices of the tetraplex,
    these two maximal dot products always belong to different octaplectic subsets
*/
__DEVICE__ float2 tetraplex_max_dp2(float4 p)
{
    float4 psi_p = psi * p;
    float4 half_p = 0.5f * p;

    float4 psi_pi = mulr_i(psi_p);
    float4 half_pj = mulr_j(half_p);
    float4 half_pk = mulr_k(half_p);
    float4 psi_pk = mulr_k(psi_p);
    float4 phi_p = psi_p + half_p;

    float4 p24 = phi_p + half_pk + psi_pi;
    float4 p48 = phi_p + half_pk - psi_pi;
    float4 p72 = phi_p + psi_pk  + half_pj;
    float4 p96 = phi_p + psi_pk  - half_pj;

    float q0 = octaplex_max_dp1(p);
    float q1 = octaplex_max_dp1(p24);
    float q2 = octaplex_max_dp1(p48);
    float q3 = octaplex_max_dp1(p72);
    float q4 = octaplex_max_dp1(p96);

    order(&q0, &q1);
    order(&q1, &q2);
    order(&q2, &q3);
    order(&q3, &q4);      /* q4 is now maximal */

    q1 = _fmaxf(q0, q1);
    q3 = _fmaxf(q2, q3);
    q3 = _fmaxf(q1, q3);   /* q3 is now second maximal */

    return to_float2(q3, q4);
}

__DEVICE__ float octa_sdf(float4 p)
{
    float2 dp = octaplex_max_dp2(p);
    float2 alpha = acos_f2(_fminf(dp, to_float2_s(1.0f)));
    float edge_sdf = alpha.x + alpha.y - 1.0495f;
    float vertex_sdf = alpha.y - 0.077f;
    return smin(edge_sdf, vertex_sdf, 0.005f);
}

__DEVICE__ float linked_octaplexi_sdf(float4 p)
{
    float4 psi_p = psi * p;
    float4 half_p = 0.5f * p;

    float4 psi_pi = mulr_i(psi_p);
    float4 half_pj = mulr_j(half_p);
    float4 half_pk = mulr_k(half_p);
    float4 psi_pk = mulr_k(psi_p);
    float4 phi_p = psi_p + half_p;

    float4 p24 = phi_p + half_pk + psi_pi;
    float4 p48 = phi_p + half_pk - psi_pi;
    float4 p72 = phi_p + psi_pk  + half_pj;
    float4 p96 = phi_p + psi_pk  - half_pj;

    float q0 = octa_sdf(p);
    float q1 = octa_sdf(p24);
    float q2 = octa_sdf(p48);
    float q3 = octa_sdf(p72);
    float q4 = octa_sdf(p96);

    return _fminf(q0, _fminf(q1, _fminf(q2, _fminf(q3, q4))));
}

//#define RENDER_SIMPLEX
//#define RENDER_DUAL_SIMPLEX
//#define RENDER_SIMPLEX_PAIR

//#define RENDER_ORTHOPLEX
//#define RENDER_DUAL_TESSERACT
//#define RENDER_ORTHOPLEX_DUAL_TESSERACT_PAIR

//#define RENDER_TESSERACT
//#define RENDER_DUAL_ORTHOPLEX
//#define RENDER_TESSERACT_DUAL_ORTHOPLEX_PAIR

//#define RENDER_OCTAPLEX
//#define RENDER_DUAL_OCTAPLEX
//#define RENDER_OCTAPLEX_PAIR
//#define RENDER_LINKED_OCTAPLEXI

//#define RENDER_TETRAPLEX
//#define RENDER_DODECAPLEX

//---------------
#define RENDER_SIMPLEX        0
#define RENDER_DUAL_SIMPLEX   1
#define RENDER_SIMPLEX_PAIR   2

#define RENDER_ORTHOPLEX      3
#define RENDER_DUAL_TESSERACT 4
#define RENDER_ORTHOPLEX_DUAL_TESSERACT_PAIR  5

#define RENDER_TESSERACT      6
#define RENDER_DUAL_ORTHOPLEX 7
#define RENDER_TESSERACT_DUAL_ORTHOPLEX_PAIR  8

#define RENDER_OCTAPLEX       9
#define RENDER_DUAL_OCTAPLEX  10
#define RENDER_OCTAPLEX_PAIR  11
#define RENDER_LINKED_OCTAPLEXI               12

#define RENDER_TETRAPLEX      13
#define RENDER_DODECAPLEX     14

#define RENDER BOTH           15

/*
   removing spheres in the centers of 3-cells of a 4-polyhedron is
   the same as removing spheres around vertices of the dual 4-polyhedron,
   so to render dodecaplex we use distance-to-vertices function of the
   dual polyhedron, the tetraplex
*/



__DEVICE__ float sdf(float4 p, inout float *id, int modus)
{
if (modus == RENDER_SIMPLEX)
{
    /* simplex is self-dual */
    float dp = clamp(simplex_max_dp(p), -1.0f, 1.0f);
    float s = _acosf(dp);
    return 1.108f - s;
}    
else if (modus == RENDER_DUAL_SIMPLEX)
{
    float2 dp = clamp(simplex_max_dp2(p), -1.0f, 1.0f);
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 1.838f;
    float v_sdf = a2 - 0.243f;
    return smin(e_sdf, v_sdf, 0.005f);
}
else if (modus == RENDER_SIMPLEX_PAIR)
{
    float2 dp = clamp(simplex_max_dp2(p), -1.0f, 1.0f);
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 1.838f;
    float v_sdf = a2 - 0.243f;
    float sdf1 = smin(e_sdf, v_sdf, 0.005f);
    float sdf2 = 1.108f - a2;
    *id = step(sdf1, sdf2);
    return _fminf(sdf1, sdf2);
}
else if (modus == RENDER_ORTHOPLEX)
{
    /* dual to orthoplex is tesseract */
    float dp = _fminf(tesseract_max_dp(p), 1.0f);
    float s = _acosf(dp);
    return 0.761f - s;
}
else if (modus == RENDER_DUAL_TESSERACT)
{
    float2 dp = _fminf(tesseract_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = 0.812f * (a1 + a2 - 1.062f);
    float v_sdf = a2 - 0.173f;
    return smin(e_sdf, v_sdf, 0.005f);
}
else if (modus == RENDER_ORTHOPLEX_DUAL_TESSERACT_PAIR)
{
    float2 dp = _fminf(tesseract_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 1.062f;
    float v_sdf = a2 - 0.173f;
    float sdf1 = smin(e_sdf, v_sdf, 0.005f);
    float sdf2 = 0.761f - a2;
    *id = step(sdf1, sdf2);
    return _fminf(sdf1, sdf2);
}
else if (modus == RENDER_TESSERACT)
{
    /* dual to tesseract is orthoplex */
    float dp = _fminf(orthoplex_max_dp(p), 1.0f);
    float s = _acosf(dp);
    return 0.914f - s;
}
else if (modus == RENDER_DUAL_ORTHOPLEX)
{
    float2 dp = _fminf(orthoplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 1.584f;
    float v_sdf = a2 - 0.181f;
    return smin(e_sdf, v_sdf, 0.005f);
}
else if (modus == RENDER_TESSERACT_DUAL_ORTHOPLEX_PAIR)
{
    float2 dp = _fminf(orthoplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 1.584f;
    float v_sdf = a2 - 0.181f;
    float sdf1 = smin(e_sdf, v_sdf, 0.005f);
    float sdf2 = 0.914f - a2;
    *id = step(sdf1, sdf2);
    return _fminf(sdf1, sdf2);
}
else if (modus == RENDER_OCTAPLEX)
{
    /* octaplex is self-dual */
    float4 dp4 = octaplex_max_dp4(p);
    swi2(dp4,x,y) = _fmaxf(swi2(dp4,x,y), swi2(dp4,z,w));
    float dp = _fminf(max(dp4.x, dp4.y), 1.0f);
    float s = _acosf(dp);
    return 0.604f - s;
}
else if (modus == RENDER_DUAL_OCTAPLEX)
{
    float2 dp = _fminf(octaplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = 1.211f * (a1 + a2 - 1.051f);
    float v_sdf = a2 - 0.137f;
    return smin(e_sdf, v_sdf, 0.005f);
}
else if (modus == RENDER_OCTAPLEX_PAIR)
{
    float2 dp = _fminf(octaplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = 1.211f * (a1 + a2 - 1.051f);
    float v_sdf = a2 - 0.137f;
    float sdf1 = smin(e_sdf, v_sdf, 0.005f);
    float sdf2 = 0.604f - a2;
    *id = step(sdf1, sdf2);
    return _fminf(sdf1, sdf2);
}
else if (modus == RENDER_LINKED_OCTAPLEXI)
{
    return linked_octaplexi_sdf(p);
}    
else if (modus == RENDER_TETRAPLEX)
{
    /*
       here we do not want to compute the distance-to-600-points function, so we use different idea
       that in the vicinity of tetraplex edges the sum of the two minimal distances will be slightly
       greater than the length of the tetraplex edge, and is substantially greater everywhere else

       the same we use to compute sdf for dual simplex and dual octaplex - the sum of the two minimal
       distances to the vertices reaches minimum on the edges of the 4-polyhedron, so if one
       subtracts slightly more than the length of the edge one gets a tube around the edge, where
       sdf is negative
    */
    float2 dp = _fminf(tetraplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 0.631f;
    float v_sdf = a2 - 0.091f;
    return smin(e_sdf, v_sdf, 0.005f);                            /* tetraplex sdf */
}
else if (modus == RENDER_DODECAPLEX)
{
    /* dual to dodecaplex is tetraplex */
    float dp = _fminf(tetraplex_max_dp(p), 1.0f);
    float s = _acosf(dp);
    return 0.351f - s;
}
//else /* RENDER BOTH */
{
    float2 dp = _fminf(tetraplex_max_dp2(p), to_float2_s(1.0f));
    float a1 = _acosf(dp.x);
    float a2 = _acosf(dp.y);
    float e_sdf = a1 + a2 - 0.631f;
    float v_sdf = a2 - 0.091f;
    float sdf1 = smin(e_sdf, v_sdf, 0.005f);                      /* tetraplex sdf  */
    float sdf2 = 0.351f - a2;                                     /* dodecaplex sdf */
    *id = step(sdf1, sdf2);
    return _fminf(sdf1, sdf2);
}    

}

/* ambient occlusion : from this website */
__DEVICE__ float ambient_occlusion(float4 p, float4 n, inout float *id, int modus) 
{
  float uuuuuuuuuuuuuuuu;
    const int ao_iterations = 16;
    const float ao_step = 0.01f;
    const float ao_scale = 7.5f;
    
  float sum = 0.0f;
  float att = 1.0f;
  float l = ao_step;
    
  for (int i = 0; i < ao_iterations; ++i) 
    {
    sum += (l - sdf(p * _cosf(l) + n * _sinf(l),id,modus)) * att;
    
    l += ao_step;
    
    att *= 0.75f;
  }
  
  return _fmaxf(1.0f - sum * ao_scale, 0.0f);
}

//==============================================================================================================================================================
//   SDF gradient :: 4-point tetrahedral evaluation
//
//   A good thing is that 3-sphere is a parallelizable manifold, so that at any point
// one can explicitly specify tangent basis triple, which smoothly varies with the point
//
//   Such a basis for example is: { p*i, p*j, p*k }
//==============================================================================================================================================================
__DEVICE__ float4 spherical_gradient4(float4 p, inout float *id, int modus)
{
    const float eps = 0.0525f;
float tttttttttttttttt;
    float4 pi = mulr_i(p);
    float4 pj = mulr_j(p);
    float4 pk = mulr_k(p);

    float v0 = sdf(normalize(p + eps * ( pi - pj - pk)),id,modus);
    float v1 = sdf(normalize(p + eps * (- pi - pj + pk)),id,modus);
    float v2 = sdf(normalize(p + eps * (- pi + pj - pk)),id,modus);
    float v3 = sdf(normalize(p + eps * ( pi + pj + pk)),id,modus);

    float3 g = normalize(
        to_float3(
              v0 - v1 - v2 + v3,
            - v0 - v1 + v2 + v3,
            - v0 + v1 - v2 + v3
        )
    );

    return g.x * pi + g.y * pj + g.z * pk;
}

//==============================================================================================================================================================
//   SDF gradient :: standard 6-point evaluation
//==============================================================================================================================================================
__DEVICE__ float4 spherical_gradient6(float4 p, inout float *id, int modus)
{
    const float eps = 0.0125f;

    float4 pi = mulr_i(p);
    float4 pj = mulr_j(p);
    float4 pk = mulr_k(p);

    float norm = _sqrtf(1.0f + eps * eps);
    float inv_norm = 1.0f / norm;
    float f = eps * inv_norm;
    float4 pn = inv_norm * p;


    float di = sdf(pn + f * pi,id,modus) - sdf(pn - f * pi,id,modus);
    float dj = sdf(pn + f * pj,id,modus) - sdf(pn - f * pj,id,modus);
    float dk = sdf(pn + f * pk,id,modus) - sdf(pn - f * pk,id,modus);

    float3 g = normalize(to_float3(di, dj, dk));
    return g.x * pi + g.y * pj + g.z * pk;
}

__DEVICE__ float4 raymarch(float4 origin, float4 ray, out float *dist, inout float *id, int modus)
{
    const float eps = 0.00005f;
    *dist = -1.0f;

    float4 p = origin;
    float d = sdf(p,id, modus);
    float t = 0.0f;

    while (t < two_pi && d > eps)
    {
        t += d;
        p = _cosf(t) * origin + _sinf(t) * ray;
        d = sdf(p,id, modus);
    }

    if (d < eps)
        *dist = t;

    return p;
}

/* p and n are assumed perpendicular and lie on the unit sphere */
__DEVICE__ float3 sample_tex4d(__TEXTURE2D__ sampler, float4 p, float4 n, float ratio, float scale)
{
    /* we use the same idea as the idea going around for 3d case -- 
       sample in all planes and blend with appropriate weights */
    //const float scale = 7.0f;

    p.x /= ratio;

    /* we have 6 possible planes xy, yz, zw, wz, xz and yw */
    float3 rgb_xy = pow_f3(swi3(texture(sampler, scale * swi2(p,x,y)),x,y,z), to_float3_s(gamma));
    float3 rgb_yz = pow_f3(swi3(texture(sampler, scale * swi2(p,y,z)),x,y,z), to_float3_s(gamma));
    float3 rgb_zw = pow_f3(swi3(texture(sampler, scale * swi2(p,z,w)),x,y,z), to_float3_s(gamma));
    float3 rgb_wx = pow_f3(swi3(texture(sampler, scale * swi2(p,w,x)),x,y,z), to_float3_s(gamma));
    float3 rgb_xz = pow_f3(swi3(texture(sampler, scale * swi2(p,x,z)),x,y,z), to_float3_s(gamma));
    float3 rgb_yw = pow_f3(swi3(texture(sampler, scale * swi2(p,y,w)),x,y,z), to_float3_s(gamma));
    
    /* projections of the area element spanned by vectors p and n 
       onto 6 coordinate planes are */
       
    float3 area_012 = swi3(p,x,y,z) * swi3(n,y,z,w) - swi3(p,y,z,w) * swi3(n,x,y,z); /* xy, yz and zw */
    float3 area_345 = swi3(p,w,x,y) * swi3(n,x,z,w) - swi3(p,x,z,w) * swi3(n,w,x,y); /* wx, xz and yw */   
    
    /* when the shaded fragment position { x, y, z, w } varies, it varies in such a way 
       that the differential in the neighbourhood stays orthogonal to both n and p directions, 
       so samples in 'perpendicular' planes should obtain higher weights, which means, in terms 
       of areas, the lower the area the higher should be the weight 
       
       also note, that sum of squares of elements of area_012 and area_345 should 
       add to 1 = the square of the area element spanned by p and n
    */
    
    const float tau = 7.0f;
    area_012 = exp_f3(-tau * area_012 * area_012);
    area_345 = exp_f3(-tau * area_345 * area_345);
    
    float w = dot(area_012 + area_345, to_float3_s(1.0f));     /* total weight */
    float inv_w = 1.0f / w;

    return inv_w * (area_012.x * rgb_xy + 
                    area_012.y * rgb_yz +
                    area_012.z * rgb_zw +
                    area_345.x * rgb_wx +
                    area_345.y * rgb_xz +
                    area_345.z * rgb_yw);
}



__KERNEL__ void Dodecaplex2JipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

CONNECT_SLIDER0(scale, 0.0f, 10.0f, 7.0f);
CONNECT_INTSLIDER0(Modus, 0, 15, 15);

//#if defined(RENDER_DUAL_SIMPLEX) || defined(RENDER_DUAL_ORTHOPLEX) || defined(RENDER_DUAL_TESSERACT) || defined(RENDER_DUAL_OCTAPLEX) || defined(RENDER_TETRAPLEX)
//float id = 1.0f;
//#else
//float id = 0.0f;
//#endif

float id = 0.0f;

if ((Modus == RENDER_DUAL_SIMPLEX) || (Modus == RENDER_DUAL_ORTHOPLEX) || (Modus == RENDER_DUAL_TESSERACT) || (Modus == RENDER_DUAL_OCTAPLEX) || (Modus == RENDER_TETRAPLEX))
{  
  id = 1.0f;
}


float IIIIIIIIIIIIIIIIIIII;

const int LIGHT_COUNT = 8;

const float4 light_ws[LIGHT_COUNT] = {
    to_float4( 0.0f,  0.0f,  0.0f,  1.0f),
    to_float4( 1.0f,  0.0f,  0.0f,  0.0f),
    to_float4( 0.0f,  1.0f,  0.0f,  0.0f),
    to_float4( 0.0f,  0.0f,  1.0f,  0.0f),
    to_float4( 0.0f,  0.0f,  0.0f, -1.0f),
    to_float4(-1.0f,  0.0f,  0.0f,  0.0f),
    to_float4( 0.0f, -1.0f,  0.0f,  0.0f),
    to_float4( 0.0f,  0.0f, -1.0f,  0.0f)
    };



    float2 uv = 1.83f * (fragCoord - 0.5f * iResolution) / iResolution.y;

    float ratio = iResolution.x / iResolution.y;

    float t = 0.5f * iTime;

    float t0 = 0.375f * t -  0.213f;
    float t1 = 0.151f * t +  2.091f;
    float t2 = 0.253f * t - 11.512f;
    float t3 = 0.853f * t +  3.277f;

    float c0 = _cosf(t0);
    float s0 = _sinf(t0);
    float c1 = _cosf(t1);
    float s1 = _sinf(t1);
    float c2 = _cosf(t2);
    float s2 = _sinf(t2);
    float c3 = _cosf(t3);
    float s3 = _sinf(t3);

    /* if anyone knows how to smoothly fly through without entering areas
       with sdf < 0 let me know */

    float4 camera_ws = to_float4(-c1 * s0, s1 *  c0 * s2, c0 * c2, s0 * s1);
    camera_ws = normalize(camera_ws);

    float4 X = mulr_i(camera_ws);
    float4 Y = mulr_j(camera_ws);

    float4 camera_X =  c3 * X + s3 * Y;
    float4 camera_Y = -s3 * X + c3 * Y;
    float4 camera_Z = mulr_k(camera_ws);

    float4 view_ray = uv.x * camera_X + uv.y * camera_Y - camera_Z;
    view_ray = normalize(view_ray);

    float dist;
    float4 position = raymarch(camera_ws, view_ray, &dist, &id, Modus);
    float3 color = to_float3_s(0.0f);

    if (dist >= 0.0f)
    {
        /*
           normal, view, and light vectors should be tangent at 'position' point on the sphere
           for the light calculations to make sense, which actually means that they must be normal
           to 'position' viewed as 4-vectors

           !! view_ray can not be used as a view vector because it is tangent at 'camera_ws'
           point, but we need a vector tangent at 'position' point
        */

        bool dual = id > 0.0f;      /* save the value as normal computation will modify it */
        float4 normal = spherical_gradient6(position, &id, Modus);
        float3 rgb = dual ? 
            sample_tex4d(iChannel1, position, normal, ratio, scale):
            sample_tex4d(iChannel0, position, normal, ratio, scale);

        float4 view = normalize(camera_ws - dot(camera_ws, position) * position);
        
        float ao = ambient_occlusion(position, normal, &id, Modus);
        
        color = 0.0125f * rgb;                      /* ambient */

        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            float4 light = light_ws[i];
            float dp = dot(light, position);
            light = light - dp * position;
            light = normalize(light);

            /* close to 1 if light is close to the fragment */
            float d = 0.5f + 0.5f * dp;
            float a = 5.24f * d * d * _expf(-1.27f * dist);

            float cos_theta = _fmaxf(dot(light, normal), 0.0f);
            float4 h = normalize(light + view);

            float cos_alpha = _fmaxf(dot(h, normal), 0.0f);
            float3 diffuse  = (0.475f * cos_theta) * rgb;
            float3 specular = to_float3_s(0.562f) * _powf(cos_alpha, 88.0f);

            color += a * (diffuse + specular) * ao;
        }        

    }
    
    /* everyone does it, i will do it also */
    //color = color / (1.0f + color);
    color = pow_f3(color, to_float3_s(1.0f / gamma));
    
    fragColor = to_float4_aw(color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}