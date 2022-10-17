
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


#define CENTER 0
#define RIGHT 1
#define TOPRIGHT 2
#define TOP 3
#define TOPLEFT 4
#define LEFT 5
#define BOTLEFT 6
#define BOTTOM 7
#define BOTRIGHT 8


struct Cell {
    float velocities[9];
};

__DEVICE__ Cell unpack(float4 rgba) {

    Cell cell;

    cell.velocities[CENTER]   = mod_f(rgba.w, 1000.0f);
    cell.velocities[RIGHT]    = _floor(rgba.w*0.001f);    
    cell.velocities[TOPRIGHT] = mod_f(rgba.x, 1000.0f);
    cell.velocities[TOP]      = _floor(rgba.x*0.001f);    
    cell.velocities[TOPLEFT]  = mod_f(rgba.y, 1000.0f);
    cell.velocities[LEFT]     = _floor(rgba.y*0.001f);    
    cell.velocities[BOTLEFT]  = mod_f(rgba.z, 500.0f);
    cell.velocities[BOTTOM]   = mod_f(_floor(rgba.z/500.0f), 500.0f);
    cell.velocities[BOTRIGHT] = _floor(rgba.z/(500.0f*500.0f));
    
    return cell;
}

__DEVICE__ float4 pack(Cell cell) {
    float4 rgba;
    
    rgba.w = _floor(_fminf(999.99f, cell.velocities[CENTER]))  + _floor(_fminf(999.99f, cell.velocities[RIGHT]))  * 1000.0f;
    rgba.x = _floor(_fminf(999.99f, cell.velocities[TOPRIGHT]))+ _floor(_fminf(999.99f, cell.velocities[TOP]))    * 1000.0f;
    rgba.y = _floor(_fminf(999.99f, cell.velocities[TOPLEFT])) + _floor(_fminf(999.99f, cell.velocities[LEFT]))   * 1000.0f;
    rgba.z = _floor(_fminf(499.99f, cell.velocities[BOTLEFT])) + _floor(_fminf(499.99f, cell.velocities[BOTTOM])) * 500.0f + _floor(_fminf(499.99f, cell.velocities[BOTRIGHT]))*500.0f*500.0f;    
    
    return rgba;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


// Stream Layer

__DEVICE__ int symdir(int dir) {
    return (dir == CENTER) ? CENTER : 1 + (dir + 3) % 8;
}

__DEVICE__ float getVelocity(int2 cellCoords, int2 neighbourRelative, int direction, float2 iResolution, __TEXTURE2D__ iChannel0) {

    int width  = int(iResolution.x);
    int height = int(iResolution.y);
    
    int2 neighbourCoords = cellCoords + neighbourRelative;
    
    bool top_border    = neighbourCoords.y==0;
    bool bottom_border = neighbourCoords.y>=height-1;    
    bool left_border   = neighbourCoords.x==0;
    bool right_border  = neighbourCoords.x>=width-1;    
float zzzzzzzzzzzzzzzzzzz;
    bool border = top_border||bottom_border||left_border||right_border;

    if(!border) {
       //return unpack(texelFetch(iChannel0, neighbourCoords, 0)).velocities[direction];
       return unpack(texture(iChannel0, (make_float2(neighbourCoords)+0.5f)/iResolution)).velocities[direction];
    }
    else {
    
        bool diagonal = direction==TOPRIGHT||direction==TOPLEFT||direction==BOTRIGHT||direction==BOTLEFT;
    
        if(!diagonal) {
            //return unpack(texelFetch(iChannel0, cellCoords, 0)).velocities[symdir(direction)];
            return unpack(texture(iChannel0, (make_float2(cellCoords)+0.5f)/iResolution)).velocities[symdir(direction)];
        }

        switch(direction) {
            case TOPRIGHT: 
                if(bottom_border) {
                    //return unpack(texelFetch(iChannel0, cellCoords+to_int2(-2, 0), 0)).velocities[BOTRIGHT];            
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(-2, 0))+0.5f)/iResolution)).velocities[BOTRIGHT];            
                }
                else {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(0, +2))+0.5f)/iResolution)).velocities[TOPLEFT];            
                }

            case TOPLEFT:  
                if(bottom_border) {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(+2, 0))+0.5f)/iResolution)).velocities[BOTLEFT];            
                }
                else {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(0, +2))+0.5f)/iResolution)).velocities[TOPRIGHT];            
                }

            case BOTRIGHT: 
                if(top_border) {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(-2, 0))+0.5f)/iResolution)).velocities[TOPRIGHT];            
                }
                else {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(0, -2))+0.5f)/iResolution)).velocities[BOTLEFT];            
                }

            case BOTLEFT:  
                if(bottom_border) {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(+2, 0))+0.5f)/iResolution)).velocities[TOPLEFT];            
                }
                else {
                    return unpack(texture(iChannel0, (make_float2(cellCoords+to_int2(0, -2))+0.5f)/iResolution)).velocities[BOTRIGHT];            
                }
         }
    }
}

__KERNEL__ void LatticeboltzmanfireballJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    const float2 velocities[] = { to_float2(0.0f, 0.0f), to_float2(1.0f, 0.0f), to_float2(1.0f, 1.0f), to_float2(0.0f, 1.0f), to_float2(-1.0f, 1.0f), to_float2(-1.0f, 0.0f), to_float2(-1.0f, -1.0f), to_float2(0.0f, -1.0f), to_float2(1.0f, -1.0f)};
    const float weight[]      = { 4.0f/9.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f}; 



    int2 cellCoords = to_int2_cfloat(fragCoord);
    int width  = (int)(iResolution.x);
    int height = (int)(iResolution.y);
   
    bool top_border    = cellCoords.y==0;
    bool bottom_border = cellCoords.y>=height-1;    
    bool left_border   = cellCoords.x==0;
    bool right_border  = cellCoords.x>=width-1;    

    bool border = top_border||bottom_border||left_border||right_border;
 
    Cell cur;
