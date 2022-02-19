

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
////////////////////////////////////////////////////////////////////////////////
//
// "Antonalog's Glassy Blobs with camera" - I took Antonalog's Glassy Blobs
// shader (https://www.shadertoy.com/view/lslGRS) and added trackball- or
// arcball-like camera control to it.
//
// Copyright 2019 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#define DO_BLOBS(DO) vec4 b; b=vec4(-0.38 + 0.25*sin(iTime+0.00), -0.60 + 0.25*cos(iTime+0.00), -0.67, 0.17); DO; b=vec4(-0.33 + 0.25*sin(iTime+1.00), -0.59 + 0.25*cos(iTime+1.00), 0.02, 0.19); DO; b=vec4(-0.33 + 0.25*sin(iTime+2.00), -0.42 + 0.25*cos(iTime+2.00), 0.48, 0.12); DO; b=vec4(-0.50 + 0.25*sin(iTime+3.00), -0.18 + 0.25*cos(iTime+3.00), -0.30, 0.15); DO; b=vec4(-0.57 + 0.25*sin(iTime+4.00), 0.09 + 0.25*cos(iTime+4.00), 0.14, 0.16); DO; b=vec4(-0.58 + 0.25*sin(iTime+5.00), -0.13 + 0.25*cos(iTime+5.00), 0.58, 0.12); DO; b=vec4(-0.48 + 0.25*sin(iTime+6.00), 0.67 + 0.25*cos(iTime+6.00), -0.66, 0.13); DO; b=vec4(-0.37 + 0.25*sin(iTime+7.00), 0.43 + 0.25*cos(iTime+7.00), -0.16, 0.18); DO; b=vec4(-0.49 + 0.25*sin(iTime+8.00), 0.41 + 0.25*cos(iTime+8.00), 0.62, 0.16); DO; b=vec4(0.19 + 0.25*sin(iTime+9.00), -0.64 + 0.25*cos(iTime+9.00), -0.47, 0.18); DO; b=vec4(0.19 + 0.25*sin(iTime+10.00), -0.43 + 0.25*cos(iTime+10.00), -0.04, 0.13); DO; b=vec4(-0.01 + 0.25*sin(iTime+11.00), -0.40 + 0.25*cos(iTime+11.00), 0.39, 0.11); DO; b=vec4(-0.12 + 0.25*sin(iTime+12.00), -0.06 + 0.25*cos(iTime+12.00), -0.70, 0.12); DO; b=vec4(0.08 + 0.25*sin(iTime+13.00), 0.18 + 0.25*cos(iTime+13.00), 0.07, 0.15); DO; b=vec4(-0.15 + 0.25*sin(iTime+14.00), -0.12 + 0.25*cos(iTime+14.00), 0.51, 0.19); DO; b=vec4(0.09 + 0.25*sin(iTime+15.00), 0.57 + 0.25*cos(iTime+15.00), -0.48, 0.10); DO; b=vec4(0.12 + 0.25*sin(iTime+16.00), 0.64 + 0.25*cos(iTime+16.00), 0.19, 0.14); DO; b=vec4(-0.11 + 0.25*sin(iTime+17.00), 0.67 + 0.25*cos(iTime+17.00), 0.42, 0.20); DO; b=vec4(0.55 + 0.25*sin(iTime+18.00), -0.69 + 0.25*cos(iTime+18.00), -0.35, 0.18); DO; b=vec4(0.33 + 0.25*sin(iTime+19.00), -0.49 + 0.25*cos(iTime+19.00), -0.03, 0.17); DO; b=vec4(0.35 + 0.25*sin(iTime+20.00), -0.66 + 0.25*cos(iTime+20.00), 0.55, 0.15); DO; b=vec4(0.51 + 0.25*sin(iTime+21.00), -0.12 + 0.25*cos(iTime+21.00), -0.66, 0.14); DO; b=vec4(0.48 + 0.25*sin(iTime+22.00), -0.08 + 0.25*cos(iTime+22.00), -0.12, 0.11); DO; b=vec4(0.50 + 0.25*sin(iTime+23.00), 0.15 + 0.25*cos(iTime+23.00), 0.60, 0.16); DO; b=vec4(0.59 + 0.25*sin(iTime+24.00), 0.43 + 0.25*cos(iTime+24.00), -0.52, 0.11); DO; b=vec4(0.50 + 0.25*sin(iTime+25.00), 0.66 + 0.25*cos(iTime+25.00), 0.15, 0.18); DO; b=vec4(0.35 + 0.25*sin(iTime+26.00), 0.44 + 0.25*cos(iTime+26.00), 0.37, 0.14); DO; 

