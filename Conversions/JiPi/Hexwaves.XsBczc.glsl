

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/* hexwaves, by mattz
   License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

   Uses the hex grid traversal code I developed in https://www.shadertoy.com/view/XdSyzK

*/


// square root of 3 over 2
const float hex_factor = 0.8660254037844386;

const vec3 fog_color = vec3(0.9, 0.95, 1.0);

#define HEX_FROM_CART(p) vec2(p.x / hex_factor, p.y)
#define CART_FROM_HEX(g) vec2(g.x * hex_factor, g.y)


//////////////////////////////////////////////////////////////////////
// Used to draw top borders

float hexDist(vec2 p) {
    p = abs(p);
    return max(dot(p, vec2(hex_factor, 0.5)), p.y) - 1.0;
}

//////////////////////////////////////////////////////////////////////
// Given a 2D position, find integer coordinates of center of nearest
// hexagon in plane.

vec2 nearestHexCell(in vec2 pos) {
    
    // integer coords in hex center grid -- will need to be adjusted
    vec2 gpos = HEX_FROM_CART(pos);
    vec2 hex_int = floor(gpos);

    // adjust integer coords
    float sy = step(2.0, mod(hex_int.x+1.0, 4.0));
    hex_int += mod(vec2(hex_int.x, hex_int.y + sy), 2.0);

    // difference vector
    vec2 gdiff = gpos - hex_int;

    // figure out which side of line we are on and modify
    // hex center if necessary
    if (dot(abs(gdiff), vec2(hex_factor*hex_factor, 0.5)) > 1.0) {
        vec2 delta = sign(gdiff) * vec2(2.0, 1.0);
        hex_int += delta;
    }

    return hex_int;
    
}

//////////////////////////////////////////////////////////////////////
// Flip normal if necessary to have positive dot product with d

vec2 alignNormal(vec2 h, vec2 d) {
    return h*sign(dot(h, CART_FROM_HEX(d)));
}

//////////////////////////////////////////////////////////////////////
// Intersect a ray with a hexagon wall with normal n

vec3 rayHexIntersect(vec2 ro, vec2 rd, vec2 h) {

    vec2 n = CART_FROM_HEX(h);

    // solve for u such that dot(n, ro+u*rd) = 1.0
    float u = (1.0 - dot(n, ro)) / dot(n, rd);

    // return the 
    return vec3(h, u);

}

//////////////////////////////////////////////////////////////////////
// Choose the vector whose z coordinate is minimal

vec3 rayMin(vec3 a, vec3 b) {
    return a.z < b.z ? a : b;
}

//////////////////////////////////////////////////////////////////////
// From Dave Hoskins' https://www.shadertoy.com/view/4djSRW

#define HASHSCALE3 vec3(.1031, .1030, .0973)

