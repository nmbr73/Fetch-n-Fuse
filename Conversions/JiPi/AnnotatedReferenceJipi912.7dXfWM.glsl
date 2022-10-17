

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define bars 32.0                 // How many buckets to divide spectrum into
#define barSize 1.0 / bars        // Constant to avoid division in main loop
#define barGap 0.1 * barSize      // 0.1 represents gap on both sides, so a bar is
                                  // shown to be 80% as wide as the spectrum it samples
#define sampleSize 0.02           // How accurately to sample spectrum, must be a factor of 1.0
                                  // Setting this too low may crash your browser!

// Helper for intensityToColour
float h2rgb(float h) {
	if(h < 0.0) h += 1.0;
	if(h < 0.166666) return 0.1 + 4.8 * h;
	if(h < 0.5) return 0.9;
	if(h < 0.666666) return 0.1 + 4.8 * (0.666666 - h);
	return 0.1;
}

// Map [0, 1] to rgb using hues from [240, 0] - ie blue to red
vec3 intensityToColour(float i) {
	// Algorithm rearranged from http://www.w3.org/TR/css3-color/#hsl-color
	// with s = 0.8, l = 0.5
	float h = 0.666666 - (i * 0.666666);
	
	return vec3(h2rgb(h + 0.333333), h2rgb(h), h2rgb(h - 0.333333));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	// Map input coordinate to [0, 1)
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	// Get the starting x for this bar by rounding down
	float barStart = floor(uv.x * bars) / bars;
	
	// Discard pixels in the 'gap' between bars
	if(uv.x - barStart < barGap || uv.x > barStart + barSize - barGap) {
		fragColor = vec4(0.0);
	}
    else
    {
	// Sample spectrum in bar area, keep cumulative total
	float intensity = 0.0;
	for(float s = 0.0; s < barSize; s += barSize * sampleSize) {
		// Shader toy shows loudness at a given frequency at (f, 0) with the same value in all channels
		intensity += texture(iChannel0, vec2(barStart + s, 0.0)).r;
	}
	intensity *= sampleSize; // Divide total by number of samples taken (which is 1 / sampleSize)
	intensity = clamp(intensity, 0.005, 1.0); // Show silent spectrum to be just poking out of the bottom
	
	// Only want to keep this pixel if it is lower (screenwise) than this bar is loud
	float i = float(intensity > uv.y); // Casting a comparison to float sets i to 0.0 or 1.0
	
	//fragColor = vec4(intensityToColour(uv.x), 1.0);       // Demo of HSL function
	//fragColor = vec4(i);                                  // Black and white output
	fragColor = vec4(intensityToColour(intensity) * i, i);  // Final output
    }
	// Note that i2c output is multiplied by i even though i is sent in the alpha channel
	// This is because alpha is not 'pre-multiplied' for fragment shaders, you need to do it yourself
}
