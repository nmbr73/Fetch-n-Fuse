

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define TWO_PI  6.28318530718
#define PI      3.14159265359
    
mat3 ZRotMatrix(in float a)
{
    return mat3( cos(a), -sin(a), 0.,
                 sin(a),  cos(a), 0., 
                 0.,      0.,     1.);
}

vec3 sdEgg( in vec3 p, in vec3 c, float Zr, in vec3 s)
{
    //apply transformation
    p -= c;
    p = ZRotMatrix(Zr) * p;
    p *= s;

    vec3 res = vec3(0.);
    
    //compute sdf
    float f = pow(1.2*dot(p.xz, p.xz), 0.8);
    p.y += 0.15 * f;
    res.x = (length(p) - 0.5);
    
    //uvs
    res.yz = vec2( (atan(p.x, p.z)) / (TWO_PI),
                   (sign(p.y)*acos(dot( normalize(p), normalize(vec3(p.x,0.0,p.z))))) / PI
                 );
                 
    return res;
}

//------------------------------------------------------------------

vec4 opU( vec4 d1, vec4 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

vec4 map( in vec3 pos  )
{
    vec4 res = vec4( 1e10,0.0,0.0,0.0 );
    
    float delay = 4.;
    float hmov = sin((iChannelTime[0]-delay) * ( iChannelTime[0]>delay ? 6.13 : 0.));
    float scale = 2.*sin((iChannelTime[0]-delay) * ( iChannelTime[0]>delay ? 24.52 : 0. ));
    res = opU( res, vec4( sdEgg( pos,vec3(-hmov*0.1,0.62 + scale*0.02,0.),hmov*0.2,vec3(1.+ scale*0.03, 0.8 - scale*0.03, 1.+ scale*0.03)), 2.) );
    res = opU( res, vec4( sdEgg( pos,vec3(-hmov*0.1-1.5,0.62 + scale*0.02,0.),hmov*0.2,vec3(1.+ scale*0.03, 0.8-scale*0.03, 1.+ scale*0.03)), 3.) );
    res = opU( res, vec4( sdEgg( pos,vec3(-hmov*0.1+1.5,0.62+ scale*0.02,0.),hmov*0.2,vec3(1.+ scale*0.03, 0.8- scale*0.03, 1.+ scale*0.03)), 4.) );
    
    return res;
}

vec4 raycast( in vec3 ro, in vec3 rd )
{
    vec4 res = vec4(-1.,-1.,-1.,-1.);

    float tmax = 20.0;

    float tp1 = (-ro.y)/rd.y;
    if( tp1>0.0 )
    {
        tmax = min( tmax, tp1 );
        res = vec4( tp1, 0.,0.,1.);
    }
    
     
    float t = 0.;
    for( int i=0; i<70 && t<tmax; i++ )
    {
        vec4 h = map( ro+rd*t );
        if( abs(h.x)<(0.0001*t) )
        { 
            res = vec4(t,h.yzw); 
            break;
        }
        t += h.x;
    }
    
    return res;
}

float softshadow( in vec3 ro, in vec3 rd)
{   
    float t = 0.1;
    float tmax = 30.0;
    float res = 1.0;
    for(int i=0; i<256; ++i)
    {
        vec3 p = ro + rd*t;
        float d = map(p).x;
        
        res = min(res, 8. * d/t);
        if(d < 0.0001 || t > tmax) break;
        
        t+=d;
    }
    return clamp(res,0.0,1.0);
}

vec3 calcNormal( in vec3 p )
{
    vec2 e = vec2(0.01,0.0);
	return normalize( vec3( map( p + e.xyy ).x - map( p - e.xyy ).x,
                            map( p + e.yxy ).x - map( p - e.yxy ).x,
                            map( p + e.yyx ).x - map( p - e.yyx ).x
                            ));
}

float calcAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = map( pos + h*nor ).x;
        occ += (h-d)*sca;
        sca *= 0.8;
        if( occ>0.35 ) break;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );
}

float triangularSignal(in float x, in float freq, in float amp)
{
    return abs((mod(x*freq,2.)-1.)*amp);
}

