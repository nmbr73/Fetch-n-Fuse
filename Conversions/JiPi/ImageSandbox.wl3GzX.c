
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//minimum amount is 1.0f
#define BLURAMOUNT 30.0f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Video' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1


__KERNEL__ void ImageSandboxFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    float time = iTime;
    float2 uv = fragCoord / iResolution;

     fragColor = texture(iChannel0,uv)/BLURAMOUNT + (BLURAMOUNT-1.0f)*_tex2DVecN(iChannel1,uv.x,uv.y,15)/BLURAMOUNT;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Video' to iChannel1
// Connect Image 'Texture: Rahmen' to iChannel2

#define BOXDEPTH 2.0f
#define MAXBOXHEIGHT 3.5f
#define PLANEDIST 5.0f
#define BOXX 16.5f
#define BOXY 9.0f
#define SHADOWPASSES 128
#define NUMBOXESX iResolution.x/5.0f
#define NUMBOXESY iResolution.y/5.0f
//#define NUMBOXESX 20.0f
//#define NUMBOXESY 10.0f
#define pixSizeX BOXX/NUMBOXESX
#define pixSizeY BOXY/NUMBOXESY

__DEVICE__ float vmax(float3 v)
{
  return _fmaxf(max(v.x, v.y), v.z);
}

// Box: correct distance to corners
__DEVICE__ float fBox(float3 p, float3 b)
{
  float3 d = abs_f3(p) - b;
  return length(_fmaxf(d, to_float3_s(0))) + vmax(_fminf(d, to_float3_s(0)));
}

// Plane with normal n (n is normalized) at some distance from the origin
__DEVICE__ float fPlane(float3 p, float3 n, float distanceFromOrigin)
{
  return dot(p, n) + distanceFromOrigin;
}

// Repeat in two dimensions
__DEVICE__ float2 pMod2(inout float2 *p, float2 size)
{
    float2 c = _floor((*p + size*0.5f)/size);
    *p = mod_f2f2(*p + size*0.5f,size) - size*0.5f;
    return c;
}

__DEVICE__ float mapImage(float3 p, float2 R, __TEXTURE2D__ iChannel0)
{
    p.x += BOXX - pixSizeX;
    p.y += BOXY + pixSizeY;
    p.z += PLANEDIST + 2.0f*BOXDEPTH;

    //limit the domain repitition to just the inner box. 
    //this is done so that the sand can be higher than the box
    //without showing up outside the box
    if(p.x < - pixSizeX || p.y < pixSizeY || p.x > 2.0f*BOXX -pixSizeX || p.y > 2.0f*BOXY + pixSizeY)
        return 1.0f;
    
    float2 _p = swi2(p,x,y);
    float2 cell = pMod2(&_p, to_float2(pixSizeX*2.0f, pixSizeY*2.0f));    
    p.x=_p.x; p.y=_p.y;
    
    float2 uv = to_float2(cell.x/NUMBOXESX, cell.y/NUMBOXESY);
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    
    float retVal = fBox(p, to_float3(pixSizeX, pixSizeY, BOXDEPTH + col.x*MAXBOXHEIGHT));
    
    return retVal;
}

__DEVICE__ float2 map(in float3 pos, float2 R,__TEXTURE2D__ iChannel0)
{    
    float plane = fPlane(pos, to_float3(0.0f, 0.0f, 1.0f), PLANEDIST);
    float box = fBox(pos, to_float3(BOXX, BOXY, PLANEDIST + 2.0f*BOXDEPTH));
    
    float2 staticScene = to_float2(_fmaxf(plane, -box), -0.5f);

    float2 imageTerrain = to_float2(mapImage(pos,R,iChannel0), 0.5f);
    
    return to_float2(_fminf(staticScene.x, imageTerrain.x),
                     staticScene.x < imageTerrain.x ? staticScene.y : imageTerrain.y);
}

//calc gradient by looking at the local neighborhood
__DEVICE__ float3 calcNormal( in float3 pos, float2 R,__TEXTURE2D__ iChannel0 )
{
  float3 nor;
    
    float2 e = to_float2(0.01f, 0.0f);
    
    nor.x = map(pos + swi3(e,x,y,y),R,iChannel0).x - map(pos - swi3(e,x,y,y),R,iChannel0).x;
    nor.y = map(pos + swi3(e,y,x,y),R,iChannel0).x - map(pos - swi3(e,y,x,y),R,iChannel0).x;
    nor.z = map(pos + swi3(e,y,y,x),R,iChannel0).x - map(pos - swi3(e,y,y,x),R,iChannel0).x;
    
    return normalize(nor);
}

