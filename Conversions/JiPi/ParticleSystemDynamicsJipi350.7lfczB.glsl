

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 is = texture(iChannel1, uv);
    float l = length(is);    
    fragColor = ((0.5 + 0.5 * sin(20.0 * l))/l) *  vec4(is.xyz, 0.0) + 0.5 * vec4(is.w, is.w, 0.0, 0.0); 
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define NUM_PARTICLES 400
#define speed 0.003
#define accel 0.0005
#define epsilon 0.5
#define invert 100000.0
#define gravity 5000.0
#define perpendicular 5000000.0

float hash(vec2 p) {
    float h = dot(p,vec2(127.1,311.7)); 
    return fract(sin(h)*43758.5453123);
}

vec2 pm(vec2 uv) {
    return mod(mod(uv, 1.0) + 1.0, 1.0);
}

int getType(vec2 uv) {
    float pos = uv.x;
    float catSize = 0.25;
    
    if (float(NUM_PARTICLES) < iResolution.x) {
        catSize = (float(NUM_PARTICLES) / iResolution.x) / 4.0;
    }    
    
    if (pos < catSize) {
        return 0;    
    } else if (pos >= catSize && pos < catSize * 2.0) {
        return 1;
    } else if (pos >= catSize * 2.0 && pos < catSize * 3.0) {
        return 2;   
    } else {
        return 3;   
    }
}

