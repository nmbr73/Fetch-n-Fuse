
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



/*
  Toying with code from :
  Shane's texture bump mapping -> https://www.shadertoy.com/view/MlXSWX
  IQ's raymarch code -> https://www.shadertoy.com/view/Xds3zN
  Nimitz's fog -> https://www.shadertoy.com/view/4ts3z2

  Thanks to Shane for the very useful help.
*/

__DEVICE__ mat3 rotate3(float3 angles)
{
    float3 c = _cosf(angles);
    float3 s = _sinf(angles);
    
    mat3 rotX = mat3( 1.0f, 0.0f, 0.0f, 0.0f,c.x,s.x, 0.0f,-s.x, c.x);
    mat3 rotY = mat3( c.y, 0.0f,-s.y, 0.0f,1.0f,0.0f, s.y, 0.0f, c.y);
    mat3 rotZ = mat3( c.z, s.z, 0.0f,-s.z,c.z,0.0f, 0.0f, 0.0f, 1.0f);

    return rotX * rotY * rotZ;
}

__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float customShape( float3 p, float2 t )
{  // 3 torus with displacements
    float t1 = length( to_float2(length(swi2(x,z,p))-t.x,p.y*0.2f) )-t.y*_cosf(p.z*1.4f);
    float t2 = length( to_float2(length(swi2(y,x,p))-t.x,p.z*0.2f) )-t.y*_cosf(p.x*1.4f);
    float t3 = length( to_float2(length(swi2(z,y,p))-t.x,p.x*0.2f) )-t.y*_cosf(p.y*1.4f);  
    return _fminf(min(t1,t2),t3);
}

__DEVICE__ float tri(in float x){return _fabs(fract(x)-0.5f);}
__DEVICE__ float3 tri3(in float3 p){return to_float3_aw( tri(p.z+tri(p.y*1.0f)), tri(p.z+tri(p.x*1.0f)), tri(p.y+tri(p.x*1.0f)));}
                                 
mat2 m2 = mat2(0.970f,  0.242f, -0.242f,  0.970f);

__DEVICE__ float triNoise3d(in float3 p, in float spd)
{
    float z=1.4f;
  float rz = 0.0f;
    float3 bp = p;
  for (float i=0.0f; i<=3.0f; i++ )
  {
        float3 dg = tri3(bp*2.0f);
        p += (dg+iTime*spd);

        bp *= 1.8f;
    z *= 1.5f;
    p *= 1.2f;
        //swi2(x,z,p)*= m2;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14f;
  }
  return rz;
}

__DEVICE__ float fogmap(in float3 p, in float d)
{
    p.x += iTime*1.5f;
    p.z += _sinf(p.x*0.5f);
    return triNoise3d(p*2.2f/(d+20.0f),0.2f)*(1.0f-smoothstep(0.0f,0.7f,p.y));
}

__DEVICE__ float3 fog(in float3 col, in float3 ro, in float3 rd, in float mt)
{
    float d = 0.5f;
    for(int i=0; i<7; i++)
    {
        float3  pos = ro + rd*d;
        float rz = fogmap(pos, d);
    float grd =  clamp((rz - fogmap(pos+0.8f-float(i)*0.1f,d))*3.0f, 0.1f, 1.0f );
        float3 col2 = (to_float3(0.1f,0.1f,0.1f)*0.5f + 0.5f*to_float3(0.1f, 0.1f, 0.1f)*(1.7f-grd))*0.55f;
        col = _mix(col,col2,clamp(rz*smoothstep(d-0.4f,d+2.0f+d*0.75f,mt),0.0f,1.0f) );
        d *= 1.5f+0.3f;
        if (d>mt)break;
    }
    return col;
}

//----------------------------------------------------------------------
__DEVICE__ float getGrey(float3 p){ return p.x*0.299f + p.y*0.587f + p.z*0.114f; }

__DEVICE__ float3 tex3D( sampler2D tex, in float3 p, in float3 n ){
  
    n = _fmaxf((_fabs(n) - 0.2f)*7.0f, 0.001f); // _fmaxf(_fabs(n), 0.001f), etc.
    n /= (n.x + n.y + n.z );  
    
  return (texture(tex, swi2(y,z,p))*n.x + texture(tex, swi2(z,x,p))*n.y + texture(tex, swi2(x,y,p))*n.z).xyz;
}

// from Shane : https://www.shadertoy.com/view/MlXSWX
__DEVICE__ float3 doBumpMap( sampler2D tex, in float3 p, in float3 nor, float bumpfactor){
   
    const float eps = 0.001f;
    float ref = getGrey(tex3D(tex,  p , nor));                 
    float3 grad = to_float3( getGrey(tex3D(tex, to_float3(p.x-eps, p.y, p.z), nor))-ref,
                      getGrey(tex3D(tex, to_float3(p.x, p.y-eps, p.z), nor))-ref,
                      getGrey(tex3D(tex, to_float3(p.x, p.y, p.z-eps), nor))-ref )/eps;
             
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
  
}

__DEVICE__ float2 opU( float2 d1, float2 d2 )
{
  return (d1.x<d2.x) ? d1 : d2;
}

