

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    // Output to screen
    vec4 col = vec4(texture(iChannel0,uv));
    col *= col;
    col.g += col.w;
    col.g *= 5.;
    
    if(abs(distance(iMouse.xy, fragCoord)-10.)<.5 && iMouse.z > 0.0) col = vec4(100);
    
    fragColor = tanh(col/3.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define T(p) texture(iChannel0,(p)/iResolution.xy)
#define dt 0.1

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 c   = vec2(1,0.95);
    
    float x  = fragCoord.x;
    float y  = fragCoord.y;
    
    float xp = mod(fragCoord.x + 1.0, iResolution.x);
    float xm = mod(fragCoord.x - 1.0, iResolution.x);
    float yp = mod(fragCoord.y + 1.0, iResolution.y);
    float ym = mod(fragCoord.y - 1.0, iResolution.y);
    
    vec2 X   = vec2(T(vec2(x,y)).x, T(vec2(x,y)).z);
    
    vec2 Xp0 = vec2(T(vec2(xp,y)).x, T(vec2(xp,y)).z);
    vec2 Xm0 = vec2(T(vec2(xm,y)).x, T(vec2(xm,y)).z);
    vec2 X0p = vec2(T(vec2(x,yp)).x, T(vec2(x,yp)).z);
    vec2 X0m = vec2(T(vec2(x,ym)).x, T(vec2(x,ym)).z);
    
    vec2 V   = vec2(T(vec2(x,y)).y, T(vec2(x,y)).w);
    
    vec2 A   = c*c * (Xp0 + Xm0 + X0p + X0m - 4.0*X);
    
    X += V * dt + 0.5 * A * dt * dt;
    X += A * dt * 0.08;
    X -= X * dt * 0.001;
    
    V += A * dt;
    
    if(iMouse.z > 0.0)
    {
        X += 50.*dt*exp(-distance(iMouse.xy, fragCoord)*distance(iMouse.xy, fragCoord)/30.);
    }
    
    if (iTime > 0.1)
    {
        fragColor = vec4(X.x,V.x,X.y,V.y);
    }
    else
    {
        float r = length(fragCoord - iResolution.xy/2.0);
        float q = 10.0*exp(-r*r/300.0);
        fragColor = vec4(q,0,q,0);
    }
}