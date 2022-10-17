

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const vec3 White = vec3(1.); 
const vec3 Orange = vec3(1.,1.,0.); 
const vec3 SkyBlue = vec3(.5294,.8078,.9216);
const vec3 Black = vec3(0.); 
const float g = -.01;
const float Pi = acos(-1.); 

float orb(vec2 center, float brightness, vec2 uv){
    float dist = length(uv-center);
    return min(1.,brightness/(dist*dist)); 
}

float orb_path(vec2 ixy,float brightness,vec2 v,float time,vec2 uv){
    vec2 loc = ixy + time*v + time*time*vec2(0.,g);
    return orb(loc,brightness,uv); 
    
}

//From hash1 in https://www.shadertoy.com/view/4ttSWf
float rand(float n){
    return fract( n*17.0*fract( n*0.3183099 ) );

}
float rand2d( vec2 p )
{
    p  = 50.0*fract( p*0.3183099 );
    return fract( p.x*p.y*(p.x+p.y) );
}
vec2 randvec2( float seed){
    const vec2 k =  vec2( 0.3183099, 0.3678794);
    return fract(seed*fract(k*seed)); 
}

vec3 randvec3( float seed){
    const vec3 k = vec3( 0.3183099, 0.3678794, 0.446234 );
    return fract(seed*fract(k*seed)); 
}

vec3 scaleTo1(vec3 v){
    float ma = max(max(v.x,v.y),v.z);
    return v/ma;
}

float rand_range(float l, float r, float seed){
    return l+rand(seed)*(r-l); 
}

float flicker(float time,float delay){
    float time_after_delay = max(0.,time-delay);
    return (cos(time*time_after_delay)+1.)/2.;
}

vec2 find_velocity(vec2 start,vec2 end,float time){
    end-=start;
    return vec2(end.x/time,(end.y-g*time*time)/time); 
}

float get_orb(vec2 start,vec2 end,float time_to_take,float time,vec2 uv){
    vec2 v = find_velocity(start,end,time_to_take);
    return orb_path(start, .0001, v, time,uv);  
}
float firework_flicker(vec2 center, float time, float seed, vec2 uv){
    uv-=center;
    uv.x = abs(uv.x); 
    float brightness = (1. - smoothstep(0.,5.,time))*.0002;
    float sdf = 0.;
    
    for(int i=1;i<=54;i++){
        float fi = float(i); 
        float flickeri = flicker(time,3.*rand(fi*seed))*brightness;
        float theta = rand_range(0.0,2.*Pi,fi*16.1*seed);
        float r = 0.05+.03*(fi/9.);
        vec2 v = r*vec2(cos(theta),sin(theta));  
        sdf += orb_path(vec2(0.),flickeri,v,time,uv);
        
    }
    
    return sdf; 
}

vec3 firework_flicker_total(float delay,float time_to_start, float time, float seed,vec2 uv){
    vec3 col = scaleTo1(randvec3(seed));
    vec2 center = randvec2(seed);
    center.x = 2.*center.x-1.;
    if(time<delay){
        return vec3(0.); 
    }
    time-=delay;
    if(time<time_to_start){
        return col*get_orb(vec2(0.,-1.),center,time_to_start,time,uv);
    }
    return col*firework_flicker(center, time-time_to_start,seed,uv); 
}



float firework_standard(vec2 center, float time, float seed, vec2 uv){
    uv-=center;
    float brightness = (1. - smoothstep(0.,5.,time))*.0002;
    float sdf = 0.;
     for(int i=1;i<=54;i++){
        float fi = float(i)/54.; 
        float theta = rand_range(0.,2.*Pi,rand(seed*20.*fi));
        float r = 0.05+.03*(2.*fi);
        vec2 v = r*vec2(cos(theta),sin(theta));  
        sdf += orb_path(vec2(0.),brightness,v,time,uv);
        
    }
    return sdf;
}

vec3 firework_standard_total(float delay,float time_to_start, float time, float seed,vec2 uv){
    vec3 col = randvec3(seed);
    vec2 center = randvec2(seed); 
    center.x=2.*center.x-1.;
    if(time<delay){
        return vec3(0.); 
    }
    time-=delay;
    if(time<time_to_start){
        return col*get_orb(vec2(0.,-1.),center,time_to_start,time,uv);
    }
    return col*firework_standard(center, time-time_to_start,seed,uv); 
}


vec3 background(vec2 uv){
    vec2 star_loc = floor(100.*uv);
    float star; 
    if(uv.y > -0.3 && rand2d(star_loc)>0.99){
        star = orb(star_loc,0.5,100.*uv);
    }else{
        star = 0.;
    }
    vec3 star3 = star*White; 
    return mix(SkyBlue,Black,(uv.y+1.))/2.+star3;
}

vec3 scene(vec2 uv){
    vec3 col;
    float time = iTime*2.5; 
    time = mod(time,110.); 
    col = background(uv);
    for(int i=0;i<=3;i++){
        float fi = float(i);
        float delay = fi;
        col += firework_standard_total(delay,3., mod(time,11.), 1.+float(int((time+11.)/11.)*(7+i)), uv );
    }
    for(int i=0;i<=3;i++){
      float fi = float(i);
      float delay = fi;
      col += firework_flicker_total(delay,3.,mod(time,11.),2.+float(int((time+11.)/11.)*(11+2*i)),uv);
    }
    //col = get_orb(vec2(0.),vec2(-1.0,0.),5.,iTime,uv)*White; 
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from -1 to 1)
    vec2 uv = fragCoord/iResolution.y;
    uv-=vec2(0.5*iResolution.x/iResolution.y,0.5);
    uv*=2.;
    
    vec3 col = scene(uv);


    // Output to screen
    fragColor = vec4(col,1.0);
}