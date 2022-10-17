

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/diameter;
    ivec2 id = ivec2(floor(uv));
    //ivec2 stripes = id % 2;
    //fragColor = vec4(vec3(0.1*float(abs(stripes.x-stripes.y))), 1.);
    fragColor = vec4(0.,0.,0.,1.);
    vec3 bgcol = vec3(0.);
    float totalW = 0.01;
    for (int i=-2; i<=2; i++) {
        for (int j=-2; j<=2; j++) {
            ivec2 disp = ivec2(i, j);
            ivec2 otherid = id+disp;
            if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
            for (int k=0; k<hashEdge; k++) {
                for (int l=0; l<hashEdge; l++) {
                    ivec2 offset = ivec2(k, l);
                    vec4 state = texelFetch(iChannel0, otherid*hashEdge+offset,0);
                    if (state.xy == vec2(-1.)) continue;
                    float dist = length(state.xy-fragCoord);
                    float W = smoothstep(diameter*2.,0.,dist);
                    totalW += W;
                    //fragColor.xyz -= vec3(0.5, state.zw+.5)*smoothstep(diameter+1., diameter, dist);
                    //fragColor.xyz = abs(fragColor.xyz);
                    fragColor.xyz += W*vec3(0.5, state.zw+.5);
                }
            }
        }
    }
    fragColor.xyz /= totalW;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//to turn on blob physics instead of hard collisions:
//#define gooey

//number of bins per cell is hashEdge * hashEdge
//cannot be greater than diameter
#define hashEdge 2

//real world scale of the cells (and particles)
#define diameter 2.

//maximum number of times to try binning a particle
#define numHashes 2

//number of particles is particleEdge.x * particleEdge.y
#define particleEdge ivec2(floor(iResolution.xy/diameter))

//starting speed
#define speed 1.
//maxspeed must be smaller than diameter
#define maxspeed 1.9
#define restitution 0.75

#ifdef gooey
#define gravity 0.01
#define drawRadius 42
#else
#define gravity 0.0
#define drawRadius 2
#endif


vec2 noise2D(vec2 uv) {
    return fract(3e4*sin((uv)*mat2(1,13.51,73.37,-57.17)));
}

ivec2 hash2D(vec2 uv) {
    return ivec2(floor(clamp(mod(3e4*sin((uv)*mat2(1,13.51,73.37,-57.17)), float(hashEdge)), 0., float(hashEdge))));
}


