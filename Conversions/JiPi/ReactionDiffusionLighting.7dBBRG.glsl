

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 lightColor = vec3(1.);


vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos(TAU*(c*t+d) );
}

vec3 palette1(float t){
    return palette(t, vec3(0.8, 0.5, 0.4), vec3(0.2, 0.4, 0.2), vec3(2.0, 1.0, 1.0), vec3(0.00, 0.25, 0.25));
}

vec3 palette2(float t){
    return palette(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 1.0), vec3(0.00, 0.33, 0.67));
}

vec3 palette3(float t){
    return palette(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(2.0, 1.0, 0.0), vec3(0.50, 0.20, 0.25));
}

vec3 palette4(float t){
    return palette(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(2.0, 1.0, 0.0), vec3(0.6627, 0.9411, 0.8196));
}

vec3 palette5(float t){
     return vec3(1., pow(cos(t*TAU*1.), 2.)*0.8+0.2, pow(cos(t*TAU*0.5), 2.));
}

vec3 palette6(float t){
    return vec3(smoothstep(0.55, 0.45, t)*0.8 +0.06);
}

vec3 getColour(float depth, vec2 uv){
    
    return palette6(depth);
}

float getDepth(vec2 uv){
    vec4 values = texture(iChannel0, uv);
    float a = values.x;
    float b = values.y;
    float depth = a-b;
    return depth;
}

//https://www.shadertoy.com/view/XlGBW3
vec3 getNormal(vec2 uv) {
    vec3 p = vec3(uv, 0.);
    vec2 e = vec2(1./iResolution.y, 0);//vec2(.0001, 0);
    float d = getDepth(uv+e.xy);

    vec3 normal = d - vec3(
        getDepth(uv-e.xy),
        getDepth(uv-e.yx),
        -1.
    );
    
    return normalize(normal);
}

vec3 getLightDir(vec2 uv){
    vec3 lightPos = vec3(iMouse.xy/iResolution.xy*1., 0.2);

    vec3 fragPos = vec3(uv, 0.);
    vec3 lightDir = vec3(lightPos.xy-vec2(0.5, 0.5), 0.5); 
    //vec3 lightDir = lightPos - fragPos;
    lightDir = normalize(lightDir);
    return lightDir;
}

//https://learnopengl.com/Lighting/Basic-Lighting
vec3 getAmbient(vec2 uv){
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    return ambient;
}

vec3 getDiffuse(vec2 uv){
    
    float diff = dot(getNormal(uv), getLightDir(uv)); 
    //diff = max(diff, 0.);
    vec3 diffuse = diff * lightColor;
    return diffuse;
}

vec3 getSpecular(vec2 uv){

    float specularStrength = 10.;

    //vec3 viewPos = vec3(uv, 1.);
    //vec3 fragPos = vec3(uv, 0.);
    //vec3 viewDir = normalize(viewPos - FragPos);
    vec3 viewDir = vec3(0., 0., 1.);
    
    vec3 reflectDir = reflect(-getLightDir(uv), getNormal(uv));  
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256.);
    
    vec3 specular = specularStrength * spec * lightColor; 
    
    return specular;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy*0.4;
    //uv.x *= iResolution.x/iResolution.y;
   

    float depth = getDepth(uv);
    
    vec3 normal = getNormal(uv);
    
    vec3 cool = normal*depth*0.8+depth*0.7;
    vec3 c = vec3(length(cool));
    
 
    vec3 lightness = getDiffuse(uv) + getAmbient(uv)+ getSpecular(uv);
    vec3 colour = getColour(depth,uv);
    // Output to screen
    fragColor = vec4(vec3(colour*lightness), 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define D_A 1.
#define D_B 0.5
#define F 0.055
#define K 0.062
#define T_STEP 0.5




const float weights[9] = float[9](
    .05,0.2,.05,
    0.2, -1., 0.2,
    .05,0.2,.05
);

vec4 laplacian(vec2 fragCoord){

    vec4 weightedLevels = vec4(0.);
    for(int i = 0; i < 9; i++){
        int dx = i%3-1;
        int dy = i/3-1;
        vec2 uv = (fragCoord + vec2(dx, dy))/iResolution.xy;

        weightedLevels += weights[i] * texture(iChannel0, uv);


    }
    
    return weightedLevels;
    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    //uv.x *= iResolution.x/iResolution.y;

    vec4 levels = texture(iChannel0, uv);
    
    float a = levels.x;
    float b = levels.y;
   
    vec4 weightedLevels = laplacian(fragCoord);
    
    //https://www.karlsims.com/rd.html
    
    float aChange = (D_A * weightedLevels.x - a * b * b + F * (1.-a))*T_STEP; 
    
    float bChange = (D_B * weightedLevels.y + a * b * b - (F+K) *b)*T_STEP; 
    
    float newA = a + aChange;
    float newB = b + bChange;
    
    if(iFrame < 10){
        newA = 0.5+hash12(uv*100.)*0.8;
        newB = step(0.5, hash12(floor(uv*50.) + vec2(10., 16.23)));
    };
    
    fragColor = vec4(newA, newB,1.0,1.0);

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define TAU 6.28318530718

//  1 out, 2 in...
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
