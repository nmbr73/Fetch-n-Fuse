

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Public Domain. By Eivind Magnus Hvidevold 30.07.2018.

// Description:
// Per-pixel particle data structure.
// Voronoi based on random graph construction and traversal.
// Will reinitialize on switch to full screen.
// Some random forces of various types applied + wave equation.
// High velocity particles are blue.

// Buf A: Particle positions: xy pos, zw old pos
// Buf B: Particle neighbour indices: x, y, z, w indexes neighbour in direction of each corner, respectively
// Buf C: x: Nearest particle to pixel position

// Inspired by https://www.shadertoy.com/view/XsjyRm .
// See also https://www.shadertoy.com/view/4dGSDR for Efficient splatting.

bool debug(out vec4 fragColor, in vec2 fragCoord) {
    int frame = 0;
    vec3 color = vec3(0.0);
    vec2 uv = fragCoord / iResolution.xy;
    for (int i = 1; i < 100; i++) {
        vec2 iv = vec2(ivec2(i / 10, i % 10));
        
        vec2 pos = texelFetch(iChannel0, ivec2(iv), 0).xy;
        
        ivec2 particleCoord = ivec2(iv);
        
        ivec4 closest = ivec4(texelFetch(iChannel1, particleCoord, 0));
        
        vec2 vnext = vec2(particleCoord);
        vec2 pos2 = getParticle(iResolution, iChannel0, iFrame, closest.x).xy;
        vec2 pos3 = getParticle(iResolution, iChannel0, iFrame, closest.y).xy;
        vec2 pos4 = getParticle(iResolution, iChannel0, iFrame, closest.z).xy;
        vec2 pos5 = getParticle(iResolution, iChannel0, iFrame, closest.w).xy;
        
        // color = vec3(0.0);
        if (int(closest.x) != -1) color += drawLine(uv, pos, pos2);
        if (int(closest.y) != -1) color += drawLine(uv, pos, pos3);
        if (int(closest.z) != -1) color += drawLine(uv, pos, pos4);
        if (int(closest.w) != -1) color += drawLine(uv, pos, pos5);
    }
	  
    fragColor = vec4(color, 1.0);
    return true;
}

vec3 vmul(float v) {
    float r = 0.5;// + pow(v, 0.1);
    float g = 0.5;
    float b = 0.5;
    float r2 = 0.0;
    float g2 = 0.0;
    float b2 = 0.1;
    float vv = v > 0.002 ? 1.0 : 0.0;
    vv = smoothstep(0.0, 0.004, v);
    return mix(vec3(r, g, b), vec3(r2, g2, b2), vv);
}