vec2 Q(float a, float b, float c)
{
	float d = b*b-4.0*a*c;
	if (d < 0.0) return vec2(1e10,-1e10);
	d=sqrt(d);	
	float oo2a = 0.5/a;
	float n = (-b-d)*oo2a;
	float x = (-b+d)*oo2a;
//	return vec2( min(n,x), max(n,x) );
	return vec2( n,x );
}

vec2 SphereT(vec3 P, vec3 V, vec3 A, float R)
{
	return Q(dot(V,V),2.0*(dot(P,V)-(dot(A,V))),dot(A,A)+dot(P,P)-R*R-(2.0*(dot(A,P))));
}

vec2 NearestBlobBound(vec3 P, vec3 V, float r)
{
	vec2 t = vec2(1e10,-1e10);
	vec2 s;
	DO_BLOBS( s=SphereT(P,V,b.xyz,r*b.w); t.x=min(t.x,s.x); t.y=max(t.y,s.y) )
	return t;
}

float k = 10.0;

float sdf(vec3 x)
{
	//http://www.johndcook.com/blog/2010/01/13/soft-maximum/
	float sum = 0.0;
	DO_BLOBS( sum += exp( k*(b.w - length(x-b.xyz)) ) )
	return log( sum ) / k;	
}

vec3 BlobNor(vec3 x)
{
	vec3 sum=vec3(0.0,0.0,0.0);

	float w;
	vec3 n;
	float L;
	vec3 v;
	DO_BLOBS( v=x-b.xyz; L=length(v); n=v*(1.0/L); w = exp(k*(b.w - L)); sum += w*n );
	return normalize( sum );	
	
}

vec3 ss_nor(vec3 X)
{
	return normalize(cross(dFdx(X),dFdy(X)));
}
vec3 ss_grad(vec3 X)
{
	return cross(dFdx(X),dFdy(X));
}

float shlick(vec3 N, vec3 V)
{
	float f = dot(-V,N);
	f = 1.0-f;	
	float ff = f;
	f *= f;		//2
//	f *= f;		//4
//	f *= ff;	//5
	float r0 = 0.075;
	f = r0 + (1.0-r0)*f;
	return f;
}

vec3 Transmittance(vec3 color, float T)
{
	return -log(color)/T;
}

vec3 Filter(float thick, vec3 trans)
{
	float conc = 0.6;
	return exp( -trans * conc * thick );
}

