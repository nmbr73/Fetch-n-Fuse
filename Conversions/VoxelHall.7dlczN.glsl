

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
    Voxel Hall Colors
    
    Forked from Shane's Voxel Corridor.
    I remove the texture and added a surface color with a time gradient.
    Changing the ambient light's color would have been more "correct" but I liked
    the way this looks.
    I also experimented a bit with different bumpmap textures, and settled on just using 
    the original procedural bricks.

    Original author's comments below.
    --------------
    
	Voxel Corridor
	--------------

	I love the voxel aesthetic, so after looking at some of Akohdr's examples, I went on a bit 
	of a voxel trip and put this simple scene together... Although, "scene" would  be putting 
	it loosely. :)

	Quasi-discreet distance calculations sound simple enough to perform in theory, but are just 
	plain fiddly to code, so I was very thankful to have fb39ca4's, IQ's, Reinder's, and everyone 
	elses voxel examples to refer to.

	The code is pretty straight forward. I tried my best to write it in such way that enables
	someone to plug in any normal distance function and have it render the voxelized version.

	Mainly based on the following:

	Voxel Ambient Occlusion - fb39ca4
    https://www.shadertoy.com/view/ldl3DS

	Minecraft - Reinder
    https://www.shadertoy.com/view/4ds3WS

	Other examples:
	Rounded Voxels - IQ
    https://www.shadertoy.com/view/4djGWR

	Sampler - w23
	https://www.shadertoy.com/view/MlfGRM

	Text In Space - akohdr
	https://www.shadertoy.com/view/4d3SWB

*/

#define PI 3.14159265
#define FAR 60.

// 2x2 matrix rotation. Note the absence of "cos." It's there, but in disguise, and comes courtesy
// of Fabrice Neyret's "ouside the box" thinking. :)
mat2 rot2( float a ){ vec2 v = sin(vec2(1.570796, 0) + a);	return mat2(v, -v.y, v.x); }

// Tri-Planar blending function. Based on an old Nvidia tutorial.
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
  
    n = max(abs(n), 0.001);//n = max((abs(n) - 0.2)*7., 0.001); //  etc.
    n /= (n.x + n.y + n.z ); 
	p = (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
    return p*p;
}

// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
vec2 path(in float z){ 
    //return vec2(0); // Straight.
    float a = sin(z * 0.11);
    float b = cos(z * 0.14);
    return vec2(a*4. -b*1.5, b*1.7 + a*1.5); 
}

/*
// Alternate distance field -- Twisted planes. 
float map(vec3 p){
    
     // You may need to reposition the light to work in with the shadows, but for
     // now, I'm repositioning the scene up a bit.
     p.y -= .75;
     p.xy -= path(p.z); // Move the scene around a sinusoidal path.
     p.xy = rot2(p.z/8.)*p.xy; // Twist it about XY with respect to distance.
    
     float n = dot(sin(p*1. + sin(p.yzx*.5 + iTime*.0)), vec3(.25)); // Sinusoidal layer.
     
     return 4. - abs(p.y) + n; // Warped double planes, "abs(p.y)," plus surface layers.
 
}
*/

// Standard perturbed tunnel function.
//
float map(vec3 p){
     
     // Offset the tunnel about the XY plane as we traverse Z.
     p.xy -= path(p.z);
    
     // Standard tunnel. Comment out the above first.
     float n = 5. - length(p.xy*vec2(1, .8));
    
     // Square tunnel. Almost redundant in a voxel renderer. :)
     //n = 4. - max(abs(p.x), abs(p.y)); 
     
     // Tunnel with a floor.
     return min(p.y + 3., n); //n = min(-abs(p.y) + 3., n);
 
}

/*
float brickShade(vec2 p){
    
    p.x -= step(p.y, 1.)*.5;
    
    p = fract(p);
    
    return pow(16.*p.x*p.y*(1.-p.x)*(1.-p.y), 0.25);
    
}
*/

// The brick groove pattern. Thrown together too quickly.
// Needs some tidy up, but it's quick enough for now.
//
const float w2h = 2.; // Width to height ratio.
const float mortW = 0.05; // Morter width.

float brickMorter(vec2 p){
	
    p.x -= step(1., p.y)*.5;
    
    p = abs(fract(p + vec2(0, .5)) - .5)*2.;
    
    // Smooth grooves. Better for bump mapping.
    return smoothstep(0., mortW, p.x)*smoothstep(0., mortW*w2h, p.y);
    
}

float brick(vec2 p){
    
	p = fract(p*vec2(0.5/w2h, 0.5))*2.;

    return brickMorter(p);
}


