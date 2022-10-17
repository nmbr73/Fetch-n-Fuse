

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define COL1 vec3(24, 32, 38) / 255.0
#define COL2 vec3(235, 241, 245) / 255.0

#define SF 2./min(iResolution.x, iResolution.y)
#define SS(l, s) smoothstep(SF, -SF, l - s)

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord - iResolution.xy * .5) / iResolution.y;
    
    float m = 0.;    
    for (float i = 0.; i < float(SIZE); i += 1.) {
        vec4 point = FD(i, 0);
        vec4 colData = FD(i, 1);
        
        float colTimeDiff = clamp(iTime - colData.x, 0., .5)*5.;        
        
        vec2 pos = point.xy;        
        
        float d = length(uv - pos)*.5;        
        float g = .1/(d*(10. + 20.*colTimeDiff))*FLASH_POWER;        

        m += g;        
    }
    
    vec3 col = mix(COL1, COL2, m);
    
    fragColor = vec4(col, 1.0);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    int x = int(fragCoord.x);
    int y = int(fragCoord.y);
    
    if(y > 2 || x > SIZE){
    	discard;
    }     
    
    vec2 rt = vec2(iResolution.x / iResolution.y, 1.);
    
    if(iFrame == 1){
        float ms = sqrt(float(SIZE));
        float yp = floor(float(x) / ms) - ms*.45;
        float xp = mod(float(x), ms) - ms*.45;
        vec2 pos = (vec2(xp,yp)) * (1./ms)*.9;
                
        pos *= rt;                       
                
        vec2 dir = normalize(vec2(hash12(fragCoord*200.)*2.-1., hash12(fragCoord * 100.)*2.-1.));   
        
        if(y==0){
            fragColor = vec4(pos, dir);                   
        }
        if(y==1){
            fragColor = vec4(-100., 0,0,0);                   
        }
        
    } else {
        vec4 iPoint = FD(x,0);
        vec2 pos = iPoint.xy;        
        vec2 dir = iPoint.zw;
                
        
        bool col = false;
                                                       
        
        if(iPoint.x <= (-.5*rt.x + RADIUS)){
            dir.x *= -1.;      
            col = true;
        }
        if(iPoint.x >= (.5*rt.x - RADIUS)){
            dir.x *= -1.; 
            col = true;
        }
        if(iPoint.y <= (-.5*rt.y + RADIUS)){
            dir.y *= -1.;            
            col = true;
        }
        if(iPoint.y >= (.5*rt.y - RADIUS)){
            dir.y *= -1.;            
            col = true;
        }
        
        for(int i=0; i<SIZE; i+=1){
            if(i!=x){
                vec4 nPoint = FD(i,0);
                vec2 nPointDir = nPoint.zw;
                if(distance(nPoint.xy, pos) < (RADIUS*2.)){
                    vec2 inV = normalize(nPoint.xy - pos);
                    if(dot(dir, inV) > 0.){                    	 
                        dir = reflect(dir, inV);                        
                    }
                    
                    col = true;                    
                }
            }        	
        }
        
        dir = normalize(dir);
        
        pos += dir * SPEED;
        
        pos.x = min(max(pos.x, -.5*rt.x + RADIUS), .5*rt.x - RADIUS);
        pos.y = min(max(pos.y, -.5*rt.y + RADIUS), .5*rt.y - RADIUS);
        
    
        if(y==0){
            fragColor = vec4(pos, dir);
        }
        if(y==1){
            if(col){
                fragColor = vec4(iTime, 0, 0, 0);
            } else {
                fragColor = FD(x,1);
            }
            
        }
        
    }
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define SIZE 122
#define FLASH_POWER .38
#define RADIUS .01
#define SPEED .0018

#define FD(x,y) texelFetch(iChannel0, ivec2(x, y), 0)

float hash12(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}