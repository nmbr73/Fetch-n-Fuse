
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Author: bitless
// Title: Through the hive

// Thanks to Patricio Gonzalez Vivo & Jen Lowe for "The Book of Shaders"
// and Fabrice Neyret (FabriceNeyret2) for https://shadertoyunofficial.wordpress.com/
// and Inigo Quilez (iq) for  https://iquilezles.org/www/index.htm
// and whole Shadertoy community for inspiration.

//Inigo Quiles article "Simple color palettes" 
//https://iquilezles.org/articles/palettes/
#define pal(t, a, b, c, d) ( a + b*_cosf( 6.28318f*(c*t+d) ) )

// Hash from "Hash without Sine" by Dave_Hoskins (https://www.shadertoy.com/view/4djSRW)
__DEVICE__ float hash11(in float x) {
    x = fract(x * 0.1031f);
    x *= x + 33.33f;
    x *= x + x;
    return fract(x);
}

__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract(to_float3(swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}



// Code from Fabrice (thanks a lot)
// Smaller, faster and doesn't cause a weird bug that wasn't present on my main computer

#define H2(p)       fract( _sinf((p+9.0f)*mat2(127.1f,311.7f, 269.5f,183.3f)) *4e4 )
#define H1(p)       H2(to_float2(p)).x

__DEVICE__ float2 hexPt(float2 p, float T, float l)  {
    float2 t = p + _floor(T/5.0f) + l;
    return p * mat2(1,-0.5f,0, 0.866f)
           + ( _mix( H2(t),  H2(t+1.0f),  smoothstep (0.0f,1.0f,mod_f(T,5.0f)) )
                -0.5f  ) * 0.433f; 
}

float Voronoi(float2 p, float M, float T, float l)  // --- Voronoi
{   
    float2 pH = _floor( p * mat2(1,0.6f,0,1) ), // pixToHex(p)
         mo, o, C, c,h;
    
    float m = 8.0f, md = m, d, f;
    for (int i=0; i<9; i++)
        c = to_float2(i%3,i/3)-1.0f,
        h = hexPt(pH + c, T, l) - p,
        d = dot(h, h),
        d < md ? md = d, mo = h, C = c : C;

    for (int i=0; i<9; i++)
        h = hexPt(pH + to_float2(i%3,i/3)-1.0f + C, T, l) - p - mo,
        d = dot(mo + h*0.5f, normalize(h)),
        f = _fmaxf(0.0f, 1.0f - _fabs(d-m)/M )/2.0f, // smin2
        m = _fminf(m, d) - M*f*f;

    return m;
}

////////////////// HEXAGONAL VORONOI///////////////




__KERNEL__ void ThroughthehiveJipiFuse(float4 O, float2 g, float iTime, float2 iResolution)
{

    float2 r = iResolution
        ,uv = (g+g-r)/r.y/2.
        ,xy;

    float   lcl = _sinf(iTime*0.2f)     // zoom speed changing cycle
            ,tm = fract (iTime)     //  1-second timer between cycles
            ,cicle = iTime-tm       //  cycle number
            ,speed = 1.5f+lcl*0.25f    //  zoom speed
            ,LAYERS =11.0f;            // num of layers 
    
    uv *= (1.0f - length(uv)*lcl*0.5f)  //  camera distortion
            /_expf(tm*_logf(speed))     // camera zoom
            *(0.3f+lcl*0.1f);
    
    O = to_float4_s(0.0f);


    float T, v, m, s;
    for (float i=LAYERS; i >= 0.0f; i--) //draw layers from the far side to the near side
    {
        T = iTime+hash11(cicle+i)*5.0f; //phase offset from the global timer for a particular layer
        xy = uv*_powf(speed,i+1.0f) + to_float2(_sinf(iTime),_cosf(iTime*2.0f))*0.07f; //local coordinates of the layer with a zoom and a small shift

        s = _fmaxf(smoothstep(5.0f,0.0f,i-tm)*0.01f              // blur the closest layers
                    +(1.0f + _sinf(T*20.0f+xy.x*10.0f-xy.y*10.0f))     // and changing layers (with a shaking effect)
                    *(smoothstep (1.5f,0.0f,mod_f(T,5.0f)))*0.02f   // in 1.5f seconds
                 , fwidth(xy.x));               // AA for far small layers

        v = Voronoi (xy+to_float2(-0.01f,0.01f), 0.2f, T, cicle+i); //voronoi with an offset to draw the highlighted edge
        m = 1.0f + smoothstep (0.04f-s,0.05f+s, v);                     //highlighted edge mask
        float4 col =  pal((iTime*2.0f+i-tm)*0.10f,to_float4_s(0.5f),to_float4_s(0.4f),to_float4_s(1.0f),to_float4(0.1f,0.2f,0.3f,1)) //layer color
                    * smoothstep(LAYERS,3.0f,i-tm)        // darken the farthest layers
                    * m;      

        v = Voronoi (xy, 0.2f, T, cicle+i); //   voronoi for current layer
        m = smoothstep (0.3f,0.07f+_sinf(T*5.0f)*0.05f, v) // layer shadow mask
            *(1.0f - tm*step(i,0.0f));                  // make the closest layer shadow transparent at the end of the cycle
        O *= 1.0f - m*0.7f;    //draw layer shadow

        m = smoothstep (0.05f+s,0.04f-s, v)   //  layer mask
            *(1.0f - tm*step(i,0.0f));          //  make the closest layer shadow transparent too 
        O = _mix (O,col,m); //draw layer
    }


  SetFragmentShaderComputedColor(O);
}