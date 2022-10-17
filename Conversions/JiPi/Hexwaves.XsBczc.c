
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel0


/* hexwaves, by mattz
   License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

   Uses the hex grid traversal code I developed in https://www.shadertoy.com/view/XdSyzK

*/

__DEVICE__ mat3 transpose(mat3 m)
{
    return(to_mat3(m.r0.x,m.r1.x,m.r2.x, m.r0.y,m.r1.y,m.r2.y, m.r0.z,m.r1.z,m.r2.z)); 	
}


// square root of 3 over 2
//__DEVICE__ const float hex_factor = 0.8660254037844386f;
#define hex_factor 0.8660254037844386f

//__DEVICE__ const float3 fog_color = to_float3(0.9f, 0.95f, 1.0f);

#define HEX_FROM_CART(p) to_float2((p).x / hex_factor, (p).y)
#define CART_FROM_HEX(g) to_float2((g).x * hex_factor, (g).y)


//////////////////////////////////////////////////////////////////////
// Used to draw top borders

__DEVICE__ float hexDist(float2 p) {
    p = abs_f2(p);
    return _fmaxf(dot(p, to_float2(hex_factor, 0.5f)), p.y) - 1.0f;
}

//////////////////////////////////////////////////////////////////////
// Given a 2D position, find integer coordinates of center of nearest
// hexagon in plane.

__DEVICE__ float2 nearestHexCell(in float2 pos) {
    
    // integer coords in hex center grid -- will need to be adjusted
    float2 gpos = HEX_FROM_CART(pos);
    float2 hex_int = _floor(gpos);

    // adjust integer coords
    float sy = step(2.0f, mod_f(hex_int.x+1.0f, 4.0f));
    hex_int += mod_f2(to_float2(hex_int.x, hex_int.y + sy), 2.0f);

    // difference vector
    float2 gdiff = gpos - hex_int;

    // figure out which side of line we are on and modify
    // hex center if necessary
    if (dot(abs_f2(gdiff), to_float2(hex_factor*hex_factor, 0.5f)) > 1.0f) {
        float2 delta = sign_f2(gdiff) * to_float2(2.0f, 1.0f);
        hex_int += delta;
    }

    return hex_int;
    
}

//////////////////////////////////////////////////////////////////////
// Flip normal if necessary to have positive dot product with d

__DEVICE__ float2 alignNormal(float2 h, float2 d) {
    return h*sign_f(dot(h, CART_FROM_HEX(d)));
}

//////////////////////////////////////////////////////////////////////
// Intersect a ray with a hexagon wall with normal n

__DEVICE__ float3 rayHexIntersect(float2 ro, float2 rd, float2 h) {

    float2 n = CART_FROM_HEX(h);

    // solve for u such that dot(n, ro+u*rd) = 1.0
    float u = (1.0f - dot(n, ro)) / dot(n, rd);

    // return the 
    return to_float3_aw(h, u);

}

//////////////////////////////////////////////////////////////////////
// Choose the vector whose z coordinate is minimal

__DEVICE__ float3 rayMin(float3 a, float3 b) {
    return a.z < b.z ? a : b;
}

//////////////////////////////////////////////////////////////////////
// From Dave Hoskins' https://www.shadertoy.com/view/4djSRW

#define HASHSCALE3 to_float3(0.1031f, 0.1030f, 0.0973f)

