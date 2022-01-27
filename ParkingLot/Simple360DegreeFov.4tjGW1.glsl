

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Licence: Public domain. Attribution/credit is a nice gesture,
//but not required
//define if you want to visualize the cubemap faces
//#define SHOW_FACE_GROUPS
//define if you want to see the normal as colors
//#define SHOW_NORMALS

//Try changing these for a different FOV.
//For example 180 deg horisontal and 90 degree vertical
float FOVX = 360.0; //Max 360 deg
float FOVY = 180.0; //Max 180 deg

const float PI = 3.1415926;

mat3 rotX(float theta){
    float s = sin(theta);
    float c = cos(theta);
    
	mat3 m =
        mat3( 1, 0,  0,
              0, c,  -s,
              0, s,  c);
    return m;
}

mat3 rotY(float theta){
    float s = sin(theta);
    float c = cos(theta);
    
	mat3 m =
        mat3( c, 0, -s,
              0, 1,  0,
              s, 0,  c);
    return m;
}

mat3 rotZ(float theta){
    float s = sin(theta);
    float c = cos(theta);
    
	mat3 m =
        mat3( c, -s, 0,
              s, c,  0,
              0, 0,  1);
    return m;
}

float deg2rad(float deg){
    return deg*PI / 180.0;
}

vec4 faceColors[6];

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    faceColors[0] = vec4(1.0, 0.0, 0.0, 1.0); //left
	faceColors[1] = vec4(0.0, 1.0, 0.0, 1.0); //right
	faceColors[2] = vec4(0.0, 0.0, 1.0, 1.0); //bottom
    faceColors[3] = vec4(1.0, 1.0, 0.0, 1.0); //top
    faceColors[4] = vec4(1.0, 0.0, 1.0, 1.0); //front
    faceColors[5] = vec4(0.0, 1.0, 1.0, 1.0); //back

	vec2 interp = fragCoord.xy / iResolution.xy;
    //Mouse coordinates in [-1, 1] range
    vec2 mp = iMouse.xy / iResolution.xy * vec2(2.0) + vec2(1.0);    
    
    interp.y = 1.0 - interp.y;
    mp.y = 1.0 - mp.y;
    
    //360 degrees around the x-axis, 180 degrees on the y-axis
    //The frustum can be split into several parts:
    //The very top is +y, the north pole, and the very bottom
    //is -y, i.e the south pole.
    //The middle consists of +z, -z, +x and -x, where -z
    //is the center of the frustum, and the left and right
    //frustum edges show +z, i.e what is behind you
    
    float fovX = deg2rad(FOVX);
    float fovY = deg2rad(FOVY);
    float hOffset = (2.0*PI - fovX)*0.5;
    float vOffset = (PI - fovY)*0.5;
    float hAngle = hOffset + interp.x * fovX;
    float vAngle = vOffset + interp.y * fovY;
    vec3 n;    
    n.x = sin(vAngle) * sin(hAngle);
    n.y = cos(vAngle);
    n.z = sin(vAngle) * cos(hAngle);
    
    //Normal pitch-yaw camera controlled with the mouse
    n = rotY(mp.x * 2.0 * PI) * rotX(mp.y * 2.0 * PI) * n;
    
    vec4 color;
    
    #ifdef SHOW_NORMALS
    color.a = 1.0;
    n = normalize(n);
    color.rgb = ((n + vec3(1.0)) * vec3(0.5));
    #else
    color = texture(iChannel0, n);    
    #ifdef SHOW_FACE_GROUPS
    	float ax = abs(n.x);
    	float ay = abs(n.y);
    	float az = abs(n.z);
    	if(ax > ay && ax > az){
    		//x-major
            color *= (n.x < 0.0 ? faceColors[0] : faceColors[1]);
        } else if(ay > ax && ay > az){
            //y-major
            color *= (n.y < 0.0 ? faceColors[2] : faceColors[3]);
        } else {
            //z-major
            color *= (n.z < 0.0 ? faceColors[4] : faceColors[5]);
        }
    #endif    
    #endif
	fragColor = color;
}