void mainImage2(out vec4 fragColor, in vec2 fragCoord, in vec2 uv) {
    /*
	if (debug(fragColor, fragCoord)) {
        return;
    }
	*/
    
    vec4 near = texelFetch(iChannel2, ivec2(fragCoord), 0);
    vec2 iv = vec2(fragCoord) + near.yz;
    int nextIndex = int(near.x);
    
    if (nextIndex == -1) {
        fragColor.r = 1.0;
        return;
    }
	
    //int nextIndex = serializeUV(iFrame, getRandomParticlePos(iResolution, iChannel0, iv, frame, i));
    vec2 closestPos = getParticle(iResolution, iChannel0, iFrame, nextIndex).xy;
    
    vec3 color = vec3(0.0);
    vec3 colorBase = vec3(1.0);
    float mind = distance(uv, closestPos);
    float oldmind = 0.0;
    //float vmul = 1000.0;
    
    if (mind <= md) {
        //fragColor = vec4(1.0);
    }
    //fragColor = vec4(float(i) / float(iter * iter));
    //return;
    //fragColor = vec4(mind / md * 0.01);
    //return;
    const int iter2 = iter * iter;
    int seen[iter2];
    for (int i = 0; i < iter2; i++) {
        seen[i] = -1;
    }
    int stack[iter2];
    for (int i = 0; i < iter2; i++) {
        stack[i] = -1;
    }
    
    int oldIndex = nextIndex;
    int closestIndex = nextIndex;
    int stackPointer = 0;
    stack[0] = nextIndex;
    for (int i = 0; i < 1; i++) {
    //for (int i = 0; i < iter2 * iter2; i++) {
    //for (int i = 0; i < 100; i++) {
        nextIndex = stack[stackPointer];
        ivec4 closest = ivec4(getParticle(iResolution, iChannel1, iFrame, nextIndex));
        seen[i] = nextIndex;
        
        vec2 pos2 = getParticle(iResolution, iChannel0, iFrame, closest.x).xy;
        vec2 pos3 = getParticle(iResolution, iChannel0, iFrame, closest.y).xy;
        vec2 pos4 = getParticle(iResolution, iChannel0, iFrame, closest.z).xy;
        vec2 pos5 = getParticle(iResolution, iChannel0, iFrame, closest.w).xy;
        
        vec2 pos = closestPos;
        // color = vec3(0.0);
        
        
        if (nextIndex != -1) {
            vec4 closest = getParticle(iResolution, iChannel0, iFrame, nextIndex);
            vec2 v = closest.xy - closest.zw;
            colorBase = vmul(length(v));
        }
        
        bool lines = LINES;
        if (lines && length(pos2) > 0.0) {
            color += colorBase * drawLine(uv, pos, pos2);
            color += colorBase * drawLine(uv, pos, pos3);
            color += colorBase * drawLine(uv, pos, pos4);
            color += colorBase * drawLine(uv, pos, pos5);
        }
        
        {
            vec2 cmp = uv;
            
            bool seenX = false;
            bool seenY = false;
            bool seenZ = false;
            bool seenW = false;
            for (int j = 0; j <= i && j <= iter2; j++) {
                seenX = seenX || (seen[j] == closest.x);
                seenY = seenY || (seen[j] == closest.y);
                seenZ = seenZ || (seen[j] == closest.z);
                seenW = seenW || (seen[j] == closest.w);
            }
            
            float d2 = MAX_DIST;
            if (!seenX && closest.x != -1) {
            	nextIndex = closest.x;
            	closestPos = pos2;
                d2 = distance(uv, closestPos);
            }
            if (!seenY && closest.y != -1 && distance(uv, pos3) < d2) {
                nextIndex = closest.y;
            	closestPos = pos3;
                d2 = distance(uv, closestPos);
            }
            if (!seenZ && closest.z != -1 && distance(uv, pos4) < d2) {
                nextIndex = closest.z;
            	closestPos = pos4;
                d2 = distance(uv, closestPos);
            }
            if (!seenW && closest.w != -1 && distance(uv, pos5) < d2) {
                nextIndex = closest.w;
            	closestPos = pos5;
                d2 = distance(uv, closestPos);
            }
        }
        
        if (nextIndex == oldIndex) {
            stackPointer = stackPointer > 0 ? stackPointer - 1 : 0;
        } else {
            stackPointer++;
            stack[stackPointer] = nextIndex;
        }
        
        float d = distance(uv, closestPos);
        if (d < mind) {
            mind = d;
            closestIndex = nextIndex;
            // accumulative glow
            color += 0.1 * colorBase;
            if (mind <= md) {
            	color += colorBase;
            }
        }
        oldmind = mind;
        oldIndex = nextIndex;
    }
    
    //colorBase = vec3(1.0, 1.0, 1.0);
    
    colorBase = vec3(1.0, 1.0, 1.0);
    if (closestIndex != -1) {
        vec4 closest = getParticle(iResolution, iChannel0, iFrame, closestIndex);
        vec2 v = closest.xy - closest.zw;
        colorBase = vmul(length(v));
    }
    
    /*
    if (closestIndex > 0 && length(uv) < 0.5) {
        colorBase.r = hash(uvec2(closestIndex+0, 0));
        colorBase.g = hash(uvec2(closestIndex+1, 0));
        colorBase.b = hash(uvec2(closestIndex+2, 0));
    }*/
    //color += colorBase * max(0.0, 1.0 - mind / md * 0.5);
    //color += colorBase * max(0.0, 1000000000.0 * pow(abs(mind - 0.1), 10.0));
    if (mind <= md) {
        //color += vec3(1.0 - mind / md * 0.5);
        //color = vec3(1.0);
    	color += colorBase;
    }
    //color = clamp(color, 0.0, 1.0);
    //fragColor = texelFetch(iChannel2, ivec2(fragCoord), 0) * 0.9 + vec4(color, 1.0);
    //color *= 10.0;
    fragColor += vec4(color, 1.0);
    //fragColor = vec4(mind / md * 0.1);
    
    //fragColor = texelFetch(iChannel2, ivec2(fragCoord), 0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord) {
    int dd = iter;
    vec2 uv = dnpos(fragCoord / iResolution.xy);
    for (int dx = -dd; dx <= dd; dx++) {
        for (int dy = -dd; dy <= dd; dy++) {
            vec2 fc = vec2(fragCoord) + 10.0 * vec2(dx, dy);
            fc = clamp(fc, vec2(0.0), iResolution.xy - vec2(dd));
            mainImage2(fragColor, fc, uv);
        }
    }
}
// >>> ___ GLSL:[Buf A] ____________________________________________________________________ <<<
vec2 getContribution(in vec3 iResolution, in sampler2D iChannel0, in int frame, in int index, in vec2 pos) {
    vec2 pos2 = getParticle(iResolution, iChannel1, iFrame, index).xy;
    vec2 v = pos2 - pos;
    vec2 va = pos2 - vec2(0.0);
    float d = length(v);
    float d2 = d + 1.0;
    float dva = length(va);
    float dva2 = dva + 1.0;
    
    float k = 0.001;
    vec2 spring = (d < k * 2.0) ? 0.1 * v / abs(k - d) : vec2(0.0);
    vec2 test = d < 0.1 ? 0.0001 * v / (d * d2) : vec2(0.0);
    vec2 gravity = d < 0.1 ? 0.001 * v / (d2 * d2) : vec2(0.0);
    vec2 antigravity = 0.000001 * -v / (d2 * d2);
    
    vec2 downgravity = vec2(0.0, -0.0000001);
    
    vec2 center = 0.0001 * -va / (dva2 * dva2);
    vec2 anticenter = 0.00001 * va / (dva2 * dva2);
    //return spring + anticenter; //spring + antigravity;
    
    vec2 a = vec2(0.0);
    int mf = frame % 5;
    if (mf == 0) {
        a = spring;
    } else if (mf == 1) {
        a = test;
    } else if (mf == 2) {
        a = gravity;
    } else if (mf == 3) {
        a = antigravity;
    } else if (mf == 4) {
        a = anticenter;
    }
    
    a = spring + test + gravity; // + downgravity;
    //a  = spring + gravity + anticenter;
    //a = spring;
    float ma = 0.0001;
    a = clamp(a, -ma, ma);
    
    return a;
}