float noise1D(float t) {
    return fract(14950.5*sin(1905.1*t));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//grid hashing

void mainImage( out vec4 state, in vec2 fragCoord )
{
    ivec2 maxIndex = particleEdge * hashEdge;
    ivec2 ind = ivec2(floor(fragCoord.xy));
    
    if (ind.x >= maxIndex.x || ind.y >= maxIndex.y) discard;
    state = texelFetch(iChannel0, ind, 0);
    ivec2 id = ind/hashEdge; //cell id
    if (iMouse.z > 0.) {
        float mousedist = length(iMouse.xy*float(hashEdge)/diameter-fragCoord);
        float keystate = texelFetch(iChannel3, ivec2(69.,0),0).x;
        if (keystate > 0.5 && mousedist < float(hashEdge*16)) {
            state = vec4(-1.,-1.,0.,0.);
        } else if (ind % hashEdge == ivec2(0) && mousedist < float(hashEdge*drawRadius)) {
        	state = vec4(vec2(id)*diameter+diameter*.5, (noise2D(fragCoord)*2.-1.)*speed);
        }
    }
    if (iFrame == 0) {
        //initialization
        if (ind % (hashEdge) == ivec2(0)) {
            state = vec4(vec2(id)*diameter+diameter*.5, (noise2D(fragCoord)*2.-1.)*speed);
        } else {
			state = vec4(-1., -1., 0., 0.);
        }
    } else {
        //update
        
        //particle leaving the cell
        if (ivec2(floor(state.xy/diameter)) != id) state = vec4(-1., -1., 0., 0.);
        
        //particle entering the cell
        for (int i=-1; i<=1; i++) {
            for (int j=-1; j<=1; j++) {
                ivec2 disp = ivec2(i, j);
                if (disp == ivec2(0)) continue;
                ivec2 otherid = id + disp;
                if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
                //check every bin inside the other cell for particles entering this cell
                for (int k=0; k<hashEdge; k++) {
                    for (int l=0; l<hashEdge; l++) {
                        ivec2 offset = ivec2(k, l);
                        vec4 otherstate = texelFetch(iChannel0, otherid*hashEdge + offset, 0);
                        ivec2 id2 = ivec2(floor(otherstate.xy/diameter));
                        if (id2 == id) {
                            for (int h=0; h<numHashes; h++) {
                                //receive the particle if this is the right bin
                                ivec2 hashOffset = hash2D(otherstate.xy+float(h)*12345.);
                                ivec2 hashInd = id*hashEdge+hashOffset;
                                vec2 state0 = texelFetch(iChannel0, hashInd, 0).xy;
                                if (state0 == vec2(-1.)) {
                                    if (hashInd == ind) {
                                        state = otherstate;
                                        return;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//state update

//x,y = position
//z,w = velocity
//x,y are negative if unoccupied
#define hardstep(h) (step(0., h)*2.-1.)

void mainImage( out vec4 state, in vec2 fragCoord )
{
    ivec2 maxIndex = particleEdge * hashEdge;
    ivec2 ind = ivec2(floor(fragCoord.xy));
    ivec2 id = ind/hashEdge; //cell id

    if (ind.x >= maxIndex.x || ind.y >= maxIndex.y) discard;
    
    state = texelFetch(iChannel0, ind, 0);
    
    if (state.xy != vec2(-1.)) {

        //collision detection
        vec2 impulse = vec2(0.);
        //vec2 push = vec2(0.);
        //int collisions = 0;
        for (int i=-2; i<=2; i++) {
            for (int j=-2; j<=2; j++) {
                ivec2 disp = ivec2(i, j);
                if (disp == ivec2(0)) continue;
                ivec2 otherid = id + disp;
                if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
                //check every bin inside the other cell for particles entering this cell
                for (int k=0; k<hashEdge; k++) {
                    for (int l=0; l<hashEdge; l++) {
                        ivec2 offset = ivec2(k, l);
                        vec4 otherstate = texelFetch(iChannel0, otherid*hashEdge + offset, 0);
                        if (otherstate.xy == vec2(-1.)) continue;
                        vec2 r = state.xy-otherstate.xy;
                        //center of mass frame
                        vec2 v_cm = (state.zw+otherstate.zw)/2.;
                        vec2 v0 = state.zw-v_cm;
                        #ifdef gooey
                        float rn = length(r);
                        float vproj = dot(v0, r)/rn;
                        float r2 = 2.*diameter-rn;
                        float force = mix(0.3*r2-0.1*vproj, -2./(rn*rn), step(0.,-r2));
                        impulse += r/rn*force;
                        #else
                        float vproj = dot(v0, r)/dot(r, r);
                        if (length(r) < diameter*2.) {
                            //collisions++;
                            //move the particles apart
                            //push += r/20.;
                            
                            //compute collision impulse
                            impulse -= (1.+restitution)*min(0.,vproj)*r;
                        }
                        #endif
                    }
                }
            }
        }
        state.zw += impulse;
        //state.xy += push;
        //if (collisions < 1)
            state.w -= gravity;

        state.xy += state.zw;
        //bounce off walls
        vec2 bounds = diameter*vec2(particleEdge)-1.;
        if (state.xy != vec2(-1.)) {
            vec2 sgn = hardstep(state.xy)*hardstep(bounds-state.xy);
            state.zw *= sgn;
            state.zw *= mix(restitution, 1., step(0., sgn.x*sgn.y));
            state.xy = min(state.xy, bounds);
            state.xy = max(state.xy, 0.);
        }
        float currspeed = length(state.zw);
        state.zw = state.zw/currspeed*min(currspeed,maxspeed);
    }
}