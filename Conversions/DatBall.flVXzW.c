
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 3x3 matrix
// ---------------------------------------------------------------------------
#if defined(USE_NATIVE_METAL_IMPL)

  typedef float3x3 mat3;

  __DEVICE__ inline float3  f3_multi_mat3( float3 v, mat3   m )  { return v*m; }
  __DEVICE__ inline float3  mat3_multi_f3( mat3   m, float3 v )  { return m*v; }
  __DEVICE__ inline mat3    mat3_multi_mat3( mat3 A, mat3 B)     { return A*B; }

#else
//**** mat3 ****
//typedef struct  
//  {  
//	float3 r0, r1, r2;  
//  } mat3;  
 

__DEVICE__ inline mat3 make_mat3_f( float A)  
  {  
	mat3 D;  
	D.r0.x = A;  
	D.r1.y = A;  
	D.r2.z = A;  
	return D;  
  }
 
 __DEVICE__ inline mat3 make_mat3_f3( float3 A, float3 B, float3 C)  
  {  
	mat3 D;  
	D.r0 = A;  
	D.r1 = B;  
	D.r2 = C;  
	return D;  
  } 
 
__DEVICE__  inline mat3 make_mat3( float A1, float B1, float C1, float A2, float B2, float C2, float A3, float B3, float C3 )  
  {  
	mat3 D;  
	D.r0 = to_float3(A1,B1,C1);  
	D.r1 = to_float3(A2,B2,C2);  
	D.r2 = to_float3(A3,B3,C3);  
	return D;  
  }  
 
__DEVICE__ inline float3 mat3_multi_f3( mat3 B, float3 A) {  
	float3 C;  

	C.x = A.x * B.r0.x + A.y * B.r1.x + A.z * B.r2.x;  
	C.y = A.x * B.r0.y + A.y * B.r1.y + A.z * B.r2.y;  
	C.z = A.x * B.r0.z + A.y * B.r1.z + A.z * B.r2.z;  
	return C;  
  }
 
__DEVICE__ inline float3 f3_multi_mat3( float3 A, mat3 B) {  
  float3 C; 
  
  C.x = A.x * B.r0.x + A.y * B.r0.y + A.z * B.r0.z;  
  C.y = A.x * B.r1.x + A.y * B.r1.y + A.z * B.r1.z;  
  C.z = A.x * B.r2.x + A.y * B.r2.y + A.z * B.r2.z; 
  return C;
 }

__DEVICE__ mat3 mat3_multi_mat3( mat3 A, mat3 B)   //  __DEVICE__ inline mat3 multi( mat3 A, mat3 B)  
{
  float r[3][3];  
  float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
                   {A.r1.x, A.r1.y, A.r1.z},  
                   {A.r2.x, A.r2.y, A.r2.z}};  
  float b[3][3] = {{B.r0.x, B.r0.y, B.r0.z},  
                   {B.r1.x, B.r1.y, B.r1.z},  
                   {B.r2.x, B.r2.y, B.r2.z}};  
     
  for( int i = 0; i < 3; ++i)  
  {  
	  for( int j = 0; j < 3; ++j)  
	  {  
		  r[i][j] = 0.0f;  
		  for( int k = 0; k < 3; ++k)  
		  {  
			r[i][j] = r[i][j] + a[i][k] * b[k][j];  
		  }  
	  }  
  }  
  mat3 R = make_mat3(r[0][0], r[0][1], r[0][2],   
                     r[1][0], r[1][1], r[1][2], 
					           r[2][0], r[2][1], r[2][2]);  
  return R;  
}  

#endif

#define lpowf _powf
__DEVICE__ float3 pow_f3(float3 a, float3 b) {float3 r; r.x = lpowf(a.x,b.x); r.y = lpowf(a.y,b.y); r.z = lpowf(a.z,b.z); return r;}
__DEVICE__ float3 cos_f3(float3 i) {float3 r; r.x = _cosf(i.x); r.y = _cosf(i.y); r.z = _cosf(i.z); return r;}
__DEVICE__ float3 sin_f3(float3 i) {float3 r; r.x = _sinf(i.x); r.y = _sinf(i.y); r.z = _sinf(i.z); return r;}
__DEVICE__ float3 abs_f3(float3 a) {return (to_float3(_fabs(a.x), _fabs(a.y),_fabs(a.z)));}
__DEVICE__ float3 exp_f3(float3 a) {return (to_float3(_expf(a.x), _expf(a.y),_expf(a.z)));}
__DEVICE__ float3 mix_f3(float3 v, float3 i, float3 m) {return to_float3(_mix(v.x,i.x,m.x),_mix(v.y,i.y,m.y),_mix(v.z,i.z,m.z));}	

#define swizy(V) to_float2((V).z,(V).y)
#define swizx(V) to_float2((V).z,(V).x)

#define swixyy(V) to_float3((V).x,(V).y,(V).y)
#define swiyxy(V) to_float3((V).y,(V).x,(V).y)
#define swiyyx(V) to_float3((V).y,(V).y,(V).x)

