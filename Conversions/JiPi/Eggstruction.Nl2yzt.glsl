

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define ro(r) mat2(cos(r),sin(r),-sin(r),cos(r))

float box(vec3 p, vec3 d) {
  p = abs(p)-d;
  return length(max(p,0.))+min(0.,max(p.x,max(p.y,p.z)));
}

float beam(vec3 p) {
  p.xy -= max(0.,dot(normalize(vec2(1.)),p.xy))*2.*normalize(vec2(1));
  p.xy -= max(0.,dot(normalize(vec2(1,-1)),p.xy))*2.*normalize(vec2(1,-1));
  p.x += .2;
  vec3 p2 = p;
  p2.y = abs(p2.y)-.2;
  p.z = asin(sin(p.z*10.))/10.;
  p.yz *= ro(radians(45.));
  return min(length(p2.xy),length(p.xy))-.02;
}

float zip(float k) {
  k += iTime;
  float id = floor(k)+.5;
  k -= id;
  k = smoothstep(-.1,.1,k)+smoothstep(-.2,.2,k)-smoothstep(-.3,.3,k);
  k+=id;
  return k;
}

float o1, o2, o3;
vec3 oop;
float scene(vec3 p) {
  p.xy *= ro(asin(sin(zip(.4)/2.))*.7);
  o1 = beam(p);
  o2 = beam(p.xzy);
  o3 = box(p,vec3(.2))-.1;
  o3 = min(o3,box(p-vec3(0,1,0),vec3(.2,.2,.4))-.1);
  o3 = min(o3,box(p-vec3(0,-5,0),vec3(.2,.05,.2))-.1);
  o3 = min(o3,box(p-vec3(0,0,.6),vec3(.2,.2,.05))-.1);
  o2 = max(o2,p.y-.79);
  o2 = max(o2,-p.y-5.);
  o1 = max(o1,p.z-.48);
  p.z+=.4;
  float op = p.z;
  p.y += 2.5 + asin(sin(zip(0.)));
  o3  = min(o3, box(p,vec3(.2,.3,.1))-0.05);
  
  vec3 p2 = p;
  p2.y=abs(p.y)-.1;
  p.z+=.4 + max(0.,asin(sin(iTime)))*8.;
  o3  = min(o3, box(p,vec3(.2,.3,.1))-0.05);
  o3 = min(o3,max(max(op,-p.z),length(p2.xy)-.02));
  p.z+=.4;
  p.xy*=ro(iTime/3.);
  oop = p;
  p.z*=.8;
  float e = length(p)-.25+smoothstep(-.5,.5,p.z)*.4-.2;
  if (asin(sin(iTime/2. - 3.14159/4.)) < 0.) e = 1e5;
  return min(min(o3,e),min(o1,o2));
}

vec3 norm(vec3 p) {
  mat3 k = mat3(p,p,p)-mat3(0.001);
  return normalize(scene(p)-vec3( scene(k[0]),scene(k[1]),scene(k[2]) ));
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
  

  vec3 cam = normalize(vec3(1.+sin(zip(.3)/2.)*.5,uv));
  float v = sin(zip(.7))*.3;
  vec3 init = vec3(-6,-2.+v,-1.+cos(zip(2.)*2.)*.25);
  
  cam.xz*=ro(.3);
  init.xz*=ro(.3);
  
  vec3 p = init;
  bool hit = false;
  float dist;
  bool trig = false;
  for(int i = 0; i<50&&!hit;i++){ 
    dist = scene(p);
    p+=cam*dist;
    hit = dist*dist<1e-6;
    if(!trig)trig=dist<0.03;
    if(dist>1e4)break;
  }
  vec3 lp = oop;
  bool io1 = dist==o1;
  bool io2 = dist==o2;
  bool io3 = dist==o3;
  vec3 n = norm(p);
  vec3 r = reflect(cam,n);
  float fact = length(sin(r*3.)*.5+.5)/sqrt(3.);
  vec3 matcol = texture(iChannel0,clamp(lp.yz*1.8+vec2(0,.3)+.5,0.,1.)).xyz;
  if(io1)matcol=vec3(.7,.2,.1);
  if(io3)matcol=vec3(.05);
  if(io2)matcol=vec3(.7,.6,.05);
  vec3 col = matcol+pow(fact,7.)*2.;
  fragColor.xyz=hit?sqrt(col):vec3(trig?0.:uv.y*.5+.5);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//from https://www.shadertoy.com/view/4sKyRK
float distanceToBottleCurve(vec2 point) {
    return point.y-0.1*sin(point.x*2.5 + 0.6) + 0.05*sin(5.0*point.x) + 0.04*sin(7.5*point.x);
}
bool texturee(vec2 uv) {
    float ang = atan(uv.y, uv.x);
    float len = floor(length(uv)*10.0);
    bool val = len == 2. || len == 6. || len == 9.;
    if (len == 3. || len == 4. || len == 5. || len == 8. || len == 10.) {
        val = distanceToBottleCurve(vec2(ang+len,0.0))*7.99 > cos(len*7.99);
    }
    return val;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy-.5;
    //I would make a better revision logo but I am too tired :(
    fragColor = vec4(1.0,91.0,188.0,1.0)/255.;
    float rad1 = abs(floor(atan(uv.x,uv.y)/2.)*.04);
    if (texturee(uv*2.5)) {
        fragColor = vec4(255.0,214.0,0.0,1.0)/255.;
    }
}