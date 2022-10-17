

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

float t=0.;

float map(vec3 p) {
  
   float pl = p.y +
  	0.1 * texture(iChannel0, sin(t*.008)+t*.008+p.xz*.5).r +
  	0.2 * texture(iChannel0, cos(-t*.004)-t*.004+p.xz*.15).r +
  	0.9 * texture(iChannel0, sin( t*.020)+t*.010+p.xz*.05).r;

  	return pl;
}


vec3 raymarch(vec3 ro,  vec3 rd) {
  	float d = 0.;
  	vec3 p = ro;
  	float li=0.;
  	for(float i=0.; i<2000.; i++) {
    	float h = map(p); 
    	if(abs(h)<.002*d) return vec3(d,i,1);
    	if(d>100.) return vec3(d,i,0);
    	d+=h;
    	p+=rd*h;
        li = i;
  	}
  	return vec3(d, li, 0);
}

vec3 destroyed_normals(vec3 p) {
    const vec2 e = vec2(0.3,0.0);
    return normalize(map(p)-vec3(map(p-e.xyy), map(p-e.yxy), map(p-e.yyx)));
}

vec3 cam(vec2 uv, vec3 cameraPos, vec3 lookAtPoint, float z) {
  	vec3 cd = normalize(lookAtPoint - cameraPos); // camera direction
	vec3 cr = normalize(cross(vec3(0, 1, 0), cd)); // camera right
	vec3 cu = normalize(cross(cd, cr));
  	return normalize(cd*z+uv.x*cr+uv.y*cu);
}


vec3 phong(vec3 lightDir, vec3 normal, vec3 rd) {
  // ambient
  float k_a = 0.6;
  vec3 i_a = vec3(0.2, 0.5, 0.8);
  vec3 ambient = k_a * i_a;

  // diffuse
  float k_d = 0.7;
  float dotLN = clamp(dot(lightDir, normal), 0., 1.);
  vec3 i_d = vec3(0., 0.3, 0.7);
  vec3 diffuse = k_d * dotLN * i_d;

  // specular
  float k_s = 0.6;
  float dotRV = clamp(dot(reflect(lightDir, normal), -rd), 0., 1.);
  vec3 i_s = vec3(.2, 0.8, 1.);
  float alpha = 12.;
  vec3 specular = k_s * pow(dotRV, alpha) * i_s;

  return ambient + diffuse + specular;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    t = mod(iTime, 100.)-50.;
    vec2 uv = ((fragCoord/iResolution.xy)-0.5) / vec2(iResolution.y / iResolution.x, 1);
    
    vec3 ro = vec3(0,0.5,-2);
  	vec3 rd = vec3(0);
  	vec3 dir = cam(uv,ro,rd,.9);
  
  	vec3 lp = vec3(1,2,2);
  
  	vec3 col = vec3(0);
  	vec3 m = raymarch(ro, dir);
  	if(m.z == 1.) {
    	vec3 p = ro+dir*m.x;
    	vec3 n = destroyed_normals(p);
    	vec3 ld = normalize(lp-p);
        
      vec3 lightPosition1 = vec3(8, 2, -20);
      vec3 lightDirection1 = normalize(lightPosition1 - m);
      float lightIntensity1 = 0.75;
      
      col = lightIntensity1 * phong(lightDirection1, n, dir); 
      
  	} else {
    	col = cos(dir)*vec3(.8, .7, 1.1)*smoothstep(0.,.1,dir.y);
  	}
  	col += pow(m.y/70., 3.);
    
    fragColor = vec4(col,1.0);
}