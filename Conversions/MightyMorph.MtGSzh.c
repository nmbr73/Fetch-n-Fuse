#define time iTime
mat3 rot(vec3 ang)
{
	mat3 x = mat3(1.0,0.0,0.0,0.0,cos(ang.x),-sin(ang.x),0.0,sin(ang.x),cos(ang.x));
	mat3 y = mat3(cos(ang.y),0.0,sin(ang.y),0.0,1.0,0.0,-sin(ang.y),0.0,cos(ang.y));
	mat3 z = mat3(cos(ang.z),-sin(ang.z),0.0,sin(ang.z),cos(ang.z),0.0,0.0,0.0,1.0);
	return z*y*x;
}

float noise3D(vec3 p)
{
	return fract(sin(dot(p ,vec3(12.9898,78.233,12.7378))) * 43758.5453);
}

float smooth3D(vec3 p)
{
    vec3 f = fract(p);
    p = floor(p);
    f = f*f*(3.0-2.0*f);

	float p0 = noise3D(p);
	float x = noise3D(p+vec3(1.0,0.0,0.0));
	float y = noise3D(p+vec3(0.0,1.0,0.0));
	float z = noise3D(p+vec3(0.0,0.0,1.0));
	float xy = noise3D(p+vec3(1.0,1.0,0.0));
	float xz = noise3D(p+vec3(1.0,0.0,1.0));
	float yz = noise3D(p+vec3(0.0,1.0,1.0));
	float xyz = noise3D(p+1.0);

    return mix(	mix(	mix(p0, x, 	 f.x),
                    	mix(y, 	xy,  f.x), 	f.y),
               	mix(	mix(z, 	xz,	 f.x),
                    	mix(yz, xyz, f.x), 	f.y), 	f.z);
}

float fbm(vec3 p)
{
 	float f = 0.5000*smooth3D(p*1.00);
    	  f+= 0.2500*smooth3D(p*2.01);
    	  f+= 0.1250*smooth3D(p*4.02);
    	  f+= 0.0625*smooth3D(p*8.03);
    	  f/= 0.9375;
    return f;
}
float sphere( vec3 rp, vec3 c, float r )
{
    return distance(rp, c) - r;
}

float map(vec3 rp)
{
    rp *= rot(vec3(time*0.25));
    float d = sphere( rp, vec3(0.0), 1.0+(fbm(rp*8.0+time)*2.0-1.0)*1.0);
    return d;
}

vec3 normal(vec3 rp)
{
    vec3 eps = vec3( 0.002 ,0.0,0.0);
	return normalize( vec3(
           map(rp+eps.xyy) - map(rp-eps.xyy),
           map(rp+eps.yxy) - map(rp-eps.yxy),   //from iq
           map(rp+eps.yyx) - map(rp-eps.yyx) ) );

}

float softShadow(vec3 rp, vec3 ld)
{
 	vec3 ro = rp;
    float ldmax = 20.0;
    float td = 0.05;
    float res = 1.0;
    float d;
    for(int i = 0; i < 256; i++)
    {
     	rp = ro + ld * td;
        d = map( rp );
        if( d < 0.001 || td >= ldmax )
        {
         	break;
        }
        res = min(res,8.0*d);
        td += d*0.1;
    }
    if( d < 0.001 )
    {
     	res = 0.0;
    }

    return res;
}
__DEVICE__ void MightyMorphKernel( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 p = 2.0*uv-1.0;
    p.x*=iResolution.x/iResolution.y;
    vec3 col = vec3(0.0);

    vec2 m = (iMouse.xy/iResolution.xy)*2.0-1.0;


    vec3 cp = vec3(0.0,0.0,-2.0);
    vec3 rd = normalize( vec3(p,0.0) - cp );
    vec3 ro = cp-vec3(0.0,0.0,2.0);
    vec3 rp;

    vec3 ang = vec3(-3.14*m.y,3.14*m.x,0.0);
    //rd*= rot(ang);
    //ro*= rot(ang);

    float td = 0.1;
    float dmax = 8.0;
    float d;
    float mind = 1000.0;

    for( int i = 0; i < 512; i++ )
    {
        rp = ro+rd*td;
        d = map(rp);
        mind = min(mind,d);
        if( d < 0.001 )
            break;
        td += d*0.1;
        if( td > dmax )
        {
            td = dmax;
            break;
        }
    }
    vec3 keepo = texture(iChannel0, uv*(0.001+0.01*abs(sin(time*0.1)))+time*0.001).rgb;
    if( d < 0.001 )
    {
        vec3 n = normal(rp);
        vec3 l = normalize(vec3(0.0,0.1,-1.0));//*rot(vec3(0.0,0.0,time));
        float sha = softShadow( rp, l );
        col = mix(vec3(0.05),keepo,clamp(0.0,1.0,smoothstep(1.0,0.0,2.0-length(rp))));
        //PHONG
        /*
        vec3 v = normalize(ro-rp);
        vec3 h = normalize(l+v);

        float a = 128.0;
        float kd = 0.33;
        float ks = 0.33;
        float ss = 0.33;

        col*=max(0.0,dot(n,l))*kd+max(0.0,pow(dot(n,h),a))*ks+sha*ss;
        */
        col*=max(0.0,dot(n,l))*sha;

    }
    else
    {
         col += (keepo*pow(dot(rd,vec3(0.0,0.0,1.0)),24.0))*exp(-mind+1.0+sin(time));
    }
    col = mix(col,keepo*(max(0.0,1.0-mind*8.0)),min(1.0,length(rp)*0.025));

    col = clamp(col, 0.0, 1.0);
    col = pow(col, vec3(0.45));

    float f = 8.0;
    col = (1.0/(1.0+exp(4.0-f*col))-0.0003)/(0.982-0.018);

    //col = 1.0-col;

    p.x/=iResolution.x/iResolution.y;
    col *= smoothstep( 1.325, 0.825, abs(p.x) );
    col *= smoothstep( 1.325, 0.825, abs(p.y) );

    float dither = (noise3D(vec3(p,time))*2.0-1.0)*2.0/256.0;
    col += dither;
	fragColor = vec4(col,1.0);
}