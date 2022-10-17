

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
        float idx = float(i)/ float(maxIt);
        idx = fract(idx*2.);
        vec3 colorX = mix(color1, color2,idx);
        shape += f * colorX;
        
        uv *= r2d(iTime*0.1 + cos(i*50.));
    }
    
    // vec3 shape = vec3(destColor * f);
    // Activar modo falopa fuerte
    // shape = sin(shape*10.+time);
    fragColor = vec4(shape,1.0);
}