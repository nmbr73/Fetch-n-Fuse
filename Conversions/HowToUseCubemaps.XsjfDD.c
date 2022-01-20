/**
 * Trivial Cubemap shader by Anthony 'Bitzawolf' Pepe
 * @bitzawolf
 * bitzawolf.com
 */

__KERNEL__ void HowToUseCubemapsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    // rotX
    // 1) Get mouse position between 0 and 1
    // 2) Multiply by 2pi
    // rotX varies betwee 0 and 2pi
    // rotY varies between 0 and pi
    // 0 at left side, 2pi at right
    float rotX = (iMouse.x / iResolution.x) * 2.0f * 3.14f;
    float rotY = (iMouse.y / iResolution.y) * 3.14f;

    // pixel position - 1/2 resolution
    //		x: [-400, 400] y: [-300, 300] (assuming 800x600 resolution)
    //
    // Multiply by 2.5
    // This scales the cubemap texture. Think as pixels move along the screen,
    //    pixels in the texture are accessed further apart. A pixel x-position
    //    of 0 will correspond to 0 in the texture, then a pixel x-position of
    //    10 will correspond to 25 in the texture. This scales the texture down.
    // 		x: [-1000, 1000] y: [-750, 750]
    //
    // We divide by resolution.xx because the render screen is not square and will
    //		warp the texture if we divide by resolution.xy
    // 		x: [-1.25, 1.25] y: [-0.9375, 0.9375]
    float2 uv = 2.5 * (fragCoord - 0.5 * iResolution) / to_float2(iResolution.x,iResolution.x);

    // Camera Orientation
    //						As the mouse moves from bottom-left to top-right
    //		x = cos(rotX) = {1, 0, -1,  0,  1}
    //		y = cos(rotY) = {1,     0,     -1}
    //		z = sin(rotX) = {0, 1,  0, -1,  0}
    // example with mouse at camera center:
    //		rotX = cos(3.14) = -1
    //		rotY = cos(1.57) = 0
    //		(-1, 0, 0)
    float3 camO = to_float3(_cosf(rotX), _cosf(rotY), _sinf(rotX));

    // Cam D
    // Negative of Cam O
    // example at center:
    //		(1, 0, 0)
    float3 camD = normalize(to_float3_s(0.0f)-camO);

    // Cam R
    // cross of CamD and (0, 1, 0)
    // example at center
    //		(1, 0, 0) x (0, 1, 0) = (0, 0, 1)
    float3 camR = normalize(cross(camD, to_float3(0.0f, 1.0f, 0.0f)));

    // Cam Up
    // cross of CamR and CamD
    // example:
   	//		(0, 0, 1) x (1, 0, 0) = (0, 1, 0)
    float3 camU = cross(camR,camD);

    // Camera direction
    //		uv.x * camR
    //		uv.y * camU
    //		camD
    // example:
    //		uv.x * (0, 0, 1) = (0, 0, uv.x)
    //		uv.y * (0, 1, 0) = (0, uv.y, 0)
    //		(1, 0, 0)
    //		result: (1, uv.y, uv.x)
    // It's very important to normalize the vector here, otherwise
    // the cubemap will be skewed.
   	float3 dir =  normalize(uv.x * camR + uv.y * camU + camD);
    //fragColor = texture(iChannel0, dir);
    fragColor = decube_f3(iChannel0, dir);

    // proof of example at mouse = camera center
	//fragColor = texture(iChannel0, vec3(1., uv.y, uv.x));

    SetFragmentShaderComputedColor(fragColor);

}