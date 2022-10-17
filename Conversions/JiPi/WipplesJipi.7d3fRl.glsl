

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//http://dept-info.labri.fr/~schlick/DOC/gem2.ps.gz
float bias(float x, float b) {
	return  x/((1./b-2.)*(1.-x)+1.);
}

float gain(float x, float g) {
	float t = (1./g-2.)*(1.-(2.*x));	
	return x<0.5 ? (x/(t+1.)) : (t-x)/(t-1.);
}

vec3 degamma(vec3 c)
{
	return pow(c,vec3(2.2));
}
vec3 gamma(vec3 c)
{
	return pow(c,vec3(1./1.6));
}

#define pi 3.1415927

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	uv.y=1.-uv.y;
	uv.x *= iResolution.x / iResolution.y;

	float h = 0.;

#if 1
	float time = iTime;
#else
	//go forwards and backwards!	
	float time = mod(iTime,30.);
	if (time > 15.) time = 30.-time;
#endif
	

#define DIVS	8
	
	for (int iy=0; iy<DIVS; iy++)
	{
		for (int ix=0; ix<DIVS*2; ix++)
		{
			//random variations for droplet
			vec4 t = texture(iChannel1,(4./256.)*vec2(float(ix),float(iy)),-100.);
			
			
			//stratify droplet positions
			vec2 p = vec2(ix,iy)*(1./float(DIVS-1));
			p += (0.75/float(DIVS-1))*(t.xy*2.-1.);
				
			//radius
			vec2 v = uv-p;
			float d = dot(v,v);
			d = pow(d,.7);
			float life = 10.;
			
			float n = time*5.*(t.w+0.2) - t.z*6.;
			n *= 0.1+ t.w;
			n = mod(n,life+t.z*3.+10.);				//repeat, plus a pause
			float x = d*99.;
			float T = x<(2.*pi*n) ? 1. : 0.;	//clip to 0 after end
			float e = max(1. - (n/life),0.);		//entirely fade out by now
			float F = e*x/(2.*pi*n);				//leading edge stronger and decay
			
			float s = sin(x-(2.*pi*n)-pi*0.5);
						   
			s = s*0.5+0.5;		//bias needs [0,1]
			s = bias(s,.6);	//shape the ripple profile
			
			
			s = (F*s)/(x+1.1)*T;			

			h+=s*100.*(0.5+t.w);			

		}
	}

	
	vec3 n = vec3(dFdx(h),17.,dFdy(h));		
	n = normalize(n);
	
	vec3 E = normalize(vec3(-uv.y*2.-1.,1.,uv.x*2.-1.));	//fake up an eye vector
	vec3 rv = reflect(-E,n);
	vec3 reflect_color = degamma(texture(iChannel2,rv).xyz);

	vec3 fn = refract(vec3(0,1,0),n,2.5);
	uv += fn.xz*0.1;
	
	float lod = length(fn.xz)*10.;
	
	vec3 c = vec3(0.);
	c += degamma(texture(iChannel0,uv+vec2(0.66,0.),lod).xyz);
	c *= 1.-h*0.0125;
	c += reflect_color*.3;
	
//	fragColor = vec4(h*0.5+0.5);
//	fragColor = vec4(n*0.5+0.5,1.);
	vec3 L = normalize(vec3(1,1,1));
	float dl = max(dot(n,L),0.)*.7+.3;
	c *= dl;
//	fragColor = vec4(vec3(dl),1.);
	
	c = gamma(c);
	fragColor = vec4(vec3(c),1.);
}