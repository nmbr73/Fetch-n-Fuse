

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
mat2 r2d(float a) {
	float c = cos(a), s = sin(a);
    return mat2(
        c, s,
        -s, c
    );
}
float hash( vec2 p )
{
    return fract( sin( dot(p, vec2( 15.79, 81.93  ) ) * 45678.9123 ) );
}

float valueNoise( vec2 p )
{

    vec2 i = floor( p );
    vec2 f = fract( p );
    
    f = f*f*(3.0 - 2.0*f);
    

    float bottomOfGrid =    mix( hash( i + vec2( 0.0, 0.0 ) ), hash( i + vec2( 1.0, 0.0 ) ), f.x );

    float topOfGrid =       mix( hash( i + vec2( 0.0, 1.0 ) ), hash( i + vec2( 1.0, 1.0 ) ), f.x );


    float t = mix( bottomOfGrid, topOfGrid, f.y );
    
    return t;
}

float fbm( vec2 uv )
{
    float sum = 0.00;
    float amp = 0.7;
    
    for( int i = 0; i < 4; ++i )
    {
        sum += valueNoise( uv ) * amp;
        uv += uv * 1.2;
        amp *= 0.4;
    }
    
    return sum;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    float time = iTime;
    float rotTime = sin(time);
    
    vec3 color1 = vec3(0.8, 0.2, 0.);
    vec3 color2 = vec3(.0, 0.2, 0.8);
    
    vec2 uv = ( fragCoord -.5*iResolution.xy )/iResolution.y;

    vec3 destColor = vec3(2.0 * rotTime, .0, 0.5);
    float f = 10.15;
    float maxIt = 18.0;
    vec3 shape = vec3(0.);
    for(float i = 0.0; i < maxIt; i++){
        float s = sin((time / 111.0) + i * cos(iTime*0.02+i)*0.05+0.05);
        float c = cos((time / 411.0) + i * (sin(time*0.02+i)*0.05+0.05));
        c += sin(iTime);
        f = (.005) / abs(length(uv / vec2(c, s)) - 0.4);
        f += exp(-400.*distance(uv, vec2(c,s)*0.5))*2.;
        // Mas Particulas
        f += exp(-200.*distance(uv, vec2(c,s)*-0.5))*2.;
        // Circulito
        f += (.008) / abs(length(uv/2. / vec2(c/4. + sin(time*4.), s/4.)));
        f += fbm( uv * 20.5 )*0.05;
        float idx = float(i)/ float(maxIt);
        idx = fract(idx*2.);
        vec3 colorX = mix(color1, color2,idx);
        shape += f * colorX;
        
        uv += fbm( uv * 20.5 )*0.02;
        uv *= r2d(iTime*0.1 + cos(i*50.)*f);
        
    }
    
    // vec3 shape = vec3(destColor * f);
    // Activar modo falopa fuerte
    // shape = sin(shape*10.+time);
    fragColor = vec4(shape,1.0);
}