

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const vec3 background = vec3(0.85);
const bool rounded = true;
const bool logo = true;
const bool shadows = true;
const int  reflections = 1;
const bool antialiasing = true;

const bool showLogoTexture = false;
const bool showBlurred = false;

const float tolerance = 0.005;
const int steps = 256;
const float minStep = 0.001;

const float studRadius = 0.3;
const float studHeight = 0.1;
const float logoHeight = 0.015;

const float pi = 3.1416;
const vec3 forward = vec3(0.0, 0.0, -1.0);
const vec3 right = vec3(1.0, 0.0, 0.0);
const vec3 up = vec3(0.0, 1.0, 0.0);

float rounding = rounded ? 0.016 : 0.0;

vec3 toSRGB(in vec3 color) { return pow(color, vec3(1.0 / 2.2)); }

float sdRound(in float sd, in float radius) { return sd - radius; }

float sdSmoothUnion(in float sd1, float sd2, in float radius) {
  float h = clamp(0.5 + 0.5 * (sd2 - sd1) / radius, 0.0, 1.0);
  return mix(sd2, sd1, h) - radius * h * (1.0 - h);
}

float sdSmoothSubtraction(in float sd1, in float sd2, in float k) {
  float h = clamp(0.5 - 0.5 * (sd1 + sd2) / k, 0.0, 1.0);
  return mix(sd1, -sd2, h) + k * h * (1.0 - h); 
}