// Surface bump function. Cheap, but with decent visual impact.
float bumpSurf3D( in vec3 p, in vec3 n){

    n = abs(n);
    
    if (n.x>0.5) p.xy = p.zy;
    else if (n.y>0.5) p.xy = p.zx;
    
    return brick(p.xy);
    
}

// Standard function-based bump mapping function.
vec3 doBumpMap(in vec3 p, in vec3 nor, float bumpfactor){
    
    const vec2 e = vec2(0.001, 0);
    float ref = bumpSurf3D(p, nor);                 
    vec3 grad = (vec3(bumpSurf3D(p - e.xyy, nor),
                      bumpSurf3D(p - e.yxy, nor),
                      bumpSurf3D(p - e.yyx, nor) )-ref)/e.x;                     
          
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
	
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(0.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
    
}


// This is just a slightly modified version of fb39ca4's code, with some
// elements from IQ and Reinder's examples. They all work the same way:
// Obtain the current voxel, then test the distance field for a hit. If
// the ray has moved into the voxelized isosurface, break. Otherwise, move
// to the next voxel. That involves a bit of decision making - due to the
// nature of voxel boundaries - and the "mask," "side," etc, variable are
// an evolution of that. If you're not familiar with the process, it's 
// pretty straight forward, and there are a lot of examples on Shadertoy, 
// plus a lot more articles online.
//
vec3 voxelTrace(vec3 ro, vec3 rd, out vec3 mask){
    
    vec3 p = floor(ro) + .5;

	vec3 dRd = 1./abs(rd); // 1./max(abs(rd), vec3(.0001));
	rd = sign(rd);
    vec3 side = dRd*(rd * (p - ro) + 0.5);
    
    mask = vec3(0);
	
	for (int i = 0; i < 64; i++) {
		
        if (map(p)<0.) break;
        
        // Note that I've put in the messy reverse step to accomodate
        // the "less than or equals" logic, rather than just the "less than."
        // Without it, annoying seam lines can appear... Feel free to correct
        // me on that, if my logic isn't up to par. It often isn't. :)
        mask = step(side, side.yzx)*(1.-step(side.zxy, side));
		side += mask*dRd;
		p += mask * rd;
	}
    
    return p;    
}


///////////
//
// This is a trimmed down version of fb39ca4's voxel ambient occlusion code with some 
// minor tweaks and adjustments here and there. The idea behind voxelized AO is simple. 
// The execution, not so much. :) So damn fiddly. Thankfully, fb39ca4, IQ, and a few 
// others have done all the hard work, so it's just a case of convincing yourself that 
// it works and using it.
//
// Refer to: Voxel Ambient Occlusion - fb39ca4
// https://www.shadertoy.com/view/ldl3DS
//
vec4 voxelAO(vec3 p, vec3 d1, vec3 d2) {
   
    // Take the four side and corner readings... at the correct positions...
    // That's the annoying bit that I'm glad others have worked out. :)
	vec4 side = vec4(map(p + d1), map(p + d2), map(p - d1), map(p - d2));
	vec4 corner = vec4(map(p + d1 + d2), map(p - d1 + d2), map(p - d1 - d2), map(p + d1 - d2));
	
    // Quantize them. It's either occluded, or it's not, so to speak.
    side = step(side, vec4(0));
    corner = step(corner, vec4(0));
    
    // Use the side and corner values to produce a more honed in value... kind of.
    return 1. - (side + side.yzwx + max(corner, side*side.yzwx))/3.;    
	
}

float calcVoxAO(vec3 vp, vec3 sp, vec3 rd, vec3 mask) {
    
    // Obtain four AO values at the appropriate quantized positions.
	vec4 vAO = voxelAO(vp - sign(rd)*mask, mask.zxy, mask.yzx);
    
    // Use the fractional voxel postion and and the proximate AO values
    // to return the interpolated AO value for the surface position.
    sp = fract(sp);
    vec2 uv = sp.yz*mask.x + sp.zx*mask.y + sp.xy*mask.z;
    return mix(mix(vAO.z, vAO.w, uv.x), mix(vAO.y, vAO.x, uv.x), uv.y);

}
///////////

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
	// Screen coordinates.
	vec2 uv = (fragCoord - iResolution.xy*0.5)/iResolution.y;
	
	// Camera Setup.
	vec3 camPos = vec3(0., 0.5, iTime*8.); // Camera position, doubling as the ray origin.
	vec3 lookAt = camPos + vec3(0.0, 0.0, 0.25);  // "Look At" position.

 
    // Light positioning. 
 	vec3 lightPos = camPos + vec3(0, 2.5, 8);// Put it a bit in front of the camera.

	// Using the Z-value to perturb the XY-plane.
	// Sending the camera, "look at," and two light vectors down the tunnel. The "path" function is 
	// synchronized with the distance function. Change to "path2" to traverse the other tunnel.
	lookAt.xy += path(lookAt.z);
	camPos.xy += path(camPos.z);
	lightPos.xy += path(lightPos.z);

    // Using the above to produce the unit ray-direction vector.
    float FOV = PI/2.; // FOV - Field of view.
    vec3 forward = normalize(lookAt-camPos);
    vec3 right = normalize(vec3(forward.z, 0., -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    
    //vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    //rd = normalize(vec3(rd.xy, rd.z - dot(rd.xy, rd.xy)*.25));    
    
    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
	rd.xy = rot2( path(lookAt.z).x/24. )*rd.xy;

    // Raymarch the voxel grid.
    vec3 mask;
	vec3 vPos = voxelTrace(camPos, rd, mask);
	
    // Using the voxel position to determine the distance from the camera to the hit point.
    // I'm assuming IQ is responsible for this clean piece of logic.
	vec3 tCube = (vPos-camPos - .5*sign(rd))/rd;
    float t = max(max(tCube.x, tCube.y), tCube.z);
    
	
    // Initialize the scene color.
    vec3 sceneCol = vec3(0);
	
	// The ray has effectively hit the surface, so light it up.
	if(t<FAR){
	
   	
    	// Surface position and surface normal.
	    vec3 sp = camPos + rd*t;
        
        // Voxel normal.
        vec3 sn = -(mask * sign( rd ));
        
        // Sometimes, it's necessary to save a copy of the unbumped normal.
        vec3 snNoBump = sn;
        
        // I try to avoid it, but it's possible to do a texture bump and a function-based
        // bump in succession. It's also possible to roll them into one, but I wanted
        // the separation... Can't remember why, but it's more readable anyway.
        //
        // Texture scale factor.
        const float tSize0 = 1./4.;
        // Texture-based bump mapping.
	    //sn = doBumpMap(iChannel0, sp*tSize0, sn, 0.02);

        // Function based bump mapping. Comment it out to see the under layer. It's pretty
        // comparable to regular beveled Voronoi... Close enough, anyway.
        sn = doBumpMap(sp, sn, .15);
        
       
	    // Ambient occlusion.
	    float ao = calcVoxAO(vPos, sp, rd, mask) ;//calculateAO(sp, sn);//*.75 + .25;

        
    	// Light direction vectors.
	    vec3 ld = lightPos-sp;

        // Distance from respective lights to the surface point.
	    float lDist = max(length(ld), 0.001);
    	
    	// Normalize the light direction vectors.
	    ld /= lDist;
	    
	    // Light attenuation, based on the distances above.
	    float atten = 1./(1. + lDist*.2 + lDist*0.1); // + distlpsp*distlpsp*0.025
    	
    	// Ambient light.
	    float ambience = 0.25;
    	
    	// Diffuse lighting.
	    float diff = max( dot(sn, ld), 0.0);
   	
    	// Specular lighting.
	    float spec = pow(max( dot( reflect(-ld, sn), -rd ), 0.0 ), 32.);
 
        // Object texturing.
        vec3 tint;
        //tint = vec3(.94, .38, .57); // rose
        //tint = vec3(1, .6, 1.); // lavender
        //tint = vec3(.5); // greyscale
        //tint =  .7 + .5*cos(6.28318*(vec3(1.,0.1,0.4)*iTime*.25 + vec3(0.5,0.15,0.25))) ;
        tint = .7 + .5*cos(6.28318*(vec3(2.0,1.,0.)*iTime*.25 + vec3(.5,.2,.25)));
        vec3 texCol = tint + step(abs(snNoBump.y), .5);

    	// Combining the above terms to produce the final color. It was based more on acheiving a
        // certain aesthetic than science.
        sceneCol = texCol*(diff + ambience) + vec3(1., 1., 1.) *spec;
        

	    // Shading.  
        sceneCol *= ao;
	}
       
    // Blend in a bit of logic-defying fog for atmospheric effect. :)
    sceneCol = mix(sceneCol, vec3(.08, .16, .34), smoothstep(0., .95, t/FAR)); // exp(-.002*t*t), etc.

    // Clamp and present the badly gamma corrected pixel to the screen.
	fragColor = vec4(sqrt(clamp(sceneCol, 0., 1.)), 1.0);
	
}