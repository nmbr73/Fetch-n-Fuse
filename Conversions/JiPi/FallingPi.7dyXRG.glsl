

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Main/image buffer:
// - Performs shading of the state from Buffer B.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 pos = ivec2(fragCoord);
    //vec4 pixel = texelFetch(iChannel0, pos, 0);
    vec4 pixel = texture(iChannel0, (vec2(pos)+0.5)/iResolution.xy);
    
    
    if (pixel.r < 0.5) {
        // Empty.
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    if (pixel.a > 0.5) {
        // Lava.
        vec2 noiseCoordA = fragCoord * vec2(0.2, 1.0) / 64.0;
        noiseCoordA.x += float(iFrame) * 0.001;
        vec2 noiseCoordB = fragCoord * vec2(0.2, 1.0) / 64.0;
        noiseCoordB -= vec2(float(iFrame) * 0.001, 0.5);
        float noiseA = texture(iChannel1, noiseCoordA).x;
        float noiseB = texture(iChannel1, noiseCoordB).x;
        float noise = noiseA * noiseB;
        fragColor = vec4(0.8, 0.4, 0.0, 1.0);
        fragColor += vec4(0.1, 0.4, 0.2, 0.0) * noise;
        return;
    }
    if (pixel.b > 0.5) {
        // Water.
        ivec2 noiseCoordA = pos + ivec2(iFrame * 23, 0);
        ivec2 noiseCoordB = pos + ivec2(0, iFrame * 14 + 1);
        //float noiseA = texelFetch(iChannel1, noiseCoordA % 64, 0).r;
        float noiseA = texture(iChannel1, (vec2(noiseCoordA % 64)+0.5)/iResolution.xy).x;
        
        //float noiseB = texelFetch(iChannel1, noiseCoordB % 64, 0).r;
        float noiseB = texture(iChannel1, (vec2(noiseCoordB % 64)+0.5)/iResolution.xy).x;
        
        float noise = noiseA * noiseB;
        fragColor = vec4(0.2, 0.6, 1.0, 1.0);
        if (noise > 0.9) {
            // Sparkle!
            fragColor += vec4(0.2, 0.2, 0.0, 0.0) * noise;
        }
        return;
    }
    if (pixel.y > 0.5) {
        // Bedrock.
        vec2 noiseCoord = fragCoord * vec2(0.25, 1.0) / 64.0;
        float noise = texture(iChannel1, noiseCoord).x;
        noise = 0.25 + noise * 0.1 + pixel.x * 0.2;
        fragColor = vec4(vec3(1) * noise, 1.0);
        return;
    }
    // Sand.
    fragColor = vec4(vec3(1.0, 0.8, 0.4) * pixel.x, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Buffer A:
// - Determines which particle to receive from neighboring pixels.
// - All movement and most interaction rules are defined here.

#ifdef ORG
const ivec2 A = ivec2(0, 1); // Above.
const ivec2 B = ivec2(0, -1); // Below.
const ivec2 L = ivec2(-1, 0); // Left.
const ivec2 R = ivec2(1, 0); // Right.
const ivec2 AL = A + L; // Above Left.
const ivec2 AR = A + R; // Above Right.
const ivec2 BL = B + L; // Below Left.
const ivec2 BR = B + R; // Below Right.
#else
#define A  ivec2(0, 1) // Above.
#define B  ivec2(0, -1) // Below.
#define L  ivec2(-1, 0) // Left.
#define R  ivec2(1, 0) // Right.
#define AL  (A + L) // Above Left.
#define AR  (A + R) // Above Right.
#define BL  (B + L) // Below Left.
#define BR  (B + R) // Below Right.
#endif

vec4 pixel(ivec2 pos) {
    if (pos.x < 0 || pos.y < 0 || pos.x >= int(iResolution.x) || pos.y >= int(iResolution.y)) {
        return vec4(0);
    }
    //return texelFetch(iChannel0, pos, 0);
    return texture(iChannel0, (vec2(pos)+0.5)/iResolution.xy);
}

float noise(ivec2 pos) {
    // 2531 = ClosestPrime(TextureArea * GoldenRatio).
    int n = iFrame % 4096 * 2531;
    ivec2 noiseCoord = pos + ivec2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (vec2((noiseCoord % 64)) + 0.5)/iResolution.xy).x;
}

ivec2 bubble(ivec2 pos, vec4 self) {
    if (self.g > 0.5 || self.a > 0.5) {
        // Self is bedrock or lava, can't bubble.
        return pos;
    }
    if (iFrame % 2 == 0) {
        // Bubble at half speed.
        return pos;
    }
    if (self.b > 0.5) {
        // Self is water.
        vec4 above = pixel(pos + A);
        if (above.r < 0.5 || above.g > 0.5 || above.b > 0.5) {
            // No sand above, can't bubble.
            return pos;
        }
        if (pos.y > 0 && pixel(pos + B).r < 0.5) {
            // Might fall, don't bubble.
            return pos;
        }
        if (pixel(pos + L).r < 0.5 || pixel(pos + R).r < 0.5) {
            // Sand might roll, don't bubble.
            return pos;
        }
        // Bubble up.
        return pos + A;
    }
    // Self is sand.
    vec4 below = pixel(pos + B);
    if (below.b < 0.5 || below.a > 0.5) {
        // No water below, can't bubble.
        return pos;
    }
    if (pos.y > 0 && (pixel(pos + BL).r < 0.5 || pixel(pos + BR).r < 0.5)) {
        // Might roll, don't bubble.
        return pos;
    }
    if (pos.y > 1 && pixel(pos + B + B).r < 0.5) {
        // Water might fall, don't bubble.
        return pos;
    }
    // Bubble down.
    return pos + B;
}

float flowRight(ivec2 pos) {
    if (pixel(pos + L).b > 0.5 && (pos.y == 0 || pixel(pos + BL).r > 0.5)) {
        // Left flowing.
        if (pixel(pos + L + L).r > 0.5) {
            // Can't flow left, flow right.
            return noise(pos + L) + 1.0;
        }
        // Flow randomly.
        return noise(pos + L);
    }
    return 0.0;
}

float flowLeft(ivec2 pos) {
    if (pixel(pos + R).b > 0.5 && (pos.y == 0 || pixel(pos + BR).r > 0.5)) {
        // Right flowing.
        if (pixel(pos + R + R).r > 0.5) {
            // Can't flow right, flow left.
            return noise(pos + R) - 1.0;
        }
        // Flow randomly.
        return noise(pos + R);
    }
    return 1.0;
}

ivec2 flow(ivec2 pos) {
    float rightFlow = flowRight(pos);
    float leftFlow = flowLeft(pos);
    if (rightFlow > 0.5 && leftFlow < 0.5) {
        // Flow contested.
        if (leftFlow + rightFlow > 1.0) {
            // Left wins, flow right.
            return pos + L;
        }
        // Right wins, flow left.
        return pos + R;
    }
    if (rightFlow > 0.5) {
        // Flow right.
        return pos + L;
    }
    if (leftFlow < 0.5) {
        // Flow left.
        return pos + R;
    }
    return pos;
}

bool rollRight(ivec2 pos) {
    vec4 aboveLeft = pixel(pos + AL);
    if (aboveLeft.g > 0.5 || aboveLeft.b > 0.5) {
        // Above left is not sand, can't roll.
        return false;
    }
    if (pixel(pos + L).r > 0.5 && aboveLeft.r > 0.5) {
        // Above left rolling.
        if (pixel(pos + AL + L).r > 0.5 || pixel(pos + L + L).r > 0.5) {
            // Can't roll left, roll right.
            return true;
        }
        if (noise(pos + AL) > 0.5) {
            // Roll right.
            return true;
        }
    }
    return false;
}

bool rollLeft(ivec2 pos) {
    vec4 aboveRight = pixel(pos + AR);
    if (aboveRight.g > 0.5 || aboveRight.b > 0.5) {
        // Above right is not sand, can't roll.
        return false;
    }
    if (pixel(pos + R).r > 0.5 && aboveRight.r > 0.5) {
        // Above right rolling.
        if (pixel(pos + AR + R).r > 0.5 || pixel(pos + R + R).r > 0.5) {
            // Can't roll right, roll left.
            return true;
        }
        if (noise(pos + AR) < 0.5) {
            // Roll left.
            return true;
        }
    }
    return false;
}

ivec2 roll(ivec2 pos) {
    bool canRollRight = rollRight(pos);
    bool canRollLeft = rollLeft(pos);
    if (canRollRight && canRollLeft) {
        // Roll contested.
        if (noise(pos + AL) + noise(pos + AR) > 1.0) {
            // Left wins, roll right.
            return pos + AL;
        }
        // Right wins, roll left.
        return pos + AR;
    }
    if (canRollRight) {
        // Roll right.
        return pos + AL;
    }
    if (canRollLeft) {
        // Roll left.
        return pos + AR;
    }
    return pos;
}

ivec2 receive(ivec2 pos) {
    vec4 self = pixel(pos);
    if (self.r > 0.5) {
        // Self not empty.
        ivec2 bubblePos = bubble(pos, self);
        if (bubblePos != pos) {
            // Receive bubbling.
            return bubblePos;
        }
        // Block.
        return pos;
    }
    vec4 above = pixel(pos + A);
    if (above.r > 0.5 && above.g < 0.5) {
        if (pixel(pos + AL).r > 0.5 && pixel(pos + AR).r > 0.5) {
            // Self contested from roll, let above fall.
            return pos + A;
        }
        if (pixel(pos + L).b > 0.5 && pixel(pos + R).b > 0.5) {
            // Self contested from flow, let above fall.
            return pos + A;
        }
    }
    ivec2 rollPos = roll(pos);
    if (rollPos != pos) {
        // Receive rolling.
        return rollPos;
    }
    ivec2 flowPos = flow(pos);
    if (flowPos != pos) {
        // Receive flowing.
        return flowPos;
    }
    if (above.r > 0.5 && above.g < 0.5) {
        // Receive falling.
        return pos + A;
    }
    return pos;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 pos = ivec2(fragCoord);
    ivec2 receivePos = receive(pos);
    ivec2 offset = receivePos - pos;
    fragColor = vec4(float(offset.x + 1) / 2.0, float(offset.y + 1) / 2.0, 0.0, 0.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 texelFetchC( sampler2D Channel, ivec2 pos, int xxx)
{
    
    if ( (pos.x) > 0 && (pos.x) < int(iResolution.x) && (pos.y) > 0 && (pos.y) < int(iResolution.y) )
    {
        return texture( Channel, (vec2(pos)+10.)/iResolution.xy );
    }
	else
		return vec4(0);
}



// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Buffer B:
// - Holds particle state and moves particles.
// - Destructive particle interaction happens here after movement.
// - Drives user interaction.

const ivec2 A = ivec2(0, 1); // Above.
const ivec2 B = ivec2(0, -1); // Below.
const ivec2 L = ivec2(-1, 0); // Left.
const ivec2 R = ivec2(1, 0); // Right.
const ivec2 BL = B + L; // Below Left.
const ivec2 BR = B + R; // Below Right.

vec4 pixel(ivec2 pos) {
    if (pos.x < 0 || pos.y < 0 || pos.x >= int(iResolution.x) || pos.y >= int(iResolution.y)) {
        return vec4(0);
    }
    //return texelFetch(iChannel0, pos, 0);
    return texture(iChannel0, (vec2(pos)+0.5)/iResolution.xy);
}

float noise(ivec2 pos) {
    // 2531 = ClosestPrime(TextureArea * GoldenRatio).
    int n = iFrame % 4096 * 2531;
    ivec2 noiseCoord = pos + ivec2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (vec2((noiseCoord*16 ))+0.)/iResolution.xy).x;
}

float noiseB(ivec2 pos) {
    int n = (iFrame + 2048) % 4096 * 2531;
    ivec2 noiseCoord = pos + ivec2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (vec2((noiseCoord *16)) + 0.5)/iResolution.xy).x;
}

ivec2 receive(ivec2 pos) {
    if (pos.y < 0 || pos.y >= int(iResolution.y)) {
        // Stop at bottom and top.
        return pos;
    }
    if (pos.x < 0) {
        // Remove at left.
        return pos + R;
    }
    if (pos.x >= int(iResolution.x)) {
        // Remove at right.
        return pos + L;
    }
    //vec4 offsetColor = texelFetch(iChannel3, pos, 0);
    vec4 offsetColor = texture(iChannel3, (vec2(pos)+0.5)/iResolution.xy);
    
    ivec2 offset = ivec2(int(offsetColor.r * 2.0 + 0.5) - 1, int(offsetColor.g * 2.0 + 0.5) - 1);
    return pos + offset;
}

vec4 move(ivec2 pos, vec4 self) {
    if (receive(pos + B) == pos) { return vec4(0); }
    if (receive(pos + BL) == pos) { return vec4(0); }
    if (receive(pos + BR) == pos) { return vec4(0); }
    if (receive(pos + L) == pos) { return vec4(0); }
    if (receive(pos + R) == pos) { return vec4(0); }
    if (receive(pos + A) == pos) { return vec4(0); }
    return self;
}

vec4 boil(ivec2 pos, vec4 self) {
    if (noise(pos) > 0.1) {
        // Boil slow.
        return self;
    }
    if (pixel(receive(pos + A)).a > 0.5) { return vec4(0); }
    if (pixel(receive(pos + B)).a > 0.5) { return vec4(0); }
    if (pixel(receive(pos + L)).a > 0.5) { return vec4(0); }
    if (pixel(receive(pos + R)).a > 0.5) { return vec4(0); }
    return self;
}

vec4 melt(ivec2 pos, vec4 self) {
    if (noise(pos) > 0.06) {
        // Melt slow.
        return self;
    }
    if (pixel(receive(pos + B)).a > 0.5) {
        // Over lava, melt.
        if (noise(pos) > 0.04) {
            // Convert some sand into lava.
            return vec4(1.0, 0.0, 1.0, 1.0);
        }
        return vec4(0);
    }
    if (noise(pos) > 0.04 && (pixel(receive(pos + BL)).a > 0.5 || pixel(receive(pos + BR)).a > 0.5)) {
        // Diagonally over lava, melt slower.
        return vec4(0);
    }
    return self;
}

vec4 freeze(ivec2 pos, vec4 self) {
    if (noise(pos) > 0.06) {
        // Freeze slow.
        return self;
    }
    bool shouldFreeze = false;
    if (
        (pixel(receive(pos + A)).b > 0.5 && pixel(receive(pos + A)).a < 0.5) ||
        (pixel(receive(pos + B)).b > 0.5 && pixel(receive(pos + B)).a < 0.5) ||
        (pixel(receive(pos + L)).b > 0.5 && pixel(receive(pos + L)).a < 0.5) ||
        (pixel(receive(pos + R)).b > 0.5 && pixel(receive(pos + R)).a < 0.5)
    ) {
        // Near water, freeze to sand.
        float value = 0.75 + 0.25 * noiseB(pos);
        return  vec4(value, 0.0, 0.0, 0.0);
    }
    return self;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 pos = ivec2(fragCoord);
    if (iMouse.z > 0.5 && distance(iMouse.xy, fragCoord) < 12.0) {
        // Left mouse button pressed.
        float value = noise(pos);
        //if (texelFetch(iChannel2, ivec2(87, 0), 0).r > 0.0) {
        if (texture(iChannel2, (vec2(87, 0)+0.5)/iResolution.xy).x > 0.0) {
            // W pressed, add water.
            value = float(value > 0.5);
            fragColor = value * vec4(1.0, 0.0, 1.0, 0.0);
            //return;
        }
        else
        if (texelFetch(iChannel2, ivec2(76, 0), 0).r > 0.0) {
            // L pressed, add lava.
            value = float(value > 0.5);
            fragColor = value * vec4(1.0, 0.0, 1.0, 1.0);
            //return;
        }
        else
        if (texelFetch(iChannel2, ivec2(66, 0), 0).r > 0.0) {
            // B pressed, add bedrock.
            value = 0.75 + value * 0.25;
            fragColor = vec4(max(0.75, value), 1.0, 0.0, 0.0);
            //return;
        }
        else
        if (texelFetch(iChannel2, ivec2(88, 0), 0).r > 0.0) {
            // X pressed, erase.
            fragColor = vec4(0);
            //return;
        }
        // No key, add sand.
        value = float(value > 0.5) * (0.5 + 0.5 * value);
        fragColor = vec4(value, 0.0, 0.0, 0.0);
        return;
    }
    // Receive particle.
    ivec2 receivePos = receive(pos);
    fragColor = pixel(receivePos);
    if (fragColor.a > 0.5) {
        // Received lava, freeze if near water.
        fragColor = freeze(pos, fragColor);
    }
    if (fragColor.b > 0.5 && fragColor.a < 0.5) {
        // Received water, boil if near lava.
        fragColor = boil(pos, fragColor);
    }
    if (fragColor.r > 0.5 && fragColor.g < 0.5 && fragColor.b < 0.5 && fragColor.a < 0.5) {
        // Received sand, melt if near lava.
        fragColor = melt(pos, fragColor);
    }
    if (receivePos == pos) {
        // Didn't receive from neighbor.
        if (fragColor.r > 0.5) {
            // Move.
            fragColor = move(ivec2(fragCoord), fragColor);
        }
    }
}