/*
  Toying with code from :
  Shane's texture bump mapping -> https://www.shadertoy.com/view/MlXSWX
  IQ's raymarch code -> https://www.shadertoy.com/view/Xds3zN
  Nimitz's fog -> https://www.shadertoy.com/view/4ts3z2

  Thanks to Shane for the very useful help.
*/

__DEVICE__ mat3 rotate3(float3 angles)
{
    float3 c = cos_f3(angles);
    float3 s = sin_f3(angles);
    
    mat3 rotX = to_mat3( 1.0f, 0.0f, 0.0f, 0.0f,c.x,s.x, 0.0f,-s.x, c.x);
    mat3 rotY = to_mat3( c.y, 0.0f,-s.y, 0.0f,1.0f,0.0f, s.y, 0.0f, c.y);
    mat3 rotZ = to_mat3( c.z, s.z, 0.0f,-s.z,c.z,0.0f, 0.0f, 0.0f, 1.0f);

    return mat3_multi_mat3(mat3_multi_mat3(rotX , rotY) , rotZ);
}

__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float customShape( float3 p, float2 t )
{  // 3 torus with displacements
    float t1 = length( to_float2(length(swixz(p))-t.x,p.y*0.2f) )-t.y*_cosf(p.z*1.4f);
    float t2 = length( to_float2(length(swiyx(p))-t.x,p.z*0.2f) )-t.y*_cosf(p.x*1.4f);
    float t3 = length( to_float2(length(swizy(p))-t.x,p.x*0.2f) )-t.y*_cosf(p.y*1.4f);  
    return _fminf(min(t1,t2),t3);
}

__DEVICE__ float tri(in float x){return _fabs(fract_f(x)-0.5f);}
__DEVICE__ float3 tri3(in float3 p){return to_float3( tri(p.z+tri(p.y*1.0f)), tri(p.z+tri(p.x*1.0f)), tri(p.y+tri(p.x*1.0f)));}
                                 
//mat2 m2 = to_mat2(0.970f,  0.242f, -0.242f,  0.970f);

__DEVICE__ float triNoise3d(in float3 p, in float spd, float iTime)
{
  float z=1.4f;
  float rz = 0.0f;
  float3 bp = p;
  for (float i=0.0f; i<=3.0f; i+=1.0f )
  {
    float3 dg = tri3(bp*2.0f);
    p += (dg+iTime*spd);

    bp *= 1.8f;
    z *= 1.5f;
    p *= 1.2f;
    //swixz(p)*= m2;

    rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
    bp += 0.14f;
  }
  return rz;
}

__DEVICE__ float fogmap(in float3 p, in float d, float iTime)
{
    p.x += iTime*1.5f;
    p.z += _sinf(p.x*0.5f);
    return triNoise3d(p*2.2f/(d+20.0f),0.2f,iTime)*(1.0f-smoothstep(0.0f,0.7f,p.y));
}

__DEVICE__ float3 fog(in float3 col, in float3 ro, in float3 rd, in float mt, float iTime)
{
    float d = 0.5f;
    for(int i=0; i<7; i++)
    {
      float3  pos = ro + rd*d;
      float rz = fogmap(pos, d, iTime);
      float grd =  clamp((rz - fogmap(pos+0.8f-(float)(i)*0.1f,d,iTime))*3.0f, 0.1f, 1.0f );
      float3 col2 = (to_float3(0.1f,0.1f,0.1f)*0.5f + 0.5f*to_float3(0.1f, 0.1f, 0.1f)*(1.7f-grd))*0.55f;
      col = _mix(col,col2,clamp(rz*smoothstep(d-0.4f,d+2.0f+d*0.75f,mt),0.0f,1.0f) );
      d *= 1.5f+0.3f;
      if (d>mt)break;
    }
    return col;
}

//----------------------------------------------------------------------
__DEVICE__ float getGrey(float3 p){ return p.x*0.299f + p.y*0.587f + p.z*0.114f; }


#define texture(tex,uv) _tex2DVecN(tex, uv.x,uv.y,15)
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){
  
    n = _fmaxf((abs_f3(n) - 0.2f)*7.0f, to_float3_s(0.001f)); // _fmaxf(_fabs(n), 0.001f), etc.
    n /= (n.x + n.y + n.z );  

  return swixyz(texture(tex, (swiyz(p)))*n.x + texture(tex, (swizx(p)))*n.y + texture(tex, (swixy(p)))*n.z);
}

