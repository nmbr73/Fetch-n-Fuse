

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Pillar Wave
//
// SDF from IQ
// AA Option from FN

float PI = 3.14159256;
float TAU = 2.*3.14159256;

// Rotation matrix around the X axis.
// https://www.shadertoy.com/view/fdjGRD
mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

struct ray{
 vec3 direction;
 vec3 origin;
};

// Rounded Box SDF Function
float sdBox( vec3 p, vec3 boxDim, vec3 boxLoc, mat3 transform ){
  p = (p - boxLoc) * transform;
  vec3 q = abs(p) - boxDim;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - 0.035;
}

vec3 calcNormalBox(in vec3 p, vec3 dimVal, vec3 loc, mat3 transform){
  vec2 e = vec2(1.0, -1.0) * .0005; 
  return normalize(
    e.xyy * sdBox(p + e.xyy, dimVal, loc, transform) +
    e.yyx * sdBox(p + e.yyx, dimVal, loc, transform) +
    e.yxy * sdBox(p + e.yxy, dimVal, loc, transform) +
    e.xxx * sdBox(p + e.xxx, dimVal, loc, transform));
}

// easings.net
float easeInOutQuint(float x){
  return x < 0.5 ? 16. * x * x * x * x * x : 1. - pow(-2. * x + 2., 5.) / 2.;
}

// Sphere Tracing
bool sphereTrace(vec3 ro, vec3 rd, out vec3 p, out vec3 pN){
  float mint = -5.0;  // Minimum trace distance
  float maxt = 20.0;   // Maximum trace distance
   
  float dist = mint;
  while(dist < maxt){
    vec3 p = ro + rd*dist;       

    vec3 boxDimension = vec3(.04,.5,.5);
    vec3 boxLocation;
        
    float minD = 9999.;
    float i;
        
    for(i=0.; i<17.; i++){
      boxLocation = vec3(-1.44+.18*i,0.,-2.0);
      float currD = sdBox(p,boxDimension, boxLocation, rotateX(PI*easeInOutQuint(fract(iTime/4.)-(i*.015))));
      minD = min(currD, minD);
      if (minD < .001) break;
    }
        
    pN = calcNormalBox(p, boxDimension, boxLocation, rotateX(PI*easeInOutQuint(fract(iTime/4.)-(i*.015))));       
    dist = dist + minD;
        
    if (minD < .001) return true;
        
  }
  return false;
}

void mainImage0( out vec4 fragColor, in vec2 fragCoord ){
  
  vec2 uv = ( fragCoord - .5* iResolution.xy ) /iResolution.y;
   
  // Background Horizon 1
  // vec3 col = vec3(smoothstep(0.,1.0,pow(19.,-abs(uv.y)-abs(uv.x)*.4)));
  
  //Background Horizon 2
  vec3 col = vec3(pow(1.-(abs(uv.y)),4.0));
   
  // Create ray at eye location, through each point in the "screen"
  ray r;
  r.origin = vec3(0.,0.,4.); 
  r.direction = normalize(vec3(uv,1.) - r.origin);

  vec3 p, pN;
  if(sphereTrace(r.origin, r.direction, p, pN)){
      
      vec3 rr = reflect(r.direction, pN); 
      vec3 reflecter = texture(iChannel0,rr).rgb;
     
      col = reflecter*.3;
     
      // Diffuse light    
      vec3 light = vec3(0.,0.,.3);
      // vec3 light = vec3(.5+.4*sin(iTime),cos(iTime),1.); 
      float dif = clamp(dot(pN, normalize(light-p)),0.,1.);
      dif *= 3./dot(light - p, light - p);
      col *= vec3(pow(dif, 0.4545));
    }
     
   fragColor = vec4(col,1.0);
}

// smart AA, from FabriceNeyret (FN).
void mainImage(out vec4 O, vec2 U) {
    mainImage0(O,U);
    bool AA = false;  // AA option
    if(AA == true)
    if ( fwidth(length(O)) > .01 ) {  // difference threshold between neighbor pixels
        vec4 o;
        for (int k=0; k < 9; k+= k==3?2:1 )
          { mainImage0(o,U+vec2(k%3-1,k/3-1)/3.); O += o; }
        O /= 9.;
    //  O.r++;                        // uncomment to see where the oversampling occurs
    }
}