float AAAAAAAAAAAAAAAAAA;
    if(border) {
        for(int i=0; i<9; i++) {
            cur.velocities[i] = 0.0f;
        }
    }
    else {
        // stream    
        for(int i=0; i<9; i++) {
            cur.velocities[i] = getVelocity(cellCoords, to_int2_cfloat(velocities[symdir(i)]), i, iResolution, iChannel0);
        }
    }
    fragColor = pack(cur); 

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Collide Layer

__DEVICE__ float equilibrium(float w, float rho, float2 u, float2 c, float cs) {
    float c_dot_u = dot(c, u);
    float u_dot_u = dot(u, u);    
    
    float eq = w*rho*(1.0f + c_dot_u/(cs*cs) + (c_dot_u*c_dot_u)/(2.0f*cs*cs*cs*cs) - u_dot_u/(2.0f*cs*cs) );

    return eq ;//* 0.999f;
}

__DEVICE__ float getMass(Cell cell) {

    float mass = 0.0f;
float bbbbbbbbbbbbbbb;
    for(int c=0; c<9; c++) {
        mass += cell.velocities[c];
    }

    return mass;
}

__DEVICE__ float getIntensity(float2 source, float2 uv, float2 iResolution) {
   float dist = length(source-uv)/iResolution.x;
   return _fmaxf(0.0f, 100.0f*(1.0f - dist/0.05f));
}

__KERNEL__ void LatticeboltzmanfireballJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;

    const float2 velocities[] = { to_float2(0.0f, 0.0f), to_float2(1.0f, 0.0f), to_float2(1.0f, 1.0f), to_float2(0.0f, 1.0f), to_float2(-1.0f, 1.0f), to_float2(-1.0f, 0.0f), to_float2(-1.0f, -1.0f), to_float2(0.0f, -1.0f), to_float2(1.0f, -1.0f)};
    const float weight[]      = { 4.0f/9.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f}; 


    int2 cellCoords = to_int2_cfloat(fragCoord);
    
    int width  = (int)(iResolution.x);
    int height = (int)(iResolution.y);
    
    bool border = (cellCoords.x==0) || (cellCoords.y==0) || (cellCoords.x >= width-1) || (cellCoords.y >= height-1);
    
    if(iFrame == 0 || Reset) {
        Cell c;

        float intensity = border ? 0.0f : 100.0f;

        for(int i=0;i<9;i++) {
            c.velocities[i] = intensity * weight[i];
        }
        
        fragColor = pack(c);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    else {
float BBBBBBBBBB;
        Cell cell = unpack(texture(iChannel0, (make_float2(cellCoords + to_int2(0,1))+0.5f)/iResolution));

        // sources
        float synchro = (float)(iFrame) * 1.0f/60.0f; 
        float2 source1 = to_float2(iResolution.x/2.0f * (1.0f + _cosf(synchro)*0.5f), iResolution.y/2.0f * (1.2f + _sinf(2.0f*synchro)*0.5f));
        float2 source2 = to_float2(iResolution.x/2.0f * (1.0f - _cosf(synchro)*0.5f), iResolution.y/2.0f * (1.2f - _sinf(1.0f*synchro)*0.5f));        

        float intensity = getIntensity(source1, fragCoord,iResolution) + getIntensity(source2, fragCoord,iResolution);
        if(iMouse.z>0.0f) intensity += getIntensity(swi2(iMouse,x,y), fragCoord,iResolution);

        for(int i=0; i<9; i++) cell.velocities[i] += intensity*weight[i];

        // collisions
        float2 momentum = to_float2_s(0.0f);

        float mass = getMass(cell);

        if(mass != 0.0f) {    
            for(int c=1; c<9; c++) {
                momentum += velocities[c] * cell.velocities[c];
            }
            momentum *= 1.0f/mass;
        }

        //equilibrium    
        float tau = 15.0f;

        for(int i=0; i<9; i++) {

            float w = weight[i];
            float rho = mass;
            float2 u = momentum;
            float2 c = velocities[i];
            float cs = 1.0f/1.73f;    

            float eq = equilibrium(w, rho, u, c, cs);
            cell.velocities[i] = cell.velocities[i] - (1.0f/tau)*(cell.velocities[i] - eq);
            
            cell.velocities[i] = _fmaxf(8.0f, cell.velocities[i]);
            cell.velocities[i] = _fminf(300.0f, cell.velocities[i]);            
            
        }                
        fragColor = pack(cell);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void LatticeboltzmanfireballJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);
  
    fragCoord+=0.5f;

    float4 cellData = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
    Cell cell = unpack(cellData);
    
    float mass = 0.0f;
    
    for(int i=0; i<9; i++) {
        mass = _fmaxf(mass, cell.velocities[i]);
    }
float IIIIIIIIIIIIIIIIIIIII;    
    mass = _fmaxf(0.0f, mass - 30.0f);
   
    float r = smoothstep(0.0f, 80.0f, mass)   + Color.x - 0.5;
    float g = smoothstep(0.0f, 150.0f, mass)  + Color.y - 0.5;
    float b = smoothstep(80.0f, 300.0f, mass) + Color.z - 0.5;    
    
    fragColor = to_float4(r, g, b, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}