__DEVICE__ float calcShadowAdv( in float3 ro, in float3 rd, in float mint, in float tmax, float2 R,__TEXTURE2D__ iChannel0 )
{
    float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<SHADOWPASSES; i++ )
    {
    float h = map( ro + rd*t,R,iChannel0 ).x;
        
        float y = (i==0) ? 0.0f : h*h/(2.0f*ph);
        
        float d = _sqrtf(h*h-y*y);
        res = _fminf( res, 10.0f*d/_fmaxf(0.0f,t-y) );
        ph = h;
        
        t += h;
        
        if( res<0.0005f || t>tmax ) break;
        
    }
    return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ mat3 rotationXY( float2 angle )
{
  float2 c = cos_f2( angle );
  float2 s = sin_f2( angle );
  
  return to_mat3(
                  c.y      ,  0.0f, -s.y,
                  s.y * s.x,  c.x,  c.y * s.x,
                  s.y * c.x, -s.x,  c.y * c.x);
}

__DEVICE__ float2 rayMarch(in float3 ro, in float3 rd, float tmax, float2 R,__TEXTURE2D__ iChannel0)
{
  //marching
    float t = 0.01f;//progress along the ray
    
    float objectID = 0.0f;
    
    for(int i=0; i<256; i++)
    {
      float3 pos = ro + rd*t;
        
        float2 h = map(pos,R,iChannel0); //returns positive or negative value
        
        if(h.x<0.0005f || t>tmax) //clipping planes
            break;
        
        //t += h.x; //variable distance to object, faster but less accurate than using constant step width
        t += _fmaxf(h.x*0.05f, t * 0.0001f); //less artifacts than the above method
        
        objectID = h.y;
    }
    
    return to_float2(t, objectID);
}

__DEVICE__ float3 render(in float3 ro, in float3 rd, in float3 initCol, float2 R, float4 iMouse, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2)
{
    float3 col = initCol;
    
  //marching
    float tmax = 25.0f;
    float2 t = rayMarch(ro, rd, tmax,R,iChannel0);
   
    //if we are inside the view frustum to render
    if(t.x<tmax)
    {
        //define light
        float3 light = normalize(to_float3(0.5f, 5.0f, 10.0f)); //light direction

        //move light source with the mouse
        {
        float2 xy = (swi2(iMouse,x,y) - iResolution * 0.5f)/iResolution;
        mat3 rot = rotationXY(to_float2(xy.y, -xy.x) );
        light = mul_mat3_f3(rot , light);
        }
        
        float3 pos = ro + rd*t.x;
        float3 nor = calcNormal(pos,R,iChannel0);
        
        //ray march soft shadows
        float sha = calcShadowAdv(pos, light, 0.5f, 5.0f,R,iChannel0);
        
        float3 materialCol = to_float3(0.80f, 0.67f, 0.53f); //sand
        if(t.y < 0.0f)
        {
            if(nor.y > 0.0f)
            materialCol = swi3(texture(iChannel2, swi2(pos,x,z)*0.2f),x,y,z)*0.3f; //box frame inner top/bottom
            else if(nor.z > 0.0f)
              materialCol = swi3(texture(iChannel2, swi2(pos,x,y)*0.2f),x,y,z)*0.3f;  //box frame top
            else
                materialCol = swi3(texture(iChannel2, swi2(pos,y,z)*0.2f),x,y,z)*0.3f;  //box frame inner left/right
        }
        
        float3 lightColor = to_float3(0.9f, 0.9f, 0.9f);
            
        //lighting:
        float dif = clamp( dot( nor, light ), 0.0f, 1.0f );
        float3 lig = lightColor* dif * sha;
        //adding light
          lig += to_float3(0.8f, 0.8f, 0.8f)*nor.y*1.5f; //base illumination
        
        float3 material = materialCol;
        material = _mix(material, lightColor/2.0f, smoothstep(0.7f, 0.9f, (nor.y+nor.x+nor.z)/3.0f));
    
        col = lig * material;
    }
    
    return col;
}

__KERNEL__ void ImageSandboxFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 p = fragCoord / iResolution;
    float2 q = -1.0f + 2.0f*p; //move origin to center of screen
    q.x *= 1.777f; //make square pixels
    
    //ray
    float3 ro = to_float3(0.0f, 0.0f, 5.0f); //camera origin
    float3 rd = normalize( to_float3_aw(q, -1.0f) ); //camera view direction
    
    //mat3 rot = rotationXY( ( swi2(iMouse,x,y) - iResolution * 0.5f ).yx * to_float2( 0.01f, -0.01f ) );
    //ro = rot * ro;
    //rd = rot * rd;
        
    //init color.. 'sky'
    float3 sky = to_float3(0.0f, 0.0f, 0.0f);
    
    //the magic happens here!
    float3 col = render(ro, rd, sky, R,iMouse, iChannel0,iChannel2);
    
    //moving color space in preparation for lighting .. gamma
    col = sqrt_f3(col);
    
    fragColor = to_float4_aw(col,1.0f);  

  SetFragmentShaderComputedColor(fragColor);
}