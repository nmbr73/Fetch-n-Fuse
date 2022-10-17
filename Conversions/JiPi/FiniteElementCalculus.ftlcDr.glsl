

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const float dx = 1.0;
const float dy = 1.0;
const float dz = 1.0;
const vec2 PosX = vec2( 1.0,  0.0); //  1,  0,  0
const vec2 NegX = vec2(-1.0,  0.0); // -1,  0,  0
const vec2 PosY = vec2( 0.0,  1.0); //  0,  1,  0
const vec2 NegY = vec2( 0.0, -1.0); //  0, -1,  0
const vec2 PosZ = vec2( 0.0,  0.0); //  0,  0,  1
const vec2 NegZ = vec2( 0.0,  0.0); //  0,  0, -1
const vec2 Zero = vec2( 0.0,  0.0); //  0,  0,  0
const int ALL_FIELDS = 0;
const int VECTOR_FIELD = 1;               // Original RGB.
const int SCALAR_FIELD = 2;               // Average of RGB.
const int GRADIENT_OF_SCALAR_FIELD = 3;   // Produces a vector field.
const int DIVERGENCE_OF_VECTOR_FIELD = 4; // Produces a scalar field.
const int LAPLACIAN_OF_SCALAR_FIELD = 5;  // Produces a scalar field.
const int LAPLACIAN_OF_SCALAR_FIELD_SIGN = 6; // Colors the Laplacian.
const int CURL_OF_VECTOR_FIELD = 7;           // Produces a vector field.
const int CURL_OF_VECTOR_FIELD_LENGTH = 8;    // Produces a scalar field.
const int CURL_OF_VECTOR_FIELD_VORTICITY = 9;      // Colors the Curl.
const float NUM_CHOICES = 10.0;

// Pick a combo of the following.
const bool USE_WEBCAM = false;
const bool USE_NYANCAT = true;
const bool USE_VIDEO = true;
const bool USE_BUFFER_A = false;

vec3 v(vec2 ij, vec2 offset) {
    vec2 uv = (ij + offset) / iResolution.xy;
    vec3 col = vec3(0.0);
    float count = 0.0;

    if (USE_VIDEO) {
        col += texture(iChannel0, uv).rgb;
        count++;
    }
        
    if (USE_WEBCAM) {
        col += texture(iChannel1, uv).rgb;
        count++;
    }
        
    if (USE_NYANCAT) {
        vec2 uv2 = vec2(-0.15 + uv.x + 0.4 * sin(iTime), uv.y + 0.1 * cos(iTime));
        // Add the nyan cat sprite: https://www.shadertoy.com/view/lsX3Rr
        vec2 uvNyan = (uv2  - vec2(0.25, 0.15)) / (vec2(0.7,0.5) - vec2(0.5, 0.15));
        uvNyan = clamp(uvNyan, 0.0, 1.0);
        float ofx = floor(mod(iTime*15.0, 6.0));
        float ww = 40.0/256.0;
        uvNyan = vec2(clamp(uvNyan.x*ww + ofx*ww, 0.0, 1.0 ), uvNyan.y);
        vec4 texel = texture(iChannel2, uvNyan);
        if (texel.a > 0.5) {
            col = texel.rgb;
            count = 1.0;
        }
    }
    
    if (USE_BUFFER_A) {
        col += texture(iChannel3, uv).rgb;
        count++;
    }
        
    return col/count;
}

float f(vec2 ij, vec2 offset) {
    vec3 col = v(ij, offset);
    return (col.x + col.y + col.z) / 3.0;
}

vec3 gradient(vec2 ij) {
    float dfdx = 0.5 / dx * (f(ij, PosX) - f(ij, NegX));
    float dfdy = 0.5 / dy * (f(ij, PosY) - f(ij, NegY));
    float dfdz = 0.5 / dz * (f(ij, PosZ) - f(ij, NegZ));
    return vec3(dfdx, dfdy, dfdz);
}

