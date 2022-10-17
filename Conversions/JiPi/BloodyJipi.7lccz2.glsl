

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// remix of milky: https://www.shadertoy.com/view/Msy3D1
// which is based in turn on https://www.shadertoy.com/view/Xsd3DB

// turns out two sim steps per frame is important for motion to
// get nice, fast waves and oscillation

// this makes pretty terrible use of the simulation domain (comment
// out RAYMARCH to see it) but i like how the result looks at this scale.
// i should reduce the simulation domain size a la https://www.shadertoy.com/view/4dKGDw


#define RAYMARCH
#define HEIGHTMAPSCALE 90.
#define MARCHSTEPS 8

float hscale = 4.;

vec3 cam( in vec2 p, out vec3 cameraPos );

float h( vec3 p ) { return hscale*texture(iChannel0, p.xz/HEIGHTMAPSCALE + 0.5 ).x; }
// boost the step size, we resort to the secant method if we overstep the surface
float DE( vec3 p ) { return 1.2 * ( p.y - h(p) ); }

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 q = fragCoord.xy/iResolution.xy;
    vec2 qq = q*2.-1.;
    float eps = 0.1;
    
#ifdef RAYMARCH
    
    vec3 L = normalize(vec3(.3,.9,1.));
    
    // raymarch the milk surface
    vec3 ro;
    vec3 rd = cam( qq, ro );
    float t = 0.;
    float d = DE(ro+t*rd);
    
    for( int i = 0; i < MARCHSTEPS; i++ )
    {
        if( abs(d) < eps )
            break;
        
        float dNext = DE(ro+(t+d)*rd);
        
        // detect surface crossing, if so use secant method
        // https://www.shadertoy.com/view/Mdj3W3
		float dNext_over_d = dNext/d;
        if( dNext_over_d < 0.0 )
        {
            // estimate position of crossing
			d /= 1.0 - dNext_over_d;
			dNext = DE( ro+rd*(t+d) );
        }
        
		t += d;
		d = dNext;
    }
    
    // hit the BLOOD
    {
        vec3 p = ro+t*rd;
        
        // finite difference normal
        float h0 = h(p);
        vec2 dd = vec2(0.01,0.);
        vec3 n = normalize(vec3( h0-h(p + dd.xyy), dd.x, h0-h(p + dd.yyx) ));
        
        // diffuse / subtle subsurface
        float ndotL = clamp(dot(n,L),0.,1.);
        float dif = 1.52*(0.7+0.3*ndotL);
        float ao = mix( 0.6, .64, smoothstep(0.,1.,(h0+1.5)/6.));
        vec3 difCol = vec3(0.82,0.,0.);
        fragColor.xyz = difCol*(dif)*ao;
        
        // specular
        float s = .6*pow( clamp( dot( L, reflect( rd, n ) ), 0., 1. ), 4000. );
        fragColor.xyz += s;
    }
    
	// vignette (borrowed from donfabio's Blue Spiral)
	vec2 uv =  q.xy-0.5;
	float distSqr = dot(uv, uv);
	fragColor.xyz *= 1.0 - 1.*distSqr;
    
#else
    float sh = 1. - texture(iChannel0, q).x;
    vec3 c =
       vec3(exp(pow(sh-.25,2.)*-5.),
            exp(pow(sh-.4,2.)*-5.),
            exp(pow(sh-.7,2.)*-20.));
    fragColor = vec4(c,1.);
#endif
}

vec3 cam( in vec2 p, out vec3 cameraPos )
{
    // camera orbits around origin
    float camRadius = 50.;
	float theta = -3.141592653/2.;
    float xoff = camRadius * cos(theta);
    float zoff = camRadius * sin(theta);
    cameraPos = vec3(xoff,30.,zoff);
     
    // camera target
    vec3 target = vec3(0.,0.,-30.);
     
    // camera frame
    vec3 fo = normalize(target-cameraPos);
    vec3 ri = normalize(vec3(fo.z, 0., -fo.x ));
    vec3 up = normalize(cross(fo,ri));
     
    // multiplier to emulate a fov control
    float fov = .5;
	
    // ray direction
    vec3 rayDir = normalize(fo + fov*p.x*ri + fov*p.y*up);
	
	return rayDir;
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Originally from tomkh's wave equation solver
// https://www.shadertoy.com/view/Xsd3DB
//

#define HEIGHTMAPSCALE 90.0

vec3 cam( in vec2 p, out vec3 cameraPos );

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 e = vec3(vec2(1.)/iResolution.xy,0.);
    vec2 q = fragCoord.xy/iResolution.xy;

    float p11 = texture(iChannel0, q).x;
    float p10 = texture(iChannel1, q-e.zy).x;
    float p01 = texture(iChannel1, q-e.xz).x;
    float p21 = texture(iChannel1, q+e.xz).x;
    float p12 = texture(iChannel1, q+e.zy).x;

    // accel on fluid surface
    float d = 0.;

    if( iMouse.z > 0. )
    {
        vec3 ro;
        vec3 rd = cam( 2.*iMouse.xy/iResolution.xy - 1., ro );
        if( rd.y < 0. )
        {
            vec3 mp = ro + rd * ro.y/-rd.y;
            vec2 uv = mp.xz/HEIGHTMAPSCALE + 0.5;
            float screenscale = iResolution.x/640.;
            d += .06*smoothstep(20.*screenscale,5.*screenscale,length(uv*iResolution.xy - fragCoord.xy));
        }
    }
    
    // force from video sampled by buffer B to avoid vid sync problems
    d += texture(iChannel1, q).y;

    // The actual propagation:
    d += -(p11-.5)*2. + (p10 + p01 + p21 + p12 - 2.);
    d *= .97; // damping
    if( iFrame == 0 ) d = 0.;
    d = d*.5 + .5;

    fragColor = vec4(d, 0.0, 0.0, 0.0);
}

