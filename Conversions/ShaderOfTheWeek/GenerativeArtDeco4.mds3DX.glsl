

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "generative art deco 3" by morisil. https://shadertoy.com/view/mdl3WX
// 2022-10-28 00:47:55

// Fork of "generative art deco 2" by morisil. https://shadertoy.com/view/ftVBDz
// 2022-10-27 22:34:54

// Fork of "generative art deco" by morisil. https://shadertoy.com/view/7sKfDd
// 2022-09-28 11:25:15

// Copyright Kazimierz Pogoda, 2022 - https://xemantic.com/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// copyright statement borrowed from Inigo Quilez

// Music by Giovanni Sollima, L'invenzione del nero:
// https://soundcloud.com/giovanni-sollima/linvenzione-del-nero

// See also The Mathematics of Perception to check the ideas behind:
// https://www.shadertoy.com/view/7sVBzK

const float SHAPE_SIZE = .618;
const float CHROMATIC_ABBERATION = .01;
const float ITERATIONS = 10.;
const float INITIAL_LUMA = .5;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

mat2 rotate2d(float _angle){
    return mat2(cos(_angle),-sin(_angle),
                sin(_angle),cos(_angle));
}

float sdPolygon(in float angle, in float distance) {
  float segment = TWO_PI / 4.0;
  return cos(floor(.5 + angle / segment) * segment - angle) * distance;
}

float getColorComponent(in vec2 st, in float modScale, in float blur) {
    vec2 modSt = mod(st, 1. / modScale) * modScale * 2. - 1.;
    float dist = length(modSt);
    float angle = atan(modSt.x, modSt.y) + sin(iTime * .08) * 9.0;
    //dist = sdPolygon(angle, dist);
    //dist += sin(angle * 3. + iTime * .21) * .2 + cos(angle * 4. - iTime * .3) * .1;
    float shapeMap = smoothstep(SHAPE_SIZE + blur, SHAPE_SIZE - blur, sin(dist * 3.0) * .5 + .5);
    return shapeMap;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    float blur = .4 + sin(iTime * .52) * .2;

    vec2 st =
        (2.* fragCoord - iResolution.xy)
        / min(iResolution.x, iResolution.y);
    vec2 origSt = st;
    st *= rotate2d(sin(iTime * .14) * .3);
    st *= (sin(iTime * .15) + 2.) * .3;
    st *= log(length(st * .428)) * 1.1;

    float modScale = 1.;

    vec3 color = vec3(0);
    float luma = INITIAL_LUMA;
    for (float i = 0.; i < ITERATIONS; i++) {
        vec2 center = st + vec2(sin(iTime * .12), cos(iTime * .13));
        //center += pow(length(center), 1.);
        vec3 shapeColor = vec3(
            getColorComponent(center - st * CHROMATIC_ABBERATION, modScale, blur),
            getColorComponent(center, modScale, blur),
            getColorComponent(center + st * CHROMATIC_ABBERATION, modScale, blur)        
        ) * luma;
        st *= 1.1 + getColorComponent(center, modScale, .04) * 1.2;
        st *= rotate2d(sin(iTime  * .05) * 1.33);
        color += shapeColor;
        color = clamp(color, 0., 1.);
//        if (color == vec3(1)) break;
        luma *= .6;
        blur *= .63;
    }
    const float GRADING_INTENSITY = .4;
    vec3 topGrading = vec3(
        1. + sin(iTime * 1.13 * .3) * GRADING_INTENSITY,
        1. + sin(iTime * 1.23 * .3) * GRADING_INTENSITY,
        1. - sin(iTime * 1.33 * .3) * GRADING_INTENSITY
    );
    vec3 bottomGrading = vec3(
        1. - sin(iTime * 1.43 * .3) * GRADING_INTENSITY,
        1. - sin(iTime * 1.53 * .3) * GRADING_INTENSITY,
        1. + sin(iTime * 1.63 * .3) * GRADING_INTENSITY
    );
    float origDist = length(origSt);
    vec3 colorGrading = mix(topGrading, bottomGrading, origDist - .5);
    fragColor = vec4(pow(color.rgb, colorGrading), 1.);
    fragColor *= smoothstep(2.1, .7, origDist);
}