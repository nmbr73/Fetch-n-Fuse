

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float df(vec3 p)
{
	float a = p.z * cos(p.z * 2.0) * 0.001;
	p.xy *= mat2(cos(a),sin(a),-sin(a), cos(a));
	p.y += sin(p.x * .5 + p.z * 0.5);
	float sp = 10. - abs(p.y * 0.5);
	float cy = max(1.0-0.2*sin(p.z*0.5)-abs(p.y*0.5),length(mod(p.xz, 1.)-0.5) - 0.2);
    return min(sp,cy);
}

vec3 nor( vec3 pos, float prec )
{
	vec3 eps = vec3( prec, 0., 0. );
	vec3 nor = vec3(
	    df(pos+eps.xyy) - df(pos-eps.xyy),
	    df(pos+eps.yxy) - df(pos-eps.yxy),
	    df(pos+eps.yyx) - df(pos-eps.yyx) );
	return normalize(nor);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 g = fragCoord;
	vec2 si = iResolution.xy;
	vec2 uv = (g+g-si)/si.y/0.8;
	vec3 ro = vec3(0,0, (iTime + 30.) * 2.); 
    vec3 cv = ro + vec3(0,0,1); 
	vec3 cu = normalize(vec3(0,1,0));
  	vec3 z = normalize(cv-ro);
    vec3 x = normalize(cross(cu,z));
  	vec3 y = cross(z,x);
    float fov = .9;
  	vec3 rd = normalize(fov * (uv.x * x + uv.y * y) + z);
	
	float s = 1., d = 0.;
	for (int i=0; i<150; i++) 
	{
		if (log(d*d/s/1e6)>0.) break;
		s=df(ro+rd*d);
		d += s * .15;
	}
	
	vec3 p = ro + rd * d;
	vec3 lid = normalize(ro-p);
	vec3 n = nor(p, 0.1);
	vec3 refl = reflect(rd,n);
	float diff = clamp( dot( n, lid ), 0.0, 1.0 );
	float fre = pow( clamp( 1. + dot(n,rd),0.0,1.0), 4. );
	float spe = pow(clamp( dot( refl, lid ), 0.0, 1.0 ),16.);
	vec3 col = vec3(.8,.5,.2);
    
    float sss = df(p - n*0.001)/0.05;
	
	fragColor.rgb = mix(
	(diff * vec3(0.8,0.2,0.5) + fre + sss * vec3(0.8,0.5,0.5)) * 0.25,
	vec3(spe * 0.5), 
	exp(-0.01 * d*d));

	vec2 q = g/si;
    fragColor.rgb *= 0.5 + 0.5*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.55 );
}