__DEVICE__ float3 hash32(float2 p) {
    float3 p3 = fract_f3((swi3(p,x,y,x)) * HASHSCALE3);
    p3 += dot(p3, swi3(p3,y,x,z)+19.19f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}

//////////////////////////////////////////////////////////////////////
// Return the cell height for the given cell center

__DEVICE__ float height_for_pos(float2 pos,float iTime) {
    
    // shift origin a bit randomly
    pos += to_float2(2.0f*_sinf(iTime*0.3f+0.2f), 2.0f*_cosf(iTime*0.1f+0.5f));
    
    // cosine of distance from origin, modulated by Gaussian
    float x2 = dot(pos, pos);
    float x = _sqrtf(x2);
    
    return 6.0f * _cosf(x*0.2f + iTime) * _expf(-x2/128.0f);
    
}

__DEVICE__ float4 surface(float3 rd, float2 cell, float4 hit_nt, float bdist, float3 fog_color, float2 iResolution, __TEXTURE2D__ iChannel0) {

    // fog coefficient is 1 near origin, 0 far way
    float fogc = _expf(-length(hit_nt.w*rd)*0.02f);

    // get the normal
    float3 n = swi3(hit_nt,x,y,z);

    // add some noise so we don't just purely reflect boring flat cubemap
    // makes a nice "disco ball" look in background
    float3 noise = (hash32(cell)-0.5f)*0.15f;
    n = normalize(n + noise);

    // gotta deal with borders

    // need to antialias more far away
    float border_scale = 2.0f/iResolution.y;

    const float border_size = 0.04f;

    float border = smoothstep(0.0f, border_scale*hit_nt.w, _fabs(bdist)-border_size);

    // don't even try to draw borders too far away
    border = _mix(border, 0.75f, smoothstep(18.0f, 45.0f, hit_nt.w));

    // light direction
    float3 L = normalize(to_float3(3, 1, 4));

    // diffuse + ambient term
    float diffamb = (clamp(dot(n, L), 0.0f, 1.0f) * 0.8f + 0.2f);

    // start out white
    float3 color = to_float3_s( 1.0f );

    // add in border color
    color = _mix(to_float3(0.1f, 0, 0.08f), color, border);

    // multiply by diffuse/ambient
    color *= diffamb;

    // cubemap fake reflection
    color = _mix(color, swi3(decube_f3(iChannel0, reflect(rd, n)),y,z,x), 0.4f*border);

    // fog
    color = _mix(fog_color, color, fogc);
    
    return to_float4_aw(color, border);

}

//////////////////////////////////////////////////////////////////////
// Return the color for a ray with origin ro and direction rd

__DEVICE__ float3 shade(in float3 ro, in float3 rd,float iTime, float3 fog_color, float2 iResolution, __TEXTURE2D__ iChannel0) {
      
    // the color we will return
    float3 color = fog_color;

    // find nearest hex center to ray origin
    float2 cur_cell = nearestHexCell(swi2(ro,x,y));

    // get the three candidate wall normals for this ray (i.e. the
    // three hex side normals with positive dot product to the ray
    // direction)

    float2 h0 = alignNormal(to_float2(0.0f, 1.0f), swi2(rd,x,y));
    float2 h1 = alignNormal(to_float2(1.0f, 0.5f), swi2(rd,x,y));
    float2 h2 = alignNormal(to_float2(1.0f, -0.5f), swi2(rd,x,y));

    // initial cell height at ray origin
    float cell_height = height_for_pos(CART_FROM_HEX(cur_cell),iTime);
    
    // reflection coefficient
    float alpha = 1.0f;

    // march along ray, one iteration per cell
    for (int i=0; i<80; ++i) {
        
        // we will set these when the ray intersects
        bool hit = false;
        float4 hit_nt;
        float bdist = 1e5;

        // after three tests, swi2(ht,x,y) holds the hex grid direction,
        // ht.z holds the ray distance parameter
        float2 cur_center = CART_FROM_HEX(cur_cell);
        float2 rdelta = swi2(ro,x,y)-cur_center;
        
        float3 ht = rayHexIntersect(rdelta, swi2(rd,x,y), h0);
        ht = rayMin(ht, rayHexIntersect(rdelta, swi2(rd,x,y), h1));
        ht = rayMin(ht, rayHexIntersect(rdelta, swi2(rd,x,y), h2));

        // try to intersect with top of cell
        float tz = (cell_height - ro.z) / rd.z;

        // if ray sloped down and ray intersects top of cell before escaping cell
        if (ro.z > cell_height && rd.z < 0.0f && tz < ht.z) {

            // set up intersection info
            hit = true;
            hit_nt = to_float4(0, 0, 1.0f, tz);   
            float2 pinter = swi2(ro,x,y) + swi2(rd,x,y) * tz;

            // distance to hex border
            bdist = hexDist(pinter - cur_center);

        } else { // we hit a cell wall before hitting top.

            // update the cell center by twice the grid direction
            cur_cell += 2.0f * swi2(ht,x,y);
            
            float2 n = CART_FROM_HEX(swi2(ht,x,y));
            cur_center = CART_FROM_HEX(cur_cell);

            float prev_cell_height = cell_height;
            cell_height = height_for_pos(cur_center,iTime);

            // get the ray intersection point with cell wall
            float3 p_intersect = ro + rd*ht.z;

            // if we intersected below the height, it's a hit
            if (p_intersect.z < cell_height) {

                // set up intersection info
                hit_nt = to_float4(n.x,n.y, 0.0f, ht.z);
                hit = true;

                // distance to wall top
                bdist = cell_height - p_intersect.z;

                // distance to wall bottom
                bdist = _fminf(bdist, p_intersect.z - prev_cell_height);

                // distance to wall outer side corner
                float2 p = swi2(p_intersect,x,y) - cur_center;
                p -= n * dot(p, n);
                bdist = _fminf(bdist, _fabs(length(p) - 0.5f/hex_factor));
            }
        }                      
        
        if (hit) {
            
            // shade surface
            float4 hit_color = surface(rd, cur_cell, hit_nt, bdist,fog_color, iResolution, iChannel0);
            
            // mix in reflection
            color = _mix(color, swi3(hit_color,x,y,z), alpha);
            
            // decrease blending coefficient for next bounce
            alpha *= 0.17f * hit_color.w;
            
            // re-iniitialize ray position & direction for reflection ray
            ro = ro + rd*hit_nt.w;
            rd = reflect(rd, swi3(hit_nt,x,y,z));
            ro += 1e-3f*swi3(hit_nt,x,y,z);
                        
            // re-initialize candidate ray directions
            h0 = alignNormal(to_float2(0.0f, 1.0f), swi2(rd,x,y));
            h1 = alignNormal(to_float2(1.0f, 0.5f), swi2(rd,x,y));
            h2 = alignNormal(to_float2(1.0f, -0.5f), swi2(rd,x,y));
        }
    }
    
    // use leftover ray energy to show sky
    color = _mix(color, fog_color, alpha);
    
    return color;
}  

//////////////////////////////////////////////////////////////////////
// Pretty much my boilerplate rendering code, just a couple of 
// fancy twists like radial distortion and vingetting.

__KERNEL__ void HexwavesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
  const float3 fog_color = to_float3(0.9f, 0.95f, 1.0f);
  
  const float yscl = 720.0f;
  const float f = 500.0f;
  
  float2 uvn = (fragCoord - 0.5f*iResolution) / iResolution.y;
  float2 uv = uvn * yscl;
  
  float3 pos = to_float3(-12.0f, 0.0f, 10.0f);
  float3 tgt = to_float3_s(0.0f);
  float3 up = to_float3(0.0f, 0.0f, 1.0f);
  
  float3 rz = normalize(tgt - pos);
  float3 rx = normalize(cross(rz,up));
  float3 ry = cross(rx,rz);
    
  // compute radial distortion
  float s = 1.0f + dot(uvn, uvn)*1.5f;
   
  float3 rd = mul_mat3_f3(to_mat3_f3(rx,ry,rz),normalize(to_float3_aw(uv*s, f)));
  float3 ro = pos;

  float thetax = -0.35f - 0.2f*_cosf(0.031513f*iTime);
  float thetay = -0.02f*iTime;
  
  if (iMouse.y > 10.0f || iMouse.x > 10.0f) { 
    thetax = (iMouse.y - 0.5f*iResolution.y) * -1.25f/iResolution.y;
    thetay = (iMouse.x - 0.5f*iResolution.x) * 6.28f/iResolution.x; 
  }

  float cx = _cosf(thetax);
  float sx = _sinf(thetax);
  float cy = _cosf(thetay);
  float sy = _sinf(thetay);
  
  mat3 Rx = to_mat3(1.0f, 0.0f, 0.0f, 
                    0.0f, cx, sx,
                    0.0f, -sx, cx);
  
  mat3 Ry = to_mat3(cy, 0.0f, -sy,
                    0.0f, 1.0f, 0.0f,
                    sy, 0.0f, cy);
      
  mat3 R = to_mat3(0.0f, 0.0f, 1.0f,
                  -1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f);
  
  rd = mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(mul_mat3_mat3(transpose(R),Ry),Rx),R),rd);
  ro = mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(mul_mat3_mat3(transpose(R),Ry),Rx),R),(pos-tgt)) + tgt;

  float3 color = shade(ro, rd, iTime, fog_color, iResolution, iChannel0);
  color = sqrt_f3(color);
    
  float2 q = fragCoord / iResolution;
   
  // stole iq's vingette code
  color *= _powf( 16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.1f );       

  fragColor = to_float4_aw(color, 1.0f);
    


  SetFragmentShaderComputedColor(fragColor);
}