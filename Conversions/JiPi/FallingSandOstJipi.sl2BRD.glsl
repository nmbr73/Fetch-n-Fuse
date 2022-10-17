

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_B_SAMPLER iChannel1
#define BUFFER_D_SAMPLER iChannel2

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// Output image

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}  

float getSandID(vec2 uv) {
    float value = texture(BUFFER_D_SAMPLER, uv).x;
    return mod(floor(value), NUM_TILE_IDS);
}

vec4 getSandColor(float sandID) {
    float pixelIndex = float(sandID+COLOR_BEGIN)/iResolution.x;
    vec2 uv = vec2(mod(pixelIndex, 1.0), pixelIndex/iResolution.y) + vec2(0.5)/iResolution.xy;
    vec4 color = texture(BUFFER_A_SAMPLER, uv);
    return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    float id = getSandID(uv);
    vec4 color = getSandColor(id); 
    
	fragColor = color;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define BUFFER_A_SAMPLER iChannel0

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// global data 

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    float index = fragCoord.x-0.5 + (fragCoord.y-0.5)*iResolution.y;
    
    if (index >= DATA_BEGIN && index < DATA_END) {
        //Default value:
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        
        if (index >= DENSITY_BEGIN && index < DENSITY_END) {
            float id = index - DENSITY_BEGIN;
            if (id == 0.0) //Air
                fragColor = vec4(0.0);
            else if (id == 1.0) // Sand
                fragColor = vec4(1.5);
            else if (id == 2.0) // Stone
                fragColor = vec4(3.0);
            else if (id == 3.0) // Oil
                fragColor = vec4(0.89);
            else if (id == 4.0) // Steam
                fragColor = vec4(-0.1);
            else if (id == 5.0) // Magma
                fragColor = vec4(2.5);
            else if (id == 6.0) // Water
                fragColor = vec4(1.0);
            else if (id == 7.0) // Fire
                fragColor = vec4(-1.0);
            else if (id == 8.0) // Glass
                fragColor = vec4(1.5);
            else if (id == 9.0) // Mud
                fragColor = vec4(1.4);
        }
        else if (index >= COLOR_BEGIN && index < COLOR_END) {
            float id = index - COLOR_BEGIN;
            if (id == 0.0) // Air
                fragColor = vec4(0.0);
            else if (id == 1.0) // Sand
                fragColor = vec4(0.8, 0.6, 0.2, 1.0);
            else if (id == 2.0) // Stone
                fragColor = vec4(0.6, 0.6, 0.6, 1.0);
            else if (id == 3.0) // Oil
                fragColor = vec4(0.5, 0.3, 0.1, 1.0);
            else if (id == 4.0) // Steam
                fragColor = vec4(0.9, 0.9, 0.9, 1.0);
            else if (id == 5.0) // Magma
                fragColor = vec4(0.8, 0.1, 0.0, 1.0);
            else if (id == 6.0) // Water
                fragColor = vec4(0.1, 0.2, 0.8, 1.0);
            else if (id == 7.0) // Fire
                fragColor = vec4(1.0, 0.5, 0.0, 1.0);
            else if (id == 8.0) // Glass
                fragColor = vec4(0.8, 0.8, 0.8, 1.0);
            else if (id == 9.0) // Mud
                fragColor = vec4(0.4, 0.2, 0.0, 1.0);
        }
        
        return;
    }
    else if (index == SELECTED_ELEMENT) {
        if (iFrame == 0) {
            fragColor = vec4(6.0);
            return;
        }
        
        
        if (texture(iChannel1, vec2(48.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(0.0);
        else if (texture(iChannel1, vec2(49.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(1.0);
        else if (texture(iChannel1, vec2(50.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(2.0);
        else if (texture(iChannel1, vec2(51.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(3.0);
        else if (texture(iChannel1, vec2(52.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(4.0);
        else if (texture(iChannel1, vec2(53.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(5.0);
        else if (texture(iChannel1, vec2(54.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(6.0);
        else if (texture(iChannel1, vec2(55.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(7.0);
        else if (texture(iChannel1, vec2(56.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(8.0);
        else if (texture(iChannel1, vec2(57.5/256.0, 0.0)).x > 0.0)
            fragColor = vec4(9.0);
        else
            fragColor = texture(BUFFER_A_SAMPLER, uv);
        return;
    }
    
    
    fragColor = vec4(uv.x*32.0, uv.y*32.0, 0.5,1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_D_SAMPLER iChannel1

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// Movement calculations.

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}  

float getDensity(float id) {
    float index = id + DENSITY_BEGIN;
    vec2 uv = vec2(mod(index/iResolution.x , 1.0), floor(index/iResolution.x)/iResolution.y) + 0.5/iResolution.xy;
    float density = texture(BUFFER_A_SAMPLER, uv).x;
    return density;
}

mat3 readMatID(vec2 fragCoord) {
    mat3 matID = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
            matID[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,-1))/iResolution.xy).x;
            matID[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,0))/iResolution.xy).x;
            matID[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,+1))/iResolution.xy).x;
    }
    return matID;
}

mat3 readMatDensity(vec2 fragCoord, mat3 matID) {
    mat3 matDensity = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
        matDensity[x][0] = getDensity(matID[x][0]);
        matDensity[x][1] = getDensity(matID[x][1]);
        matDensity[x][2] = getDensity(matID[x][2]);
    }
    return matDensity;
    
}

vec2 calcMove(mat3 matID, mat3 matDensity, vec2 fragCoord) {
    int hash = int(1024.0*rand(vec2(rand(fragCoord), iTime)));
    int hash_3 = int(mod(float(hash), 3.0)+0.5);
    int hash_2 = int(mod(float(hash), 4.0)+0.5);
    float density = matDensity[1][1];
    
    //if (density <= 0.0 && hash_2 != 1)
    //    return vec2(0.0);
    
    if (matDensity[1][0] < density-1.25)
        return vec2(0, -1);
    
    if (matDensity[0][2] < density-1.25)
        return vec2(-1, -1);
    
    if (matDensity[2][2] < density-1.25)
        return vec2(1, -1);
    
    
    if (hash_3 == 0 && matDensity[0][0] < density)
        return vec2(-1, -1);
    
    if (hash_3 == 2 && matDensity[2][0] < density)
        return vec2(+1, -1);
    
    if (matDensity[1][1] < density)
        return vec2(0, -1);
        
    if (hash_3 == 0)
        return vec2(+1, 0);
    
    if (hash_3 == 2)
        return vec2(-1, 0);
    
    /*if (hash_9 == 3)
        return vec2(-1, +0);
    
    if (hash_9 == 4)
        return vec2(+0, +0);
    
    if (hash_9 == 5)
        return vec2(+1, +0);
    
    if (hash_9 == 6)
        return vec2(-1, +1);
    
    if (hash_9 == 7)
        return vec2(+0, +1);
    
    if (hash_9 == 8)
        return vec2(+1, +1);*/
    
    
    return vec2(0,-1);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    mat3 matID = readMatID(fragCoord);
    mat3 matDensity = readMatDensity(fragCoord, matID);
    //float[3][3] sand = {{vec4(0.0,vec4(0.0,vec4(0.0},{vec4(0.0,vec4(0.0,vec4(0.0},{vec4(0.0,vec4(0.0,vec4(0.0}};
    
    vec2 move = calcMove(matID, matDensity, fragCoord);
    
    fragColor = vec4(move.x, move.y, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_B_SAMPLER iChannel1
#define BUFFER_D_SAMPLER iChannel2

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// Pixel movements are paired.

float getDensity(float id) {
    float index = id + DENSITY_BEGIN;
    vec2 uv = vec2(mod(index/iResolution.x , 1.0), floor(index/iResolution.x)/iResolution.y) + 0.5/iResolution.xy;
    float density = texture(BUFFER_A_SAMPLER, uv).x;
    return density;
}

mat3 readMatID(vec2 fragCoord) {
    mat3 matID = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
        matID[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,0-1))/iResolution.xy).x;
        matID[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,1-1))/iResolution.xy).x;
        matID[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,2-1))/iResolution.xy).x;
    }
    return matID;
}

mat3 readMatDensity(vec2 fragCoord, mat3 matID) {
    mat3 matDensity = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
        matDensity[x][0] = getDensity(matID[x][0]);
        matDensity[x][1] = getDensity(matID[x][1]);
        matDensity[x][2] = getDensity(matID[x][2]);
    }
    return matDensity;
    
}

void readMatMove(out mat3 matMoveX, out mat3 matMoveY, vec2 fragCoord) {
    mat3 matID = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
        vec2 v0 = texture(BUFFER_B_SAMPLER, (fragCoord+vec2(x-1,0-1))/iResolution.xy).xy;
        vec2 v1 = texture(BUFFER_B_SAMPLER, (fragCoord+vec2(x-1,1-1))/iResolution.xy).xy;
        vec2 v2 = texture(BUFFER_B_SAMPLER, (fragCoord+vec2(x-1,2-1))/iResolution.xy).xy;
        matMoveX[x][0] = v0.x;
        matMoveY[x][0] = v0.y;
        matMoveX[x][1] = v1.x;
        matMoveY[x][1] = v1.y;
        matMoveX[x][2] = v2.x;
        matMoveY[x][2] = v2.y;
    }
}


vec2 calcMovePaired(const mat3 matID, const mat3 matDensity, const mat3 matMoveX, const mat3 matMoveY, const vec2 fragCoord) {
    float density = matDensity[1][1];
     vec2 move = vec2(matMoveX[1][1], matMoveY[1][1]);
     int ox = int(1.0+move.x);
     int oy = int(1.0+move.y);
    
    
    if (matMoveY[1][2] == -1.0 && matMoveX[1][2] == 0.0 && matDensity[1][2] > density)
        return vec2(0.0, 1.0);
    
    if (matMoveY[0][2] == -1.0 && matMoveX[0][2] == 1.0 && matDensity[0][2] > density)
        return vec2(-1.0, 1.0);
    
    if (matMoveY[2][2] == -1.0 && matMoveX[2][2] == -1.0 && matDensity[2][2] > density)
        return vec2(1.0, 1.0);
    
    
    if (matMoveY[2][1] == 0.0 && matMoveX[2][1] == -1.0)
        return vec2(1.0, 0.0);
    
    if (matMoveY[0][1] == 0.0 && matMoveX[0][1] == 1.0)
        return vec2(-1.0, 0.0);
    
   
    /*if ((matDensity[0][0] <= density && ox == 0 && oy == 0 && matMoveX[0][0] == 1.0 && matMoveY[0][0] == 1.0) ||
        (matDensity[1][0] <= density && ox == 1 && oy == 0 && matMoveX[1][0] == 0.0 && matMoveY[1][0] == 1.0) ||
        (matDensity[2][0] <= density && ox == 2 && oy == 0 && matMoveX[2][0] == -1.0 && matMoveY[2][0] == 1.0) ||
        (matDensity[0][1] <= density && ox == 0 && oy == 1 && matMoveX[0][1] == 1.0 && matMoveY[0][1] == 0.0) ||
        (matDensity[1][1] <= density && ox == 1 && oy == 1 && matMoveX[1][1] == 0.0 && matMoveY[1][1] == 0.0) ||
        (matDensity[2][1] <= density && ox == 2 && oy == 1 && matMoveX[2][1] == -1.0 && matMoveY[2][1] == 0.0) ||
        (matDensity[0][2] <= density && ox == 0 && oy == 2 && matMoveX[0][2] == 1.0 && matMoveY[0][2] == -1.0) ||
        (matDensity[1][2] <= density && ox == 1 && oy == 2 && matMoveX[1][2] == 0.0 && matMoveY[1][2] == -1.0) ||
        (matDensity[2][2] <= density && ox == 2 && oy == 2 && matMoveX[2][2] == -1.0 && matMoveY[2][2] == -1.0))
        return move;*/
    
    return move;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    mat3 matID = readMatID(fragCoord);
    mat3 matDensity = readMatDensity(fragCoord, matID);
    //float[3][3] sand = {{vec4(0.0,vec4(0.0,vec4(0.0},{vec4(0.0,vec4(0.0,vec4(0.0},{vec4(0.0,vec4(0.0,vec4(0.0}};
    mat3 matMoveX = mat3(0);
    mat3 matMoveY = mat3(0);
    
    readMatMove(matMoveX, matMoveY, fragCoord);
    
    
    vec2 move = calcMovePaired(matID, matDensity, matMoveX, matMoveY, fragCoord);
    
    
    fragColor = vec4(move.x, move.y, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define BUFFER_A_SAMPLER iChannel0
#define BUFFER_C_SAMPLER iChannel1
#define BUFFER_D_SAMPLER iChannel2

#define NUM_TILE_IDS 32.0
#define DATA_BEGIN 0.0
#define DENSITY_BEGIN DATA_BEGIN
#define DENSITY_END (DENSITY_BEGIN+NUM_TILE_IDS)
#define COLOR_BEGIN DENSITY_END
#define COLOR_END (COLOR_BEGIN+NUM_TILE_IDS)
#define DATA_END COLOR_END
#define SELECTED_ELEMENT DATA_END

// State buffer. States are updated here.
float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}  


float getSelectedElement() {
    float index = SELECTED_ELEMENT;
    vec2 uv = vec2(mod(index/iResolution.x , 1.0), floor(index/iResolution.x)/iResolution.y) + 0.5/iResolution.xy;
    float id = texture(BUFFER_A_SAMPLER, uv).x;
    return id;
}

mat3 readMatID(vec2 fragCoord) {
    mat3 matID = mat3(0.0);
    
    for (int x = 0; x < 3; ++x) {
        matID[x][0] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,0-1))/iResolution.xy).x;
        matID[x][1] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,1-1))/iResolution.xy).x;
        matID[x][2] = texture(BUFFER_D_SAMPLER, (fragCoord+vec2(x-1,2-1))/iResolution.xy).x;
    }
    return matID;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    if (iFrame <= 10) {
    	float id = floor(texture(iChannel3, uv).x*12.0);
        if (id-0.5 > NUM_TILE_IDS)
            id = 5.0;
        
        else if (id == 5.0)
            id = 0.0;
        else if (id == 5.0)
            id == 0.0;
        
        fragColor = vec4(id, vec3(0.0));
        return;
    }
    
    vec2 move = texture(BUFFER_C_SAMPLER, uv).xy;
    vec2 otherMove = texture(BUFFER_C_SAMPLER, uv+move/iResolution.xy).xy;
    if (otherMove.x != -move.x || otherMove.y != -move.y)
        move = vec2(0.0);
    
    float oldID = texture(BUFFER_D_SAMPLER, uv).x;
    float id = texture(BUFFER_D_SAMPLER, uv+move/iResolution.xy).x;
    
    if (length(fragCoord.xy - iMouse.xy) < 8.0 && iMouse.z >= 1.0)
        id = getSelectedElement();
    
    int hash = int(1000.0*rand(vec2(rand(fragCoord), iTime)));
    
    bool hasFire = false;
    bool hasPlant = false;
    bool hasAir = false;
    bool hasWater = false;
    
    mat3 matID = readMatID(fragCoord);
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            float id = matID[x][y];
            hasFire = hasFire || id == 5.0 || id == 7.0;
            hasAir = hasAir || id == 0.0;
            hasWater = hasWater || id == 6.0;
            hasPlant = hasPlant;
        }
    }
    
    if (id == 4.0 && hash < 3) // Steam > water
        id = 6.0;
    if (id == 6.0 && oldID == 5.0) // water + magma > steam
        id = 4.0;
    if (id == 5.0 && oldID == 6.0) // magma + water > stone
        id = 2.0;
    if (id == 1.0 && oldID == 6.0) // sand + water > mud
        id = 9.0;
    if (id == 6.0 && oldID == 1.0) // water + sand > air
        id = 0.0;
    if (id == 9.0 && oldID == 0.0) // Mud + air > sand
        id = 1.0;
    if (id == 0.0 && oldID == 9.0) // air + mud > water
        id = 6.0;
    if (id == 7.0 && hash < 10) // fire > air
        id = 0.0;
    
    // Oil + fire + air > fire
    if (id == 3.0 && hasFire && hasAir && hash < 200) 
        id = 7.0;
    
    // Sand + fire > glass
    if (id == 1.0 && hasFire) 
        id = 8.0;
    
    // Glass + water > sand
    if (id == 8.0 && hasWater)
        id = 1.0;
    
    // water + fire > steam
    if (id == 6.0 && hasFire && hash < 20)
        id = 4.0;
    
    // magma + water > stone
    if (id == 5.0 && hasWater)
        id = 2.0;
    
    
        
    fragColor = vec4(id, 0.0, 0.0, 0.0);
}