__DEVICE__ float2 map( in float3 p )
{
    float dist;
    float2 obj1, obj2, rmap;
       
     float3 pr1 = rotate3(to_float3(0.2f,0.13f,0.0f)*iTime*2.0f)*p;
     float3 pr2 = rotate3(to_float3(-0.24f,-0.13f,1.1f)*iTime)*p;    

  dist = sdSphere(pr1,0.8f) ;
    obj1 = to_float2 ( dist, 1 );

    dist = customShape( pr2-to_float3( 0.0f, 0.0f, 0.0f), to_float2(1.3f,0.1f) );
    obj2 = to_float2 ( dist, 2 );
    
    rmap = opU(obj1,obj2);
    return rmap;
}

__DEVICE__ float3 calcNormal( in float3 pos )
{
  float3 eps = to_float3( 0.01f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(x,y,y,eps)).x - map(pos-swi3(x,y,y,eps)).x,
      map(pos+swi3(y,x,y,eps)).x - map(pos-swi3(y,x,y,eps)).x,
      map(pos+swi3(y,y,x,eps)).x - map(pos-swi3(y,y,x,eps)).x );
  return normalize(nor);
}

__DEVICE__ float2 castRay( in float3 ro, in float3 rd )
{
    float tmax = 20.0f;  
    float t = 1.0f;
    float oid = -1.0f;
    
    for( int i=0; i<550; i++ )
    {
      float2 res = map( ro+rd*t );
        if( res.x< 0.001f || t>tmax ) break;
        t += res.x;
        oid = res.y; 
    }

    if( t>tmax ) oid=-1.0f;
    return to_float2(t,oid);
}

__DEVICE__ float3 render( in float3 ro, in float3 rd )
{ 
    float3 col = to_float3_s(0.0f);
    float2 res = castRay(ro,rd);
    float3 p = ro +rd*res.x;        
    
    if ( res.x < 20.0f ) 
    {
        
        float3 n = calcNormal(p);
        float3 lightDir=normalize(to_float3(1.0f,1.0f,-1.0f));
        
        float3 light2Pos=to_float3(2.0f,-1.0f,-4.0f);
        float3 light2Dir=normalize(light2Pos-p);

        float3 tex = to_float3_s(0.0f);
        mat3 rotMat = mat3( 0.0f );
        if ( res.y == 1.0f ) // sphere
        {
            float3 rotationValues = to_float3(0.2f,0.13f,0.0f) * iTime*2.0f;
            rotMat = rotate3( rotationValues );
          p = rotMat * p;
            n = rotMat * n;
            
            n = doBumpMap( iChannel1, p, n, 0.045f);
            tex = tex3D( iChannel1, p, n);
        } else //( res.y == 2.0f ) // custom shape
        {
            float3 rotationValues = to_float3(-0.24f,-0.13f,1.1f) * iTime; 
            rotMat = rotate3( rotationValues );
          p = rotMat * p;
            n = rotMat * n;
        
            n = doBumpMap( iChannel2, p, n, 0.045f);
            tex = tex3D( iChannel2, p, n);
        }
    
        lightDir = rotMat * lightDir;
        light2Dir = rotMat * light2Dir;
        float b=dot(lightDir,n);
        float b2=dot(light2Dir,n);
        col = to_float3_aw( (b+b2)*tex+_powf( (b+b2)*0.5f,9.0f));    
        
    } else 
    {
        col = to_float3(0.0f,0.0f,0.0f);
        p = _mix(p,ro+rd*20.0f,_expf(rd/20.0f));
        float3 btex = tex3D(iChannel1,p/20.0f, -rd).xyz;
        col = _mix(col,btex,0.9f);
    }
    
    col = fog( col, ro, rd, 2.2f);
    
   return to_float3( clamp(col,0.0f,1.0f) );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3_aw(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

__KERNEL__ void DatBallFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel1, sampler2D iChannel2)
{

  float2 q = fragCoord/iResolution;
    float2 p = -1.0f+2.0f*q;
  p.x *= iResolution.x/iResolution.y;
    float2 mo = iMouse.xy/iResolution;
     
  float time = 15.0f + iTime;

  // camera  
  float3 ro = to_float3( 4.0f, 0.0f, 0.0f);
    
  float3 ta = to_float3( 0.0f, 0.0f, 0.0f );
  
  // camera-to-world transformation
    mat3 ca = setCamera( ro, ta, 0.0f );
    
    // ray direction
  float3 rd = ca * normalize( to_float3(swi2(x,y,p),2.0f) );

    // render  
    float3 col = render( ro, rd );

  col = _powf( col, to_float3_s(0.6f) );

     // vignetting from : https://www.shadertoy.com/view/4lSXDm
     float falloff = 0.6f;
    float rf = _sqrtf(dot(p, p)) * falloff;
    float rf2_1 = rf * rf + 1.0f;
    float e = 1.0f / (rf2_1 * rf2_1);
    
  fragColor = to_float4_aw(col * e * 1.3f, 1.0f);
    


  SetFragmentShaderComputedColor(fragColor);
}


