

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Modified version of 2D Cloth https://www.shadertoy.com/view/4dG3R1
// with texture mapping, mouse interaction, and inversion-resistant constraints

// The MIT License
// Copyright © 2016 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

float hash1( vec2 p ) { float n = dot(p,vec2(127.1,311.7)); return fract(sin(n)*153.4353); }

vec4 getParticle( vec2 id )
{
    return texture( iChannel0, (id+0.5)/iResolution.xy );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.y;
    
    vec3 f = vec3(0.0);
    for( int j=1; j<10; j++ )
    for( int i=1; i<10; i++ )
    {
        vec2 ij0 = vec2(i-1,j-1);
        vec2 ij1 = vec2(i  ,j  );
        vec4 p00 = getParticle(vec2(ij0.x,ij0.y));
        vec4 p01 = getParticle(vec2(ij0.x,ij1.y));
        vec4 p10 = getParticle(vec2(ij1.x,ij0.y));
        vec4 p11 = getParticle(vec2(ij1.x,ij1.y));
        vec2 d0 = (uv.yx - p00.yx) * vec2(1.0, -1.0);
        vec2 d1 = (uv.yx - p11.yx) * vec2(1.0, -1.0);
        vec2 n00 = normalize(p01.xy - p00.xy);
        vec2 n10 = normalize(p11.xy - p10.xy);
        vec2 n01 = normalize(p10.xy - p00.xy);
        vec2 n11 = normalize(p11.xy - p01.xy);
        vec2 uv0 = vec2( dot(d0, n00),-dot(d0, n01));
        vec2 uv1 = vec2(-dot(d1, n10), dot(d1, n11));
        vec2 max_uv = max(uv0, uv1);
        vec2 texture_uv = (uv0 / (uv0 + uv1) + ij0) / 9.0;
        vec3 col = texture( iChannel1, texture_uv ).rgb;
        float alpha = 1.0-smoothstep( 0.0, 0.002, max(max_uv.x, max_uv.y) );
        f = mix( f, col, alpha);
    }
    fragColor = vec4(f.brg,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define SIZE 0.07
#define FRICTION 0.25

float hash1( vec2  p ) { float n = dot(p,vec2(127.1,311.7)); return fract(sin(n)*43758.5453); }

vec4 getParticle( vec2 id )
{
    return textureLod( iChannel0, (id+0.5)/iResolution.xy, 0.0);
}

vec4 react( in vec4 p, in vec2 qid, in vec2 rid)
{
    vec2 q = getParticle( qid ).xy;
    vec2 r = getParticle( rid ).xy;
    
    vec2 m = (q + r) * 0.5;
    vec2 n = normalize((q - r).yx * vec2(1.0,-1.0)) * SIZE * 0.7071;
    
    p.xy = mix(p.xy, m + n, 0.2);
    
    return p;
}

vec4 solveConstraints( in vec2 id, in vec4 p )
{
    if( id.x > 0.5 && id.y > 0.5)  p = react( p, id + vec2(-1.0, 0.0), id + vec2( 0.0,-1.0));
    if( id.x > 0.5 && id.y < 8.5)  p = react( p, id + vec2( 0.0, 1.0), id + vec2(-1.0, 0.0));
    if( id.x < 8.5 && id.y > 0.5)  p = react( p, id + vec2( 0.0,-1.0), id + vec2( 1.0, 0.0));
    if( id.x < 8.5 && id.y < 8.5)  p = react( p, id + vec2( 1.0, 0.0), id + vec2( 0.0, 1.0));
    return p;
}    

vec4 move( in vec4 p, in vec2 id )
{
    const float g = 0.6;

    // acceleration
    p.xy += iTimeDelta*iTimeDelta*vec2(0.0,-g);
    
    // collide screen
    if( p.x < 0.00 ) { p.x = 0.00; p.w = mix(p.w, p.y, FRICTION);}
    if( p.x > 1.77 ) { p.x = 1.77; p.w = mix(p.w, p.y, FRICTION);}
    if( p.y < 0.00 ) { p.y = 0.00; p.z = mix(p.z, p.x, FRICTION);}        
    if( p.y > 1.00 ) { p.y = 1.00; p.z = mix(p.z, p.x, FRICTION);}

    // constrains
    p = solveConstraints( id, p );
        
    #if 0
    if( id.y > 8.5 ) p.xy = 0.05 + 0.1*id;
    #endif
    
    // inertia
    vec2 np = 2.0*p.xy - p.zw;
    p.zw = p.xy;
    p.xy = np;

    return p;
}




void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 id = floor( fragCoord-0.4 );
    
    if( id.x>9.5 || id.y>9.5 ) discard;
    
    vec4 p = getParticle(id);
    
    if( iFrame==0 )
    {
        p.xy = 0.15 + id * SIZE;
        p.zw = p.xy - 0.01*vec2(0.5+0.5*hash1(id),0.0);
    }
    else
    {
    	p = move( p, id );
    }
    if(iMouse.z > 0.5 && id.x < 0.5 && id.y < 0.5){
        p.xy = iMouse.xy/iResolution.xy * vec2(1.77, 1.0);
        //p.zw = p.xy;
    }

    fragColor = p;
}