

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float pseudoRefraction = 0.1; // not real refraction ;)
vec3 lightColor = vec3(0.7, 0.8, 1.0);

float getHeight(vec2 uv) {
  return texture(iChannel0, uv).r;
}

// From dmmn's 'Height map to normal map' - https://www.shadertoy.com/view/MsScRt
vec4 bumpFromDepth(vec2 uv, vec2 resolution, float scale) {
  vec2 step = 1. / resolution;
    
  float height = getHeight(uv);
    
  vec2 dxy = height - vec2(
      getHeight(uv + vec2(step.x, 0.)), 
      getHeight(uv + vec2(0., step.y))
  );
    
  return vec4(normalize(vec3(dxy * scale / step, 1.)), height);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
  vec2 uv = fragCoord.xy / iResolution.xy;
  
  vec3 heightColor = vec3(getHeight(uv));
  
  vec3 normal = bumpFromDepth(uv, iResolution.xy, 1.0).rgb;
  heightColor = vec3(0.5) + 0.5 * pow(heightColor, vec3(2.0));
  
  vec3 camDir = normalize(vec3(uv, 2.0));
  
  vec2 lightPosXY = Rotate2D(iTime) * vec2(-10.0, -10.0);
  
  vec3 lightPos = normalize(vec3(lightPosXY, 2.0)) + vec3(0.5, 0.5, 0.0);
  
  vec3 lightDir = vec3(uv, 0.0) - lightPos;
  lightDir = reflect(lightDir, normal);
  
  float scalar = dot(camDir, lightDir);
  scalar = clamp(scalar, 0.0, 1.0);
  
  vec3 shine = vec3(scalar);
  
  uv.x *= (iResolution.x/iResolution.y);
  uv *= iResolution.x / 1024.0;
  
  vec3 color = texture(iChannel1, uv - normal.xz * pseudoRefraction).rgb;
  color *= heightColor;
  color += shine * lightColor;
  
  fragColor = vec4(color, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float density1 = 3.0;
float density2 = 3.0;
float density3 = 3.0;

// 1.0 is typical Voronoi edge distance, increase to flatten faces
float flattening1 = 15.0;
float flattening2 = 8.0;
float flattening3 = 3.0;

vec2 N22(vec2 p)
{
    vec3 a = fract(p.xyx * vec3(1278.67, 3134.61, 298.647));
    a += dot(a, a + 318.978);
    return fract(vec2(a.x * a.y, a.y * a.z)) * 0.516846;
}

float N21(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void VoronoiUV(vec2 uvIn, float flatteningIn, float timeIn, out vec2 uvOut, out float nuclearDistOut, out float edgeDistOut, out vec2 cellID)
{
    cellID = vec2(0.0);

    vec2 gv = fract(uvIn) - 0.5;
    vec2 id = floor(uvIn);

    nuclearDistOut = 100000.0;
    edgeDistOut = 100000.0;

    for (float y = -1.0; y <= 1.0; y++)
    {
        for(float x = -1.0; x <= 1.0; x++)
        {
            vec2 offset = vec2(x, y);

            vec2 cellindex = id + offset;
            vec2 n = N22(cellindex);
            vec2 p = offset + sin(n * timeIn) * 0.5;

            vec2 diff = gv - p;
            float d = length(diff);

            if (d < nuclearDistOut)
            {
                nuclearDistOut = d;
                cellID = cellindex;
                uvOut = diff;
            }
        }
    }

    for (float y = -1.0; y <= 1.0; y++)
    {
        for (float x = -1.0; x <= 1.0; x++)
        {
            vec2 offset = vec2(x, y);

            vec2 cellindex = id + offset;
            vec2 n = N22(cellindex);
            vec2 p = offset + sin(n * timeIn) * 0.5;

            vec2 diff = gv - p;

            vec2 toCenter = (uvOut + diff) * 0.5;
            vec2 cellDifference = normalize(diff - uvOut);
            float edgeDistance = dot(toCenter, cellDifference);
            edgeDistOut = min(edgeDistOut, edgeDistance);
        }
    }
    
    edgeDistOut = edgeDistOut * flatteningIn;
    edgeDistOut *= 2.0;
    edgeDistOut = min(1.0, edgeDistOut);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    uv.x *= iResolution.x / iResolution.y;

    vec2 uv1;
    float nuclearDist1;
    float edgeDist1;
    vec2 cellID1;
    VoronoiUV(uv * density1, flattening1, iTime + 5.4864, uv1, nuclearDist1,  edgeDist1, cellID1);
    
    float rotSpeed1 = N21(cellID1) * 2.0 - 1.0;
    uv1 = Rotate2D(rotSpeed1 * iTime * 0.5) * uv1;
    
    vec2 uv2;
    float nuclearDist2;
    float edgeDist2;
    vec2 cellID2;
    VoronoiUV(uv1 * density2, flattening2, iTime + 12.4864, uv2, nuclearDist2,  edgeDist2, cellID2);
    
    float rotSpeed2 = N21(cellID2) * 2.0 - 1.0;
    uv2 = Rotate2D(rotSpeed2 * iTime * 0.5) * uv2;
    
    vec2 uv3;
    float nuclearDist3;
    float edgeDist3;
    vec2 cellID3;
    VoronoiUV(uv2 * density3, flattening3, iTime + 37.0846, uv3, nuclearDist3,  edgeDist3, cellID3);
    
    vec3 color = vec3(edgeDist1 * edgeDist2 * edgeDist3);
    
    fragColor = vec4(color, 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
mat2 Rotate2D(float angle)
{
    return mat2(cos(angle), -sin(angle),
                sin(angle), cos(angle));
}