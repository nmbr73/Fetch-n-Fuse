

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by David Gallardo - xjorma/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0


#define AA
#define GAMMA 1

const vec3 light = vec3(0.,4.,2.);
const float boxHeight = 0.45;

/*
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 p = ivec2(fragCoord);
    float h = texelFetch(iChannel0, p, 0).x;
    float w = texelFetch(iChannel1, p, 0).x; 
    fragColor = vec4(h, w, w, 1.0);
}*/


vec2 getHeight(in vec3 p)
{
    p = (p + 1.0) * 0.5;
    vec2 p2 = p.xz * vec2(float(textureSize)) / iResolution.xy;
    p2 = min(p2, vec2(float(textureSize) - 0.5) / iResolution.xy);
	//float h = texture(iChannel0, p2).x;
	//float w = h + texture(iChannel1, p2).x;
    vec2 h = texture(iChannel0, p2).xy;
    h.y += h.x;
	return h - boxHeight;
} 

vec3 getNormal(in vec3 p, int comp)
{
    float d = 2.0 / float(textureSize);
    float hMid = getHeight(p)[comp];
    float hRight = getHeight(p + vec3(d, 0, 0))[comp];
    float hTop = getHeight(p + vec3(0, 0, d))[comp];
    return normalize(cross(vec3(0, hTop - hMid, d), vec3(d, hRight - hMid, 0)));
}

vec3 terrainColor(in vec3 p, in vec3 n, out float spec)
{
    spec = 0.1;
    vec3 c = vec3(0.21, 0.50, 0.07);
    float cliff = smoothstep(0.8, 0.3, n.y);
    c = mix(c, vec3(0.25), cliff);
    spec = mix(spec, 0.3, cliff);
    float snow = smoothstep(0.05, 0.25, p.y) * smoothstep(0.5, 0.7, n.y);
    c = mix(c, vec3(0.95, 0.95, 0.85), snow);
    spec = mix(spec, 0.4, snow);
    vec3 t = texture(iChannel1, p.xz * 5.0).xyz;
    return mix(c, c * t, 0.75);
}

vec3 undergroundColor(float d)
{
    vec3 color[4] = vec3[](vec3(0.5, 0.45, 0.5), vec3(0.40, 0.35, 0.25), vec3(0.55, 0.50, 0.4), vec3(0.45, 0.30, 0.20));
    d *= 6.0;
    d = min(d, 3.0 - 0.001);
    float fr = fract(d);
    float fl = floor(d);
    return mix(color[int(fl)], color[int(fl) + 1], fr);
}



vec3 Render(in vec3 ro, in vec3 rd)
{
    vec3 n;
    vec2 ret = boxIntersection(ro, rd, vec3(1, boxHeight, 1), n);
    if(ret.x > 0.0)
    {
        vec3 pi = ro + rd * ret.x;
        // Find Terrain
        vec3 tc;
        vec3 tn;
        float tt = ret.x;
        vec2 h = getHeight(pi);
        float spec;
        if(pi.y < h.x)
        {
            tn = n;
            tc = undergroundColor(h.x - pi.y);
        }
        else
        {
            for (int i = 0; i < 80; i++)
            {
                vec3 p = ro + rd * tt;
                float h = p.y - getHeight(p).x;
                if (h < 0.0002 || tt > ret.y)
                    break;
                tt += h * 0.4;
            }
            tn = getNormal(ro + rd * tt, 0);
            tc = terrainColor(ro + rd * tt, tn, spec);
        }
        
        {
            vec3 lightDir = normalize(light - (ro + rd * tt));
            tc = tc * (max( 0.0, dot(lightDir, tn)) + 0.3);
            spec *= pow(max(0., dot(lightDir, reflect(rd, tn))), 10.0);
            tc += spec;            
        }
        
        if(tt > ret.y)
        {
            tc = backgroundColor;
        }
        
        // Find Water
        float wt = ret.x;
        h = getHeight(pi);
        vec3 waterNormal;
        if(pi.y < h.y)
        {
            waterNormal = n;
        }
        else
        {
            for (int i = 0; i < 80; i++)
            {
                vec3 p = ro + rd * wt;
                float h = p.y - getHeight(p).y;
                if (h < 0.0002 || wt > min(tt, ret.y))
                    break;
                wt += h * 0.4;
            }
            waterNormal = getNormal(ro + rd * wt, 1);
        }
        
        if(wt < ret.y)
        {
            float dist = (min(tt, ret.y) - wt);
            vec3 p = waterNormal;
            vec3 lightDir = normalize(light - (ro + rd * wt));
                        
            tc = applyFog( tc, vec3(0,0,0.4), dist * 15.0);

            float spec = pow(max(0., dot(lightDir, reflect(rd, waterNormal))), 20.0);
            tc += 0.5 * spec * smoothstep(0.0, 0.1, dist);
        }

        
        return tc;
    }
   
    return backgroundColor;
}


