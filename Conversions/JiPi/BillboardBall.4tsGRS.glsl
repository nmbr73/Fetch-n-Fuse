

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[] _________________________________________________________________________ <<<
// Created by Stephane Cuillerdier - Aiekick/2015
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define USE_SPHERE_OR_BOX

//////2D FUNC TO MODIFY////////////////////
// from
vec3 effect(vec2 uv) 
{
    uv /=4.;
    vec2 s,m;
    s = iResolution.xy;
    float yVar = 5.*(sin(iTime*.01)*0.5+0.5);
    float grid = yVar * 10.+5.;
    float step_x = 0.0015625;
    float step_y = step_x * s.x / s.y;
	float offx = floor(uv.x  / (grid * step_x));
    float offy = floor(uv.y  / (grid * step_y));
    vec3 res = texture(iChannel2, vec2(offx * grid * step_x , offy * grid * step_y)).rgb;
    vec2 prc = fract(uv / vec2(grid * step_x, grid * step_y));
    vec2 pw = pow(abs(prc - 0.5), vec2(2.0));
    float  rs = pow(0.45, 2.0);
    float gr = smoothstep(rs - 0.1, rs + 0.1, pw.x + pw.y);
    float y = (res.r + res.g + res.b) / 3.0;
    vec3 ra = res / y;
    float ls = 0.3;
    float lb = ceil(y / ls);
    float lf = ls * lb + 0.3;
    res = lf * res;
    vec3 col = mix(res, vec3(0.1, 0.1, 0.1), gr);
    return col;
}

///////FRAMEWORK////////////////////////////////////
vec4 displacement(vec3 p)
{
    vec3 col = effect(p.xz);
    
    col = clamp(col, vec3(0), vec3(1.));
    
    float dist = dot(col,vec3(0.1));
    
    return vec4(dist,col);
}

////////BASE OBJECTS///////////////////////
float obox( vec3 p, vec3 b ){ return length(max(abs(p)-b,0.0));}
float osphere( vec3 p, float r ){ return length(p)-r;}
////////MAP////////////////////////////////
vec4 map(vec3 p)
{
   	float scale = 3.;
    float dist = 0.;
    
    float x = 6.;
    float z = 6.;
    
    vec4 disp = displacement(p);
        
    float y = 1. - smoothstep(0., 1., disp.x) * scale;
    
    #ifdef USE_SPHERE_OR_BOX
        dist = osphere(p, +5.-y);
    #else    
        if ( p.y > 0. ) dist = obox(p, vec3(x,1.-y,z));
        else dist = obox(p, vec3(x,1.,z));
	#endif
    
    return vec4(dist, disp.yzw);
}

///////////////////////////////////////////
//FROM IQ Shader https://www.shadertoy.com/view/Xds3zN
float softshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<16; i++ )
    {
		float h = map( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.1, 0., 0. );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
}

float calcAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}

///////////////////////////////////////////
float march(vec3 ro, vec3 rd, float rmPrec, float maxd, float mapPrec)
{
    float s = rmPrec;
    float d = 0.;
    for(int i=0;i<150;i++)
    {      
        if (s<rmPrec||s>maxd) break;
        s = map(ro+rd*d).x*mapPrec;
        d += s;
    }
    return d;
}

////////MAIN///////////////////////////////
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime*0.2;
    float cam_a = time; // angle z
    
    #ifdef USE_SPHERE_OR_BOX
        float cam_e = 5.52; // elevation
        float cam_d = 1.88; // distance to origin axis
   	#else
        float cam_e = 1.; // elevation
        float cam_d = 1.8; // distance to origin axis
    #endif
    
    vec3 camUp=vec3(0,1,0);//Change camere up vector here
  	vec3 camView=vec3(0,0,0); //Change camere view here
  	float li = 0.6; // light intensity
    float prec = 0.00001; // ray marching precision
    float maxd = 50.; // ray marching distance max
    float refl_i = 0.45; // reflexion intensity
    float refr_a = 0.7; // refraction angle
    float refr_i = 0.8; // refraction intensity
    float bii = 0.35; // bright init intensity
    float marchPrecision = 0.5; // ray marching tolerance precision
    
    vec2 uv = fragCoord.xy / iResolution.xy * 2. -1.;
    uv.x*=iResolution.x/iResolution.y;
    
    vec3 col = vec3(0.);
    
    vec3 ro = vec3(-sin(cam_a)*cam_d, cam_e+1., cos(cam_a)*cam_d); //
  	vec3 rov = normalize(camView-ro);
    vec3 u = normalize(cross(camUp,rov));
  	vec3 v = cross(rov,u);
  	vec3 rd = normalize(rov + uv.x*u + uv.y*v);
    
    float b = bii;
    
    float d = march(ro, rd, prec, maxd, marchPrecision);
    
    if (d<maxd)
    {
        vec2 e = vec2(-1., 1.)*0.005; 
    	vec3 p = ro+rd*d;
        vec3 n = calcNormal(p);
        
        b=li;
        
        vec3 reflRay = reflect(rd, n);
		vec3 refrRay = refract(rd, n, refr_a);
        
        vec3 cubeRefl = texture(iChannel0, reflRay).rgb * refl_i;
        vec3 cubeRefr = texture(iChannel0, refrRay).rgb * refr_i;
        
        col = cubeRefl + cubeRefr + pow(b, 15.);
        
       	// lighting        
        float occ = calcAO( p, n );
		vec3  lig = normalize( vec3(-0.6, 0.7, -0.5) );
		float amb = clamp( 0.5+0.5*n.y, 0.0, 1.0 );
        float dif = clamp( dot( n, lig ), 0.0, 1.0 );
        float bac = clamp( dot( n, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-p.y,0.0,1.0);
        float dom = smoothstep( -0.1, 0.1, reflRay.y );
        float fre = pow( clamp(1.0+dot(n,rd),0.0,1.0), 2.0 );
		float spe = pow(clamp( dot( reflRay, lig ), 0.0, 1.0 ),16.0);
        
        dif *= softshadow( p, lig, 0.02, 2.5 );
       	dom *= softshadow( p, reflRay, 0.02, 2.5 );

		vec3 brdf = vec3(0.0);
        brdf += 1.20*dif*vec3(1.00,0.90,0.60);
		brdf += 1.20*spe*vec3(1.00,0.90,0.60)*dif;
        brdf += 0.30*amb*vec3(0.50,0.70,1.00)*occ;
        brdf += 0.40*dom*vec3(0.50,0.70,1.00)*occ;
        brdf += 0.30*bac*vec3(0.25,0.25,0.25)*occ;
        brdf += 0.40*fre*vec3(1.00,1.00,1.00)*occ;
		brdf += 0.02;
		col = col*brdf;

    	col = mix( col, vec3(0.8,0.9,1.0), 1.0-exp( -0.0005*d*d ) );
        
       	col = mix(col, map(p).yzw, 0.5);
    }
    else
    {
        b+=0.1;
        col = texture(iChannel0, rd).rgb;
    }
    
	fragColor.rgb = col;
}