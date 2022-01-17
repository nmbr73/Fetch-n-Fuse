
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect '/presets/tex00.jpg' to iChannel1
// Connect 'Texture: Font 1' to iChannel0


// just debugging keyboard texture and scan codes!



__DEVICE__ float2 font_from_screen(float2 tpos, float font_size, float2 char_pos) {
    const float GLYPHS_PER_UV = 16.0f;
    return (tpos/font_size + char_pos + 0.5f)/GLYPHS_PER_UV;
}

__DEVICE__ float3 sample_grad_dist(float2 uv, float font_size, __TEXTURE2D__ iChannel0) {

    const float FONT_TEX_BIAS = 127.0f/255.0f;

    // float3 grad_dist = (textureLod(iChannel0, uv, 0.0f).yzw - FONT_TEX_BIAS) * font_size;
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/textureLod.xhtml
    float4 tmp4=_tex2DVecN(iChannel0, uv.x, uv.y, 15);
    float3 grad_dist = (swi3(tmp4,y,z,w) - FONT_TEX_BIAS) * font_size;

    grad_dist.y = -grad_dist.y;
    float2 tmp2 = normalize(swi2(grad_dist,x,y) + 1e-5);
    grad_dist.x =tmp2.x;
    grad_dist.y =tmp2.y;

    return grad_dist;

}


__KERNEL__ void KeyboardDebuggingFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    const float2 TABLE_RES = to_float2(16.0f, 16.0f);
    const float2 CELL_DIMS = to_float2(1.5f, 1.0f);
    const float2 TABLE_DIMS = TABLE_RES * CELL_DIMS;

    const float MARGIN = 2.0f;


    float3 color = to_float3_s(1.0f);

    float scl = 1.0f / _floor( iResolution.y / (TABLE_RES.y + MARGIN) );

    float2 p = (fragCoord - 0.5f - 0.5f*iResolution)*scl + 0.5f*TABLE_DIMS;

    float2 b = abs_f2(p - 0.5f*TABLE_DIMS) - 0.5f*TABLE_DIMS;
    float dbox = _fmaxf(b.x, b.y);

    if (dbox < 0.0f) {

        float2 cell = _floor(p/CELL_DIMS);

        int keycode = (int)(cell.x) + (int)((15.0f-cell.y)*16.0f);

        bool hit = false;

        bool ktex[3];
        for (int i=0; i<3; ++i) {
            // ktex[i] = texelFetch(iChannel1, to_int2(keycode, i), 0).x > 0.0f;
            // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/texelFetch.xhtml
            //ktex[i] = _tex2DVecN(iChannel1, (float(keycode)+0.5f)/iResolution.x,(float(i)+0.5f)/iResolution.y, 0).x > 0.0f;
            ktex[i] = _tex2DVecN(iChannel1, ((float)(keycode)+0.5f)/256.0f, ((float)(i)+0.5f)/3.0f, 0).x > 0.0f;
        }

        color = ktex[0] ? to_float3(1.0f, 0.25f, 0.25f) : to_float3_s(0.8f);

        float dtext = 1e5;

        //const int place[3] = int[3]( 100, 10, 1 );
        const int place[3] = { 100, 10, 1 };
        bool nonzero = false;

        float i0 = (keycode >= 100 ? 1.0f : keycode >= 10 ? 1.5f : 2.0f);

        for (int i=0; i<3; ++i) {

            int digit = keycode / place[i];
            keycode -= digit * place[i];

            if (digit > 0 || nonzero || i == 2) {

                float2 p0 = (cell + to_float2(0.5f + ((float)(i)-i0)*0.3f, 0.5f))*CELL_DIMS;
                float2 uv = font_from_screen((p - p0), 1.0f, to_float2(digit, 12));
                float2 dbox = abs_f2(p - p0) - 0.5f;
                dtext = _fminf(dtext, _fmaxf(max(dbox.x, dbox.y), sample_grad_dist(uv, 1.0f,iChannel0).z));
                nonzero = true;

            }

        }

        float3 textcolor = ktex[2] ? to_float3_s(0.0f) : (ktex[1] || ktex[0]) ? to_float3_s(1.0f) : to_float3_s(0.9f);

        if (ktex[1]) {
            float2 q = (p/CELL_DIMS - cell);
            float b = _powf( 24.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.5f );
            color = _mix(color, to_float3(1.0f, 1.0f, 0.25f), 1.0f-b);
        }

        color = _mix(color, textcolor, smoothstep(0.5f*scl, -0.5f*scl, dtext));

        float2 p0 = _floor(p/CELL_DIMS + 0.5f)*CELL_DIMS;
        float2 dp0 = abs_f2(p - p0);
        dbox = _fminf(_fabs(dbox), _fminf(_fabs(dp0.x), _fabs(dp0.y)));

    }

    color *= smoothstep(0.0f, scl, _fabs(dbox));

    fragColor = to_float4_aw(color, 1);




  SetFragmentShaderComputedColor(fragColor);
}