mat3 setCamera( in vec3 ro, in vec3 ta )
{
	vec3 cw = normalize(ta-ro);
	vec3 up = vec3(0, 1, 0);
	vec3 cu = normalize( cross(cw,up) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}


vec3 vignette(vec3 color, vec2 q, float v)
{
    color *= 0.3 + 0.8 * pow(16.0 * q.x * q.y * (1.0 - q.x) * (1.0 - q.y), v);
    return color;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec3 tot = vec3(0.0);
    
    vec2 mouse = iMouse.xy;
    if(length(mouse.xy) < 10.0)
        mouse = iResolution.xy * 0.5;
        
#ifdef AA
	vec2 rook[4];
    rook[0] = vec2( 1./8., 3./8.);
    rook[1] = vec2( 3./8.,-1./8.);
    rook[2] = vec2(-1./8.,-3./8.);
    rook[3] = vec2(-3./8., 1./8.);
    for( int n=0; n<4; ++n )
    {
        // pixel coordinates
        vec2 o = rook[n];
        vec2 p = (-iResolution.xy + 2.0*(fragCoord+o))/iResolution.y;
#else //AA
        vec2 p = (-iResolution.xy + 2.0*fragCoord)/iResolution.y;
#endif //AA
 
        // camera
        
        float theta	= radians(360.)*(mouse.x/iResolution.x-0.5) + iTime*.2;
        float phi	= radians(90.)*(mouse.y/iResolution.y-0.5)-1.;
        vec3 ro = 2.0 * vec3( sin(phi)*cos(theta),cos(phi),sin(phi)*sin(theta));
        //vec3 ro = vec3(0.0,.2,4.0);
        vec3 ta = vec3( 0 );
        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta );
        //vec3 cd = ca[2];    
        
        vec3 rd =  ca*normalize(vec3(p,1.5));        
        
        vec3 col = Render(ro, rd);
        
        tot += col;
            
#ifdef AA
    }
    tot /= 4.;
#endif
    
    tot = vignette(tot, fragCoord / iResolution.xy, 0.6);
    #if GAMMA
    	tot = pow(tot, vec3(1. / 2.2));
    #endif

	fragColor = vec4( tot, 1.0 );
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const int textureSize = 256;
// Render
const vec3 backgroundColor = vec3(0.2);
// Terrain
const float transitionTime = 5.0;
const float transitionPercent = 0.3;
const int octaves = 7;
// Water simulation
const float attenuation = 0.995;
const float strenght = 0.25;
const float minTotalFlow = 0.0001;
const float initialWaterLevel = 0.05;

mat2 rot(in float ang) 
{
   return mat2(
			cos(ang), -sin(ang),
			sin(ang),  cos(ang));
}

// hash from Dave_Hoskins https://www.shadertoy.com/view/4djSRW
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

// Box intersection by IQ https://iquilezles.org/articles/boxfunctions

vec2 boxIntersection( in vec3 ro, in vec3 rd, in vec3 rad, out vec3 oN ) 
{
    vec3 m = 1.0 / rd;
    vec3 n = m * ro;
    vec3 k = abs(m) * rad;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
	
    if( tN > tF || tF < 0.0) return vec2(-1.0); // no intersection
    
    oN = -sign(rd)*step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz);

    return vec2( tN, tF );
}


