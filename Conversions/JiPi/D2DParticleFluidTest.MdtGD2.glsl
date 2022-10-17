

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//COMPOSITE

const vec3 lightPt = vec3(0.5,0.75,0.0);
const float diffuseCheat = 0.85;
const vec3 baseColor = vec3(0.0,1.0,0.0);
const float specP = 8.0;
const float specA = 0.75;

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    
    vec4 normalData = texture(iChannel0,fragCoord/iResolution.xy);
    
    vec3 color = texture(iChannel2,fragCoord/iResolution.x).xyz;
    
    if (normalData.a > 0.0) {
        
        vec3 normal = -normalData.xyz;
        vec3 intersectPt = vec3(fragCoord/iResolution.x,1.0-normal.z*0.1);
        vec3 curCameraRayUnit = normalize(intersectPt);//not quite correct but whatever
        
        vec3 lightGap = lightPt-intersectPt;
        vec3 lightGapNorm = normalize(lightGap);
        float litAmt = dot(normal,lightGapNorm);
        litAmt = litAmt*(1.0-diffuseCheat)+diffuseCheat;

        float lightDist = length(lightGap);
        lightDist /= 16.0;
        lightDist = max(lightDist,0.0);
        lightDist = min(lightDist,1.0);
        lightDist = pow(1.0-lightDist,2.0);

        float specular = max(0.0,dot(normalize(lightGapNorm-curCameraRayUnit),normal));

        color *= (-normal.z)*0.75;
        color += baseColor*litAmt*lightDist + pow(specular,specP)*specA;
        
    } else {
    	color.g += (texture(iChannel1,fragCoord/iResolution.xy).r > 0.1) ? 0.5 : 0.0;
    }
    
    fragColor = vec4(min(vec3(1.0),color),1.0);
    
}
// >>> ___ GLSL:[Buf A] ____________________________________________________________________ <<<
//PHYSICS
const float ballRad = 0.01;
const float ballCount = 200.0;

vec2 coordToWorld(vec2 fragCoord){
    vec2 uv = fragCoord/iResolution.xy;
    uv.y /= (iResolution.x/iResolution.y);
    uv.y = 1.0-uv.y;
    return uv;
}

//



const float gravity = 0.001;

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    
    if (fragCoord.y != 0.5 || fragCoord.x >= ballCount+0.5) return;
    
    
    if (iFrame == 0) {
    	
        fragColor = vec4(0.25+fragCoord.x*0.003,0.5+fragCoord.x*0.00005,0.0,0.0);
        
    } else {
        
        
        
        vec4 myData = texture(iChannel0,fragCoord/iResolution.xy);
        myData.a += gravity;
        
        myData.rg += myData.ba;
        
        
        vec4 myDataOff = vec4(0.0);
        float intersections = 0.0;
        
        for(float i=0.0; i<ballCount; i++){
            
            if (intersections < 4.0) {//arbitrary

                vec2 otherCoord = vec2(i,0) + vec2(0.5);
                if (length(otherCoord-fragCoord) == 0.0) continue;//skip test against self

                vec4 otherData = texture(iChannel0,otherCoord/iResolution.xy);
                vec2 gap = myData.xy-otherData.xy;
                float gapdist = length(gap);
                
                if (gapdist < ballRad*0.5) {//fix interlock
                    // this DOESN'T move the other particle -- it just lies about my sense of where it is!
                    float ejectAng = otherCoord.x+otherCoord.y;
                    otherData.xy += vec2(cos(ejectAng),sin(ejectAng))*ballRad*0.01;

                    gap = myData.xy-otherData.xy;
                    gapdist = length(gap);
                }
                if (gapdist < ballRad*2.0) {
                    vec2 gapnorm = normalize(gap);
                    float myVelAlongCollisionAxis = myData.b*gapnorm.x + myData.a*gapnorm.y;
                    float otherVelAlongCollisionAxis = otherData.b*gapnorm.x + otherData.a*gapnorm.y;

                    myDataOff.ba += (otherVelAlongCollisionAxis-myVelAlongCollisionAxis)*gapnorm*0.5;
                    myDataOff.rg += ((ballRad*2.0-gapdist)*0.5)*gapnorm;
                    intersections++;
                }

            }
            
        }
        
        myData.rg += myDataOff.rg;
        myData.ba += myDataOff.ba;
        
        
        
        
        if (iMouse.z > 0.0) {
        	vec2 worldMouse = coordToWorld(iMouse.xy);
            vec2 mouseGap = worldMouse-myData.rg;
            float mouseGapDist = length(mouseGap);
            vec2 mouseGapNorm = normalize(mouseGap);
            
            float pullStrength = mouseGapDist/0.9;
            pullStrength = min(1.0,max(pullStrength,0.0));
            pullStrength = pow(pullStrength,0.75);
            pullStrength = sin((pullStrength-0.25)*6.28)*0.5+0.5;
            pullStrength *= 0.0013;
            
            myData.ba += mouseGapNorm*pullStrength;
        }
        
        if (myData.g > 1.0-ballRad) {
            myData.a += -abs(myData.a)*1.5;
            myData.g = 1.0-ballRad;
        }
        if (myData.g < ballRad) {
            myData.a += abs(myData.a)*1.5;
            myData.g = ballRad;
        }
        if (myData.r > 1.0-ballRad) {
            myData.b += -abs(myData.b)*1.5;
            myData.r = 1.0-ballRad;
        }
        if (myData.r < ballRad) {
            myData.b += abs(myData.b)*1.5;
            myData.r = ballRad;
        }
        
        myData.ba = myData.ba*0.95;
        
        fragColor = myData;
        
    }
    
}
// >>> ___ GLSL:[Buf B] ____________________________________________________________________ <<<
//CURRENT FRAME'S BLOB
const float ballRad = 0.01;
const float ballCount = 200.0;