vec2 getWaveContribution(in vec3 iResolution, in sampler2D iChannel0, in int frame, in int index, in vec2 pos) {
    vec4 pos22 = getParticle(iResolution, iChannel1, iFrame, index);
    vec2 pos2 = pos22.xy;
    vec2 oldPos2 = pos22.zw;
    vec2 v = pos2 - oldPos2;
    return v;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // fragColor = vec4(0.0,0.0,1.0,1.0);
    //if (fragCoord.y > 0.1) return;
    vec2 uv = fragCoord / iResolution.xy;
    if (iFrame == 0 || texelFetch(iChannel1, ivec2(iResolution.xy) - ivec2(1.0), 0).xy == vec2(0.0)) {
        vec2 pos = dnpos(vec2(rand(fragCoord), rand(-fragCoord.yx)));
        vec2 oldPos = pos;
        fragColor = vec4(pos, oldPos);
    } else {
        vec4 old = texelFetch(iChannel1, ivec2(fragCoord), 0);
        vec2 oldPos = old.zw;
        vec2 pos = old.xy;
        vec2 v = pos - oldPos;
        
        vec2 a = vec2(0.0);
        
        ivec4 neighbourClosest = ivec4(texelFetch(iChannel2, ivec2(fragCoord), 0));

        a += getContribution(iResolution, iChannel1, iFrame, neighbourClosest.x, pos);
        a += getContribution(iResolution, iChannel1, iFrame, neighbourClosest.y, pos);
        a += getContribution(iResolution, iChannel1, iFrame, neighbourClosest.z, pos);
        a += getContribution(iResolution, iChannel1, iFrame, neighbourClosest.w, pos);
        for (int i = 0; i < forceIter * forceIter; i++) {
            ivec2 particleUV = getRandomParticlePos(iResolution, iChannel0, fragCoord, iFrame, i);
            a += getContribution(iResolution, iChannel1, iFrame, serializeUV(iResolution, iFrame, particleUV), pos);
        }
        vec2 vw = vec2(0.0);
        vw += getWaveContribution(iResolution, iChannel1, iFrame, neighbourClosest.x, pos);
        vw += getWaveContribution(iResolution, iChannel1, iFrame, neighbourClosest.y, pos);
        vw += getWaveContribution(iResolution, iChannel1, iFrame, neighbourClosest.z, pos);
        vw += getWaveContribution(iResolution, iChannel1, iFrame, neighbourClosest.w, pos);
        vw *= 0.25;
        v += 0.01 * vw;
        
        v += a;
        // friction
        v *= 0.99;
        
        oldPos = pos;
        if (pos.x + v.x <= -1.0 || pos.x + v.x >= 1.0) {
            v.x = -v.x;
            //v *= 0.5;
        }
        if (pos.y + v.y <= -1.0 || pos.y + v.y >= 1.0) {
            v.y = -v.y;
            //v *= 0.5;
        }
        pos += v;
        
        /*
        if (length(pos) < 0.01) {
            pos = vec2(0.5);
            oldPos = pos;
        	//pos = vec2(rand(fragCoord + 1.0), rand(-fragCoord.yx - 1.0));
        	//oldPos = pos;
        }*/
        
        fragColor = vec4(pos, oldPos);
    }
    //vec2 uv = fragCoord / iResolution.xy * 40.0;
    //fragColor = vec4(uv, uv);
}
// >>> ___ GLSL:[Buf B] ____________________________________________________________________ <<<
#define checkPos checkPos2