// Fog by IQ https://iquilezles.org/articles/fog

vec3 applyFog( in vec3  rgb, vec3 fogColor, in float distance)
{
    float fogAmount = exp( -distance );
    return mix( fogColor, rgb, fogAmount );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// compute Terrain and update water level 1st pass


float boxNoise( in vec2 p, in float z )
{
    vec2 fl = floor(p);
    vec2 fr = fract(p);
    fr = smoothstep(0.0, 1.0, fr);    
    float res = mix(mix( hash13(vec3(fl, z)),             hash13(vec3(fl + vec2(1,0), z)),fr.x),
                    mix( hash13(vec3(fl + vec2(0,1), z)), hash13(vec3(fl + vec2(1,1), z)),fr.x),fr.y);
    return res;
}

float Terrain( in vec2 p, in float z, in int octaveNum)
{
	float a = 1.0;
	float f = .0;
	for (int i = 0; i < octaveNum; i++)
	{
		f += a * boxNoise(p, z);
		a *= 0.45;
		p = 2.0 * rot(radians(41.0)) * p;
	}
	return f;
}

vec2 readHeight(ivec2 p)
{
	p = clamp(p, ivec2(0), ivec2(textureSize - 1));
	return texelFetch(iChannel0, p, 0).xy;
} 

vec4 readOutFlow(ivec2 p)
{
	if(p.x < 0 || p.y < 0 || p.x >= textureSize || p.y >= textureSize)
		return vec4(0);
	return texelFetch(iChannel1, p, 0);
} 

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Outside ?
    if( max(fragCoord.x, fragCoord.y) > float(textureSize) )
        discard;
           
    // Terrain
    vec2 uv = fragCoord / float(textureSize);
    float t = iTime / transitionTime;
    float terrainElevation = mix(Terrain(uv * 4.0, floor(t), octaves), Terrain(uv * 4.0, floor(t) + 1.0, octaves), smoothstep(1.0 - transitionPercent, 1.0, fract(t))) * 0.5;
    
    // Water
    float waterDept = initialWaterLevel;
    if(iFrame != 0)
    {
        ivec2 p = ivec2(fragCoord);
        vec2 height = readHeight(p);
        vec4 OutFlow = texelFetch(iChannel1, p, 0);
        float totalOutFlow = OutFlow.x + OutFlow.y + OutFlow.z + OutFlow.w;
        float totalInFlow = 0.0;
        totalInFlow += readOutFlow(p  + ivec2( 1,  0)).z;
        totalInFlow += readOutFlow(p  + ivec2( 0,  1)).w;
        totalInFlow += readOutFlow(p  + ivec2(-1,  0)).x;
        totalInFlow += readOutFlow(p  + ivec2( 0, -1)).y;
        waterDept = height.y - totalOutFlow + totalInFlow;
    }
    fragColor = vec4(terrainElevation, waterDept, 0, 1);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Update Outflow 1st pass

vec2 readHeight(ivec2 p)
{
	p = clamp(p, ivec2(0), ivec2(textureSize - 1));
	return texelFetch(iChannel0, p, 0).xy;
} 

float computeOutFlowDir(vec2 centerHeight, ivec2 pos)
{
	vec2 dirHeight = readHeight(pos);
	return max(0.0f, (centerHeight.x + centerHeight.y) - (dirHeight.x + dirHeight.y));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 p = ivec2(fragCoord);
    // Init to zero at frame 0
    if(iFrame == 0)
    {
        fragColor = vec4(0);
        return;
    }    
    
    // Outside ?
    if( max(p.x, p.y) > textureSize )
        discard;
        
    
   	vec4 oOutFlow = texelFetch(iChannel1, p, 0);
	vec2 height = readHeight(p);
	vec4 nOutFlow;        
	nOutFlow.x = computeOutFlowDir(height, p + ivec2( 1,  0));
	nOutFlow.y = computeOutFlowDir(height, p + ivec2( 0,  1));
	nOutFlow.z = computeOutFlowDir(height, p + ivec2(-1,  0));
	nOutFlow.w = computeOutFlowDir(height, p + ivec2( 0, -1));
	nOutFlow = attenuation * oOutFlow + strenght * nOutFlow;
	float totalFlow = nOutFlow.x + nOutFlow.y + nOutFlow.z + nOutFlow.w;
	if(totalFlow > minTotalFlow)
	{
		if(height.y < totalFlow)
		{
			nOutFlow = nOutFlow * (height.y / totalFlow);
		}
	}
	else
	{
		nOutFlow = vec4(0);
	}


    fragColor = nOutFlow;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// water level 2nd pass

vec2 readHeight(ivec2 p)
{
	p = clamp(p, ivec2(0), ivec2(textureSize - 1));
	return texelFetch(iChannel0, p, 0).xy;
} 

vec4 readOutFlow(ivec2 p)
{
	if(p.x < 0 || p.y < 0 || p.x >= textureSize || p.y >= textureSize)
		return vec4(0);
	return texelFetch(iChannel1, p, 0);
} 

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Outside ?
    if( max(fragCoord.x, fragCoord.y) > float(textureSize) )
        discard;
           
    // Water
    ivec2 p = ivec2(fragCoord);
    vec2 height = readHeight(p);
    vec4 OutFlow = texelFetch(iChannel1, p, 0);
    float totalOutFlow = OutFlow.x + OutFlow.y + OutFlow.z + OutFlow.w;
    float totalInFlow = 0.0;
    totalInFlow += readOutFlow(p  + ivec2( 1,  0)).z;
    totalInFlow += readOutFlow(p  + ivec2( 0,  1)).w;
    totalInFlow += readOutFlow(p  + ivec2(-1,  0)).x;
    totalInFlow += readOutFlow(p  + ivec2( 0, -1)).y;
    float waterDept = height.y - totalOutFlow + totalInFlow;

    fragColor = vec4(height.x, waterDept, 0, 1);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Update Outflow 2nd pass

vec2 readHeight(ivec2 p)
{
	p = clamp(p, ivec2(0), ivec2(textureSize - 1));
	return texelFetch(iChannel0, p, 0).xy;
} 

float computeOutFlowDir(vec2 centerHeight, ivec2 pos)
{
	vec2 dirHeight = readHeight(pos);
	return max(0.0f, (centerHeight.x + centerHeight.y) - (dirHeight.x + dirHeight.y));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 p = ivec2(fragCoord);
    
    // Outside ?
    if( max(p.x, p.y) > textureSize )
        discard;
        
    
   	vec4 oOutFlow = texelFetch(iChannel1, p, 0);
	vec2 height = readHeight(p);
	vec4 nOutFlow;        
	nOutFlow.x = computeOutFlowDir(height, p + ivec2( 1,  0));
	nOutFlow.y = computeOutFlowDir(height, p + ivec2( 0,  1));
	nOutFlow.z = computeOutFlowDir(height, p + ivec2(-1,  0));
	nOutFlow.w = computeOutFlowDir(height, p + ivec2( 0, -1));
	nOutFlow = attenuation * oOutFlow + strenght * nOutFlow;
	float totalFlow = nOutFlow.x + nOutFlow.y + nOutFlow.z + nOutFlow.w;
	if(totalFlow > minTotalFlow)
	{
		if(height.y < totalFlow)
		{
			nOutFlow = nOutFlow * (height.y / totalFlow);
		}
	}
	else
	{
		nOutFlow = vec4(0);
	}


    fragColor = nOutFlow;
}