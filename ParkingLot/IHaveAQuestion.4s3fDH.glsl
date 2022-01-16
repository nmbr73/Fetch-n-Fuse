

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define pi 3.1415927
#define phi 1.61803398875
mat3 rot (vec3 u) {
		vec3 s = sin(u), c = cos(u);
		mat3 x = mat3(1,0,0, 		0,c.x,s.x, 		0,-s.x,c.x);
		mat3 y = mat3(c.y,0,s.y, 	0,1,0, 			-s.y,0,c.y);
		mat3 z = mat3(s.z,c.z,0,	-c.z,s.z,0,		0,0,1);
		return x*y*z;}
void tri (vec3 p, vec3 d, mat3 tr, inout vec4 col, inout float depth, inout vec3 norm) {
    vec3 n = normalize(cross(tr[1]-tr[0],tr[2]-tr[0]));
    if (dot(n,-d)<0.) n *= -1.;
    mat3 nn = mat3(
    	normalize(cross(n,tr[2]-tr[1])),
    	normalize(cross(n,tr[0]-tr[2])),
    	normalize(cross(n,tr[1]-tr[0]))
    );
    vec3 w = p - tr[0];
    float x = -dot(w,n)/dot(d,n);
    if (x < 0.) return;
    vec3 i = p + d*x;
    vec3 ipol = vec3(dot(nn[0],i-tr[1]),dot(nn[1],i-tr[2]),dot(nn[2],i-tr[0]))/vec3(dot(nn[0],tr[0]-tr[1]),dot(nn[1],tr[1]-tr[2]),dot(nn[2],tr[2]-tr[0]));
    vec4 c = vec4(1);
    c.w *= step(0.,ipol.x);
    c.w *= step(0.,ipol.y);
    c.w *= step(0.,ipol.z);
    if (c.w==0.) return;
    if (x < depth) {
        vec3 u = normalize(reflect(p-i,n));
        c.xyz = texture(iChannel0,u).xyz*abs(u)-1.+smoothstep(0.02,0.025,ipol.x)*smoothstep(0.02,0.025,ipol.y)*smoothstep(0.02,0.025,ipol.z);
        col=c;
        depth = x;
        norm = n;
    }
}
void sph (vec3 p, vec3 d, vec4 cr, inout vec4 col, inout float depth, inout vec3 norm) {
	vec3 w = p-cr.xyz;
    float B = 2.*dot(w,d);
    float C = dot(w,w)-cr.w*cr.w;
    float dl = B*B-4.*C;
    if (dl < 0.) return;
    float x = 0.5*(-B-sqrt(dl));
    if (x < 0.) return;
    vec3 i = p + d*x;
    vec4 c = vec4(1);
    if (x < depth) {
        norm = normalize(i-cr.xyz);
        vec3 r = normalize(reflect(p-i,norm));
    	c.xyz = abs(r)*texture(iChannel0,r).xyz;
    	col = c;
        depth = x;
        
    }	
}
void scene (inout vec3 p, inout vec3 d, inout vec4 col, float i) {
	float depth = 1e3;
    vec3 norm=vec3(0);
    vec4 c = vec4(0);
    float t = iTime/pi;
    for (int i = 0; i < 4; i++) {
        tri(p,d,
            rot(pi*float(i)*vec3(1,0,0)*2./4.)*mat3(
               	4,0,0,
                0,4,0,
                0,0,4
            )
        ,c,depth,norm);
    	tri(p,d,
            rot(pi*float(i)*vec3(1,0,0)*2./4.)*mat3(
               	-4,0,0,
                0,-4,0,
                0,0,-4
            )
        ,c,depth,norm);
        tri(p,d,
            rot(vec3(t,0,0)+pi*float(i)*vec3(1,0,0)*2./4.)*mat3(
               	1,0,0,
                0,1,0,
                0,0,1
            )
        ,c,depth,norm);
        tri(p,d,
            rot(vec3(t,0,0)+pi*float(i)*vec3(1,0,0)*2./4.)*mat3(
               	-1,0,0,
                0,-1,0,
                0,0,-1
            )
        ,c,depth,norm);
        
        sph (p,d,vec4(sin(t+float(i)),cos(t+float(i)),sin(phi*t+float(i*i)),.2), c, depth, norm);
    } 
    p = p + d*depth;
    d = reflect(d,norm);
    p += 0.0001*d;
    col += 0.6*c/(1.+0.3*i);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = ((fragCoord)/iResolution.xy*2.-1.)*iResolution.xy/iResolution.yy;
	mat3 m = rot(vec3(1,2,3)*0.1*iTime);
    vec3 p = m*vec3(0,0,-2);
    vec3 d = m*normalize(vec3(uv,2));
    
    vec4 col = vec4(0,0,0,0);
    
    for (int i = 0; i < 10; i++) scene (p,d,col,float(i));
    
    
    fragColor = col;
}