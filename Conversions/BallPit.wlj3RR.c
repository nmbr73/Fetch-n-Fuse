#define texelFetch_i2(C,XY,M) _tex2DVecN((C),(XY).x,(XY).y,(M))

// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


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
#define particleEdge to_int2_f2(_floor(iResolution/diameter))

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


__DEVICE__ float2 noise2D(float2 uv) {
    return fract(3e4*_sinf((uv)*mat2(1,13.51f,73.37f,-57.17f)));
}

int2 hash2D(float2 uv) {
    return to_int2_f2(_floor(mod_f2(3e4*_sinf((uv)*mat2(1,13.51f,73.37f,-57.17f)),float(hashEdge))));
}

__DEVICE__ float noise1D(float t) {
    return fract(14950.5f*_sinf(1905.1f*t));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Preset: Keyboard' to iChannel3
// Connect 'Previsualization: Buffer B' to iChannel0


//grid hashing

__KERNEL__ void BallPitFuse__Buffer_A(float4 state, float2 fragCoord, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel3,
  float2 iResolution)
{

//  int2 maxIndex = particleEdge * hashEdge;
    int2 ind = to_int2_f2(_floor(fragCoord));

//  if (ind.x >= maxIndex.x || ind.y >= maxIndex.y) discard;  ?!?!?!!!
    state = texelFetch_i2(iChannel0, ind, 0);
    int2 id = ind/hashEdge; //cell id
    if (iMouse.z > 0.0f) {
        float mousedist = length(swi2(iMouse,x,y)*float(hashEdge)/diameter-fragCoord);
        float keystate = texelFetch_i2(iChannel3, to_int2_2f(69.0f,0.0f),0).x;
        if (keystate > 0.5f && mousedist < float(hashEdge*16)) {
            state = to_float4(-1.0f,-1.0f,0.0f,0.0f);
//      } else if ( ind % hashEdge == to_int2_s(0) && mousedist < float(hashEdge*drawRadius)) {
        } else if ( ind.x % hashEdge ==0 && ind.y % hashEdge ==0 && mousedist < float(hashEdge*drawRadius)) {
          state = to_float4_f2f2(to_float2_i2(id)*diameter+diameter*0.5f, (noise2D(fragCoord)*2.0f-1.0f)*speed);
        }
    }
    if (iFrame == 0) {
        //initialization
//      if (ind % (hashEdge) == to_int2_s(0)) {
        if (ind.x % hashEdge ==0 && ind.y % hashEdge ==0) {
            state = to_float4_f2f2(to_float2_i2(id)*diameter+diameter*0.5f, (noise2D(fragCoord)*2.0f-1.0f)*speed);
        } else {
      state = to_float4(-1.0f, -1.0f, 0.0f, 0.0f);
        }
    } else {
        //update

        //particle leaving the cell
//      if (to_int2_f2(_floor(state.xy/diameter)) != id) state = to_float4(-1.0f, -1.0f, 0.0f, 0.0f);
        int2 dmy=to_int2_f2(_floor(state.xy/diameter));
        if (dmy.x!=id.x || dmy.y!=id.y) state = to_float4(-1.0f, -1.0f, 0.0f, 0.0f);

        //particle entering the cell
        for (int i=-1; i<=1; i++) {
            for (int j=-1; j<=1; j++) {
                int2 disp = to_int2(i, j);
//              if (disp == to_int2_s(0)) continue;
                if (disp.x == 0 && disp.y==0) continue;
                int2 otherid = id + disp;
                if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
                //check every bin inside the other cell for particles entering this cell
                for (int k=0; k<hashEdge; k++) {
                    for (int l=0; l<hashEdge; l++) {
                        int2 offset = to_int2(k, l);
                        float4 otherstate = texelFetch_i2(iChannel0, otherid*hashEdge + offset, 0);
                        int2 id2 = to_int2_f2(_floor(otherstate.xy/diameter));
//                      if (id2 == id) {
                        if (id2.x == id.x && id.y == id.y) {
                            for (int h=0; h<numHashes; h++) {
                                //receive the particle if this is the right bin
                                int2 hashOffset = hash2D(swi2(otherstate,x,y)+float(h)*12345.0f);
                                int2 hashInd = id*hashEdge+hashOffset;
                                float2 state0 = texelFetch_i2(iChannel0, hashInd, 0).xy;
//                              if (state0 == to_float2_s(-1.0f)) {
                                if (state0.x == -1.0f && state0.y == -1.0f) {
//                                  if (hashInd == ind) {
                                    if (hashInd.x == ind.x && hashInd.y == ind.y) {
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


  SetFragmentShaderComputedColor(state);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


//state update

//x,y = position
//z,w = velocity
//x,y are negative if unoccupied
#define hardstep(h) (step(0.0f, h)*2.0f-1.0f)


__KERNEL__ void BallPitFuse__Buffer_B(float4 state, float2 fragCoord, sampler2D iChannel0,
  float2 iResolution)
{

//  int2 maxIndex = particleEdge * hashEdge;
    int2 ind = to_int2_f2(_floor(fragCoord));
    int2 id = ind/hashEdge; //cell id

    //if (ind.x >= maxIndex.x || ind.y >= maxIndex.y) discard; ?!?!?!!!


    state = texelFetch_i2(iChannel0, ind, 0);

//  if (swi2(state,x,y) != to_float2_s(-1.0f)) {
    if ( state.x != -1.0f || state.y != -1.0f) {

        //collision detection
        float2 impulse = to_float2_s(0.0f);
        //vec2 push = to_float2_s(0.0f);
        //int collisions = 0;
        for (int i=-2; i<=2; i++) {
            for (int j=-2; j<=2; j++) {
                int2 disp = to_int2(i, j);
//              if (eq_i2_1i(disp,0)) continue;
                if (disp.x==0 && disp.y==0) continue;
                int2 otherid = id + disp;
                if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
                //check every bin inside the other cell for particles entering this cell
                for (int k=0; k<hashEdge; k++) {
                    for (int l=0; l<hashEdge; l++) {
                        int2 offset = to_int2(k, l);
                        float4 otherstate = texelFetch_i2(iChannel0, otherid*hashEdge + offset, 0);
                        if (otherstate.x == -1.0f && otherstate.y == -1.0f) continue;
                        float2 r = swi2(state,x,y)-swi2(otherstate,x,y);
                        //center of mass frame
                        float2 v_cm = (swi2(state,z,w)+swi2(otherstate,z,w))/2.0f;
                        float2 v0 = swi2(state,z,w)-v_cm;
                        #ifdef gooey
                        float rn = length(r);
                        float vproj = dot(v0, r)/rn;
                        float r2 = 2.0f*diameter-rn;
                        float force = _mix(0.3f*r2-0.1f*vproj, -2.0f/(rn*rn), step(0.0f,-r2));
                        impulse += r/rn*force;
                        #else
                        float vproj = dot(v0, r)/dot(r, r);
                        if (length(r) < diameter*2.0f) {
                            //collisions++;
                            //move the particles apart
                            //push += r/20.0f;

                            //compute collision impulse
                            impulse -= (1.0f+restitution)*_fminf(0.0f,vproj)*r;
                        }
                        #endif
                    }
                }
            }
        }
        swi2(state,z,w) += impulse;
        //swi2(state,x,y) += push;
        //if (collisions < 1)
            state.w -= gravity;

        swi2(state,x,y) += swi2(state,z,w);
        //bounce off walls
        float2 bounds = diameter*to_float2_i2(particleEdge)-1.0f;
//      if (swi2(state,x,y) != to_float2(-1.0f)) {
        if (state.x != -1.0f || state.y != -1.0f) {
            float2 sgn = hardstep(swi2(state,x,y))*hardstep(bounds-swi2(state,x,y));
            swi2(state,z,w) *= sgn;
            swi2(state,z,w) *= _mix(restitution, 1.0f, step(0.0f, sgn.x*sgn.y));
            swi2(state,x,y) = _fminf(swi2(state,x,y), bounds);
            swi2(state,x,y) = _fmaxf(swi2(state,x,y), 0.0f);
        }
        float currspeed = length(swi2(state,z,w));
        swi2(state,z,w) = state.zw/currspeed*_fminf(currspeed,maxspeed);
    }


  SetFragmentShaderComputedColor(state);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BallPitFuse(float4 fragColor, float2 fragCoord, sampler2D iChannel0,
  float2 iResolution)
{

    float2 uv = fragCoord/diameter;
    int2 id = to_int2_f2(_floor(uv));
    //ivec2 stripes = id % 2;
    //fragColor = to_float4_aw(to_float3(0.1f*float(_fabs(stripes.x-stripes.y))), 1.0f);
    fragColor = to_float4(0.0f,0.0f,0.0f,1.0f);
//  float3 bgcol = to_float3_s(0.0f);
    float totalW = 0.01f;
    for (int i=-2; i<=2; i++) {
        for (int j=-2; j<=2; j++) {
            int2 disp = to_int2(i, j);
            int2 otherid = id+disp;
            if (otherid.x < 0 || otherid.y < 0 || otherid.x >= particleEdge.x || otherid.y >= particleEdge.y) continue;
            for (int k=0; k<hashEdge; k++) {
                for (int l=0; l<hashEdge; l++) {
                    int2 offset = to_int2(k, l);
                    float4 state = texelFetch_i2(iChannel0, otherid*hashEdge+offset,0);
//                  if (swi2(state,x,y) == to_float2(-1.0f)) continue;
                    if (state.x==-1.0f && state.y==-1.0f) continue;
                    float dist = length(swi2(state,x,y)-fragCoord);
                    float W = smoothstep(diameter*2.0f,0.0f,dist);
                    totalW += W;
                    //swi3(fragColor,x,y,z) -= to_float3(0.5f, swi2(state,z,w)+0.5f)*smoothstep(diameter+1.0f, diameter, dist);
                    //swi3(fragColor,x,y,z) = _fabs(swi3(fragColor,x,y,z));
//                  swi3(fragColor,x,y,z) += W*to_float3(0.5f, swi2(state,z,w)+0.5f);
                    swi3(fragColor,x,y,z) += W*to_float3(0.5f, state.z+0.5f, state.w+0.5f);
                }
            }
        }
    }
    swi3(fragColor,x,y,z) /= totalW;


  SetFragmentShaderComputedColor(fragColor);
}