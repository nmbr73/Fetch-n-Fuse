

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//cambios por jorge flores p.---->jorge2017a2
//21-feb-2022
///referencia y fork
//https://www.shadertoy.com/view/MtlfRs.....by zackpudil in 2017-12-11

float hash(vec2 n) {
	return fract(dot(vec2(sin(n.x*2343.34), cos(n.y*30934.0)), vec2(sin(n.y*309392.34), cos(n.x*3991.0))));
}

// Minkowski operators, can be seen at http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.90.803&rep=rep1&type=pdf
// It's a paper on generalized distance functions, but it intros on minkowski operators (generalizing them).
float len(vec3 p, float l) {
	p = pow(abs(p), vec3(l));
	return pow(p.x + p.y + p.z, 1.0/l);
}

vec2 GetDist(vec3 p  )
{
	vec3 q = p;
	vec2 c = floor((p.xz + 3.0)/6.0);
	
	q.xz = mod(q.xz + 3.0, 6.0) - 3.0;
	q.y -= 0.5;
    // use random value to produce different shape.
    //return vec2(min(len(q, 1.5 + 9.0*hash(c)) - 1.5, p.y + 1.0),1.0);
    float d1=len(q, 1.5 + 9.0*hash(c)) - 1.5;
    float d2=p.y + 1.0;
	
    return vec2(min(d1, d2),1.0);
}

// basic trace, with some LOD
float RayMarch(vec3 o, vec3 d, float m) {
	float t = 0.0;
	for(int i = 0; i < 200; i++) {
		float d = GetDist(o + d*t).x;
		if(d < (0.001 + 0.0001*t) || t >= m) break;
        t += d*0.67;
	}
	return t;
}

// basic normal.
vec3 normal(vec3 p) {
	vec2 h = vec2(0.001, 0.0);
	vec3 n = vec3(
		GetDist(p + h.xyy).x - GetDist(p - h.xyy).x,
		GetDist(p + h.yxy).x - GetDist(p - h.yxy).x,
		GetDist(p + h.yyx).x - GetDist(p - h.yyx).x
	);
	return normalize(n);
}

float getSoftShadow(vec3 p, vec3 lightPos)
{   float res = 1.0;
    float dist = 0.01;
    float lightSize = 0.03;
    for (int i = 0; i < 15; i++) {
        float hit = GetDist(p + lightPos * dist).x;
        res = min(res, hit / (dist * lightSize));
        dist += hit;
        if (hit < 0.0001 || dist > 60.0) break;
    }
    return clamp(res, 0.0, 1.0);
}

float ambOcclusion(vec3 pos, vec3 nor)
{   float sca = 2.0, occ = 0.0;
    for(int i = 0; i < 10; i++) {
        float hr = 0.01 + float(i) * 0.5 / 4.0;
        float dd = GetDist(nor * hr + pos).x;
        occ += (hr - dd)*sca;
        sca *= 0.6;
    }
    return clamp( 1.0 - occ, 0.0, 1.0 );
}

//euclidean cameras.
mat3 camera(vec3 o, vec3 l) 
{	vec3 w = normalize(l - o);
	vec3 u = normalize(cross(vec3(0, 1, 0), w));
	vec3 v = normalize(cross(w, u));	
	return mat3(u, v, w);
}

vec3 iluminacion(vec3 pos,vec3 rd,vec3 nor, vec3 ref, vec3 lig)
{
    vec3 rcol =vec3(0.0);    
    // occlusion and shadows
    float occ = ambOcclusion(pos, nor);
	//float sha = step(5.0, trace(pos + nor*0.001, lig, 5.0));
    float sha = getSoftShadow(pos, normalize(lig));
			
    // lighting ambient + diffuse + fresnel + specular
	rcol += 0.2*occ;
	rcol += clamp(dot(lig, nor), 0.0, 1.0)*occ*sha;
	rcol += pow(clamp(1.0 + dot(rd, nor), 0.0, 1.0), 2.0)*occ;
	rcol += 2.0*pow(clamp(dot(ref, lig), 0.0, 1.0), 30.0)*occ;
			
    // simple material.
	if(pos.y > -0.99)
        rcol *= vec3(1.2, 0.7, 0.7);
        //rcol *= vec3(0.92, 0.27, 0.57);
	else
    {
        rcol *= 0.2 + 0.5*mod(floor(pos.x) + floor(pos.z), 2.0);
        //rcol*=vec3(0.5);
     }
        
	return rcol;		
}        

vec3 render(vec3 ro, vec3 rd) 
{
    vec3 col = vec3(0.45, 0.8, 1.0);
	vec3 lig = normalize(vec3(0.8, 0.7, -0.6));
    float t;
    
    	for(int i = 0; i < 3; i++) {
		 t = RayMarch(ro, rd, 50.0);
		if(t < 50.0) 
        {
			vec3 rcol = vec3(0);
            // geometry, hit position, normal, reflect
			vec3 pos = ro + rd*t;
			vec3 nor = normal(pos);
			vec3 ref = reflect(rd, nor);
            rcol=iluminacion(pos,rd, nor, ref, lig);
            // set up the ray orgin and direction for reflection.
			ro = pos + nor*0.001;
			rd = ref;
            // sky fog.
			rcol = mix(rcol, vec3(0.45, 0.8, 1.0), 1.0 - exp(-0.00715*t));
            // lighten intensity on each successive reflect.
			if(i == 0)
               col = rcol;
			else
                col *= mix(rcol, vec3(1), 1.0 - exp(-0.8*float(i)));
		}
	}
    return col;
}


vec3 linear2srgb(vec3 c) 
{ return mix(12.92 * c,1.055 * pow(c, vec3(1.0/1.8)) - 0.055, step(vec3(0.0031308), c)); }

vec3 exposureToneMapping(float exposure, vec3 hdrColor) 
{ return vec3(1.0) - exp(-hdrColor * exposure); }


vec3 ACESFilm(vec3 x)
{   float a,b,c,d,e;
    a = 2.51; b = 0.03; c = 2.43; 
    d = 0.59; e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 p = (-iResolution.xy + 2.0*fragCoord)/iResolution.y;
	
	vec3 ro = vec3(2.7 + cos(iTime*0.3), 0, iTime);
	vec3 rd = camera(ro, vec3(2.5 + 0.9*cos(iTime*0.3 + 0.3), 0, iTime + 0.2))*normalize(vec3(p, 1.97));
	
    vec3 col= render(ro, rd);
        
    // tone mapping and gamma correction.
	//col = 1.0 - exp(-0.5*col);
	//col = pow(abs(col), vec3(1.0/2.2));
	col= (ACESFilm(col)+linear2srgb(col)+col+ exposureToneMapping(3.0, col))/4.0 ;
	fragColor = vec4(col, 1);
}