// from Shane : https://www.shadertoy.com/view/MlXSWX
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 nor, float bumpfactor){
   
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

__DEVICE__ float2 map( in float3 p, float iTime )
{
    float dist;
    float2 obj1, obj2, rmap;
       
    float3 pr1 = mat3_multi_f3(rotate3(to_float3(0.2f,0.13f,0.0f)*iTime*2.0f),p);
    float3 pr2 = mat3_multi_f3(rotate3(to_float3(-0.24f,-0.13f,1.1f)*iTime),p);    

    dist = sdSphere(pr1,0.8f) ;
    obj1 = to_float2 ( dist, 1 );

    dist = customShape( pr2-to_float3( 0.0f, 0.0f, 0.0f), to_float2(1.3f,0.1f) );
    obj2 = to_float2 ( dist, 2 );
    
    rmap = opU(obj1,obj2);
    return rmap;
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime )
{
  float3 eps = to_float3( 0.01f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swixyy(eps), iTime).x - map(pos-swixyy(eps), iTime).x,
      map(pos+swiyxy(eps), iTime).x - map(pos-swiyxy(eps), iTime).x,
      map(pos+swiyyx(eps), iTime).x - map(pos-swiyyx(eps), iTime).x );
  return normalize(nor);
}

__DEVICE__ float2 castRay( in float3 ro, in float3 rd, float iTime )
{
    float tmax = 20.0f;  
    float t = 1.0f;
    float oid = -1.0f;
    
    for( int i=0; i<550; i++ )
    {
      float2 res = map( ro+rd*t, iTime );
        if( res.x< 0.001f || t>tmax ) break;
        t += res.x;
        oid = res.y; 
    }

    if( t>tmax ) oid=-1.0f;
    return to_float2(t,oid);
}

__DEVICE__ float3 render( in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2 )
{ 
    float3 col = to_float3_s(0.0f);
    float2 res = castRay(ro,rd,iTime);
    float3 p = ro +rd*res.x;        

    if ( res.x < 20.0f ) 
    {
        
        float3 n = calcNormal(p,iTime);
        float3 lightDir=normalize(to_float3(1.0f,1.0f,-1.0f));
        
        float3 light2Pos=to_float3(2.0f,-1.0f,-4.0f);
        float3 light2Dir=normalize(light2Pos-p);

        float3 tex = to_float3_s(0.0f);
        mat3 rotMat = make_mat3_f( 0.0f );
        if ( res.y == 1.0f ) // sphere
        {
            float3 rotationValues = to_float3(0.2f,0.13f,0.0f) * iTime*2.0f;
            rotMat = rotate3( rotationValues );
            p = mat3_multi_f3(rotMat , p);
            n = mat3_multi_f3(rotMat , n);
            
            n = doBumpMap( iChannel1, p, n, 0.045f);
            tex = tex3D( iChannel1, p, n);
        } else //( res.y == 2.0f ) // custom shape
        {
            float3 rotationValues = to_float3(-0.24f,-0.13f,1.1f) * iTime; 
            rotMat = rotate3( rotationValues );
            p = mat3_multi_f3(rotMat , p);
            n = mat3_multi_f3(rotMat , n);
        
            n = doBumpMap( iChannel2, p, n, 0.045f);
            tex = tex3D( iChannel2, p, n);
        }
    
        lightDir = mat3_multi_f3(rotMat , lightDir);
        light2Dir = mat3_multi_f3(rotMat , light2Dir);
        float b=dot(lightDir,n);
        float b2=dot(light2Dir,n);
        col = ( (b+b2)*tex+_powf( (b+b2)*0.5f,9.0f));    

    } else 
    {
        col = to_float3(0.0f,0.0f,0.0f);
        p = mix_f3(p,ro+rd*20.0f,exp_f3(rd/20.0f));
        float3 btex = swixyz(tex3D(iChannel1,p/20.0f, -rd));
        col = _mix(col,btex,0.9f);
    }

    col = fog( col, ro, rd, 2.2f, iTime);
    
   return ( clamp(col,0.0f,1.0f) );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
  return make_mat3_f3( cu, cv, cw );
}
//****************************************************************************************************************
__KERNEL__ void DatBallFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel1, sampler2D iChannel2)
{

  float2 q = fragCoord/iResolution;
  float2 p = -1.0f+2.0f*q;
  p.x *= iResolution.x/iResolution.y;
  float2 mo = swixy(iMouse)/iResolution;
     
  float time = 15.0f + iTime;

  // camera  
  float3 ro = to_float3( 4.0f, 0.0f, 0.0f);
    
  float3 ta = to_float3( 0.0f, 0.0f, 0.0f );
  
  // camera-to-world transformation
  mat3 ca = setCamera( ro, ta, 0.0f );
    
  // ray direction
  float3 rd = mat3_multi_f3(ca , normalize( to_float3_aw(swixy(p),2.0f) ));

  // render  
  float3 col = render( ro, rd, iTime, iChannel1, iChannel2 );

  col = pow_f3( col, to_float3_s(0.6f) );

  // vignetting from : https://www.shadertoy.com/view/4lSXDm
  float falloff = 0.6f;
  float rf = _sqrtf(dot(p, p)) * falloff;
  float rf2_1 = rf * rf + 1.0f;
  float e = 1.0f / (rf2_1 * rf2_1);
    
  fragColor = to_float4_aw(col * e * 1.3f, 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}


