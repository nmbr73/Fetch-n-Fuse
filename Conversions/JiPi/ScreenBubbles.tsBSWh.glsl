

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void mainImage( out vec4 Q, in vec2 U)
{
    vec3 light = vec3(2.5*R,1e5);
    vec3 me    = vec3(U,0);
	vec3 r = vec3(U,100);
    vec4 a = A(U);
    vec4 b = B(U);
    float 
  		n = A(U+vec2(0,1)).z,
  		e = A(U+vec2(1,0)).z,
  		s = A(U-vec2(0,1)).z,
  		w = A(U-vec2(1,0)).z;
   	vec3 no = normalize(vec3(e-w,n-s,-2.));
    vec3 li = reflect((r-light),no);
   	float o = ln(me,r,li);
    vec2 u = U-b.xy;
    u *= mat2(cos(b.w),-sin(b.w),sin(b.w),cos(b.w));
    Q = abs(sin(10.*a.w+.3*a.z*vec4(1,2,3,4)+smoothstep(4.,3.,abs(length(U-b.xy)-b.z))));
    Q *= smoothstep(-1.,-2.,length(U-b.xy)-b.z);
    Q += exp(-2.*o);
    Q *= .8;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 D (vec2 U) {return texture(iChannel0,U/R);}
vec4 T (vec2 U) {
	return A(U-A(U).xy);
}
float ln (vec2 p, vec2 a, vec2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = T(U);
    vec4 b = B(U),
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0));
   Q.x -= .25*(e.z-w.z+Q.w*(n.w-s.w));
   Q.y -= .25*(n.z-s.z+Q.w*(e.w-w.w));
   Q.z  = .25*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   Q.w  = .25*((s.x-n.x+w.y-e.y)-Q.w);
   float p = smoothstep(2.,0.,length(U-b.xy)-b.z);
   Q.z += .03*p;
   Q.z*=0.975;
   vec4 mo = texture(iChannel3,vec2(0));
   float l = ln(U,mo.xy,mo.zw);
   if (mo.z > 0. && length(mo.xy-mo.zw)>0.) Q.xy += .01*(mo.xy-mo.zw)*smoothstep(40.,0.,l);

   if (U.x < 2. || U.y < 2. || R.x-U.x<2.) Q.xy=mix(Q.xy,vec2(-.3,0),.99);
   if (iFrame < 1) {
       Q = vec4(-.3,0,0,0);
   }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define R iResolution.xy
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 D (vec2 U) {return texture(iChannel0,U/R);}
void swap (vec2 U, inout vec4 A, vec4 B) {if (length(U-B.xy)-B.z < length(U-A.xy)-A.z) A = B;}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = B(U);
    
    swap(U,Q,B(U+vec2(0,1)));
    swap(U,Q,B(U+vec2(1,0)));
    swap(U,Q,B(U-vec2(0,1)));
    swap(U,Q,B(U-vec2(1,0)));
    swap(U,Q,B(U+vec2(2,2)));
    swap(U,Q,B(U+vec2(2,-2)));
    swap(U,Q,B(U-vec2(2,-2)));
    swap(U,Q,B(U-vec2(2,2)));
    
    Q.xyw += A(Q.xy).xyw*vec3(1,1,6.2);
    
    if (R.x-U.x < 1. && mod(float(iFrame) , 20.) == 1.) {
        float y = round((U.y+5.)/10.)*10.-5.;
        Q = vec4(
            R.x,y,
        0.5+0.5*sin(y+(y+.45)*mod(float(iFrame),1e3)),0.
       );
       Q.z = -1.5*log(1e-4+Q.z);
    }
    
    
   if (iFrame < 1) {
       Q = vec4(0);
   }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 p = texture(iChannel2,U/iResolution.xy);
   	if (iMouse.z>0.) {
      if (p.z>0.) Q =  vec4(iMouse.xy,p.xy);
    	else Q =  vec4(iMouse.xy,iMouse.xy);
   	}else Q = vec4(-iResolution.xy,-iResolution.xy);
}