vec3 hash32(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

//////////////////////////////////////////////////////////////////////
// Return the cell height for the given cell center

float height_for_pos(vec2 pos) {
    
    // shift origin a bit randomly
    pos += vec2(2.0*sin(iTime*0.3+0.2), 2.0*cos(iTime*0.1+0.5));
    
    // cosine of distance from origin, modulated by Gaussian
    float x2 = dot(pos, pos);
    float x = sqrt(x2);
    
    return 6.0 * cos(x*0.2 + iTime) * exp(-x2/128.0);
    
}

vec4 surface(vec3 rd, vec2 cell, vec4 hit_nt, float bdist) {

    // fog coefficient is 1 near origin, 0 far way
    float fogc = exp(-length(hit_nt.w*rd)*0.02);

    // get the normal
    vec3 n = hit_nt.xyz;

    // add some noise so we don't just purely reflect boring flat cubemap
    // makes a nice "disco ball" look in background
    vec3 noise = (hash32(cell)-0.5)*0.15;
    n = normalize(n + noise);

    // gotta deal with borders

    // need to antialias more far away
    float border_scale = 2.0/iResolution.y;

    const float border_size = 0.04;

    float border = smoothstep(0.0, border_scale*hit_nt.w, abs(bdist)-border_size);

    // don't even try to draw borders too far away
    border = mix(border, 0.75, smoothstep(18.0, 45.0, hit_nt.w));

    // light direction
    vec3 L = normalize(vec3(3, 1, 4));

    // diffuse + ambient term
    float diffamb = (clamp(dot(n, L), 0.0, 1.0) * 0.8 + 0.2);

    // start out white
    vec3 color = vec3( 1.0 );

    // add in border color
    color = mix(vec3(0.1, 0, 0.08), color, border);

    // multiply by diffuse/ambient
    color *= diffamb;

    // cubemap fake reflection
    color = mix(color, texture(iChannel0, reflect(rd, n)).yzx, 0.4*border);

    // fog
    color = mix(fog_color, color, fogc);
    
    return vec4(color, border);

}

//////////////////////////////////////////////////////////////////////
// Return the color for a ray with origin ro and direction rd

vec3 shade(in vec3 ro, in vec3 rd) {
    	
    // the color we will return
    vec3 color = fog_color;

    // find nearest hex center to ray origin
    vec2 cur_cell = nearestHexCell(ro.xy);

    // get the three candidate wall normals for this ray (i.e. the
    // three hex side normals with positive dot product to the ray
    // direction)

    vec2 h0 = alignNormal(vec2(0.0, 1.0), rd.xy);
    vec2 h1 = alignNormal(vec2(1.0, 0.5), rd.xy);
    vec2 h2 = alignNormal(vec2(1.0, -0.5), rd.xy);

    // initial cell height at ray origin
    float cell_height = height_for_pos(CART_FROM_HEX(cur_cell));
    
    // reflection coefficient
    float alpha = 1.0;

    // march along ray, one iteration per cell
    for (int i=0; i<80; ++i) {
        
        // we will set these when the ray intersects
        bool hit = false;
        vec4 hit_nt;
        float bdist = 1e5;

        // after three tests, ht.xy holds the hex grid direction,
        // ht.z holds the ray distance parameter
        vec2 cur_center = CART_FROM_HEX(cur_cell);
        vec2 rdelta = ro.xy-cur_center;
        
        vec3 ht = rayHexIntersect(rdelta, rd.xy, h0);
        ht = rayMin(ht, rayHexIntersect(rdelta, rd.xy, h1));
        ht = rayMin(ht, rayHexIntersect(rdelta, rd.xy, h2));

        // try to intersect with top of cell
        float tz = (cell_height - ro.z) / rd.z;

        // if ray sloped down and ray intersects top of cell before escaping cell
        if (ro.z > cell_height && rd.z < 0.0 && tz < ht.z) {

            // set up intersection info
            hit = true;
            hit_nt = vec4(0, 0, 1.0, tz);   
            vec2 pinter = ro.xy + rd.xy * tz;

            // distance to hex border
            bdist = hexDist(pinter - cur_center);

        } else { // we hit a cell wall before hitting top.

            // update the cell center by twice the grid direction
            cur_cell += 2.0 * ht.xy;
            
            vec2 n = CART_FROM_HEX(ht.xy);
            cur_center = CART_FROM_HEX(cur_cell);

            float prev_cell_height = cell_height;
            cell_height = height_for_pos(cur_center);

            // get the ray intersection point with cell wall
            vec3 p_intersect = ro + rd*ht.z;

            // if we intersected below the height, it's a hit
            if (p_intersect.z < cell_height) {

                // set up intersection info
                hit_nt = vec4(n, 0.0, ht.z);
                hit = true;

                // distance to wall top
                bdist = cell_height - p_intersect.z;

                // distance to wall bottom
                bdist = min(bdist, p_intersect.z - prev_cell_height);

                // distance to wall outer side corner
                vec2 p = p_intersect.xy - cur_center;
                p -= n * dot(p, n);
                bdist = min(bdist, abs(length(p) - 0.5/hex_factor));

            }

        }                      
        
        if (hit) {
            
            // shade surface
            vec4 hit_color = surface(rd, cur_cell, hit_nt, bdist);
            
            // mix in reflection
            color = mix(color, hit_color.xyz, alpha);
            
            // decrease blending coefficient for next bounce
            alpha *= 0.17 * hit_color.w;
            
            // re-iniitialize ray position & direction for reflection ray
            ro = ro + rd*hit_nt.w;
            rd = reflect(rd, hit_nt.xyz);
            ro += 1e-3*hit_nt.xyz;
                        
            // re-initialize candidate ray directions
            h0 = alignNormal(vec2(0.0, 1.0), rd.xy);
            h1 = alignNormal(vec2(1.0, 0.5), rd.xy);
            h2 = alignNormal(vec2(1.0, -0.5), rd.xy);

        }

    }
    
    // use leftover ray energy to show sky
    color = mix(color, fog_color, alpha);
    
    return color;
	
}	

//////////////////////////////////////////////////////////////////////
// Pretty much my boilerplate rendering code, just a couple of 
// fancy twists like radial distortion and vingetting.

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	
	const float yscl = 720.0;
	const float f = 500.0;
	
    vec2 uvn = (fragCoord.xy - 0.5*iResolution.xy) / iResolution.y;
	vec2 uv = uvn * yscl;
	
	vec3 pos = vec3(-12.0, 0.0, 10.0);
	vec3 tgt = vec3(0.);
	vec3 up = vec3(0.0, 0.0, 1.0);
	
	vec3 rz = normalize(tgt - pos);
	vec3 rx = normalize(cross(rz,up));
	vec3 ry = cross(rx,rz);
    
    // compute radial distortion
    float s = 1.0 + dot(uvn, uvn)*1.5;
	 
	vec3 rd = mat3(rx,ry,rz)*normalize(vec3(uv*s, f));
	vec3 ro = pos;

	float thetax = -0.35 - 0.2*cos(0.031513*iTime);
	float thetay = -0.02*iTime;
	
	if (iMouse.y > 10.0 || iMouse.x > 10.0) { 
		thetax = (iMouse.y - 0.5*iResolution.y) * -1.25/iResolution.y;
		thetay = (iMouse.x - 0.5*iResolution.x) * 6.28/iResolution.x; 
	}

	float cx = cos(thetax);
	float sx = sin(thetax);
	float cy = cos(thetay);
	float sy = sin(thetay);
	
	mat3 Rx = mat3(1.0, 0.0, 0.0, 
				   0.0, cx, sx,
				   0.0, -sx, cx);
	
	mat3 Ry = mat3(cy, 0.0, -sy,
				   0.0, 1.0, 0.0,
				   sy, 0.0, cy);
    
    mat3 R = mat3(0.0, 0.0, 1.0,
                  -1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);
	
	rd = transpose(R)*Ry*Rx*R*rd;
	ro = transpose(R)*Ry*Rx*R*(pos-tgt) + tgt;

	vec3 color = shade(ro, rd);
    color = sqrt(color);
    
    vec2 q = fragCoord.xy / iResolution.xy;
    
    // stole iq's vingette code
    color *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 );       

    fragColor = vec4(color, 1.0);
    
}