

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
    By Cole Peterson (Plento)
    
    
    A WIP experiment in making a ton of individual particles that behave like you'd expect.
    Still some things to work out but so far Im happy.
    
    Cells store mass and velocity.
    
    Use the MOUSE to drag stuff around.
    
    Press A to zoom while using the mouse.
    
*/


void zoom(inout vec2 u){
    u -= iMouse.xy;
    u *= .6;
    u += iMouse.xy;
}

vec3 pal(float t){
    return 0.5+0.44*cos(vec3(1.4, 1.1, 1.4)*t + vec3(1.1, 6.1, 4.4) + .5);
}

// Main color
vec3 color(vec2 u, vec4 bA){
    float t = abs(bA.z*2.) + abs(bA.w*4.) + 3.6*length(bA.zw);
    vec3 col = vec3(clamp(bA.x, 0., 1.)) * pal(t*.2 + .3);
    return col * 1.6;
}


// glow effect
float glow(vec2 u){
    vec2 uv = u / R;
    float blur = 0.;

    const float N = 3.;
    
    for(float i = 0.; i < N; i++)
    {
        blur += texture(iChannel0, uv + vec2(i*.001, 0.), 1.1).x;
        blur += texture(iChannel0, uv - vec2(i*.001, 0.), 1.1).x;
    
        blur += texture(iChannel0, uv + vec2(0., i*.001), 1.1).x;
        blur += texture(iChannel0, uv - vec2(0., i*.001), 1.1).x;
    }
        
    return blur / N*4.;
}


void mainImage( out vec4 f, in vec2 u ){
    vec2 uv = u / R;
        
    if(KEY(65., 0.) > 0.) zoom(u);

    vec4 bA = A(u);

    vec3 col = color(u, bA);

    float g = glow(u);
    
    col += 0.08*g*pal(1.5*length(bA.zw));
    
    f = vec4(sqrt(clamp(col, 0.0, 1.0)), 1.0);
}





// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define m vec2((iMouse.xy - .5*R) / R.y)
#define KEY(v,m) texelFetch(iChannel3, ivec2(v, m), 0).x
#define ss(a, b, t) smoothstep(a, b, t)

#define A(p) texelFetch(iChannel0,  ivec2(p), 0)
#define B(p) texelFetch(iChannel1,  ivec2(p), 0)
#define C(p) texelFetch(iChannel2,  ivec2(p), 0)
#define D(p) texelFetch(iChannel3,  ivec2(p), 0)



// Add the force coming from the side
#define SIDE_FORCE



// Constants
const vec2 UP = vec2(0., 1.);
const vec2 DOWN = vec2(0., -1.);
const vec2 LEFT = vec2(-1., 0.);
const vec2 RIGHT = vec2(1., 0.);
const vec2 UPL = vec2(-1., 1.);
const vec2 DOWNL = vec2(-1., -1.);
const vec2 UPR = vec2(1., 1.);
const vec2 DOWNR = vec2(1., -1.);



// https://www.shadertoy.com/view/4djSRW
vec2 hash22(vec2 p){
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

float hash12(vec2 p){
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
    Buffer A: 
    
    Track cell's mass and velocity.
    
    4 main things hapening.
    
    1. Add mass to cell proportional  to: (change in mass) * (dot(mass gradient direction, cell  velocity))
    2. Subtract mass from cell proportional to: (average mass of neighborhood) * (dot(mass gradient direction, cell  velocity))
    3. Apply acceleration to cell velocity: CellVelocity += (VelocityAverage - CellVelocity) 
    4. Apply a gravity ish force to cell velocity. This "de snakes" the particles and clumps them into points.
    
    x = mass
    zw = velocity
    
*/


 
// Neighborhood averages
vec4 Grad(vec2 u)
{
    float h = 1.;
    
    vec4 up = A((u + vec2(0., h)));
    vec4 down = A((u - vec2(0., h)));
    vec4 left = A((u - vec2(h, 0.)));
    vec4 right = A((u + vec2(h, 0.)));
    
    
    vec4 upl = A((u + UPL));
    vec4 downl = A((u + DOWNL));
    vec4 upr = A((u + UPR));
    vec4 downr = A((u + DOWNR));
    
    vec4 avg = (up+down+left+right+upr+downr+upl+downl) / 8.;
    return avg;
}

// Direction of most mass relative to cell
vec2 massGrad(vec2 u)
{
    float up = A((u + UP)).x;
    float down = A((u + DOWN)).x;
    float left = A((u + LEFT)).x;
    float right = A((u + RIGHT)).x;

    float upl = A((u + UPL)).x;
    float downl = A((u + DOWNL)).x;
    float upr = A((u + UPR)).x;
    float downr = A((u + DOWNR)).x;
    
    vec2 cm = vec2(0., 0.);
    cm += up*UP;
    cm += down*DOWN;
    cm += left*LEFT;
    cm += right*RIGHT;
    
    cm += upl*UPL;
    cm += downl*DOWNL;
    cm += upr*UPR;
    cm += downr*DOWNR;
    
    return cm;
}


void mainImage( out vec4 f, in vec2 u ){
    vec2 uv = vec2(u.xy - 0.5*R.xy)/R.y;
    
    // bA.x = mass, bA.zw = velocity
    vec4 bA = A(u);
    
    vec4 grad = Grad(u); // Gradient at point of cell
    vec2 vAvg = grad.zw; // Average velocity of surrounding cells
    vec2 acc = vAvg - bA.zw; // Cell acceleration
    acc = clamp(acc, -1., 1.);

    bA = mix(bA, grad, .02); // Substitute in a little bit of the average

    vec2 mg = massGrad(u); // Direction relative to current cell of most mass

    float massAvg = grad.x; // Average surrounding mass
    float massDif = massAvg - bA.x; // How fast the mass should be changing
    
    // Add and subtract mass from cell based on how much its moving in the direction of most mass
    // and how fast the mass is changing
    float dp = dot(mg, bA.zw); // How much the cell is moving toward the center of mass gradient
    bA.x -= dp*massAvg;
    bA.x += .999*dp*massDif;
    
    // Apply acceleration
    bA.zw += acc;
    
    // Apply a gravity ish force (clumps stuff into single points)
    float r = max(length(mg), 1.);
    float grav = -(3.5*massAvg*bA.x) / (r*r);
    bA.zw -= (mg)*grav;
    
    // Apply some friction
    bA.zw -= bA.zw*.0003;
    
    
    // Add stuff with mouse
    if(iMouse.z > 0.){
       vec2 v = (C(u).xy - iMouse.xy);
       if(length(v) < 100.){
        
            float d = length(iMouse.xy-u);
            float frc = .025*exp2(-d*.025);       
        
            bA.zw -= frc * v;    
            bA.x+=.08*frc;
        }
    }
   
    
    #ifdef SIDE_FORCE
    // Apple a small force coming from the left and add some mass
    float sf = .002*ss(R.x, 0., u.x * 0.9);
    bA.zw += sf*vec2(1., 0.);
    bA.x += .06*sf;
    #endif
   
    // Inita
    if(iFrame < 8){
        bA = vec4(0);
        
        if(hash12(u*343. + 232.) < .1){
            bA.zw = 12.*(2.*hash22(u*543. + 332.) - 1.);
            bA.x = hash22(u*543. + 332.).x;
        }
    }
    
    
    bA.zw = clamp(bA.zw, -120., 120.);
    bA.x = clamp(bA.x, 0., 1.);
   
    
    f = bA;
 
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Store mouse last pos nothin to see here
void mainImage( out vec4 f, in vec2 u ){
    f = vec4(iMouse.xy, 0., 0.);
}