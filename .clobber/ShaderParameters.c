// vec3 iResolution;
// float iTimeDelta;
// float iFrame;
// float iChannelTime[4];
// vec4 iDate;
// float iSampleRate;
// vec3 iChannelResolution[4];
// samplerXX iChanneli;

float frequency; // animation spped - set to 1.0f if you don't want to attach it to a crtl

float iTime; // float iTime - but it's multiplied with frequency
float mouse_x,mouse_y,mouse_z,mouse_w; // vec4 iMouse

// Own
float r0,g0,b0,a0; // iColor0
float r1,g1,b1,a1; // iColor1

int   width, height; // vec3 iResolution; ... third element is the pixel aspect ratio
int   compOrder;

int iChannel0_width;
int iChannel0_height;
