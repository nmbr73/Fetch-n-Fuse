
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//Building on ideas from 
//https://www.shadertoy.com/view/NsKGDy
//https://www.shadertoy.com/view/7sKGRy
//https://www.shadertoy.com/view/fsyGD3

#define MDIST 350.0
#define STEPS 200.0
#define pi 3.1415926535
#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))
#define pmod(p,x) (mod_f(p,x)-0.5f*(x))

//iq palette
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d ){
    //if (t>0.5 && t <0.75) return to_float3_s(0.0f);
      
    return a + b*cos_f3(2.0f*pi*(c*t+d));
}
__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y),to_float2(12.9898f,78.233f)))*43758.5453123f);
}
__DEVICE__ float h11 (float a) {
    return fract(_sinf((a)*12.9898f)*43758.5453123f);
}
__DEVICE__ float box(float3 p, float3 b){
    float3 d = abs_f3(p)-b;
    return _fmaxf(d.x,_fmaxf(d.y,d.z));
}



__DEVICE__ float2 blocks(float3 p, float3 scl, float3 rd, float gscl, float iTime){
    float t = iTime*(11.0f/8.0f);

    float3 dMin = to_float3_s(-0.5f) * scl;
    float3 dMax = to_float3_s(0.5f) * scl;
    float id = 0.0f;
        
    float MIN_SIZE = 0.3f;
    float ITERS = 6.0f;
    float MIN_ITERS = 1.0f;
    float PAD_FACTOR = 1.01f;
    float seed = _floor(t/(ITERS+5.0f))+0.1f;
    t = mod_f(t,ITERS+5.0f);

    //Offset and clamp the time at 0 so the cube stays uncut for a short time
    t= clamp(t-1.0f,0.0f,ITERS);
    
    //calculate initial box dimensions
    float3 dim = dMax - dMin;
    
    //Big thanks for @0b5vr for cleaner version of subdiv algo
    for (float i = 0.0f; i < ITERS; i+=1.0f) {
    
        //If this is the final cut when animating then break
        if(i>_floor(t)) break;
        
        //divide the box into eight
        float3 divHash = to_float3(
            h21( to_float2( i + id, seed )),
            h21( to_float2( i + id + 2.44f, seed )),
            h21( to_float2( i + id + 7.83f, seed ))
        );
        float3 divide = divHash * dim + dMin;
        
        //Clamp Division Line
        divide = clamp(divide, dMin + MIN_SIZE * PAD_FACTOR, dMax - MIN_SIZE * PAD_FACTOR);
        
        //Un-altered division line for coloring moving cells 
        float3 divideFull = divide;
        
        //find smallest dimension of divison
        float3 minAxis = _fminf(abs_f3(dMin - divide), abs_f3(dMax - divide));
        float minSize = _fminf(minAxis.x, _fminf(minAxis.y, minAxis.z));
        
        //if the next cut will be below the minimum cut size then break out
        if (minSize < MIN_SIZE && i + 1.0f > MIN_ITERS) {break ;}

        //If the current iteration is the cutting one
        //Smooth it between 0 and its final position
        float tt = smoothstep(0.0f,1.0f,fract(t));
        if(i == _floor(t) &&mod_f(t,2.0f)<1.0f){
            divide=_mix(dMin,divide,tt);
        }
        else if(i == _floor(t)){
            divide=_mix(dMax,divide,tt);
        }

        
        // update the box domain
        dMax = mix_f3( dMax, divide, step( p, divide ));
        dMin = mix_f3( divide, dMin, step( p, divide ));

        // id will be used for coloring and hash seeding
        float3 diff = mix_f3( -divideFull, divideFull, step( p, divide));
        id = length(diff + 10.0f);
    
        // recalculate the dimension
        dim = dMax - dMin;
    }
    float volume = dim.x*dim.y*dim.z;
    float3 center = (dMin + dMax)/2.0f;

    //Calculate the distance to the outside of the current cell bounds
    //to avoid overstepping
    float3 edgeAxis = mix_f3( dMin, dMax, step( to_float3_s(0.0f), rd ) );
    float3 dAxis = abs_f3( p - edgeAxis ) / ( abs_f3( rd ) + 1E-4 );
    float dEdge = _fminf(dAxis.x,_fminf(dAxis.y,dAxis.z));
    float b=dEdge;

    float3 d = abs_f3(center);

    //Scale the cubes down so they stay the same size when they "explode"
    dim*=gscl;
    float a = box(p-center,dim*0.5f);
    
    //Take the minimum between the actual box and the intersection 
    //to the outside of the cell
    a = _fminf(a, b);
    
    //extra randomize the ID
    id = h11(id)*1000.0f;

    return to_float2(a,id);
}
//This is a smooth square wave approximation function but I really like
//The curve it makes so I'm using it instead of a smoothstep to animate the
//spin and "explode". It's a bit long because I made it start at 0 and repeat every 3.0f 
__DEVICE__ float swave(float x, float a){
    return (_sinf(x*pi/3.0f-pi/2.0f)/_sqrtf(a*a+_sinf(x*pi/3.0f-pi/2.0f)*_sinf(x*pi/3.0f-pi/2.0f))+1.0f/_sqrtf(a*a+1.0f))*0.5f;
}
__DEVICE__ float2 map(float3 p, float iTime, float *gscl, float3 rdg){
    float t = iTime*(11.0f/8.0f);
    //The cycle for the spinning and explosion doesn't exaclty line up
    //with the cycle for the cutting because the spin needs to continue after the
    //cuts have reset, so the cycle is offset fowards a bit
    t = mod_f(t-1.0f,11.0f)+1.0f;
    
    //timing out the spin+explode animation and making sure it happens
    //once per cycle
    float wav = swave(clamp(t*0.78f-4.0f,0.0f,6.0f),0.1f);
    
    //Set the global scale variable that controls the exploding of boxes
    *gscl = 1.0f-wav*0.5f;
    
    //rotation amount
    float rotd = wav*pi*4.02f;
       
    //rotate the cube
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(rotd)));
    
    //move the cube up when it rotates so it fits nicely in the screen
    p.y-=wav*3.0f;
    
    //Scale space so the entire thing gets bigger (but the boxes are later re-sized,
    //so it looks like they stay the same time)
    p*=*gscl;
    
    //Initalize output sdf, with float2 for ID
    float2 a = to_float2_s(1);
    
    //Size of the subdivision fractal
    float3 scl = to_float3_s(10);
    
    //get the global ray direction and rotate it with the same rotation
    //as the cube so that the outside cell intersection still works correctly
    float3 rd2 = rdg;
    swi2S(rd2,x,z, mul_f2_mat2(swi2(rd2,x,z),rot(rotd)));
    
    a = blocks(p,scl,rd2,*gscl,iTime)+0.01f;
    
    //use a box to optimize areas outside of the fractal to minimize steps
    //Also I found you can instead the step distance of the fractal when it explodes
    //without causing artifacts which is free performance
    a.x = _fmaxf(box(p,(scl*0.49f)),a.x*(1.0f+wav));

    return a;
}
__DEVICE__ float3 norm(float3 p,float iTime, float *gscl, float3 rdg){
    float2 e = to_float2(0.0001f,0.0f);
    return normalize(map(p,iTime,gscl,rdg).x-to_float3(
    map(p-swi3(e,x,y,y),iTime,gscl,rdg).x,
    map(p-swi3(e,y,x,y),iTime,gscl,rdg).x,
    map(p-swi3(e,y,y,x),iTime,gscl,rdg).x));
}
__KERNEL__ void AnimatedSubdivision3DFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
    CONNECT_SLIDER0(Lower, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER1(Offset, -10.0f, 10.0f, 0.0f);  

    CONNECT_SLIDER2(TexSize,   -10.0f, 10.0f, 0.15f);
    CONNECT_SLIDER3(TexOffsetX, -10.0f, 10.0f, 0.0f);     
    CONNECT_SLIDER4(TexOffsetY, -10.0f, 10.0f, 0.0f);     

    //global ray direction
    float3 rdg = to_float3_s(0);
    //block scale factor
    float gscl = 1.0f;

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float t = iTime;
    float3 col = to_float3_s(0);
    float3 ro = to_float3(0,11.5f,-20)*1.45f;
    if(iMouse.z>0.0f){
      swi2S(ro,z,x, mul_f2_mat2(swi2(ro,z,x),rot(-7.0f*(iMouse.x/iResolution.x-0.5f))));
    }
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z),rot(-pi/4.0f)));
    float3 lk = to_float3(0,0.0f,0);
    float3 f = normalize(lk-ro);
    float3 r = normalize(cross(to_float3(0,1,0),f));
    float3 rd = normalize(f*(gscl*0.9f+0.13f)+uv.x*r+uv.y*cross(f,r));    
    rdg = rd;
    float3 p = ro;
    float dO = 0.0f;
    float2 d = to_float2_s(0);
    bool hit = false;
    for(float i = 0.0f; i<STEPS; i+=1.0f){
        p = ro+rd*dO;
        d = map(p,iTime,&gscl,rdg);
        dO+=d.x*0.99f;
        if(d.x<0.0001f){
            hit = true;
            break;
        }
        if(d.x>MDIST||i==STEPS-1.0f){
            dO=MDIST;
            break;
        }
    }
    float3 al;
    if(hit){
        float3 ld = normalize(to_float3(0.5f,0.9f,-0.9f));
        float3 n = norm(p,iTime,&gscl,rdg);
        float3 e = to_float3_s(0.5f);
              al = pal(fract(d.y)*0.8f-0.15f,e*1.3f,e,e*2.0f,to_float3(0,0.33f,0.66f));
        
        float id = fract(d.y)*0.8f-0.15f;
        
        if(id>Lower && id<Lower+Offset)
        {
        
          float2 tuv = to_float2(p.x+TexOffsetX,p.y+TexOffsetY)*TexSize;
          al = swi3(texture(iChannel0, tuv),x,y,z);
        }
        
        //al = swi3(texture(iChannel0, tuv),x,y,z);
        
        col = al;
        float diff = length(sin_f3(n*2.0f)*0.5f+0.8f)/_sqrtf(3.0f);
        col = al*diff;
        float shadow = 1.0f;
        
        //Mini hard shadow code
        //Need to make sure global ray direction is updated
        rdg = ld;
        for(float h = 0.09f; h<10.0f;){
            float dd = map(p+ld*h,iTime,&gscl,rdg).x;
            if(dd<0.001f){shadow = 0.6f; break;}
            h+=dd;
        }     
        col*=shadow;
    }
    float3 bg = to_float3(0.741f,0.498f,0.498f)*(1.0f-length(uv)*0.5f);
    col = _mix(col,bg,dO/MDIST);
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}