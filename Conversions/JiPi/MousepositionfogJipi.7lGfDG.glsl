

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Written by Denilson SÃ¡ <denilsonsa@gmail.com>
// http://denilson.sa.nom.br/
//
// GLSL Sandbox version at:
// http://glslsandbox.com/e#12315.2
//
// The source is also available at:
// https://bitbucket.org/denilsonsa/atmega8-magnetometer-usb-mouse/src/tip/html_javascript/
// https://github.com/denilsonsa/atmega8-magnetometer-usb-mouse/tree/master/html_javascript
//
// The original code (above) was restructured to work with Shadertoy.

// Code originally from draw_to_main().
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord.xy / iResolution.xy;
	fragColor = texture(iChannel0, uv);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define fb iChannel0

// Code originally from draw_to_fb().
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 pos;
	vec2 posnorm;
	vec2 poscanv;
	vec2 zooming;
	vec4 color;
	float dist;

    // These have the same size in Shadertoy:
    vec2 canvassize = iResolution.xy;
	vec2 fbsize = canvassize;

    bool mouse_hold = !(iMouse.z < 0.0);
    vec2 mouse = iMouse.xy;

	// Position of the current pixel
	pos = fragCoord.xy;
	// Position of the current pixel, normalized between 0.0 and 1.0
	posnorm = fragCoord.xy / iResolution.xy;
	// Position of the current pixel, in canvas coordinates (the same as the
	// mouse)
	poscanv = posnorm*canvassize;


	// Distance between the current pixel and the mouse, already taking into
	// account the difference between the coordinate systems
	dist = distance(mouse, poscanv);
	if ( dist < 16.0 ) {
		// Drawing a circle around the mouse with a solid color
		// Blue if mouse button is not pressed, orange if it is pressed
		color = vec4(0.2, 0.25, 1.0, 0.0) + vec4(1.0, 0.5, -1.0, 0.0) * float(mouse_hold);
	} else {
		// Zooming trick
		zooming = ((mouse - poscanv) * (0.015625)) / canvassize;
		color = texture(fb, posnorm + zooming);

		color -= (color * 0.00390625) + 0.001953125;
	}

	fragColor = color;
}