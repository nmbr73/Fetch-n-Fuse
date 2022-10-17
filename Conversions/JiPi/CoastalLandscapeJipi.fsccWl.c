
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Author: bitless
// Title: Coastal Landscape

// Thanks to Patricio Gonzalez Vivo & Jen Lowe for "The Book of Shaders"
// and Fabrice Neyret (FabriceNeyret2) for https://shadertoyunofficial.wordpress.com/
// and Inigo Quilez (iq) for  https://iquilezles.org/www/index.htm
// and whole Shadertoy community for inspiration.

#define p(t, a, b, c, d) ( a + b*cos_f3( 6.28318f*(c*t+d) ) ) //IQ's palette function (https://www.iquilezles.org/www/articles/palettes/palettes.htm)
#define sp(t) p(t,to_float3(0.26f,0.76f,0.77f),to_float3(1,0.3f,1),to_float3(0.8f,0.4f,0.7f),to_float3(0,0.12f,0.54f)) //sky palette
#define hue(v) ( 0.6f + 0.76f * cos_f4(6.3f*(v) + to_float4(0,23,21,0) ) ) //hue

// "Hash without Sine" by Dave_Hoskins.
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}
////////////////////////

__DEVICE__ float2 rotate2D (float2 st, float a){
    return  mul_mat2_f2(to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)),st);
}

__DEVICE__ float st(float a, float b, float s) //AA bar
{
    return smoothstep (a-s, a+s, b);
}

__DEVICE__ float noise( in float2 p ) //gradient noise
{
    float2 i = _floor( p );
    float2 f = fract_f2( p );
   
    float2 u = f*f*(3.0f-2.0f*f);

    return _mix( _mix( dot( hash22( i+to_float2(0,0) ), f-to_float2(0,0) ), 
                       dot( hash22( i+to_float2(1,0) ), f-to_float2(1,0) ), u.x),
                 _mix( dot( hash22( i+to_float2(0,1) ), f-to_float2(0,1) ), 
                       dot( hash22( i+to_float2(1,1) ), f-to_float2(1,1) ), u.x), u.y);
}

