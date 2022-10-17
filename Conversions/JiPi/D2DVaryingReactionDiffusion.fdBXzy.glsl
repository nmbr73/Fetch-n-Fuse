

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAXZ (100.0/iResolution.x)

float heightAtLod(vec2 uv, float lod){
    return MAXZ*textureLod(iChannel0, uv, lod).g;
}

#define NSTEPS 32
float visibleAmt(vec3 o, vec3 d){
  // Determine the t at which o + d t reaches the z=MAXZ plane
  float tMax = (MAXZ-o.z)/d.z;
  // Difference in t between sample points - note that we sample at halves:
  float dt = tMax/float(NSTEPS);
  
  float amt = 1.0;
  for(int i = 0; i < NSTEPS; i++){
    float t = (float(i) + 0.5)*dt;
    vec3 p = o + d*t;
    float height = heightAtLod(p.xy, 4.0+log2(t));
    
    float insideness = height - p.z; // >spreadZ -> 0, -spreadZ -> 1
    float spreadZ = 0.2 * t;
    amt = min(amt, 0.5 - 0.5*insideness/spreadZ);
  }
  
  return max(0.0, amt);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    // Get the height at the pixel
    float z = heightAtLod(uv, 0.0);
    vec3 origin = vec3(uv, z);
    
    vec3 visibility = vec3(0.0);
    float aspect = iResolution.x/iResolution.y;
    vec3 L0 = normalize(vec3(normalize(vec2(0.866, 0.5*aspect)), 1.0));
    vec3 L1 = normalize(vec3(normalize(vec2(-0.866, 0.5*aspect)), 1.0));
    vec3 L2 = normalize(vec3(0.0, -1.0, 1.0));
    visibility.r = visibleAmt(origin, L0);
    visibility.g = visibleAmt(origin, L1);
    visibility.b = visibleAmt(origin, L2);
    
    // Estimate local curvature for an AO look
    float gaussianZEst = 0.25*z + 0.25*heightAtLod(uv, 2.0) + 0.2*heightAtLod(uv, 4.0) + 0.15*heightAtLod(uv, 6.0) + 0.1*heightAtLod(uv, 8.0);
    float aoEst = clamp(1.0 + 3.0*(z - gaussianZEst)/MAXZ, 0.0, 1.0);
    
    // Estimate normal for some specularity
    vec3 drez = vec3(1.0/iResolution.xy, 0.0);
    vec3 n = vec3(-(heightAtLod(uv+drez.xz, 0.0)-z)*iResolution.x, -(heightAtLod(uv+drez.zy, 0.0)-z)*iResolution.y, 1.0);
    n = normalize(n);
    
    vec3 diffuse = max(vec3(dot(n, L0), dot(n, L1), dot(n, L2)), vec3(0.0)) * aoEst * visibility;
    
    vec3 halfVec = normalize(n+vec3(0.,0.,1.));
    vec3 spec = max(vec3(dot(halfVec, L0), dot(halfVec, L1), dot(halfVec, L2)), vec3(0.0));
    spec = 17.0 * pow(spec, vec3(128.0)) * visibility;
    
    vec3 col = mix(diffuse, spec, 0.05);
    
    // Output to screen
    fragColor = vec4(pow(col * aoEst, vec3(1.0/2.2)), 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float stepAndOutputRNGFloat(inout uint rngState)
{
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
  rngState  = rngState * 747796405u + 1u;
  uint word = ((rngState >> ((rngState >> 28) + 4u)) ^ rngState) * 277803737u;
  word      = (word >> 22) ^ word;
  return float(word) / 4294967295.0f;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    if(iFrame == 0){
        // Reinitialize
        uint rngState = (uint(iFrame) << 20) + (uint(fragCoord.x/16.) << 10) + uint(fragCoord.y/16.);
        fragColor = vec4(stepAndOutputRNGFloat(rngState), uv.x*stepAndOutputRNGFloat(rngState), 0.0f, 0.0f);
        return;
    }
    
    ivec2 iFC = ivec2(fragCoord);
    vec2 ab = texelFetch(iChannel0, iFC, 0).rg;
    float weights[] = float[9](0.05, 0.2, 0.05,
                        0.2, -1.0, 0.2,
                        0.05, 0.2, 0.05);
    vec2 laplacian = ab * weights[4];
    for(int y = -1; y <= 1; y++){
        for(int x = -1; x <= 1; x++){
            if(x == 0 && y == 0) continue;
            laplacian += weights[3*y+x+4] * texelFetch(iChannel0, iFC + ivec2(x, y), 0).rg;
        }
    }
    
    if(iMouse.z > 0.0f){
        uv = iMouse.xy / iResolution.xy;
    }
    
    float high = mix(0.4+0.3*(1.0-pow(1.0-uv.x,3.0)), 0.9-(0.9-0.57)*uv.x, 1.0-pow(1.0-uv.x, 2.0));
    float low = mix(0.2*(1.0-pow(1.0-uv.x,3.0)), 0.74-(0.74-0.54)*uv.x, 1.0-pow(1.0-uv.x, 3.8));
    float f = 0.1 * uv.x;
    float k = 0.1 * mix(low, high, uv.y);
    
    float dt = 0.9;
    
    // Update A and B
    ab = ab + dt*(
        vec2(1.0, 0.5)*laplacian
        + vec2(-1.0, 1.0) * ab.x * ab.y * ab.y
        + vec2(f*(1.0-ab.x), -(k+f)*ab.y));
        
    ab = clamp(ab, vec2(0.0), vec2(1.0));
    
    fragColor = vec4(ab, 0.0, 1.0);
}