vec2 coordToWorld(vec2 fragCoord){
    vec2 uv = fragCoord/iResolution.xy;
    uv.y /= (iResolution.x/iResolution.y);
    uv.y = 1.0-uv.y;
    return uv;
}

//


//#define DEBUGVIEW

float mySmooth( float a, float b, float k ){
    float h = clamp(0.5+0.5*(b-a)/k,0.0,1.0);
    return mix(b,a,h) - k*h*(1.0-h);
}

float blobDist(vec2 uv){
    
    float dist = 9999.9;
    
    for(float i=0.0; i<ballCount; i++){
        vec4 curData = texture(iChannel0,vec2(i+0.5,0.5)/iResolution.xy);
        float curDist = length(curData.rg-uv);
        #ifdef DEBUGVIEW
        dist = min(dist,curDist);
        #else
        dist = mySmooth(dist,curDist,0.07);
        #endif
    }
    
    return dist;
    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
        
    

    vec2 uv = coordToWorld(fragCoord);

    float dist = blobDist(uv);
    
    
    
    
    #ifdef DEBUGVIEW
    fragColor = vec4( (dist < ballRad) ? 1.0 : 0.0 );
    #else
    
    
    
    float height = 1.0-dist/ballRad*0.3;
    if (height >= 0.0) {
        height = min(1.0,max(0.0,height));
        height = height*1.57;
        
        const vec2 e = vec2(0.00001,0.0);
        vec2 normal2d = normalize(vec2(dist-blobDist(uv-e.xy),
                                       dist-blobDist(uv-e.yx)));
        
        vec3 normal3d = vec3(normal2d.x*cos(height),normal2d.y*cos(height),sin(height));

        fragColor = vec4(normal3d,1.0);
    } else {
    	fragColor = vec4(0.0);   
    }
    
    #endif
    
}
// >>> ___ GLSL:[Buf C] ____________________________________________________________________ <<<
//TRAIL
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    
    vec4 curFrame = texture(iChannel0,fragCoord/iResolution.xy);
    vec4 oldFrame = texture(iChannel1,fragCoord/iResolution.xy);
    
    fragColor = vec4(oldFrame.a*0.9+curFrame.a*curFrame.z);
    
}