// --- addition-start by MacSlow ----------
vec3 cam (vec2 uv, vec3 ro, vec3 aim, float zoom)
{
    vec3 f = normalize (aim - ro);
    vec3 wu = vec3 (.0, 1., .0);
    vec3 r = normalize (cross (wu, f));
    vec3 u = normalize (cross (f, r));
    vec3 c = ro + f*zoom;
    return normalize (c + r*uv.x + u*uv.y - ro);
}
// --- addition-end by MacSlow ----------

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec3 P, V;

	// --- addition-start by MacSlow ----------
    float r = 2.5;
    float azimuthAngle = ((iMouse.x / iResolution.x) * 2. - 1.) * 179.;
    float elevationAngle = ((iMouse.y / iResolution.y) * 2. - 1.) * 79.;
    float x = r*cos (radians (azimuthAngle));
    float y = r*sin (radians (elevationAngle));
    float z = r*sin (radians (azimuthAngle));
    P = vec3(x, y, z);
    vec3 aim = vec3 (.0);
    vec2 uv = fragCoord.xy/iResolution.xy;
    vec2 uvRaw = uv;
    uv = uv*2. - 1.;
    uv.x *= iResolution.x/iResolution.y;
    V = cam (uv, P, aim, 1.75);
	// --- addition-end by MacSlow ----------
	
	float overb = 1.5;
	
	vec3 bg = texture(iChannel1,V).xyz * overb;
	vec3 bg_V = V;
	vec2 bound=NearestBlobBound(P, V, 2.0);
	float t = bound.x;
	
	vec3 trans=Transmittance(vec3(0.3,0.7,0.1), 1.0);
	
	if (t < 1e10)
	{
		int steps=0;
		float d = -1e10;
		float old_d = -1e10;
		vec3 X;

		float inside=0.0;
		float last_surface_t=0.0;
	
		float thick=0.0;
		#define STEPS	64
		float t_step = (bound.y-bound.x)*(1.0/float(STEPS));
		
		vec3 c = vec3(0.0,0.0,0.0);
		
		float last_f=0.0;
		
		vec3 filter_col = vec3(1.0);
		
		float blocked=0.0;
		
		for (int i=0; i<STEPS; i++)
		{			
			X = P+V*t;
			d = sdf(X);
		
			if (d * old_d < 0.0)	//a crossing
			{
				inside = 1.0 - inside;
				float int_t = mix(t-t_step,t,abs(old_d)/(abs(d)+abs(old_d)));
								//t-d; 
							//0.5*(t-t_step-old_d + t-d);	
			
				vec3 int_X = P+V*int_t;
				vec3 N = BlobNor(int_X);				
	
				if (inside < 0.5)	//just came out
				{
					float this_thick = (int_t - last_surface_t);
					filter_col *= Filter(this_thick,trans) * (1.0-last_f);
					
					thick += this_thick;
					
				//	V = bg_V = normalize( refract(bg_V,N,0.995) );
				}
				else	//just went in
				{		
					float f = shlick(N,V);
					last_f = f;
					
					vec3 refV = reflect(V,N);
					vec3 ref = f*texture(iChannel1,refV).xyz; 
					
					vec2 blocker=NearestBlobBound(int_X, refV, 1.0);
					if (blocker.x > 0.01 && blocker.x < 1e5) 
					{
						blocked = blocker.y-blocker.x;
						ref *= blocked;
					}
					
					c += ref * filter_col;

					V = bg_V = normalize( refract(bg_V,N,0.995) );

				}
				
				last_surface_t = int_t;
			}			
						
			//stop if grad is pointing away from view ray...
			//saves steps but introduces some nasty artifacts on some cards
	/*		vec3 G=-ss_grad(X);
			if (dot(G,V) < 0.0) 
			{
				break;
			}*/
			
		
		//	t -= d;		
			t += t_step;
			
			old_d = d;
			
			steps++;
		}
				
//		float S = float(steps)/64.0; ///8.0;
	//	fragColor = vec4(S,S,S,1.0);
				
	//	fragColor = vec4(vec3(thick),1.0);
		
	//	c += Filter(thick,trans)*bg;
		vec3 ref_bg = texture(iChannel1,bg_V).xyz  * overb;
		c += filter_col * ref_bg;
	//	c += (1.0-thick)*bg;

		fragColor = vec4(c,1.0);
		
	//	fragColor = vec4(vec3(last_f),1.0);
	//	fragColor = vec4(vec3(blocked),1.0);
	}
	else
	{
		fragColor = vec4(bg,1.0);
	}

	// --- addition-start by MacSlow ----------
    fragColor.rgb *= 1. - .7*length (uvRaw*2. - 1.);
    fragColor = pow (fragColor, vec4 (1./2.2));
    // --- addition-end by MacSlow ----------

}