// 4 closest points
void checkPos1(in int i, in vec2 pos, in vec2 pos2, inout vec4 mind, inout ivec4 closest) {
    float d = distance(pos, pos2);
    if (d == 0.0) return;
    
    if (d < mind.x) {
        mind.w = mind.z;
        closest.w = closest.z;

        mind.z = mind.y;
        closest.z = closest.y;

        mind.y = mind.x;
        closest.y = closest.x;

        mind.x = d;
        closest.x = i;
    } else if (d < mind.y) {
        mind.w = mind.z;
        closest.w = closest.z;

        mind.z = mind.y;
        closest.z = closest.y;

        mind.y = d;
        closest.y = i;
    } else if (d < mind.z) {
        mind.w = mind.z;
        closest.w = closest.z;

        mind.z = d;
        closest.z = i;
    } else if (d < mind.w) {
        mind.w = d;
        closest.w = i;
    }
}

// closest points in each corner direction
void checkPos2(in int i, in vec2 pos, in vec2 pos2, inout vec4 mind, inout ivec4 closest) {
    float d = distance(pos, pos2);
    if (d == 0.0) return;
                
    if (d < mind.x && pos2.x < pos.x && pos2.y < pos.y) {
        mind.x = d;
        closest.x = i;
    }
    if (d < mind.y && pos2.x > pos.x && pos2.y > pos.y) {
        mind.y = d;
        closest.y = i;
    }
    if (d < mind.z && pos2.y < pos.y && pos2.x > pos.x) {
        mind.z = d;
        closest.z = i;
    }
    if (d < mind.w && pos2.y > pos.y && pos2.x < pos.x) {
        mind.w = d;
        closest.w = i;
    }
}

