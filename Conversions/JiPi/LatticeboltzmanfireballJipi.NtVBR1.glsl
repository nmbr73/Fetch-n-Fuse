

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec4 cellData = texelFetch(iChannel1, ivec2(fragCoord), 0);
    Cell cell = unpack(cellData);
    
    float mass = 0.;
    
    for(int i=0; i<9; i++) {
        mass = max(mass, cell.velocities[i]);
    }
    
    mass = max(0., mass - 30.);
   
    float r = smoothstep(0., 80., mass);
    float g = smoothstep(0., 150., mass);
    float b = smoothstep(80., 300., mass);    
    
    fragColor = vec4(r, g, b, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Stream Layer

int symdir(int dir) {
    return (dir == CENTER) ? CENTER : 1 + (dir + 3) % 8;
}

float getVelocity(ivec2 cellCoords, ivec2 neighbourRelative, int direction) {

    int width  = int(iResolution.x);
    int height = int(iResolution.y);
    
    ivec2 neighbourCoords = cellCoords + neighbourRelative;
    
    bool top_border    = neighbourCoords.y==0;
    bool bottom_border = neighbourCoords.y>=height-1;    
    bool left_border   = neighbourCoords.x==0;
    bool right_border  = neighbourCoords.x>=width-1;    

    bool border = top_border||bottom_border||left_border||right_border;

    if(!border) {
       return unpack(texelFetch(iChannel0, neighbourCoords, 0)).velocities[direction];
    }
    else {
    
        bool diagonal = direction==TOPRIGHT||direction==TOPLEFT||direction==BOTRIGHT||direction==BOTLEFT;
    
        if(!diagonal) {
            return unpack(texelFetch(iChannel0, cellCoords, 0)).velocities[symdir(direction)];
        }

        switch(direction) {
            case TOPRIGHT: 
                if(bottom_border) {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(-2, 0), 0)).velocities[BOTRIGHT];            
                }
                else {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(0, +2), 0)).velocities[TOPLEFT];            
                }

            case TOPLEFT:  
                if(bottom_border) {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(+2, 0), 0)).velocities[BOTLEFT];            
                }
                else {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(0, +2), 0)).velocities[TOPRIGHT];            
                }

            case BOTRIGHT: 
                if(top_border) {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(-2, 0), 0)).velocities[TOPRIGHT];            
                }
                else {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(0, -2), 0)).velocities[BOTLEFT];            
                }

            case BOTLEFT:  
                if(bottom_border) {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(+2, 0), 0)).velocities[TOPLEFT];            
                }
                else {
                    return unpack(texelFetch(iChannel0, cellCoords+ivec2(0, -2), 0)).velocities[BOTRIGHT];            
                }
         }
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord) {

    ivec2 cellCoords = ivec2(fragCoord);
    int width  = int(iResolution.x);
    int height = int(iResolution.y);
   
    bool top_border    = cellCoords.y==0;
    bool bottom_border = cellCoords.y>=height-1;    
    bool left_border   = cellCoords.x==0;
    bool right_border  = cellCoords.x>=width-1;    

    bool border = top_border||bottom_border||left_border||right_border;
 
    Cell cur;

    if(border) {
        for(int i=0; i<9; i++) {
            cur.velocities[i] = 0.;
        }
    }
    else {
        // stream    
        for(int i=0; i<9; i++) {
            cur.velocities[i] = getVelocity(cellCoords, ivec2(velocities[symdir(i)]), i);
        }
    }

    fragColor = pack(cur); 
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Collide Layer

float equilibrium(float w, float rho, vec2 u, vec2 c, float cs) {
    float c_dot_u = dot(c, u);
    float u_dot_u = dot(u, u);    
    
    float eq = w*rho*(1. + c_dot_u/(cs*cs) + (c_dot_u*c_dot_u)/(2.*cs*cs*cs*cs) - u_dot_u/(2.*cs*cs) );

    return eq ;//* 0.999;
}

float getMass(Cell cell) {

    float mass = 0.;

    for(int c=0; c<9; c++) {
        mass += cell.velocities[c];
    }

    return mass;
}

float getIntensity(vec2 source, vec2 uv) {
   float dist = length(source-uv)/iResolution.x;
   return max(0., 100.*(1. - dist/0.05));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord) {

    ivec2 cellCoords = ivec2(fragCoord);
    
    int width  = int(iResolution.x);
    int height = int(iResolution.y);
    
    bool border = (cellCoords.x==0) || (cellCoords.y==0) || (cellCoords.x >= width-1) || (cellCoords.y >= height-1);
    
    if(iFrame == 0) {

        Cell c;

        float intensity = border ? 0. : 100.;

        for(int i=0;i<9;i++) {
            c.velocities[i] = intensity * weight[i];
        }
        
        fragColor = pack(c);
        return;
    }
    else {

        Cell cell = unpack(texelFetch(iChannel0, cellCoords + ivec2(0,1), 0));

        // sources
        float synchro = float(iFrame) * 1./60.; 
        vec2 source1 = vec2(iResolution.x/2. * (1. + cos(synchro)*0.5), iResolution.y/2. * (1.2 + sin(2.*synchro)*0.5));
        vec2 source2 = vec2(iResolution.x/2. * (1. - cos(synchro)*0.5), iResolution.y/2. * (1.2 - sin(1.*synchro)*0.5));        

        float intensity = getIntensity(source1, fragCoord) + getIntensity(source2, fragCoord);
        if(iMouse.z>0.) intensity += getIntensity(iMouse.xy, fragCoord);

        for(int i=0; i<9; i++) cell.velocities[i] += intensity*weight[i];

        // collisions
        vec2 momentum = vec2(0.);

        float mass = getMass(cell);

        if(mass != 0.) {    
            for(int c=1; c<9; c++) {
                momentum += velocities[c] * cell.velocities[c];
            }

            momentum *= 1./mass;
        }

        //equilibrium    
        float tau = 15.;

        for(int i=0; i<9; i++) {

            float w = weight[i];
            float rho = mass;
            vec2 u = momentum;
            vec2 c = velocities[i];
            float cs = 1./1.73;    

            float eq = equilibrium(w, rho, u, c, cs);
            cell.velocities[i] = cell.velocities[i] - (1./tau)*(cell.velocities[i] - eq);
            
            cell.velocities[i] = max(8., cell.velocities[i]);
            cell.velocities[i] = min(300., cell.velocities[i]);            
            
        }                
    
        fragColor = pack(cell);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define CENTER 0
#define RIGHT 1
#define TOPRIGHT 2
#define TOP 3
#define TOPLEFT 4
#define LEFT 5
#define BOTLEFT 6
#define BOTTOM 7
#define BOTRIGHT 8

const vec2 velocities[] = vec2[] (vec2(0., 0.), vec2(1., 0.), vec2(1., 1.), vec2(0., 1.), vec2(-1., 1.), vec2(-1., 0.), vec2(-1., -1.), vec2(0., -1.), vec2(1., -1.));
const float weight[]    = float[] (4./9., 1./9., 1./36., 1./9., 1./36., 1./9., 1./36., 1./9., 1./36.); 

struct Cell {
    float velocities[9];
};

Cell unpack(vec4 rgba) {

    Cell cell;

    cell.velocities[CENTER]   = mod(rgba.a, 1000.);
    cell.velocities[RIGHT]    = floor(rgba.a*0.001);    
    cell.velocities[TOPRIGHT] = mod(rgba.r, 1000.);
    cell.velocities[TOP]      = floor(rgba.r*0.001);    
    cell.velocities[TOPLEFT]  = mod(rgba.g, 1000.);
    cell.velocities[LEFT]     = floor(rgba.g*0.001);    
    cell.velocities[BOTLEFT]  = mod(rgba.b, 500.);
    cell.velocities[BOTTOM]   = mod(floor(rgba.b/500.), 500.);
    cell.velocities[BOTRIGHT] = floor(rgba.b/(500.*500.));
    
    return cell;
}

vec4 pack(Cell cell) {
    vec4 rgba;
    
    rgba.a = floor(min(999.99, cell.velocities[CENTER]))  + floor(min(999.99, cell.velocities[RIGHT]))  * 1000.;
    rgba.r = floor(min(999.99, cell.velocities[TOPRIGHT]))+ floor(min(999.99, cell.velocities[TOP]))    * 1000.;
    rgba.g = floor(min(999.99, cell.velocities[TOPLEFT])) + floor(min(999.99, cell.velocities[LEFT]))   * 1000.;
    rgba.b = floor(min(499.99, cell.velocities[BOTLEFT])) + floor(min(499.99, cell.velocities[BOTTOM])) * 500. + floor(min(499.99, cell.velocities[BOTRIGHT]))*500.*500.;    
    
    return rgba;
}