float getRule(int type0, int type1) {
    vec2 stepX = vec2(float(type1) / iResolution.x, 0.0);
    vec2 stepY = vec2(0.0, float(type0) / iResolution.y);
    vec2 halfStep = 0.5 / iResolution.xy;
    
    vec2 uv = stepX + stepY + halfStep;
    return texture(iChannel2, uv).x;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 is = texture(iChannel0, uv);
    vec2 pos = is.xy;
    vec2 vel = is.zw;
    
    vec2 accum = vec2(0.0);
    
    vec2 c = fragCoord.xy;
    vec2 r = iResolution.xy;
    vec2 r_n = vec2(0.0, iResolution.y);
    vec2 r_e = vec2(iResolution.x, 0.0);
    vec2 stepX = vec2(1.0 / iResolution.x, 0.0);
    vec2 stepY = vec2(0.0, 1.0 / iResolution.y);
    vec2 halfStep = 0.5 / iResolution.xy;
    
    vec2 posS = pos * r;
    
    int myType = getType(uv);

    for (int i = 0; i < NUM_PARTICLES; i++) {
        vec2 xpos = stepX * float(i);
        vec2 uvN = pm(xpos) + stepY * floor(xpos.x) + halfStep;
        vec2 posN = r * texture(iChannel0, uvN).xy;
        
        int thisType = getType(uvN);
        float rule = getRule(myType, thisType);
        
        vec2 velN = texture(iChannel0, uvN).zw;
        
        if (length(posN - posS) > epsilon) {  
        
            
            float rule = getRule(myType, thisType);
            
        	vec2 minV = vec2(0.0, 0.0);
        	float minD = 10000.0;
        	float d[5];
        	d[0] = distance(posN, posS);
        	d[1] = distance(posN + r_n, posS);
        	d[2] = distance(posN + r_e, posS);
        	d[3] = distance(posN - r_n, posS);
        	d[4] = distance(posN - r_e, posS);

            vec2 v[5];
            v[0] = posN - posS;
            v[1] = posN + r_n - posS;
            v[2] = posN + r_e - posS;
            v[3] = posN - r_n - posS;
            v[4] = posN - r_e - posS;

            for (int i = 0; i < 5; i++) {
                if (d[i] < minD) {
                    minD = d[i];
                    minV = v[i];
                }
            }

            accum += gravity * rule * (normalize(velN) + normalize(minV)) / (minD * minD);
            accum -= invert * rule * normalize(minV) / (minD * minD * minD);
            accum += perpendicular * rule * normalize(velN) * length(cross(vec3(vel, 0.0), vec3(velN, 0.0))) / (minD * minD * minD);
        }
    }  
    
    float mouseScale = 1.0 + 0.2 / (distance(posS, iMouse.xy) / iResolution.x);
    vec2 tempVel = vel + accel * accum;
    vec2 norm = normalize(tempVel);
    vel = length(tempVel) > 1.0 ? norm : tempVel;
    vel *= mouseScale;

    // initialize with noise
    if(iFrame<10) {
        fragColor = vec4(hash(uv + 1.1), hash(uv + 2.3), hash(uv + 3.8) - 0.5, hash(uv + 4.2) - 0.5);
    } else {
        fragColor = vec4(pm(pos + speed * vel), vel);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define NUM_PARTICLES 400
#define scale 0.5

vec4 getType(vec2 uv) {
    float pos = uv.x;
    float catSize = 0.25;
    
    if (float(NUM_PARTICLES) < iResolution.x) {
        catSize = (float(NUM_PARTICLES) / iResolution.x) / 4.0;
    }   
    
    if (pos < catSize) {
        return vec4(1.0, 0.0, 0.0, 0.0);    
    } else if (pos >= catSize && pos < catSize * 2.0) {
        return vec4(0.0, 1.0, 0.0, 0.0); 
    } else if (pos >= catSize * 2.0 && pos < catSize * 3.0) {
        return vec4(0.0, 0.0, 1.0, 0.0);   
    } else {
        return vec4(0.0, 0.0, 0.0, 1.0);    
    }
}

vec2 pm(vec2 uv) {
    return mod(mod(uv, 1.0) + 1.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 stepX = vec2(1.0 / iResolution.x, 0.0);
    vec2 stepY = vec2(0.0, 1.0 / iResolution.y);
    vec2 halfStep = 0.5 / iResolution.xy;
    
    vec4 accum = vec4(0.0);
    
    vec2 c = fragCoord.xy;
    vec2 r = iResolution.xy;
    vec2 r_n = vec2(0.0, iResolution.y);
    vec2 r_e = vec2(iResolution.x, 0.0);
   

    for (int i = 0; i < NUM_PARTICLES; i++) {
        vec2 xpos = stepX * float(i);
        vec2 uvN = pm(xpos) + stepY * floor(xpos.x) + halfStep;
        vec2 pos = r * texture(iChannel0, uvN).xy;

        float d = distance(c, pos);
        float d_n = distance(c, pos + r_n);
        float d_e = distance(c, pos + r_e);
        float d_s = distance(c, pos - r_n);
        float d_w = distance(c, pos - r_e);
        float minWrap = min(min(d_n, d_s), min(d_e, d_w));
        float minDistance = min(minWrap, d);
        accum += getType(uvN) * scale / (minDistance);
    }
    fragColor = clamp(accum, 0.0, 1.0);

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 U )
{
    // define our rule table
    
    int x = int(U.x);
    int y = int(U.y);
    
    vec4 one = vec4(1.0);
    vec4 neg_three = vec4(-3.0);
    vec4 neg = vec4(-1.0);
    
    
    /*    
          A  B  C  D
        A 1 -3  1  1
        B 1  1 -3  1
        C 1 -3  1  1
        D 1  1 -3  1
    */

    if (y == 0) {
        if (x == 1) {
            O = neg_three;   
        } else {
            O = one;
        }
    } else if (y == 1) {
        if (x == 2) {
            O = neg_three;   
        } else {
            O = one;
        }
    } else if (y == 2) {
        if (x == 1) {
            O = neg_three;   
        } else {
            O = one;
        }       
    } else {
        if (x == 2) {
            O = neg_three;   
        } else {
            O = one;
        } 
    }


    // Two alternative rules:
    
    /*
         1 -1 -1  1
        -1  1  1 -1
        -1 -1  1  1
         1  1 -1 -1

    if (y == 0) {
        if (x == 0 || x == 3) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 1) {
        if (x == 0 || x == 3) {
            O = neg;   
        } else {
            O = one;
        }
    } else if (y == 2) {
        if (x == 0 || x == 1) {
            O = neg;   
        } else {
            O = one;
        }       
    } else {
        if (x == 0 || x == 1) {
            O = one;   
        } else {
            O = neg;
        } 
    }
    */

    
    /* 
        -1  1 -1 -1
        -1 -1  1 -1
        -1 -1 -1  1
         1 -1 -1 -1


    if (y == 0) {
        if (x == 1) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 1) {
        if (x == 2) {
            O = one;   
        } else {
            O = neg;
        }
    } else if (y == 2) {
        if (x == 3) {
            O = one;   
        } else {
            O = neg;
        }       
    } else {
        if (x == 0) {
            O = one;   
        } else {
            O = neg;
        } 
    }
    */
    

    
}