// closest points in each direction
void checkPos3(in int i, in vec2 pos, in vec2 pos2, inout vec4 mind, inout ivec4 closest) {
    float d = distance(pos, pos2);
    if (d == 0.0) return;
        
    if (d < mind.x && pos2.x < pos.x) {
        mind.x = d;
        closest.x = i;
    }
    if (d < mind.y && pos2.x > pos.x) {
        mind.y = d;
        closest.y = i;
    }
    if (d < mind.z && pos2.y < pos.y) {
        mind.z = d;
        closest.z = i;
    }
    if (d < mind.w && pos2.y > pos.y) {
        mind.w = d;
        closest.w = i;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 old = texelFetch(iChannel0, ivec2(fragCoord), 0);
    vec2 pos = old.xy;
    int self = int(fragCoord.x);
    vec4 mind = vec4(MAX_DIST);
    ivec4 closest = ivec4(-1);
    int frame = iFrame;
    if (iFrame > 0) {
        closest = ivec4(texelFetch(iChannel1, ivec2(fragCoord), 0));
        
        int oldFrame = iFrame - 1;
        //vec2 pos2 = getRandomParticle(iResolution, iChannel0, fragCoord, oldFrame, closest.x).xy;
        vec2 pos2 = getParticle(iResolution, iChannel0, frame, closest.x).xy;
        vec2 pos3 = getParticle(iResolution, iChannel0, frame, closest.y).xy;
        vec2 pos4 = getParticle(iResolution, iChannel0, frame, closest.z).xy;
        vec2 pos5 = getParticle(iResolution, iChannel0, frame, closest.w).xy;
        
        mind.x = closest.x != -1 ? distance(pos, pos2) : MAX_DIST;
        mind.y = closest.y != -1 ? distance(pos, pos3) : MAX_DIST;
        mind.z = closest.z != -1 ? distance(pos, pos4) : MAX_DIST;
        mind.w = closest.w != -1 ? distance(pos, pos5) : MAX_DIST;
    }
    for (int i = 0; i < iter * iter; i++) {
        // if (i == self) continue;
        //ivec2 p = ivec2(uvec2(fragCoord) + 1920U*1080U*uint(iFrame));
        ivec2 particleUV = getRandomParticlePos(iResolution, iChannel0, fragCoord, iFrame, i);
        vec4 particle = texelFetch(iChannel0, particleUV, 0); //getRandomParticle(iResolution, iChannel0, fragCoord, iFrame, i);
        vec2 pos2 = particle.xy;
        int index = serializeUV(iResolution, frame, particleUV);
        checkPos(index, pos, pos2, mind, closest);
        
        {
            //ivec2 particleUV = getRandomParticlePos(iResolution, iChannel0, fragCoord, iFrame, i);
        	ivec4 neighbourClosest = ivec4(texelFetch(iChannel1, ivec2(particleUV), 0));

            vec2 pos3 = getParticle(iResolution, iChannel0, frame, neighbourClosest.x).xy;
            vec2 pos4 = getParticle(iResolution, iChannel0, frame, neighbourClosest.y).xy;
            vec2 pos5 = getParticle(iResolution, iChannel0, frame, neighbourClosest.z).xy;
            vec2 pos6 = getParticle(iResolution, iChannel0, frame, neighbourClosest.w).xy;
            
            checkPos(neighbourClosest.x, pos, pos3, mind, closest);
            checkPos(neighbourClosest.y, pos, pos4, mind, closest);
            checkPos(neighbourClosest.z, pos, pos5, mind, closest);
            checkPos(neighbourClosest.w, pos, pos6, mind, closest);
			
        }
    }
    
    fragColor = vec4(closest);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const int xParticles = 200;
//#define xParticles min(1000, iResolution.y)
const int yParticles = xParticles;
//#define yParticles (20 + frame % 2)

const int particles = xParticles * yParticles;
const float md = max(0.005, 0.2 / float(xParticles)); // circle radius

const int iter = 2;
const int forceIter = 4;

const float MAX_DIST = 10.0;

const bool LINES = true;

// from https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float rand(vec2 n) {
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

// from iq
float hash(uvec2 x) {
    uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return float(n) * (1.0/float(0xffffffffU));
}


ivec2 getRandomParticlePos(in vec3 iResolution, in sampler2D iChannel0, in vec2 fragCoord, in int frame, int i) {
    //uvec2 b = uvec2(i / iter - iter / 2, i % iter - iter / 2);
    uvec2 b = uvec2(0);
    uvec2 p1 = uvec2(fragCoord) + b + uvec2(frame, 13 * frame);
    uvec2 p2 = uvec2(fragCoord.yx) + b + uvec2(29 * frame, frame);
    float f1 = hash(p1);
    float f2 = hash(p2);
    //int xp = min(xParticles, int(iResolution.y));
    //int yp = min(yParticles, int(iResolution.y)); // + 10 * (frame % 10 + 1);
    ivec2 p3 = ivec2(f1 * float(xParticles), f2 * float(yParticles));
    //p3 = ivec2(fragCoord);
    //i = (i + frame) % (iter * iter);
    p3 += ivec2(i / iter - iter / 2, i % iter - iter / 2);
    //p3 += ivec2(i / iter, i % iter);
    p3.x = abs(p3.x % xParticles);
    p3.y = abs(p3.y % yParticles);
    return p3;
}

int serializeUV(in vec3 iResolution, in int frame, ivec2 uv) {
    //int yp = min(yParticles, int(iResolution.y));
    return uv.x * yParticles + uv.y;
}

ivec2 deserializeUV(in vec3 iResolution, in int frame, int index) {
    //int yp = min(yParticles, int(iResolution.y));
    return ivec2(index / yParticles, index % yParticles);
}

vec4 getRandomParticle2(in vec3 iResolution, in sampler2D iChannel0, in vec2 fragCoord, in int frame, int i) {
	return texelFetch(iChannel0, getRandomParticlePos(iResolution, iChannel0, fragCoord, frame, i), 0);
}

// TODO: rename to getParticleData   
vec4 getParticle(in vec3 iResolution, in sampler2D iChannel0, in int frame, int index) {
    ivec2 uv = deserializeUV(iResolution, frame, index);
    //getRandomParticlePos(iResolution, iChannel0, fragCoord, frame, i)
	return texelFetch(iChannel0, uv, 0);
}

vec2 npos(vec2 pos) {
    return (pos + 1.0) / 2.0;
}

vec2 dnpos(vec2 pos) {
    return (pos - 0.5) * 2.0;
}

// from http://stackoverflow.com/questions/15276454/is-it-possible-to-draw-line-thickness-in-a-fragment-shader
#define Thickness 0.0005
float drawLine(vec2 uv, vec2 p1, vec2 p2) {
  float a = abs(distance(p1, uv));
  float b = abs(distance(p2, uv));
  float c = abs(distance(p1, p2));

  if ( a >= c || b >=  c ) return 0.0;

  float p = (a + b + c) * 0.5;

  // median to (p1, p2) vector
  float h = 2.0 / c * sqrt( p * ( p - a) * ( p - b) * ( p - c));

  return mix(1.0, 0.0, smoothstep(0.5 * Thickness, 1.5 * Thickness, h));
}
// >>> ___ GLSL:[Buf C] ____________________________________________________________________ <<<

int depthFirst(in int nextIndex, in vec2 closestPos, in vec2 uv, in float mind) {
    const int iter2 = iter * iter;
    int seen[iter2];
    for (int i = 0; i < iter2; i++) {
        seen[i] = -1;
    }
    int stack[iter2];
    for (int i = 0; i < iter2; i++) {
        stack[i] = -1;
    }
    
    int oldIndex = nextIndex;
    int stackPointer = 0;
    int closestIndex = nextIndex;
    stack[0] = nextIndex;
    for (int i = 0; i < iter2; i++) {
    //for (int i = 0; i < 100; i++) {
        nextIndex = stack[stackPointer];
        ivec4 closest = ivec4(getParticle(iResolution, iChannel1, iFrame, nextIndex));
        seen[i] = nextIndex;
        
        vec2 pos2 = getParticle(iResolution, iChannel0, iFrame, closest.x).xy;
        vec2 pos3 = getParticle(iResolution, iChannel0, iFrame, closest.y).xy;
        vec2 pos4 = getParticle(iResolution, iChannel0, iFrame, closest.z).xy;
        vec2 pos5 = getParticle(iResolution, iChannel0, iFrame, closest.w).xy;
        
        vec2 pos = closestPos;
        
        {
            vec2 cmp = uv;
            
            bool seenX = false;
            bool seenY = false;
            bool seenZ = false;
            bool seenW = false;
            for (int j = 0; j <= i && j <= iter2; j++) {
                seenX = seenX || (seen[j] == closest.x);
                seenY = seenY || (seen[j] == closest.y);
                seenZ = seenZ || (seen[j] == closest.z);
                seenW = seenW || (seen[j] == closest.w);
            }
            
            float d2 = MAX_DIST;
            if (!seenX && closest.x != -1) {
            	nextIndex = closest.x;
            	closestPos = pos2;
                d2 = distance(uv, closestPos);
            }
            if (!seenY && closest.y != -1 && distance(uv, pos3) < d2) {
                nextIndex = closest.y;
            	closestPos = pos3;
                d2 = distance(uv, closestPos);
            }
            if (!seenZ && closest.z != -1 && distance(uv, pos4) < d2) {
                nextIndex = closest.z;
            	closestPos = pos4;
                d2 = distance(uv, closestPos);
            }
            if (!seenW && closest.w != -1 && distance(uv, pos5) < d2) {
                nextIndex = closest.w;
            	closestPos = pos5;
                d2 = distance(uv, closestPos);
            }
            
            /*
            if (!seenX && closest.x != -1 && cmp.x < pos.x && cmp.y < pos.y) {
                nextIndex = closest.x;
                closestPos = pos2;
            }
            if (!seenY && closest.y != -1 && cmp.x > pos.x && cmp.y > pos.y) {
                nextIndex = closest.y;
                closestPos = pos3;
            }
            if (!seenZ && closest.z != -1 && cmp.y < pos.y && cmp.x > pos.x) {
                nextIndex = closest.z;
                closestPos = pos4;
            }
            if (!seenW && closest.w != -1 && cmp.y > pos.y && cmp.x < pos.x) {
                nextIndex = closest.w;
                closestPos = pos5;
            }*/
        }
        
        if (nextIndex == oldIndex) {
            stackPointer = stackPointer > 0 ? stackPointer - 1 : 0;
        } else {
            stackPointer++;
            stack[stackPointer] = nextIndex;
        }
        
        float d = distance(uv, closestPos);
        if (d < mind) {
            mind = d;
            closestIndex = nextIndex;
        }
        oldIndex = nextIndex;
    }
    return closestIndex;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = dnpos(fragCoord / iResolution.xy);
    vec3 color = vec3(0.0);
    
    float mind = MAX_DIST;
    int closestIndex = -1;
    vec2 closestPos = vec2(0.0);
    //ivec2 next = ivec2(0.0);
    //next.x = next.x % xParticles;
    //next.y = next.y % yParticles;
    
    int frame = iFrame;
    
    if (iFrame > 0) {
        int dd = 1;
        for (int dx = -dd; dx <= dd; dx++) {
            for (int dy = -dd; dy <= dd; dy++) {
                ivec2 niv = ivec2(fragCoord) + ivec2(dx, dy);
                int index = int(texelFetch(iChannel2, niv, 0).x);
                
                //ivec2 niv = getRandomParticlePos(iResolution, iChannel2, iv, frame, j);
                
                if (index == -1) continue;
                
                vec2 pos = getParticle(iResolution, iChannel0, iFrame, index).xy;
                float d = distance(uv, pos);
                
                if (d > 0.0 && d < mind) {
                    mind = d;
                    closestIndex = index;
                    closestPos = pos;
                }
            }
        }
        /*if (closestIndex != -1) {
        	return;
        }*/
    }
    
    for (int i = 0; i < iter * iter; i++) {
        vec2 iv = fragCoord;
        int j = i;
        /*
        if (iFrame > 0) {
            ivec2 niv = getRandomParticlePos(iResolution, iChannel2, iv, frame, j);
            j = int(texelFetch(iChannel2, niv, 0).x);
            if (j == -1) continue;
            
            vec2 pos = getParticle(iResolution, iChannel0, iFrame, j).xy;

            float d = distance(uv, pos);
            if (d > 0.0 && d < mind) {
                mind = d;
                closestIndex = j;
                closestPos = pos;
            }
        }*/
        
        ivec2 particleCoord = getRandomParticlePos(iResolution, iChannel0, iv, frame, i);
        vec4 particle = texelFetch(iChannel0, particleCoord, 0);
        // getRandomParticle(iResolution, iChannel0, fragCoord, iFrame + i);
        vec2 pos = particle.xy;
        
        float d = distance(uv, pos);
        if (d > 0.0 && d < mind) {
            mind = d;
            closestIndex = i;
            closestPos = pos;
        }
    }
    
    fragColor.x = float(depthFirst(closestIndex, closestPos, uv, mind));
    
    //fragColor.x = float(closestIndex);
    //fragColor.x = float(serializeUV(iFrame, getRandomParticlePos(iResolution, iChannel0, fragCoord, iFrame, 0)));
    //fragColor.x = 0.0;
    
    return;
}