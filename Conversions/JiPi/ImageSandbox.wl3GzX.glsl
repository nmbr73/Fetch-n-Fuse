

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define BOXDEPTH 2.0
#define MAXBOXHEIGHT 3.5
#define PLANEDIST 5.0
#define BOXX 16.5
#define BOXY 9.
#define SHADOWPASSES 128
#define NUMBOXESX iResolution.x/5.
#define NUMBOXESY iResolution.y/5.
//#define NUMBOXESX 20.0
//#define NUMBOXESY 10.0
#define pixSizeX BOXX/NUMBOXESX
#define pixSizeY BOXY/NUMBOXESY

float vmax(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

// Box: correct distance to corners
float fBox(vec3 p, vec3 b)
{
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}

// Plane with normal n (n is normalized) at some distance from the origin
float fPlane(vec3 p, vec3 n, float distanceFromOrigin)
{
	return dot(p, n) + distanceFromOrigin;
}

// Repeat in two dimensions
vec2 pMod2(inout vec2 p, vec2 size)
{
    vec2 c = floor((p + size*0.5)/size);
    p = mod(p + size*0.5,size) - size*0.5;
    return c;
}

float mapImage(vec3 p)
{
    p.x += BOXX - pixSizeX;
    p.y += BOXY + pixSizeY;
    p.z += PLANEDIST + 2.*BOXDEPTH;
    
    //limit the domain repitition to just the inner box. 
    //this is done so that the sand can be higher than the box
    //without showing up outside the box
    if(p.x < - pixSizeX || p.y < pixSizeY || p.x > 2.*BOXX -pixSizeX || p.y > 2.*BOXY + pixSizeY)
        return 1.0;
    
    vec2 cell = pMod2(p.xy, vec2(pixSizeX*2., pixSizeY*2.));    
    
    vec2 uv = vec2(cell.x/NUMBOXESX, cell.y/NUMBOXESY);
    vec3 col = texture(iChannel0, uv).rgb;
    
    float retVal = fBox(p, vec3(pixSizeX, pixSizeY, BOXDEPTH + col.r*MAXBOXHEIGHT));
    
    return retVal;
}

vec2 map(in vec3 pos)
{    
    float plane = fPlane(pos, vec3(.0, .0, 1.0), PLANEDIST);
    float box = fBox(pos, vec3(BOXX, BOXY, PLANEDIST + 2.*BOXDEPTH));
    
    vec2 staticScene = vec2(max(plane, -box), -0.5);
    
    vec2 imageTerrain = vec2(mapImage(pos), 0.5);
    
    return vec2(min(staticScene.x, imageTerrain.x),
                staticScene.x < imageTerrain.x ? staticScene.y : imageTerrain.y);
}

//calc gradient by looking at the local neighborhood
vec3 calcNormal( in vec3 pos )
{
	vec3 nor;
    
    vec2 e = vec2(0.01, 0.0);
    
    nor.x = map(pos + e.xyy).x - map(pos - e.xyy).x;
    nor.y = map(pos + e.yxy).x - map(pos - e.yxy).x;
    nor.z = map(pos + e.yyx).x - map(pos - e.yyx).x;
    
    return normalize(nor);
}

float calcShadowAdv( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<SHADOWPASSES; i++ )
    {
		float h = map( ro + rd*t ).x;
        
        float y = (i==0) ? 0.0 : h*h/(2.0*ph);
        
        float d = sqrt(h*h-y*y);
        res = min( res, 10.0*d/max(0.0,t-y) );
        ph = h;
        
        t += h;
        
        if( res<0.0005 || t>tmax ) break;
        
    }
    return clamp( res, 0.0, 1.0 );
}

mat3 rotationXY( vec2 angle )
{
	vec2 c = cos( angle );
	vec2 s = sin( angle );
	
	return mat3(
		c.y      ,  0.0, -s.y,
		s.y * s.x,  c.x,  c.y * s.x,
		s.y * c.x, -s.x,  c.y * c.x);
}

