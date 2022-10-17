

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Building on ideas from 
//https://www.shadertoy.com/view/NsKGDy
//https://www.shadertoy.com/view/7sKGRy
//https://www.shadertoy.com/view/fsyGD3

#define MDIST 350.0
#define STEPS 200.0
#define pi 3.1415926535
#define rot(a) mat2(cos(a),sin(a),-sin(a),cos(a))
#define pmod(p,x) (mod(p,x)-0.5*(x))

//iq palette
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d ){
    return a + b*cos(2.*pi*(c*t+d));
}
float h21 (vec2 a) {
    return fract(sin(dot(a.xy,vec2(12.9898,78.233)))*43758.5453123);
}
float h11 (float a) {
    return fract(sin((a)*12.9898)*43758.5453123);
}
float box(vec3 p, vec3 b){
    vec3 d = abs(p)-b;
    return max(d.x,max(d.y,d.z));
}

//global ray direction
vec3 rdg = vec3(0);
//block scale factor
float gscl = 1.;

vec2 blocks(vec3 p, vec3 scl, vec3 rd){
    float t = iTime*(11./8.);
      
    vec3 dMin = vec3(-0.5) * scl;
    vec3 dMax = vec3(0.5) * scl;
    float id = 0.;
    
    
    float MIN_SIZE = 0.3;
    float ITERS = 6.;
    float MIN_ITERS = 1.;
    float PAD_FACTOR = 1.01;
    float seed = floor(t/(ITERS+5.0))+0.1;
    t = mod(t,ITERS+5.0);

    //Offset and clamp the time at 0 so the cube stays uncut for a short time
    t= clamp(t-1.0,0.0,ITERS);
    
    //calculate initial box dimensions
    vec3 dim = dMax - dMin;
    
    //Big thanks for @0b5vr for cleaner version of subdiv algo
    for (float i = 0.; i < ITERS; i++) {
    
        //If this is the final cut when animating then break
        if(i>floor(t)) break;
        
        //divide the box into eight
        vec3 divHash = vec3(
            h21( vec2( i + id, seed )),
            h21( vec2( i + id + 2.44, seed )),
            h21( vec2( i + id + 7.83, seed ))
        );
        vec3 divide = divHash * dim + dMin;
        
        //Clamp Division Line
        divide = clamp(divide, dMin + MIN_SIZE * PAD_FACTOR, dMax - MIN_SIZE * PAD_FACTOR);
        
        //Un-altered division line for coloring moving cells 
        vec3 divideFull = divide;
        
        //find smallest dimension of divison
        vec3 minAxis = min(abs(dMin - divide), abs(dMax - divide));
        float minSize = min(minAxis.x, min(minAxis.y, minAxis.z));
        
        //if the next cut will be below the minimum cut size then break out
        if (minSize < MIN_SIZE && i + 1. > MIN_ITERS) {break ;}

        //If the current iteration is the cutting one
        //Smooth it between 0 and its final position
        float tt = smoothstep(0.,1.,fract(t));
        if(i == floor(t) &&mod(t,2.0)<1.0){
            divide=mix(dMin,divide,tt);
        }
        else if(i == floor(t)){
            divide=mix(dMax,divide,tt);
        }

        
        // update the box domain
        dMax = mix( dMax, divide, step( p, divide ));
        dMin = mix( divide, dMin, step( p, divide ));

        // id will be used for coloring and hash seeding
        vec3 diff = mix( -divideFull, divideFull, step( p, divide));
        id = length(diff + 10.0);
    
        // recalculate the dimension
        dim = dMax - dMin;
    }
    float volume = dim.x*dim.y*dim.z;
    vec3 center = (dMin + dMax)/2.0;

    //Calculate the distance to the outside of the current cell bounds
    //to avoid overstepping
    vec3 edgeAxis = mix( dMin, dMax, step( 0.0, rd ) );
    vec3 dAxis = abs( p - edgeAxis ) / ( abs( rd ) + 1E-4 );
    float dEdge = min(dAxis.x,min(dAxis.y,dAxis.z));
    float b=dEdge;

    vec3 d = abs(center);

    //Scale the cubes down so they stay the same size when they "explode"
    dim*=gscl;
    float a = box(p-center,dim*0.5);
    
    //Take the minimum between the actual box and the intersection 
    //to the outside of the cell
    a = min(a, b);
    
    //extra randomize the ID
    id = h11(id)*1000.0;

    return vec2(a,id);
}
//This is a smooth square wave approximation function but I really like
//The curve it makes so I'm using it instead of a smoothstep to animate the
//spin and "explode". It's a bit long because I made it start at 0 and repeat every 3. 
float swave(float x, float a){
    return (sin(x*pi/3.-pi/2.)/sqrt(a*a+sin(x*pi/3.-pi/2.)*sin(x*pi/3.-pi/2.))+1./sqrt(a*a+1.))*0.5;
}
vec2 map(vec3 p){
    float t = iTime*(11./8.);
    //The cycle for the spinning and explosion doesn't exaclty line up
    //with the cycle for the cutting because the spin needs to continue after the
    //cuts have reset, so the cycle is offset fowards a bit
    t = mod(t-1.0,11.0)+1.0;
    
    //timing out the spin+explode animation and making sure it happens
    //once per cycle
    float wav = swave(clamp(t*0.78-4.0,0.0,6.0),0.1);
    
    //Set the global scale variable that controls the exploding of boxes
    gscl = 1.0-wav*0.5;
    
    //rotation amount
    float rotd = wav*pi*4.02;
    
    //rotate the cube
    p.xz*=rot(rotd);
    
    //move the cube up when it rotates so it fits nicely in the screen
    p.y-=wav*3.0;
    
    //Scale space so the entire thing gets bigger (but the boxes are later re-sized,
    //so it looks like they stay the same time)
    p*=gscl;
    
    //Initalize output sdf, with vec2 for ID
    vec2 a = vec2(1);
    
    //Size of the subdivision fractal
    vec3 scl = vec3(10);
    
    //get the global ray direction and rotate it with the same rotation
    //as the cube so that the outside cell intersection still works correctly
    vec3 rd2 = rdg;
    rd2.xz*=rot(rotd);
    
    a = blocks(p,scl,rd2)+0.01;
    
    //use a box to optimize areas outside of the fractal to minimize steps
    //Also I found you can instead the step distance of the fractal when it explodes
    //without causing artifacts which is free performance
    a.x = max(box(p,vec3(scl*0.49)),a.x*(1.0+wav));
    
    return a;
}
vec3 norm(vec3 p){
    vec2 e = vec2(0.0001,0.);
    return normalize(map(p).x-vec3(
    map(p-e.xyy).x,
    map(p-e.yxy).x,
    map(p-e.yyx).x));
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-0.5*iResolution.xy)/iResolution.y;
    float t = iTime;
    vec3 col = vec3(0);
    vec3 ro = vec3(0,11.5,-20)*1.45;
    if(iMouse.z>0.){
    ro.zx*=rot(-7.0*(iMouse.x/iResolution.x-0.5));
    }
    ro.xz*=rot(-pi/4.);
    vec3 lk = vec3(0,0.,0);
    vec3 f = normalize(lk-ro);
    vec3 r = normalize(cross(vec3(0,1,0),f));
    vec3 rd = normalize(f*(gscl*0.9+0.13)+uv.x*r+uv.y*cross(f,r));    
    rdg = rd;
    vec3 p = ro;
    float dO = 0.;
    vec2 d = vec2(0);
    bool hit = false;
    for(float i = 0.; i<STEPS; i++){
        p = ro+rd*dO;
        d = map(p);
        dO+=d.x*0.99;
        if(d.x<0.0001){
            hit = true;
            break;
        }
        if(d.x>MDIST||i==STEPS-1.){
            dO=MDIST;
            break;
        }
    }
    if(hit){
        vec3 ld = normalize(vec3(0.5,0.9,-0.9));
        vec3 n = norm(p);
        vec3 e = vec3(0.5);
        vec3 al = pal(fract(d.y)*0.8-0.15,e*1.3,e,e*2.0,vec3(0,0.33,0.66));
        col = al;
        float diff = length(sin(n*2.)*.5+.8)/sqrt(3.);
        col = al*diff;
        float shadow = 1.;
        
        //Mini hard shadow code
        //Need to make sure global ray direction is updated
        rdg = ld;
        for(float h = 0.09; h<10.;){
            float dd = map(p+ld*h).x;
            if(dd<0.001){shadow = 0.6; break;}
            h+=dd;
        }     
        col*=shadow;
    }
    vec3 bg = vec3(0.741,0.498,0.498)*(1.0-length(uv)*0.5);
    col = mix(col,bg,dO/MDIST);
    fragColor = vec4(col,1.0);
}