vec3 cam( in vec2 p, out vec3 cameraPos )
{
    // camera orbits around origin
    float camRadius = 50.;
	float theta = -3.141592653/2.;
    float xoff = camRadius * cos(theta);
    float zoff = camRadius * sin(theta);
    cameraPos = vec3(xoff,30.,zoff);
     
    // camera target
    vec3 target = vec3(0.,0.,-30.);
     
    // camera frame
    vec3 fo = normalize(target-cameraPos);
    vec3 ri = normalize(vec3(fo.z, 0., -fo.x ));
    vec3 up = normalize(cross(fo,ri));
     
    // multiplier to emulate a fov control
    float fov = .5;
	
    // ray direction
    vec3 rayDir = normalize(fo + fov*p.x*ri + fov*p.y*up);
	
	return rayDir;
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Originally from tomkh's wave equation solver
// https://www.shadertoy.com/view/Xsd3DB
//

#define HEIGHTMAPSCALE 90.0

vec3 cam( in vec2 p, out vec3 cameraPos );

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 e = vec3(vec2(1.)/iResolution.xy,0.);
    vec2 q = fragCoord.xy/iResolution.xy;

    float p11 = texture(iChannel0, q).x;

    float p10 = texture(iChannel1, q-e.zy).x;
    float p01 = texture(iChannel1, q-e.xz).x;
    float p21 = texture(iChannel1, q+e.xz).x;
    float p12 = texture(iChannel1, q+e.zy).x;

    // accel on fluid surface
    float d = 0.;

    if( iMouse.z > 0. )
    {
        vec3 ro;
        vec3 rd = cam( 2.*iMouse.xy/iResolution.xy - 1., ro );
        if( rd.y < 0. )
        {
            vec3 mp = ro + rd * ro.y/-rd.y;
            vec2 uv = mp.xz/HEIGHTMAPSCALE + 0.5;
            float screenscale = iResolution.x/640.;
            d += .06*smoothstep(20.*screenscale,5.*screenscale,length(uv*iResolution.xy - fragCoord.xy));
        }
    }
    
    // sample video
    vec2 vuv = q*3.-vec2(1.,.17);
    float d_vid = 0.;
    //if( vuv.x > 0. && vuv.x < 1. && vuv.y > 0. && vuv.y < 0. )
	    d_vid = 0.106*(texture(iChannel2,vuv).x-0.7);
    d += d_vid;
    
    // The actual propagation:
    d += -(p11-.5)*2. + (p10 + p01 + p21 + p12 - 2.);
    d *= .97; // damping
    if( iFrame == 0 ) d = 0.;
    d = d*.5 + .5;

    fragColor = vec4(d, d_vid, 0.0, 0.0);
}

vec3 cam( in vec2 p, out vec3 cameraPos )
{
    // camera orbits around origin
    float camRadius = 50.;
	float theta = -3.141592653/2.;
    float xoff = camRadius * cos(theta);
    float zoff = camRadius * sin(theta);
    cameraPos = vec3(xoff,30.,zoff);
     
    // camera target
    vec3 target = vec3(0.,0.,-30.);
     
    // camera frame
    vec3 fo = normalize(target-cameraPos);
    vec3 ri = normalize(vec3(fo.z, 0., -fo.x ));
    vec3 up = normalize(cross(fo,ri));
     
    // multiplier to emulate a fov control
    float fov = .5;
	
    // ray direction
    vec3 rayDir = normalize(fo + fov*p.x*ri + fov*p.y*up);
	
	return rayDir;
}