vec2 rayMarch(in vec3 ro, in vec3 rd, float tmax)
{
	//marching
    float t = 0.01;//progress along the ray
    
    float objectID = 0.0;
    
    for(int i=0; i<256; i++)
    {
    	vec3 pos = ro + rd*t;
        
        vec2 h = map(pos); //returns positive or negative value
        
        if(h.x<0.0005 || t>tmax) //clipping planes
            break;
        
        //t += h.x; //variable distance to object, faster but less accurate than using constant step width
        t += max(h.x*0.05, t * 0.0001); //less artifacts than the above method
        
        objectID = h.y;
    }
    
    return vec2(t, objectID);
}

vec3 render(in vec3 ro, in vec3 rd, in vec3 initCol)
{
    vec3 col = initCol;
    
	//marching
    float tmax = 25.0;
    vec2 t = rayMarch(ro, rd, tmax);
   
    //if we are inside the view frustum to render
    if(t.x<tmax)
    {
        //define light
        vec3 light = normalize(vec3(0.5, 5., 10.0)); //light direction

        //move light source with the mouse
        {
        vec2 xy = (iMouse.xy - iResolution.xy * 0.5)/iResolution.xy;
        mat3 rot = rotationXY(vec2(xy.y, -xy.x) );
        light = rot * light;
        }
        
        vec3 pos = ro + rd*t.x;
        vec3 nor = calcNormal(pos);
        
        //ray march soft shadows
        float sha = calcShadowAdv(pos, light, 0.5, 5.0);
        
        vec3 materialCol = vec3(0.80, 0.67, 0.53); //sand
        if(t.y < 0.0)
        {
            if(nor.y > .0)
        		materialCol = texture(iChannel1, pos.xz*0.2).rgb*0.3; //box frame inner top/bottom
            else if(nor.z > .0)
            	materialCol = texture(iChannel1, pos.xy*0.2).rgb*0.3;  //box frame top
            else
                materialCol = texture(iChannel1, pos.yz*0.2).rgb*0.3;  //box frame inner left/right
        }
        
        vec3 lightColor = vec3(0.9, 0.9, 0.9);
            
        //lighting:
        float dif = clamp( dot( nor, light ), 0.0, 1.0 );
        vec3 lig = lightColor* dif * sha;
        //adding light
        	lig += vec3(0.8, 0.8, 0.8)*nor.y*1.5; //base illumination
        
        vec3 material = materialCol;
        material = mix(material, lightColor/2.0, smoothstep(0.7, 0.9, (nor.y+nor.x+nor.z)/3.));
		
        col = lig * material;
    }
    
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
 	vec2 p = fragCoord.xy / iResolution.xy;
    vec2 q = -1.0 + 2.0*p; //move origin to center of screen
    q.x *= 1.777; //make square pixels
    
    //ray
    vec3 ro = vec3(.0, .0, 5.0); //camera origin
    vec3 rd = normalize( vec3(q, -1.0) ); //camera view direction
    
    //mat3 rot = rotationXY( ( iMouse.xy - iResolution.xy * 0.5 ).yx * vec2( 0.01, -0.01 ) );
	//ro = rot * ro;
    //rd = rot * rd;
        
    //init color.. 'sky'
    vec3 sky = vec3(0.0, 0.0, 0.0);
    
    //the magic happens here!
    vec3 col = render(ro, rd, sky);
    
    //moving color space in preparation for lighting .. gamma
    col = sqrt(col);
    
	fragColor = vec4(col,1.0);  
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime;
    vec2 uv = fragCoord.xy / iResolution.xy;

   	fragColor = texture(iChannel0,uv)/BLURAMOUNT + (BLURAMOUNT-1.)*texture(iChannel1,uv)/BLURAMOUNT;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//minimum amount is 1.0
#define BLURAMOUNT 30.0