float sdBox(in vec3 position, in vec3 dimensions) {
  vec3 q = abs(position) - dimensions;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdBrick(in vec3 position, in vec3 size) {
  float brick = sdBox(position, size - rounding);
  float result = sdRound(brick, rounding);

  return result;
}

float sdCylinder(in vec3 position, in float radius, in float height) {
  vec2 d = abs(vec2(length(position.xz), position.y)) - vec2(radius, height);
  return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdStuds(in vec3 position, in vec3 size) {
  position.xz = fract(position.xz);
  position.xz -= vec2(0.5);
  position.y -= size.y + studHeight - rounding * 2.0;
    
  float studs = sdCylinder(position, 
                           studRadius - rounding * 2.0, 
                           studHeight);
    
  return sdRound(studs, rounding * 2.0);
}

float sdLogo(in vec3 position) {
  position.xz = fract(position.xz);
    
  float tex = texture(iChannel2, clamp(vec2(position.x, 1.0 - position.z), 0.01, 1.0)).r;
  return -pow(tex, 0.4) * logoHeight;
}

float sdCutout(in vec3 position, in vec3 size) {
  position.y += 0.2 - logoHeight * 2.0;
    
  float box = sdBox(position, vec3(size.x - 0.18 - rounding, size.y, size.z - 0.18 - rounding));
    
  return sdRound(box, rounding);
}

float sdStudsCutout(in vec3 position, in vec3 size) {   
  position.x -= fract(size.x);
  position.z -= fract(size.z);
  position.xz = fract(position.xz);
  float cutouts = sdCylinder(position - vec3(0.5, 0.0, 0.5), 0.15 - rounding, size.y - rounding - 0.01);
    
  return sdRound(cutouts, rounding);  
}

float sdTubes(in vec3 position,  in vec3 size, in float innerRadius) {
  float radius = innerRadius - rounding;
  float height = size.y - rounding - 0.05;
  
  float halfWidth = size.x * 0.5;
 
  if (size.x > 0.5) {
    position.x = clamp(position.x, -size.x + 0.5, size.x - 0.5);
  }
  if (size.z > 0.5) {
    position.z = clamp(position.z, -size.z + 0.5, size.z - 0.5);
  }
  position.x -= fract(size.x) - (size.x < 1.0 ? 0.0 : 0.5);
  position.z -= fract(size.z) - (size.z < 1.0 ? 0.0 : 0.5);
  position.xz = fract(position.xz);
  position -= vec3(0.5, -0.05, 0.5);
  float tubes = sdRound(sdCylinder(position, radius + 0.11, height), rounding);
  float cutout = sdCylinder(position, innerRadius + 0.01, height + 0.2);

  return sdSmoothSubtraction(tubes, cutout, rounding);
}

vec2 rotation = vec2(0.0, 0.68);
vec3 brickSize = vec3(1.0, 0.6, 1.5);
vec3 brickColor = vec3(0.03, 0.55, 0.79);

float map(in vec3 position) {
  float result = sdBrick(position, brickSize);
    
  if (position.y < brickSize.y) {  
    float cutout = sdCutout(position, brickSize);
    float studs = sdStudsCutout(position, brickSize); 
      
    bool smallTube = brickSize.x < 1.0 || brickSize.z < 1.0;
      
    result = sdSmoothSubtraction(result, cutout, rounding);
    result = sdSmoothSubtraction(result, studs, rounding);
      
    if (brickSize.x > 0.5 || brickSize.z > 0.5) {
      float tubes = sdTubes(position, brickSize, smallTube ? 0.073 : studRadius);
      result = sdSmoothUnion(result, tubes, rounding);
    }
  }
    
  if (position.y > brickSize.y) {
    position.x -= mod(brickSize.x, 1.0);
    position.z -= mod(brickSize.z, 1.0);
    float studs = sdStuds(position, brickSize);
    if (studs < logoHeight * 2.0) { 
      studs += logo ? sdLogo(position) : 0.0; 
    }
    result = sdSmoothUnion(result, studs, rounded ? 0.015 : 0.0);  
  }    
    
  return result;
}

vec3 calculateNormal(in vec3 position, in float pixelSize) {
  vec2 e = vec2(1.0, -1.0) * pixelSize * 0.1;
  return normalize(
      e.xyy * map(position + e.xyy) + e.yyx * map(position + e.yyx) +
      e.yxy * map(position + e.yxy) + e.xxx * map(position + e.xxx));
}

struct AABB {
  vec3 min;
  vec3 max;
};

struct Ray {
  vec3 origin;
  vec3 direction;
};
    
mat4 rotationMatrix(in vec3 axis, in float angle) {
  axis = normalize(axis);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1.0 - c;

  return mat4(
      oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
      oc * axis.z * axis.x + axis.y * s, 0.0, oc * axis.x * axis.y + axis.z * s,
      oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
      oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s,
      oc * axis.z * axis.z + c, 0.0, 0.0, 0.0, 0.0, 1.0);
}

mat4 translationMatrix(in vec3 translation) {
  return mat4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
              translation.x, translation.y, translation.z, 1.0);
}
    
Ray createRayPerspective(in vec2 resolution, in vec2 screenPosition,
                         in float verticalFov) {
  vec2 topLeft = vec2(-resolution.x, -resolution.y) * 0.5;
  float z = (resolution.x * 0.5) / abs(tan(verticalFov * 0.5));

  return Ray(vec3(0.0), normalize(vec3(topLeft + screenPosition, -z)));
}

vec3 positionOnRay(in Ray ray, in float t) {
  return ray.origin + ray.direction * t;
}

void transformRay(inout Ray ray, mat4 matrix) {
  ray.origin = (matrix * vec4(ray.origin, 1.0)).xyz;
  ray.direction = normalize(matrix * vec4(ray.direction, 0.0)).xyz;
}

void reflectRay(inout Ray ray, vec3 position, vec3 normal) {
  ray.origin = position + normal * 0.01;
  ray.direction = reflect(ray.direction, normal);
}

void transformNormal(inout vec3 normal, in mat4 matrix) {
  normal = normalize((matrix * vec4(normal, 0.0)).xyz);
}

bool rayIntersectsAABB(in Ray ray, in AABB aabb, out float t0, out float t1) {
  vec3 invR = 1.0 / ray.direction;

  vec3 tbot = invR * (aabb.min - ray.origin);
  vec3 ttop = invR * (aabb.max - ray.origin);

  vec3 tmin = min(ttop, tbot);
  vec3 tmax = max(ttop, tbot);

  vec2 t = max(tmin.xx, tmin.yz);
  t0 = max(t.x, t.y);
  t = min(tmax.xx, tmax.yz);
  t1 = min(t.x, t.y);

  return t0 <= t1;
}

vec3 shade(in Ray ray, in vec3 position, in vec3 normal, in vec3 light) {
  vec3 reflection = reflect(-light, normal); 
    
  float diffuse = max(0.0, dot(light, normal));
  float specular = pow(max(dot(-ray.direction, reflection), 0.0), 16.0);
        
  return diffuse * brickColor + vec3(0.3) * specular;
}

vec4 blend(in vec4 under, in vec4 over) {
  vec4 result = mix(under, over, over.a);
  result.a = over.a + under.a * (1.0 - over.a);
    
  return result;
}

bool intersectsBrick(inout Ray ray, in AABB aabb) {
  float t, t1;
  if (!rayIntersectsAABB(ray, aabb, t, t1)) {
    return false;
  }
 
  t = max(t, 0.0);  
  for (int i = 0; i < steps; i++) {
    vec3 position = positionOnRay(ray, t);
    float sd = map(position);
     
    if (sd < tolerance) {
      ray.origin = position;
      return true;
    }
    
    t += max(tolerance, sd);
      
    if (t > t1) {
      return false;
    }
  }
    
  return false;
}

float intersectsBrickShadow(inout Ray ray, in AABB aabb) {
  float res = 1.0;
  float ph = 1e10;
  float k = 10.0;
    
  float t, t0, t1;
  if (!rayIntersectsAABB(ray, aabb, t, t1)) {
    return res;
  }
  t = max(0.0, t0);
  for (int i = 0; i < steps; i++) {
    vec3 position = positionOnRay(ray, t);
    float sd = map(position);
     
    float y = sd * sd / (2.0 * ph);
    float d = sqrt(sd * sd - y * y);
    res = min(res, k * d / max(0.0, t - y));
    ph = sd;
      
    t += min(sd, 0.005);
      
    if (res < 0.0001 || t > t1) {
      break;
    }
  }
    
  return clamp(res, 0.0, 1.0);
}

vec4 trace(in Ray ray) {
  AABB aabb = AABB(-brickSize, brickSize);
  aabb.min -= vec3(0.01);
  aabb.max += vec3(0.01);
  aabb.max.y += (studHeight + logoHeight) * 2.0;
    
  mat4 transform = rotationMatrix(up, rotation.x) * 
                   rotationMatrix(right, rotation.y) *
                   translationMatrix(vec3(0.0, 0.0, 6.0));
    
  vec3 light = vec3(-0.9, 0.9, 2.5);
  
  vec3 forward = forward;
  transformRay(ray, transform);
  transformNormal(light, transform);
  transformNormal(forward, transform);
  
  float mul = 1.0;
  vec3 result = vec3(0.0);
  for (int i = 0; i <= reflections; i++) {
    if (intersectsBrick(ray, aabb)) {
      vec3 normal = calculateNormal(ray.origin, 0.001);
      float shadow = 1.0;
      
      if (shadows) {
        Ray shadowRay = Ray(ray.origin, normalize(light));
        shadowRay.origin += normal * 0.001;
        shadow = intersectsBrickShadow(shadowRay, aabb);
      }
          
      result += mul * brickColor * 0.2;
      result += mul * shadow * shade(ray, ray.origin, normal, light);
      
      reflectRay(ray, ray.origin, normal);
      mul *= 0.045;
    } else {
      result += mul * background;
      break;
    }
  }
    
  return vec4(result, 1.0);
}

vec4 takeSample(in vec2 position, float pixelSize) {
  const float fov = pi / 2.0;
  Ray ray = createRayPerspective(iResolution.xy, position, fov);
  return trace(ray);
}

vec4 superSample(in vec2 fragCoord) {
  const int sampleCount = antialiasing ? 4 : 1;
  const vec2[] samplePositions = vec2[](                             
    vec2(-0.125, -0.375), vec2(0.375, -0.125),      
    vec2(-0.375,  0.125), vec2(0.125,  0.375)       
  );                                                         
  vec4 result = vec4(0.0);                                    
  float samplesSqrt = sqrt(float(sampleCount));                        
  for (int i = 0; i < sampleCount; i++) {                              
    result += takeSample(fragCoord + samplePositions[i],               
                         1.0 / samplesSqrt);                           
  }                                                                    
                                                                         
  return result / float(sampleCount);                                  
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  rotation = texelFetch(STORAGE, rotationDataLocation, 0).xy;
  brickSize = texelFetch(STORAGE, brickSizeDataLocation, 0).rgb;
  brickColor = texelFetch(STORAGE, brickColorDataLocation, 0).rgb;
    
  if (showLogoTexture) {
    vec2 uv = (fragCoord - iResolution.xy * 0.5) / iResolution.y;
    uv += vec2(0.5);
    if (showBlurred) {
      fragColor = texture(iChannel2, uv);
    } else {
      fragColor = texture(iChannel1, uv);
    }
    return;
  }  
    
  fragColor = vec4(toSRGB(superSample(fragCoord).rgb), 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define STORAGE iChannel0
const ivec2 resolutionDataLocation = ivec2(0);
const ivec2 rotationDataLocation = ivec2(1, 0);
const ivec2 brickSizeDataLocation = ivec2(2, 0);
const ivec2 brickColorDataLocation = ivec2(3, 0);

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const int KEY_SHIFT = 16;
const int KEY_SPACE = 32;
const int KEY_LEFT  = 37;
const int KEY_UP    = 38;
const int KEY_RIGHT = 39;
const int KEY_DOWN  = 40;
const int KEY_C     = 67;

bool saveResolutionData(in ivec2 fragCoord, out vec4 fragColor) {
  if (fragCoord == resolutionDataLocation) {
    vec4 previousResolutionData = texelFetch(STORAGE, fragCoord, 0);
    vec2 oldResolution = previousResolutionData.xy;
    fragColor = vec4(iResolution.xy, oldResolution);

    return true;
  }

  return false;
}

bool isKeyUp(in int key) {
  return texelFetch(iChannel1, ivec2(key, 1), 0).x > 0.0;
}

bool isKeyToggled(in int key) {
  return texelFetch(iChannel1, ivec2(key, 2), 0).x > 0.0;
}

bool saveRotationData(in ivec2 fragCoord, out vec4 fragColor) {
  if (fragCoord == rotationDataLocation) {
    if (iFrame == 0) {
      fragColor = vec4(0.0, 0.68, 0.0, 0.0);
    } else {
      vec2 rotation = texelFetch(STORAGE, rotationDataLocation, 0).xy;
      rotation.x += isKeyToggled(KEY_SPACE) ? 0.0 : 0.003;
      rotation.y = iMouse.x > 0.0 
        ? mix(rotation.y, -iMouse.y / iResolution.y * 8.0 - 1.0, 0.03) 
        : rotation.y;
      fragColor = vec4(rotation, 0.0, 0.0);
    }
    return true;
  }
    
  return false;
}


bool saveBrickSizeData(in ivec2 fragCoord, out vec4 fragColor) {
  if (fragCoord == brickSizeDataLocation) {
    if (iFrame == 0) {
      fragColor = vec4(vec3(1.0, 0.6, 1.5), 0.0);
    } else {
      vec3 size = texelFetch(STORAGE, brickSizeDataLocation, 0).rgb;
      if (isKeyUp(KEY_DOWN)) {
        size.z -= 0.5;
      }
      if (isKeyUp(KEY_UP)) {
        size.z += 0.5;
      }
      if (isKeyUp(KEY_LEFT)) {
        size.x -= 0.5;
      }
      if (isKeyUp(KEY_RIGHT)) {
        size.x += 0.5;
      }
      size.xz = clamp(size.xz, 0.5, 8.0);
      size.y = isKeyToggled(KEY_SHIFT) ? 0.2 : 0.6;
        
      fragColor = vec4(size, 0.0);
    }
    return true;
  }
    
  return false;
}

const int colorsLength = 12;
const vec3[] colors = vec3[](
  vec3(0.949, 0.803, 0.215), // Bright Yellow
  vec3(0.996, 0.541, 0.094), // Bright Orange
  vec3(0.788, 0.101, 0.035), // Bright Red
  vec3(0.784, 0.439, 0.627), // Bright Purple
  vec3(0.0  , 0.333, 0.749), // Bright Blue
  vec3(0.039, 0.203, 0.388), // Earth Blue
  vec3(0.027, 0.545, 0.788), // Dark Azur
  vec3(0.137, 0.470, 0.254), // Dark Green
  vec3(0.294, 0.623, 0.290), // Bright Green
  vec3(0.733, 0.913, 0.043), // Bright Yellowish Green
  vec3(0.345, 0.164, 0.070), // Reddish Brown
  vec3(0.019, 0.074, 0.113)  // Black
);

vec3 toLinear(in vec3 color) { return pow(color, vec3(2.2)); }

bool saveBrickColorData(in ivec2 fragCoord, out vec4 fragColor) {
  if (fragCoord == brickColorDataLocation) {
    if (iFrame == 0) {
      int index = 2;
      fragColor = vec4(toLinear(colors[index]), float(index));
    } else {
      vec4 color = texelFetch(STORAGE, brickColorDataLocation, 0);
      if (isKeyUp(KEY_C)) {
        color.w += 1.0;
      }
      color.w = float(int(color.w) % colorsLength);
      color.rgb = mix(color.rgb, toLinear(colors[int(color.w)]), 0.15);
        
      fragColor = color;
    }
    return true;
  }
    
  return false;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  fragColor = vec4(0.0);
  ivec2 iFragCoord = ivec2(fragCoord);  
    
  if (saveResolutionData(iFragCoord, fragColor)) {
    return;
  }
    
  if (saveRotationData(iFragCoord, fragColor)) {
    return;
  }
    
  if (saveBrickSizeData(iFragCoord, fragColor)) {
    return;
  }
    
  if (saveBrickColorData(iFragCoord, fragColor)) {
    return;
  }
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Created with Shadertoy-SVG: https://zduny.github.io/shadertoy-svg/

const int samples = 4; // (square root, actual number = samples^2)

const vec3 positions[225] =
    vec3[225](vec3(0.9408613367791228, 0.43095909706765456, 0),
              vec3(0.9264762435267317, 0.44352663509707624, 0),
              vec3(0.8947861254762883, 0.4638692865687953, 0),
              vec3(0.8777536433400679, 0.4715814511436973, 0),
              vec3(0.841995189500208, 0.48193008580309815, 0),
              vec3(0.8048872251098951, 0.48530129825064333, 0),
              vec3(0.7861678547856077, 0.4842916850607259, 0),
              vec3(0.7490796945333518, 0.47672464768502254, 0),
              vec3(0.7309834472672143, 0.47010427463184123, 0),
              vec3(0.6967507470536414, 0.45128546857518964, 0),
              vec3(0.6815966127536668, 0.4397110894047105, 0),
              vec3(0.6551836294986841, 0.41262743744267205, 0),
              vec3(0.634461035245403, 0.3809322504759403, 0),
              vec3(0.626419887197555, 0.3635934525495947, 0),
              vec3(0.6174678863278233, 0.3359891364381042, 0),
              vec3(0.47899216991926896, -0.24549629059456585, 0),
              vec3(0.4741737266143069, -0.2751084016236367, 0),
              vec3(0.473508033500468, -0.29441126217125624, 0),
              vec3(0.4778079185647557, -0.33209980412273227, 0),
              vec3(0.4891422806195873, -0.36776620212046324, 0),
              vec3(0.5069424579864692, -0.40045137004997605, 0),
              vec3(0.5306397889869092, -0.42919622179679773, 0),
              vec3(0.5445221803251843, -0.44179131469092675, 0),
              vec3(0.575999001128787, -0.4628274056990749, 0),
              vec3(0.5934512651744912, -0.471028632284476, 0),
              vec3(0.6313136680076146, -0.48217771366078943, 0),
              vec3(0.6700652677983625, -0.48548096726553785, 0),
              vec3(0.7077538097498388, -0.48118108220125, 0),
              vec3(0.7434202077475696, -0.46984672014641854, 0),
              vec3(0.7761053756770824, -0.45204654277953654, 0),
              vec3(0.8048502274239044, -0.4283492117790967, 0),
              vec3(0.8286956768735618, -0.3993233888235917, 0),
              vec3(0.8466826379115826, -0.36553773559151465, 0),
              vec3(0.8557512754292231, -0.33741694258942456, 0),
              vec3(0.996423388384482, 0.25472647096242323, 0),
              vec3(1, 0.29294405649279703, 0),
              vec3(0.9960265197448488, 0.3307089760104401, 0),
              vec3(0.9846915584576656, 0.3670555420215273, 0),
              vec3(0.7570787710671805, -0.3279246723576778, 0),
              vec3(0.7481456607517563, -0.34489128805259295, 0),
              vec3(0.7363053501222789, -0.3595089915158228, 0),
              vec3(0.7220133343540593, -0.37147883456812697, 0),
              vec3(0.705725108622407, -0.38050186903026445, 0),
              vec3(0.6878961681026325, -0.3862791467229943, 0),
              vec3(0.6689820079700466, -0.3885117194670758, 0),
              vec3(0.649438123399958, -0.38690063908326805, 0),
              vec3(0.6304205367457778, -0.3813442914822551, 0),
              vec3(0.6058327710290847, -0.3668259574428918, 0),
              vec3(0.5925016292489125, -0.3537028573471296, 0),
              vec3(0.5819678219692868, -0.33835579970027974, 0),
              vec3(0.5720660623804614, -0.31216168851770726, 0),
              vec3(0.5701398889260951, -0.28352583320630764, 0),
              vec3(0.7155770729851352, 0.3272211318267263, 0),
              vec3(0.7245101833005594, 0.3441877475216413, 0),
              vec3(0.7363504939300369, 0.35880545098487127, 0),
              vec3(0.7506425096982563, 0.3707752940371754, 0),
              vec3(0.7669307354299086, 0.37979832849931283, 0),
              vec3(0.7847596759496831, 0.38557560619204273, 0),
              vec3(0.803673836082269, 0.38780817893612424, 0),
              vec3(0.8330139122006925, 0.38387499214489884, 0),
              vec3(0.8511765563761193, 0.3765710374186516, 0),
              vec3(0.8670548875320945, 0.36613614277101764, 0),
              vec3(0.8803725907900135, 0.35302297421472734, 0),
              vec3(0.8908533512712726, 0.3376841977625116, 0),
              vec3(0.8982208540972669, 0.3205724794271009, 0),
              vec3(0.9021987843893928, 0.3021404852212259, 0),
              vec3(0.49404521200236484, 0.32051485979824523, 0),
              vec3(0.48453276188440575, 0.3577040643779089, 0),
              vec3(0.4771903875232675, 0.3751107130887582, 0),
              vec3(0.4578154766529401, 0.40706596689622404, 0),
              vec3(0.432829864860236, 0.4345676472763559, 0),
              vec3(0.4030049704751837, 0.4568443358991251, 0),
              vec3(0.3691122118278114, 0.47312461443450343, 0),
              vec3(0.3508814516338892, 0.4787750316912871, 0),
              vec3(0.3319230072481476, 0.4826370645524623, 0),
              vec3(0.29392752080375106, 0.4846864612988684, 0),
              vec3(0.2612007307844826, 0.48048830820567073, 0),
              vec3(0.2302822471806536, 0.47103805054705644, 0),
              vec3(0.20168320536497242, 0.4567572335473808, 0),
              vec3(0.17591474071014757, 0.43806740243099923, 0),
              vec3(0.15348798858888713, 0.41539010242226715, 0),
              vec3(0.13491408437389962, 0.3891468787455399, 0),
              vec3(0.12070416343789314, 0.35975927662517293, 0),
              vec3(0.1132180666353646, 0.33590866248028545, 0),
              vec3(-0.025338065404089227, -0.24549634892148534, 0),
              vec3(-0.030138663741522187, -0.27510780484715514, 0),
              vec3(-0.03077473905960626, -0.29440849931717467, 0),
              vec3(-0.026382729388821247, -0.33208369115772834, 0),
              vec3(-0.021501053563444672, -0.35021629512770946, 0),
              vec3(-0.0067323828414551645, -0.3844667856656981, 0),
              vec3(0.01422201609720819, -0.4152246749992118, 0),
              vec3(0.026835852193426657, -0.4289915274635997, 0),
              vec3(0.05597077473090595, -0.45269631448625475, 0),
              vec3(0.07234545200867437, -0.46239235564396874, 0),
              vec3(0.10834321567353711, -0.47686699975078806, 0),
              vec3(0.12770761787487328, -0.4814513703741509, 0),
              vec3(0.16645647691437215, -0.4847091805549651, 0),
              vec3(0.20413166875492594, -0.4803171708841801, 0),
              vec3(0.23976561979432254, -0.46886097801576615, 0),
              vec3(0.27239075643035005, -0.45092623860369346, 0),
              vec3(0.30103950506079724, -0.4270985893019323, 0),
              vec3(0.31357036712323705, -0.41315793489678415, 0),
              vec3(0.3344403332411665, -0.38158898948668446, 0),
              vec3(0.3489149773479858, -0.3455912258218219, 0),
              vec3(0.42894164996584827, -0.011973834455500498, 0),
              vec3(0.42972497189919556, 0.005285807701407088, 0),
              vec3(0.4243979338193449, 0.022557766661810377, 0),
              vec3(0.410058067874302, 0.03884635586226634, 0),
              vec3(0.3951169066462894, 0.04640446369357787, 0),
              vec3(0.381196346171893, 0.04853615910943173, 0),
              vec3(0.29306584924039014, 0.048288253738397215, 0),
              vec3(0.274830481833795, 0.04272753468825939, 0),
              vec3(0.2605446253882744, 0.030976607914030365, 0),
              vec3(0.25170207774583986, 0.014529271257721282, 0),
              vec3(0.24954095118037345, 0.000010362544709030003, 0),
              vec3(0.25170207774583986, -0.014508546168303221, 0),
              vec3(0.2605446253882744, -0.030955882824612358, 0),
              vec3(0.274830481833795, -0.042706809598841385, 0),
              vec3(0.283547839032084, -0.04635430745454747, 0),
              vec3(0.31863004442757803, -0.048515434020013674, 0),
              vec3(0.25202421179653345, -0.32792467235767736, 0),
              vec3(0.2430911014811088, -0.3448912880525925, 0),
              vec3(0.22438277766140802, -0.36584357985454286, 0),
              vec3(0.20067054935175999, -0.380501869030264, 0),
              vec3(0.18284160883198575, -0.38627914672299396, 0),
              vec3(0.15420575352058608, -0.3882053201773602, 0),
              vec3(0.13463670610345768, -0.3845783042799128, 0),
              vec3(0.11660773985991546, -0.37726960481225597, 0),
              vec3(0.09378165831698615, -0.3605708705373532, 0),
              vec3(0.07691326269863996, -0.3383557997002793, 0),
              vec3(0.06947573809880092, -0.3212402796776524, 0),
              vec3(0.0647789303657329, -0.29324752838512047, 0),
              vec3(0.06639001074954098, -0.2737036438150323, 0),
              vec3(0.20590410104503487, 0.3127127704626628, 0),
              vec3(0.21430798481922464, 0.33602689053214585, 0),
              vec3(0.2307763633722193, 0.3590033597484454, 0),
              vec3(0.24497227602406446, 0.37110352110207073, 0),
              vec3(0.2612004704622315, 0.3802611176093988, 0),
              vec3(0.288338328393416, 0.3877891213826372, 0),
              vec3(0.31743891786417455, 0.3869192865064165, 0),
              vec3(0.34946754816430725, 0.37512846657511933, 0),
              vec3(0.3628577585368089, 0.365782121700417, 0),
              vec3(0.37946114119164776, 0.34811165849076364, 0),
              vec3(0.3938616180155652, 0.3191469867055984, 0),
              vec3(0.3981023608114145, 0.2852790298923329, 0),
              vec3(0.40366307986155214, 0.26704366248573774, 0),
              vec3(0.40885812964828094, 0.2593136830277175, 0),
              vec3(0.42314398609380155, 0.24756275625348856, 0),
              vec3(0.4463802520051028, 0.24175413183231625, 0),
              vec3(0.470219090251927, 0.2480492150414748, 0),
              vec3(0.4849219338265358, 0.2599629758397277, 0),
              vec3(0.4902778529410525, 0.2677196751665549, 0),
              vec3(-0.4238290609360067, -0.3879545880996168, 0),
              vec3(-0.42399143141451245, -0.3857869964555996, 0),
              vec3(-0.3435303791941341, -0.04876145696646751, 0),
              vec3(-0.1881453922760149, -0.04826752864897948, 0),
              vec3(-0.17862738206770856, -0.04635430745454769, 0),
              vec3(-0.16218004541139952, -0.037511759812112966, 0),
              vec3(-0.15042911863717046, -0.023225903366592502, 0),
              vec3(-0.14678162078146462, -0.014508546168303545, 0),
              vec3(-0.1448683995870328, 0.005011261049414758, 0),
              vec3(-0.15042911863717046, 0.02324662845600997, 0),
              vec3(-0.16218004541139952, 0.037532484901530376, 0),
              vec3(-0.17862738206770856, 0.04637503254396521, 0),
              vec3(-0.19314629078072088, 0.04853615910943141, 0),
              vec3(-0.3201711275962441, 0.04853615910943141, 0),
              vec3(-0.2395580256922737, 0.3877292902425807, 0),
              vec3(-0.013614947063175453, 0.3879753131890342, 0),
              vec3(0.001553809488533675, 0.3901364397545005, 0),
              vec3(0.010271166686822708, 0.3937839376102065, 0),
              vec3(0.024557023132343314, 0.4055348643844355, 0),
              vec3(0.03339957077477784, 0.42198220104074474, 0),
              vec3(0.035560697340244474, 0.436501109753757, 0),
              vec3(0.03339957077477784, 0.45102001846676926, 0),
              vec3(0.024557023132343314, 0.46746735512307847, 0),
              vec3(0.010271166686822708, 0.4792182818973075, 0),
              vec3(0.001553809488533675, 0.4828657797530134, 0),
              vec3(-0.012965099224478305, 0.4850269063184797, 0),
              vec3(-0.28168609287903024, 0.48485067306612756, 0),
              vec3(-0.3043423804615203, 0.47698065006041873, 0),
              vec3(-0.32039964632717177, 0.4593075787165124, 0),
              vec3(-0.5323543124694348, -0.42474224806260563, 0),
              vec3(-0.5337338883980041, -0.43998512769155146, 0),
              vec3(-0.5289140955923151, -0.45796681352017904, 0),
              vec3(-0.5177230286574426, -0.4726176319384635, 0),
              vec3(-0.5012621748910309, -0.48217218549675583, 0),
              vec3(-0.48805568326353965, -0.4848137981736519, 0),
              vec3(-0.18106368398796246, -0.4846778019002089, 0),
              vec3(-0.16282831658136732, -0.47911708285007126, 0),
              vec3(-0.1485424601358467, -0.46736615607584225, 0),
              vec3(-0.13969991249341207, -0.4509188194195329, 0),
              vec3(-0.13780384309711902, -0.4313999159620832, 0),
              vec3(-0.14369958214938683, -0.41318230940945977, 0),
              vec3(-0.1559911147398615, -0.3989255304682293, 0),
              vec3(-0.17289041011825634, -0.3901082881133118, 0),
              vec3(-0.8900128517657221, -0.3879545880996168, 0),
              vec3(-0.6958469474912434, 0.43024439706208367, 0),
              vec3(-0.6970680454957241, 0.44937401560737006, 0),
              vec3(-0.7052111612782105, 0.46615542921161646, 0),
              vec3(-0.711534261055692, 0.4730785079255896, 0),
              vec3(-0.7279951148221036, 0.4828153067067156, 0),
              vec3(-0.7474699697899465, 0.48548096726553785, 0),
              vec3(-0.756841335071212, 0.48394209929187737, 0),
              vec3(-0.7735159477880796, 0.4757893172039107, 0),
              vec3(-0.7858110168317786, 0.461806574093651, 0),
              vec3(-0.789707952521583, 0.45298025484212406, 0),
              vec3(-0.9986204823983503, -0.42409851472697496, 0),
              vec3(-1, -0.4393413360290013, 0),
              vec3(-0.9951802071943109, -0.4573230218576287, 0),
              vec3(-0.9839891402594383, -0.4719738402759131, 0),
              vec3(-0.9675282864930268, -0.48152839383420565, 0),
              vec3(-0.9543217948655356, -0.48417000651110154, 0),
              vec3(-0.6473297955899583, -0.48403401023765885, 0),
              vec3(-0.629094428183363, -0.4784732911875213, 0),
              vec3(-0.6148085717378426, -0.4667223644132922, 0),
              vec3(-0.6059660240954079, -0.4502750277569829, 0),
              vec3(-0.6040688937631473, -0.43076335438168084, 0),
              vec3(-0.6099273821747845, -0.41268783466083125, 0),
              vec3(-0.6220949031388547, -0.3986636757539205, 0),
              vec3(-0.6387381593037988, -0.3900488756991408, 0),
              vec3(-0.6530549597150334, -0.3879545880996168, 0),
              vec3(-0.1875131137334065, -0.3879545880996168, 0),
              vec3(0.4962741058527447, 0.2909237200595889, 0),
              vec3(0.9001729001451884, 0.2680691160390561, 0),
              vec3(0.9714728267726251, 0.392803750658161, 0));
const ivec3 triangles[219] = ivec3[219](
    ivec3(0, 61, 1), ivec3(0, 62, 61), ivec3(0, 224, 62), ivec3(1, 60, 2),
    ivec3(1, 61, 60), ivec3(2, 60, 3), ivec3(3, 59, 4), ivec3(3, 60, 59),
    ivec3(4, 58, 5), ivec3(4, 59, 58), ivec3(5, 58, 6), ivec3(6, 57, 7),
    ivec3(6, 58, 57), ivec3(7, 56, 8), ivec3(7, 57, 56), ivec3(8, 56, 9),
    ivec3(9, 55, 10), ivec3(9, 56, 55), ivec3(10, 54, 11), ivec3(10, 55, 54),
    ivec3(11, 53, 12), ivec3(11, 54, 53), ivec3(12, 52, 13), ivec3(12, 53, 52),
    ivec3(13, 52, 14), ivec3(14, 52, 15), ivec3(15, 51, 16), ivec3(15, 52, 51),
    ivec3(16, 51, 17), ivec3(17, 50, 18), ivec3(17, 51, 50), ivec3(18, 50, 19),
    ivec3(19, 49, 20), ivec3(19, 50, 49), ivec3(20, 48, 21), ivec3(20, 49, 48),
    ivec3(21, 47, 22), ivec3(21, 48, 47), ivec3(22, 47, 23), ivec3(23, 46, 24),
    ivec3(23, 47, 46), ivec3(24, 46, 25), ivec3(25, 45, 26), ivec3(25, 46, 45),
    ivec3(26, 44, 27), ivec3(26, 45, 44), ivec3(27, 43, 28), ivec3(27, 44, 43),
    ivec3(28, 42, 29), ivec3(28, 43, 42), ivec3(29, 41, 30), ivec3(29, 42, 41),
    ivec3(30, 40, 31), ivec3(30, 41, 40), ivec3(31, 39, 32), ivec3(31, 40, 39),
    ivec3(32, 38, 33), ivec3(32, 39, 38), ivec3(33, 38, 223),
    ivec3(33, 223, 34), ivec3(34, 223, 35), ivec3(35, 65, 36),
    ivec3(35, 223, 65), ivec3(36, 64, 37), ivec3(36, 65, 64),
    ivec3(37, 63, 224), ivec3(37, 64, 63), ivec3(62, 224, 63),
    ivec3(66, 143, 67), ivec3(66, 144, 143), ivec3(66, 222, 144),
    ivec3(67, 143, 68), ivec3(68, 142, 69), ivec3(68, 143, 142),
    ivec3(69, 142, 70), ivec3(70, 141, 71), ivec3(70, 142, 141),
    ivec3(71, 140, 72), ivec3(71, 141, 140), ivec3(72, 139, 73),
    ivec3(72, 140, 139), ivec3(73, 139, 74), ivec3(74, 139, 75),
    ivec3(75, 138, 76), ivec3(75, 139, 138), ivec3(76, 137, 77),
    ivec3(76, 138, 137), ivec3(77, 137, 78), ivec3(78, 136, 79),
    ivec3(78, 137, 136), ivec3(79, 135, 80), ivec3(79, 136, 135),
    ivec3(80, 134, 81), ivec3(80, 135, 134), ivec3(81, 134, 82),
    ivec3(82, 133, 83), ivec3(82, 134, 133), ivec3(83, 133, 84),
    ivec3(84, 132, 85), ivec3(84, 133, 132), ivec3(85, 131, 86),
    ivec3(85, 132, 131), ivec3(86, 131, 87), ivec3(87, 130, 88),
    ivec3(87, 131, 130), ivec3(88, 130, 89), ivec3(89, 129, 90),
    ivec3(89, 130, 129), ivec3(90, 128, 91), ivec3(90, 129, 128),
    ivec3(91, 128, 92), ivec3(92, 127, 93), ivec3(92, 128, 127),
    ivec3(93, 126, 94), ivec3(93, 127, 126), ivec3(94, 126, 95),
    ivec3(95, 125, 96), ivec3(95, 126, 125), ivec3(96, 124, 97),
    ivec3(96, 125, 124), ivec3(97, 123, 98), ivec3(97, 124, 123),
    ivec3(98, 123, 99), ivec3(99, 122, 100), ivec3(99, 123, 122),
    ivec3(100, 121, 101), ivec3(100, 122, 121), ivec3(101, 121, 102),
    ivec3(102, 120, 103), ivec3(102, 121, 120), ivec3(103, 119, 104),
    ivec3(103, 120, 119), ivec3(104, 109, 105), ivec3(104, 119, 109),
    ivec3(105, 107, 106), ivec3(105, 109, 107), ivec3(107, 109, 108),
    ivec3(109, 119, 110), ivec3(110, 113, 111), ivec3(110, 114, 113),
    ivec3(110, 118, 114), ivec3(110, 119, 118), ivec3(111, 113, 112),
    ivec3(114, 118, 115), ivec3(115, 117, 116), ivec3(115, 118, 117),
    ivec3(144, 147, 145), ivec3(144, 148, 147), ivec3(144, 149, 148),
    ivec3(144, 222, 149), ivec3(145, 147, 146), ivec3(149, 222, 150),
    ivec3(150, 222, 151), ivec3(152, 181, 153), ivec3(152, 186, 181),
    ivec3(152, 187, 186), ivec3(152, 221, 187), ivec3(153, 181, 154),
    ivec3(154, 165, 155), ivec3(154, 180, 165), ivec3(154, 181, 180),
    ivec3(155, 159, 156), ivec3(155, 160, 159), ivec3(155, 164, 160),
    ivec3(155, 165, 164), ivec3(156, 158, 157), ivec3(156, 159, 158),
    ivec3(160, 163, 161), ivec3(160, 164, 163), ivec3(161, 163, 162),
    ivec3(165, 180, 166), ivec3(166, 177, 167), ivec3(166, 178, 177),
    ivec3(166, 180, 178), ivec3(167, 172, 168), ivec3(167, 177, 172),
    ivec3(168, 171, 169), ivec3(168, 172, 171), ivec3(169, 171, 170),
    ivec3(172, 177, 173), ivec3(173, 175, 174), ivec3(173, 176, 175),
    ivec3(173, 177, 176), ivec3(178, 180, 179), ivec3(181, 185, 182),
    ivec3(181, 186, 185), ivec3(182, 184, 183), ivec3(182, 185, 184),
    ivec3(187, 193, 188), ivec3(187, 194, 193), ivec3(187, 221, 194),
    ivec3(188, 190, 189), ivec3(188, 193, 190), ivec3(190, 192, 191),
    ivec3(190, 193, 192), ivec3(195, 205, 196), ivec3(195, 206, 205),
    ivec3(195, 211, 206), ivec3(195, 212, 211), ivec3(195, 220, 212),
    ivec3(196, 205, 197), ivec3(197, 205, 198), ivec3(198, 200, 199),
    ivec3(198, 205, 200), ivec3(200, 205, 201), ivec3(201, 205, 202),
    ivec3(202, 205, 203), ivec3(203, 205, 204), ivec3(206, 210, 207),
    ivec3(206, 211, 210), ivec3(207, 209, 208), ivec3(207, 210, 209),
    ivec3(212, 218, 213), ivec3(212, 219, 218), ivec3(212, 220, 219),
    ivec3(213, 218, 214), ivec3(214, 218, 215), ivec3(215, 218, 216),
    ivec3(216, 218, 217));
const int len = 219;

bool sameSide(vec3 p1, vec3 p2, vec3 a, vec3 b) {
  vec3 cp1 = cross(b - a, p1 - a);
  vec3 cp2 = cross(b - a, p2 - a);

  return dot(cp1, cp2) >= 0.0;
}

bool pointInTriangle(vec3 p, vec3 a, vec3 b, vec3 c) {
  return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}

bool inPath(vec2 p) {
  for (int i = 0; i < len; i++) {
    ivec3 triangle = triangles[i];
    vec3 a = positions[triangle[0]];
    vec3 b = positions[triangle[1]];
    vec3 c = positions[triangle[2]];

    if (pointInTriangle(vec3(p, 0.0), a, b, c)) {
      return true;
    }
  }

  return false;
}

bool resolutionChanged() {
  vec4 resolutionData = texelFetch(STORAGE, resolutionDataLocation, 0);
  return resolutionData.xy != resolutionData.zw;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  if (iFrame > 0 && !resolutionChanged()) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = texture(iChannel1, uv);
    return;
  }  
    
  fragColor = vec4(vec3(0.0), 1.0);
  float normalizer = float(samples * samples);  
  float step = 1.0 / float(samples);
      
  for (int sx = 0; sx < samples; sx++) {
    for (int sy = 0; sy < samples; sy++) {  
      vec2 uv = (fragCoord + vec2(float(sx), float(sy)) * step) / iResolution.xy;
      uv *= 2.0;
      uv -= vec2(1.0);
      uv *= 2.24;
      if (inPath(uv)) {
        fragColor += vec4(1.0);
      }
    }
  }
  fragColor /= normalizer;
  fragColor.a = 1.0;
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
const int samples = 30;
const float sigma = float(samples) * 0.25;

const float pi = 3.1416;

#define pow2(x) (x * x)

float gaussian(vec2 i) {
  return 1.0 / (2.0 * pi * pow2(sigma)) *
         exp(-((pow2(i.x) + pow2(i.y)) / (2.0 * pow2(sigma))));
}

vec3 blur(sampler2D sp, vec2 uv, vec2 scale) {
  vec3 col = vec3(0.0);
  float accum = 0.0;
  float weight;
  vec2 offset;

  for (int x = -samples / 2; x < samples / 2; ++x) {
    for (int y = -samples / 2; y < samples / 2; ++y) {
      offset = vec2(x, y);
      weight = gaussian(offset);
      col += texture(sp, uv + scale * offset).rgb * weight;
      accum += weight;
    }
  }

  return col / accum;
}

bool resolutionChanged() {
  vec4 resolutionData = texelFetch(STORAGE, resolutionDataLocation, 0);
  return resolutionData.xy != resolutionData.zw;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  if (iFrame > 0 && !resolutionChanged()) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = texture(iChannel2, uv);
    return;
  }
  vec2 uv = fragCoord / iResolution.xy;
  fragColor = vec4(blur(iChannel1, uv, vec2(0.00045)), 1.0);
}