float divergence(vec2 ij) {
    float dfdx = 0.5 / dx * (f(ij, PosX) - f(ij, NegX));
    float dfdy = 0.5 / dy * (f(ij, PosY) - f(ij, NegY));
    float dfdz = 0.5 / dz * (f(ij, PosZ) - f(ij, NegZ));
    return dfdx + dfdy + dfdz;
}

vec3 curl(vec2 ij) {
    float dvzdy = 0.5 / dy * (v(ij, PosY).z - v(ij, NegY).z);
    float dvydz = 0.5 / dz * (v(ij, PosZ).y - v(ij, NegZ).y);
    float dvxdz = 0.5 / dz * (v(ij, PosZ).x - v(ij, NegZ).x);
    float dvzdx = 0.5 / dx * (v(ij, PosX).z - v(ij, NegX).z);
    float dvydx = 0.5 / dy * (v(ij, PosX).y - v(ij, NegX).y);
    float dvxdy = 0.5 / dx * (v(ij, PosY).x - v(ij, NegY).x);
    return vec3(dvzdy - dvydz, dvxdz - dvzdx, dvydx - dvxdy);
}

float laplacian(vec2 ij) {
    return ( f(ij, PosX) + f(ij, NegX)
           + f(ij, PosY) + f(ij, NegY)
           + f(ij, PosZ) + f(ij, NegZ)
           - 6.0 * f(ij, Zero)
           ) / (dx * dy);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    int choice = int(iMouse.x * NUM_CHOICES / iResolution.x);
    if (choice == 0)
        choice = int(fragCoord.x * (NUM_CHOICES-1.0) / iResolution.x) + 1;
    
    float intensity = 1.0;
    vec3 col;
    if (choice == VECTOR_FIELD) {
        col = v(fragCoord, vec2(0, 0));
    } else if (choice == SCALAR_FIELD) {
        col = vec3(f(fragCoord, vec2(0, 0)));
    } else if (choice == GRADIENT_OF_SCALAR_FIELD) {
        vec3 G = gradient(fragCoord);        
        col = G;
        // Boost the intensity.
        intensity = 6.0;
    } else if (choice == DIVERGENCE_OF_VECTOR_FIELD) {
        col = vec3(divergence(fragCoord));
        // Boost the intensity
        intensity = 6.0;
    } else if (choice == LAPLACIAN_OF_SCALAR_FIELD) {
        col = vec3(laplacian(fragCoord));
        
        // Boost the intensity
        intensity = 12.0;
    } else if (choice == LAPLACIAN_OF_SCALAR_FIELD_SIGN) {
        float L = laplacian(fragCoord);
        
        // Differentiate between negative, positive, and zero
        // Red is less than zero, Green is greater than zero, Blue is none.
        if (L < 0.0)
            col = vec3(-L, 0.0, 0.0);
        else if (L > 0.0)
            col = vec3(0.0, L, 0.0);
        else
            col = vec3(0.0, 0.0, 0.0);
        
        // Boost the intensity
        intensity = 12.0;
    } else if (choice == CURL_OF_VECTOR_FIELD) {
        col = curl(fragCoord);

        // Boost the intensity
        intensity = 12.0;
    } else if (choice == CURL_OF_VECTOR_FIELD_LENGTH) {
        float C = length(curl(fragCoord));
        col = vec3(C);
        
        // Don't boost the intensity.
        intensity = 1.0;
    } else if (choice == CURL_OF_VECTOR_FIELD_VORTICITY) {
        float C = dot(curl(fragCoord), vec3(0.0, 0.0, 1.0));
        
        // This is vorticity because we are using a 2D field.
        
        // Differentiate between negative, positive, and zero
        // Red is less than zero, Green is greater than zero, Blue is none.
        if (C < 0.0)
            col = vec3(-C, 0.0, 0.0);
        else if (C > 0.0)
            col = vec3(0.0, C, 0.0);
        else
            col = vec3(0.0, 0.0, 0.0);
        
        // Boost the intensity
        intensity = 6.0;
    }
    
    if (intensity != 1.0)
        col = intensity * col / (1.0 + intensity * col);

    // Output to screen
    fragColor = vec4(col,1.0);
}