vec3 render( in vec3 ro, in vec3 rd)
{ 
    // background
    vec3 col = vec3(0.7, 0.7, 0.9) - max(rd.y,0.0)*0.3;
     
    vec4 res = raycast(ro,rd);
    float t = res.x;
	float m = res.w;
    vec2 uvs = res.yz;
    if( m>-0.5 )
    {
        vec3 pos = ro + t*rd;
        vec3 nor = (m<1.5) ? vec3(0.0,1.0,0.0) : calcNormal( pos );
        vec3 ref = reflect( rd, nor );
        
        // specular coeff        
        float ks = 0.;
        
        //ground
        if( m<1.5 )
        {
            col = vec3(0.1,0.2,0.3);
            ks = 0.1;
        }
        //yellow egg
        else if( m<2.5 )
        {
            col = vec3(0.4,0.3,0.01);
            ks = 1.4;
            
            col = mix(col, vec3(0.,0.05,0.2), smoothstep(-0.21,-0.2, uvs.y) * smoothstep(-0.12, -0.125, uvs.y));
            col = mix(col, vec3(0.,0.2,0.2), smoothstep(-0.105,-0.10,uvs.y) * smoothstep(-0.005, -0.01, uvs.y));
            
            float redstripefunc = triangularSignal(uvs.x,30.,0.04);
            float redstripefactor = smoothstep( uvs.y-0.155,uvs.y-0.15, redstripefunc) * smoothstep( uvs.y-0.1, uvs.y-0.105, redstripefunc );
            col = mix(col, vec3(0.3,0.,0.), redstripefactor);
            ks = mix(ks, 0.6, redstripefactor);
            
            float bluestripefunc = triangularSignal(uvs.x,30.,0.04);
            float bluestripefactor = smoothstep( uvs.y-0.225, uvs.y-0.22, bluestripefunc) * smoothstep( uvs.y-0.185, uvs.y-0.19, bluestripefunc);
            col = mix(col, vec3(0.1,0.3,0.5)*0.4, bluestripefactor);
            ks = mix(ks, 0.6, bluestripefactor);
                        
            float pinkdotsfunc = smoothstep(0.5, 0.6,(abs(sin(uvs.x*70.))*sin(uvs.y*55.-0.5)));
            float pinkdotsfactor = pinkdotsfunc * step(0., uvs.y)*step(uvs.y,0.07);
            col = mix(col, vec3(0.5,0.4,0.4), pinkdotsfactor );
            ks = mix(ks, 0.6, pinkdotsfactor);
            
            float reddotsfunc = smoothstep(0.5, 0.6,(abs(sin(uvs.x*70.-1.5))*sin(uvs.y*50.-2.)));
            float reddotsfactor = reddotsfunc * step(-0.1, uvs.y)*step(uvs.y,0.);
            col = mix(col, vec3(0.5,0.,0.), reddotsfactor);
            ks = mix(ks, 0.6, reddotsfactor);
            
            col = texture(iChannel1, uvs).xyz;
            
        }
        else if( m<3.5 )
        {
            ks = 0.8;
            col = vec3(0.2,0.0,0.3);
            
            float gl1 = smoothstep(-0.31,-0.3, uvs.y) * smoothstep(-0.23, -0.24, uvs.y);
            float gl2 = smoothstep(-0.11,-0.105, uvs.y) * smoothstep(-0.04, -0.045, uvs.y);
            float gl3 = smoothstep(0.08,0.085, uvs.y) * smoothstep(0.135, 0.13, uvs.y);
            float gl4 = smoothstep(0.23,0.235, uvs.y) * smoothstep(0.3  , 0.295, uvs.y);
            float gl5 = smoothstep(0.34,0.345, uvs.y) * smoothstep( 0.365, 0.36, uvs.y);
            
            col = mix(col, vec3(0.3),gl1);
            ks = mix(ks, 0.5, gl1);
            col = mix(col, vec3(0.3),gl2);
            ks = mix(ks, 0.5, gl2);
            col = mix(col, vec3(0.3),gl3);
            ks = mix(ks, 0.5, gl3);
            col = mix(col, vec3(0.3),gl4);
            ks = mix(ks, 0.5, gl4);
            col = mix(col, vec3(0.5, 0.2, 0.2),gl5);
            ks = mix(ks, 0.5, gl5);
            
            float pinkdotsbot = smoothstep(0.7, 0.8,(sin(uvs.x*30.)*sin(uvs.y*20.-1.5)));
            float pinkdotsbotfactor = pinkdotsbot * step(-0.2, uvs.y)*step(uvs.y,-0.1);
            col = mix(col, vec3(0.5, 0.2, 0.2), pinkdotsbotfactor );
            ks = mix(ks, 0.5, pinkdotsbotfactor);
            
            float yellowdotsbot = smoothstep(0.7, 0.8,(sin(uvs.x*30.+3.2)*sin(uvs.y*20.-1.5)));
            float yellowdotsbotfactor = yellowdotsbot * step(-0.2, uvs.y)*step(uvs.y,-0.1);
            col = mix(col, vec3(0.6, 0.4, 0.), yellowdotsbotfactor );
            ks = mix(ks, 0.5, yellowdotsbotfactor);
            
            float pinkdotstop = smoothstep(0.7, 0.8,(sin(uvs.x*30.)*sin(uvs.y*20.-2.1)));
            float pinkdotstopfactor =pinkdotstop * step(0.1, uvs.y)*step(uvs.y,0.3);
            col = mix(col, vec3(0.5, 0.2, 0.2), pinkdotstopfactor);
            ks = mix(ks, 0.5, pinkdotstopfactor);
            
            float yellowdotstop = smoothstep(0.7, 0.8,(sin(uvs.x*30.+3.2)*sin(uvs.y*20.-2.1)));
            float yellowdotstopfactor = yellowdotstop * step(0.1, uvs.y)*step(uvs.y,0.3);
            col = mix(col, vec3(0.6, 0.4, 0.), yellowdotstopfactor);
            ks = mix(ks, 0.5, yellowdotstopfactor);
            
            float pinkwave = sin(uvs.x*50.)*0.04;
            float pinkwavefactor = smoothstep( uvs.y-0.03, uvs.y-0.025, pinkwave) * smoothstep( uvs.y-0.01, uvs.y-0.015, pinkwave);
            col = mix(col, vec3(0.5, 0.2, 0.2), pinkwavefactor);
            ks = mix(ks, 0.5, pinkwavefactor);
            
        }
        else if( m<4.5 )
        {
            col = mix(col, vec3(0.7, 0.6, 0.1)*0.4, smoothstep(0.3,0.5,(sin(uvs.x*170.+3.2)*sin(uvs.y*120.-2.1))));
            ks = 0.8;
        }

        // lighting taken from iq primitives shader https://www.shadertoy.com/view/Xds3zN
        float occ = calcAO( pos, nor );
        
		vec3 lin = vec3(0.0);

        // sun
        {
            vec3  lig = normalize( vec3(0.5, 0.4, 0.5) );
            vec3  hal = normalize( lig-rd );
            float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
          	      dif *= softshadow( pos, lig);
			float spe = pow( clamp( dot( nor, hal ), 0.0, 1.0 ),16.0);
                  spe *= dif;
                  spe *= 0.04+0.96*pow(clamp(1.0-dot(hal,lig),0.0,1.0),5.0);
            lin += col*2.20*dif*vec3(1.30,1.00,0.70);
            lin +=     15.00*spe*vec3(1.30,1.00,0.70)*ks;
            lin *= 0.5;
        }
        // sky
        {
            float dif = sqrt(clamp( 0.5+0.5*nor.y, 0.0, 1.0 ));
                  dif *= occ;
            float spe = smoothstep( -0.2, 0.2, ref.y );
                  spe *= dif;
                  spe *= 0.04+0.96*pow(clamp(1.0+dot(nor,rd),0.0,1.0), 5.0 );
                  spe *= softshadow( pos, ref);
            lin += col*0.80*dif*vec3(0.40,0.60,1.15);
            lin +=     2.00*spe*vec3(0.40,0.60,1.30)*ks;
        }
        // back
        {
        	float dif = clamp( dot( nor, normalize(vec3(0.5,0.0,0.6))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);
                  dif *= occ;
        	lin += col*1.*dif*vec3(0.1,0.2,0.3);
        }
        // sss
        {
            float dif = pow(clamp(1.0+dot(nor,rd),0.0,1.0),2.0);
                  dif *= occ;
        	lin += col*0.25*dif*vec3(1.00,1.00,1.00);
        }
        
		col = lin;

        //horizon
        col = mix( col, vec3(0.7,0.7,0.9), 1.0-exp( -0.0001*t*t*t ) );
    }

	return vec3( clamp(col,0.0,1.0) );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 scrPos = fragCoord/iResolution.xy;
	scrPos = scrPos*2.0 - 1.0;
    scrPos.x *= iResolution.x / iResolution.y;
    
    vec3 ta = vec3( 0., 0.8, 0. );
    vec3 ro = vec3(0., 1., 3.5);
    
    vec3 f = normalize(ta-ro);
    vec3 r = normalize(cross(f,vec3(0.,1.,0.)));
    vec3 t = normalize(cross(r,f));
    vec3 rd = normalize( vec3(scrPos.x*r+scrPos.y*t+f*2.5) );
     
    vec3 col = render( ro, rd);

    fragColor = vec4( pow( col, vec3(0.4545)) , 1.0 );
}