__KERNEL__ void CoastalLandscapeJipiFuse(float4 O, float2 g, float iTime, float2 iResolution)
{
  
  // mat2
  CONNECT_POINT0(SunPosition, 0.0f, 0.0f );

  CONNECT_COLOR0(Water1, 0.0f, 0.1f, 0.5f, 1.0f);
  CONNECT_COLOR1(Water2, 0.35f, 0.35f, 0.0f, 1.0f);
  CONNECT_COLOR2(Stroke, 1.0f, 1.0f, 0.0f, 1.0f);
  CONNECT_COLOR3(Grass1, 0.7f, 0.6f, 0.2f, 1.0f);
  CONNECT_COLOR4(Grass2, 0.0f, 1.0f, 0.0f, 1.0f);
  
  CONNECT_SLIDER0(GrassSize, -10.0f, 100.0f, 50.0f);
  
  CONNECT_COLOR5(ForeGround, 0.5f, 0.3f, 0.0f, 1.0f);
  CONNECT_COLOR6(Sky, 1.0f, 1.0f, 1.0f, 1.0f);
  

  CONNECT_SLIDER1(foliageX, -10.0f, 100.0f, 30.0f);
  CONNECT_SLIDER2(foliageY, -10.0f, 100.0f, 15.0f);

  //CONNECT_SLIDER3(trunkX, -10.0f, 100.0f, 10.0f);
  //CONNECT_SLIDER4(trunkY, -10.0f, 100.0f, 60.0f);

  //CONNECT_SLIDER5(stripX, -10.0f, 100.0f, 0.0f);
  //CONNECT_SLIDER6(stripY, -10.0f, 100.0f, 0.0f);

    float2 r = iResolution
        ,uv = (g+g-r)/r.y
        ,sun_pos = to_float2(r.x/r.y*0.42f,-0.53f)+SunPosition //sun position 
        ,tree_pos = to_float2(-r.x/r.y*0.42f,-0.2f) //tree position 
        ,sh, u, id, lc, t;

    float3 f, c;
    float xd, yd, h, a, l;
    float4 C;
    
    float sm = 3.0f/r.y; //smoothness factor for AA

    sh = rotate2D(sun_pos, noise(uv+iTime*0.25f)*0.3f); //big noise on the sky
     
    if (uv.y > -0.4f) //drawing the sky
    {
        u = uv + sh;
        
        yd = 60.0f; //number of rings 
        
        id =  to_float2((length(u)+0.01f)*yd,0); //segment id: x - ring number, y - segment number in the ring  
        xd = _floor(id.x)*0.09f; //number of ring segments
        h = (hash12(_floor(swi2(id,x,x)))*0.5f+0.25f)*(iTime+10.0f)*0.25f; //ring shift
        t = rotate2D (u,h); //rotate the ring to the desired angle
    
        id.y = _atan2f(t.y,t.x)*xd;
        lc = fract_f2(id); //segment local coordinates
        id -= lc;
    
        // determining the coordinates of the center of the segment in uv space
        t = to_float2(_cosf((id.y+0.5f)/xd)*(id.x+0.5f)/yd,_sinf((id.y+0.5f)/xd)*(id.x+0.5f)/yd); 
        t = rotate2D(t,-h) - sh;
    
        h = noise(t*to_float2(0.5f,1)-to_float2(iTime*0.2f,0)) //clouds
            * step(-0.25f,t.y); //do not draw clouds below -.25
        h = smoothstep (0.052f,0.055f, h);
        
        
        lc += (noise(lc*to_float2(1,4)+id))*to_float2(0.7f,0.2f); //add fine noise
        
        f = _mix (sp(_sinf(length(u)-0.1f))*0.35f, //sky background
                  _mix(sp(_sinf(length(u)-0.1f)+(hash12(id)-0.5f)*0.15f),swi3(Sky,x,y,z),h), //to_float3_s(1),h), //mix sky color and clouds
                  st(_fabs(lc.x-0.5f),0.4f,sm*yd)*st(_fabs(lc.y-0.5f),0.48f,sm*xd));
    };

    if (uv.y < -0.35f) //drawing water
    {

        float cld = noise(-sh*to_float2(0.5f,1)  - to_float2(iTime*0.2f,0)); //cloud density opposite the center of the sun
        cld = 1.0f- smoothstep(0.0f,0.15f,cld)*0.5f;

        u = uv*to_float2(1,15);
        id = _floor(u);

        for (float i = 1.0f; i > -1.0f; i-=1.0f) //drawing a wave and its neighbors from above and below
        {
            if (id.y+i < -5.0f)
            {
                lc = fract_f2(u)-0.5f;
                lc.y = (lc.y+(_sinf(uv.x*12.0f-iTime*3.0f+id.y+i))*0.25f-i)*4.0f; //set the waveform and divide it into four strips
                h = hash12(to_float2(id.y+i,_floor(lc.y))); //the number of segments in the strip and its horizontal offset
                
                xd = 6.0f+h*4.0f;
                yd = 30.0f;
                lc.x = uv.x*xd+sh.x*9.0f; //divide the strip into segments
                lc.x += _sinf(iTime * (0.5f + h*2.0f))*0.5f; //add a cyclic shift of the strips horizontally
                h = 0.8f*smoothstep(5.0f,0.0f,_fabs(_floor(lc.x)))*cld+0.1f; //determine brightness of the sun track 
                
                //f = _mix(f,_mix(to_float3(0,0.1f,0.5f),to_float3(0.35f,0.35f,0),h),st(lc.y,0.0f,sm*yd)); //mix the color of the water and the color of the track for the background of the water 
                f = _mix(f,_mix(swi3(Water1,x,y,z),swi3(Water2,x,y,z),h),st(lc.y,0.0f,sm*yd)); //mix the color of the water and the color of the track for the background of the water 

                lc += noise(lc*to_float2(3,0.5f))*to_float2(0.1f,0.6f); //add fine noise to the segment
           
                f = _mix(f,                                                                         //mix the background color 
                    _mix(swi3(hue(hash12(_floor(lc))*0.1f+0.56f),x,y,z)*(1.2f+_floor(lc.y)*0.17f),swi3(Stroke,x,y,z),h) //to_float3(1,1,0),h)     //and the stroke color
                    ,st(lc.y,0.0f,sm*xd)
                    *st(_fabs(fract(lc.x)-0.5f),0.48f,sm*xd)*st(_fabs(fract(lc.y)-0.5f),0.3f,sm*yd)
                    );
            }
        }
    }
    
    O = to_float4_aw(f,1);

    ////////////////////// drawing the grass
    a = 0.0f;
    u = uv+noise(uv*2.0f)*0.1f + to_float2(0,_sinf(uv.x*1.0f+3.0f)*0.4f+0.8f);
    
    //f = _mix(to_float3(0.7f,0.6f,0.2f),to_float3(0,1,0),_sinf(iTime*0.2f)*0.5f+0.5f); //color of the grass, changing from green to yellow and back again
    f = _mix(swi3(Grass1,x,y,z),swi3(Grass2,x,y,z),_sinf(iTime*0.2f)*0.5f+0.5f); //color of the grass, changing from green to yellow and back again
    O = _mix(O,to_float4_aw(f*0.4f,1),step(u.y,0.0f)); //draw grass background

    xd = GrassSize;//60.0f;  //grass size
    u = u*to_float2(xd,xd/3.5f); 
    

    if (u.y < 1.2f)
    {
        for (float y = 0.0f; y > -3.0f; y-=1.0f)
          {
            for (float x = -2.0f; x <3.0f; x+=1.0f)
            {
                id = _floor(u) + to_float2(x,y);
                lc = (fract_f2(u) + to_float2(1.0f-x,-y))/to_float2(5,3);
                h = (hash12(id)-0.5f)*0.25f+0.5f; //shade and length for an individual blade of grass

                lc-= to_float2(0.3f,0.5f-h*0.4f);
                lc.x += _sinf(((iTime*1.7f+h*2.0f-id.x*0.05f-id.y*0.05f)*1.1f+id.y*0.5f)*2.0f)*(lc.y+0.5f)*0.5f;
                t = abs_f2(lc)-to_float2(0.02f,0.5f-h*0.5f);
                l =  length(_fmaxf(t,to_float2_s(0.0f))) + _fminf(max(t.x,t.y),0.0f); //distance to the segment (blade of grass)

                l -= noise (lc*7.0f+id)*0.1f; //add fine noise
                C = to_float4_aw(f*0.25f,st(l,0.1f,sm*xd*0.09f)); //grass outline                
                C = _mix(C,to_float4_aw(f                  //grass foregroud
                            *(1.2f+lc.y*2.0f)              //the grass is a little darker at the root
                            *(1.8f-h*2.5f),1.0f)           //brightness variations for individual blades of grass
                            ,st(l,0.04f,sm*xd*0.09f));
                
                O = _mix (O,C,C.w*step (id.y,-1.0f));
                a = _fmaxf (a, C.w*step (id.y,-5.0f));  //a mask to cover the trunk of the tree with grasses in the foreground
            }
        }
    }

    float T = _sinf(iTime*0.5f); //tree swing cycle
 
    if (_fabs(uv.x+tree_pos.x-0.1f-T*0.1f) < 0.6f) // drawing the tree
    {
        u = uv + tree_pos;
        // draw the trunk of the tree first
        u.x -= _sinf(u.y+1.0f)*0.2f*(T+0.75f); //the trunk bends in the wind
        u += noise(u*4.5f-7.0f)*0.25f; //trunk curvature
        
        xd = 10.0f;//,
        yd = 60.0f; 
        t = u * to_float2(1,yd); //divide the trunk into segments
        h = hash12(_floor(swi2(t,y,y))); //horizontal shift of the segments and the color tint of the segment  
        t.x += h*0.01f;
        t.x *= xd;
        
        lc = fract_f2(t); //segment local coordinates
        
        float m = st(_fabs(t.x-0.5f),0.5f,sm*xd)*step(_fabs(t.y+20.0f),45.0f); //trunk mask
        C = _mix(to_float4_s(0.07f) //outline color
                ,ForeGround*(0.4f+h*0.4f) //,to_float4(0.5f,0.3f,0,1)*(0.4f+h*0.4f) //foreground color 
                ,st(_fabs(lc.y-0.5f),0.4f,sm*yd)*st(_fabs(lc.x-0.5f),0.45f,sm*xd));
        C.w = m;
        
        xd = foliageX;//30.0f, 
        yd = foliageY;//15.0f;
        
        for (float xs =0.0f;xs<4.0f;xs+=1.0f) //drawing four layers of foliage
        {
            u = uv + tree_pos + to_float2 (xs/xd*0.5f -(T +0.75f)*0.15f,-0.7f); //crown position
            u += noise(u*to_float2(2,1)+to_float2(-iTime+xs*0.05f,0))*to_float2(-0.25f,0.1f)*smoothstep (0.5f,-1.0f,u.y+0.7f)*0.75f; //leaves rippling in the wind
    
            t = u * to_float2(xd,1.0f);
            h = hash12(_floor(swi2(t,x,x))+xs*1.4f); //number of segments for the row
            
            yd = 5.0f+ h*7.0f + foliageY;
            t.y *= yd;
    
            sh = t;
            lc = fract_f2(t);
            h = hash12(t-lc); //segment color shade
    
            
            t = (t-lc)/to_float2(xd,yd)+to_float2(0,0.7f);
            
            m = (step(0.0f,t.y)*step (length(t),0.45f) //the shape of the crown - the top 
                + step (t.y,0.0f)*step (-0.7f+_sinf((_floor(u.x)+xs*0.5f)*15.0f)*0.2f,t.y)) //the bottom
                *step (_fabs(t.x),0.5f) //crown size horizontally
                *st(_fabs(lc.x-0.5f),0.35f,sm*xd*0.5f); 
    
            lc += noise((sh)*to_float2(1.0f,3.0f))*to_float2(0.3f,0.3f); //add fine noise
           
            f = swi3(hue((h+(_sinf(iTime*0.2f)*0.5f+0.5f))*0.2f),x,y,z)-t.x; //color of the segment changes cyclically
    
            C = _mix(C,
                    to_float4_aw(_mix(f*0.15f,f*0.6f*(0.7f+xs*0.2f), //mix outline and foreground color
                                 st(_fabs(lc.y-0.5f),0.47f,sm*yd)*st(_fabs(lc.x-0.5f),0.2f,sm*xd)),m)
                    ,m);
        }

        O = _mix (O,C,C.w*(1.0f-a));
    }

  SetFragmentShaderComputedColor(O);
}