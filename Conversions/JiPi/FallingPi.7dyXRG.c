
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Gray Noise Small' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Buffer A:
// - Determines which particle to receive from neighboring pixels.
// - All movement and most interaction rules are defined here.

#ifdef ORG
const int2 A = to_int2(0, 1); // Above.
const int2 B = to_int2(0, -1); // Below.
const int2 L = to_int2(-1, 0); // Left.
const int2 R = to_int2(1, 0); // Right.
const int2 AL = A + L; // Above Left.
const int2 AR = A + R; // Above Right.
const int2 BL = B + L; // Below Left.
const int2 BR = B + R; // Below Right.
#else
#define A  to_int2(0, 1) // Above.
#define B  to_int2(0, -1) // Below.
#define L  to_int2(-1, 0) // Left.
#define R  to_int2(1, 0) // Right.
#define AL  (A + L) // Above Left.
#define AR  (A + R) // Above Right.
#define BL  (B + L) // Below Left.
#define BR  (B + R) // Below Right.
#endif

__DEVICE__ float4 pixel(int2 pos) {
    if (pos.x < 0 || pos.y < 0 || pos.x >= int(iResolution.x) || pos.y >= int(iResolution.y)) {
        return to_float4_s(0);
    }
    //return texelFetch(iChannel0, pos, 0);
    return texture(iChannel0, (to_float2(pos)+0.5f)/iResolution);
}

__DEVICE__ float noise(int2 pos) {
    // 2531 = ClosestPrime(TextureArea * GoldenRatio).
    int n = iFrame % 4096 * 2531;
    int2 noiseCoord = pos + to_int2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (to_float2((noiseCoord % 64)) + 0.5f)/iResolution).x;
}

int2 bubble(int2 pos, float4 self) {
    if (self.y > 0.5f || self.w > 0.5f) {
        // Self is bedrock or lava, can't bubble.
        return pos;
    }
    if (iFrame % 2 == 0) {
        // Bubble at half speed.
        return pos;
    }
    if (self.z > 0.5f) {
        // Self is water.
        float4 above = pixel(pos + A);
        if (above.x < 0.5f || above.y > 0.5f || above.z > 0.5f) {
            // No sand above, can't bubble.
            return pos;
        }
        if (pos.y > 0 && pixel(pos + B).r < 0.5f) {
            // Might fall, don't bubble.
            return pos;
        }
        if (pixel(pos + L).r < 0.5f || pixel(pos + R).r < 0.5f) {
            // Sand might roll, don't bubble.
            return pos;
        }
        // Bubble up.
        return pos + A;
    }
    // Self is sand.
    float4 below = pixel(pos + B);
    if (below.z < 0.5f || below.w > 0.5f) {
        // No water below, can't bubble.
        return pos;
    }
    if (pos.y > 0 && (pixel(pos + BL).r < 0.5f || pixel(pos + BR).r < 0.5f)) {
        // Might roll, don't bubble.
        return pos;
    }
    if (pos.y > 1 && pixel(pos + B + B).r < 0.5f) {
        // Water might fall, don't bubble.
        return pos;
    }
    // Bubble down.
    return pos + B;
}

__DEVICE__ float flowRight(int2 pos) {
    if (pixel(pos + L).b > 0.5f && (pos.y == 0 || pixel(pos + BL).r > 0.5f)) {
        // Left flowing.
        if (pixel(pos + L + L).r > 0.5f) {
            // Can't flow left, flow right.
            return noise(pos + L) + 1.0f;
        }
        // Flow randomly.
        return noise(pos + L);
    }
    return 0.0f;
}

__DEVICE__ float flowLeft(int2 pos) {
    if (pixel(pos + R).b > 0.5f && (pos.y == 0 || pixel(pos + BR).r > 0.5f)) {
        // Right flowing.
        if (pixel(pos + R + R).r > 0.5f) {
            // Can't flow right, flow left.
            return noise(pos + R) - 1.0f;
        }
        // Flow randomly.
        return noise(pos + R);
    }
    return 1.0f;
}

int2 flow(int2 pos) {
    float rightFlow = flowRight(pos);
    float leftFlow = flowLeft(pos);
    if (rightFlow > 0.5f && leftFlow < 0.5f) {
        // Flow contested.
        if (leftFlow + rightFlow > 1.0f) {
            // Left wins, flow right.
            return pos + L;
        }
        // Right wins, flow left.
        return pos + R;
    }
    if (rightFlow > 0.5f) {
        // Flow right.
        return pos + L;
    }
    if (leftFlow < 0.5f) {
        // Flow left.
        return pos + R;
    }
    return pos;
}

__DEVICE__ bool rollRight(int2 pos) {
    float4 aboveLeft = pixel(pos + AL);
    if (aboveLeft.y > 0.5f || aboveLeft.z > 0.5f) {
        // Above left is not sand, can't roll.
        return false;
    }
    if (pixel(pos + L).r > 0.5f && aboveLeft.x > 0.5f) {
        // Above left rolling.
        if (pixel(pos + AL + L).r > 0.5f || pixel(pos + L + L).r > 0.5f) {
            // Can't roll left, roll right.
            return true;
        }
        if (noise(pos + AL) > 0.5f) {
            // Roll right.
            return true;
        }
    }
    return false;
}

__DEVICE__ bool rollLeft(int2 pos) {
    float4 aboveRight = pixel(pos + AR);
    if (aboveRight.y > 0.5f || aboveRight.z > 0.5f) {
        // Above right is not sand, can't roll.
        return false;
    }
    if (pixel(pos + R).r > 0.5f && aboveRight.x > 0.5f) {
        // Above right rolling.
        if (pixel(pos + AR + R).r > 0.5f || pixel(pos + R + R).r > 0.5f) {
            // Can't roll right, roll left.
            return true;
        }
        if (noise(pos + AR) < 0.5f) {
            // Roll left.
            return true;
        }
    }
    return false;
}

int2 roll(int2 pos) {
    bool canRollRight = rollRight(pos);
    bool canRollLeft = rollLeft(pos);
    if (canRollRight && canRollLeft) {
        // Roll contested.
        if (noise(pos + AL) + noise(pos + AR) > 1.0f) {
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

int2 receive(int2 pos) {
    float4 self = pixel(pos);
    if (self.x > 0.5f) {
        // Self not empty.
        int2 bubblePos = bubble(pos, self);
        if (bubblePos != pos) {
            // Receive bubbling.
            return bubblePos;
        }
        // Block.
        return pos;
    }
    float4 above = pixel(pos + A);
    if (above.x > 0.5f && above.y < 0.5f) {
        if (pixel(pos + AL).r > 0.5f && pixel(pos + AR).r > 0.5f) {
            // Self contested from roll, let above fall.
            return pos + A;
        }
        if (pixel(pos + L).b > 0.5f && pixel(pos + R).b > 0.5f) {
            // Self contested from flow, let above fall.
            return pos + A;
        }
    }
    int2 rollPos = roll(pos);
    if (rollPos != pos) {
        // Receive rolling.
        return rollPos;
    }
    int2 flowPos = flow(pos);
    if (flowPos != pos) {
        // Receive flowing.
        return flowPos;
    }
    if (above.x > 0.5f && above.y < 0.5f) {
        // Receive falling.
        return pos + A;
    }
    return pos;
}

__KERNEL__ void FallingPiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    int2 pos = to_int2(fragCoord);
    int2 receivePos = receive(pos);
    int2 offset = receivePos - pos;
    fragColor = to_float4_aw(float(offset.x + 1) / 2.0f, float(offset.y + 1) / 2.0f, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Gray Noise Small' to iChannel1
// Connect Buffer B 'Preset: Keyboard' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel3
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


vec4 texelFetchC( sampler2D Channel, int2 pos, int xxx)
{
    
    if ( (pos.x) > 0 && (pos.x) < int(iResolution.x) && (pos.y) > 0 && (pos.y) < int(iResolution.y) )
    {
        return texture( Channel, (to_float2(pos)+10.0f)/iResolution );
    }
  else
    return to_float4_aw(0);
}



// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Buffer B:
// - Holds particle state and moves particles.
// - Destructive particle interaction happens here after movement.
// - Drives user interaction.

const int2 A = to_int2(0, 1); // Above.
const int2 B = to_int2(0, -1); // Below.
const int2 L = to_int2(-1, 0); // Left.
const int2 R = to_int2(1, 0); // Right.
const int2 BL = B + L; // Below Left.
const int2 BR = B + R; // Below Right.

__DEVICE__ float4 pixel(int2 pos) {
    if (pos.x < 0 || pos.y < 0 || pos.x >= int(iResolution.x) || pos.y >= int(iResolution.y)) {
        return to_float4(0);
    }
    //return texelFetch(iChannel0, pos, 0);
    return texture(iChannel0, (to_float2(pos)+0.5f)/iResolution);
}

__DEVICE__ float noise(int2 pos) {
    // 2531 = ClosestPrime(TextureArea * GoldenRatio).
    int n = iFrame % 4096 * 2531;
    int2 noiseCoord = pos + to_int2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (to_float2((noiseCoord*16 ))+0.0f)/iResolution).x;
}

__DEVICE__ float noiseB(int2 pos) {
    int n = (iFrame + 2048) % 4096 * 2531;
    int2 noiseCoord = pos + to_int2(n, n / 64);
    //return texelFetch(iChannel1, noiseCoord % 64, 0).r;
    return texture(iChannel1, (to_float2((noiseCoord *16)) + 0.5f)/iResolution).x;
}

int2 receive(int2 pos) {
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
    float4 offsetColor = texture(iChannel3, (to_float2(pos)+0.5f)/iResolution);
    
    int2 offset = to_int2(int(offsetColor.x * 2.0f + 0.5f) - 1, int(offsetColor.y * 2.0f + 0.5f) - 1);
    return pos + offset;
}

__DEVICE__ float4 move(int2 pos, float4 self) {
    if (receive(pos + B) == pos) { return to_float4_aw(0); }
    if (receive(pos + BL) == pos) { return to_float4(0); }
    if (receive(pos + BR) == pos) { return to_float4(0); }
    if (receive(pos + L) == pos) { return to_float4(0); }
    if (receive(pos + R) == pos) { return to_float4(0); }
    if (receive(pos + A) == pos) { return to_float4(0); }
    return self;
}

__DEVICE__ float4 boil(int2 pos, float4 self) {
    if (noise(pos) > 0.1f) {
        // Boil slow.
        return self;
    }
    if (pixel(receive(pos + A)).a > 0.5f) { return to_float4_aw(0); }
    if (pixel(receive(pos + B)).a > 0.5f) { return to_float4(0); }
    if (pixel(receive(pos + L)).a > 0.5f) { return to_float4(0); }
    if (pixel(receive(pos + R)).a > 0.5f) { return to_float4(0); }
    return self;
}

__DEVICE__ float4 melt(int2 pos, float4 self) {
    if (noise(pos) > 0.06f) {
        // Melt slow.
        return self;
    }
    if (pixel(receive(pos + B)).a > 0.5f) {
        // Over lava, melt.
        if (noise(pos) > 0.04f) {
            // Convert some sand into lava.
            return to_float4(1.0f, 0.0f, 1.0f, 1.0f);
        }
        return to_float4_aw(0);
    }
    if (noise(pos) > 0.04f && (pixel(receive(pos + BL)).a > 0.5f || pixel(receive(pos + BR)).a > 0.5f)) {
        // Diagonally over lava, melt slower.
        return to_float4(0);
    }
    return self;
}

__DEVICE__ float4 freeze(int2 pos, float4 self) {
    if (noise(pos) > 0.06f) {
        // Freeze slow.
        return self;
    }
    bool shouldFreeze = false;
    if (
        (pixel(receive(pos + A)).b > 0.5f && pixel(receive(pos + A)).a < 0.5f) ||
        (pixel(receive(pos + B)).b > 0.5f && pixel(receive(pos + B)).a < 0.5f) ||
        (pixel(receive(pos + L)).b > 0.5f && pixel(receive(pos + L)).a < 0.5f) ||
        (pixel(receive(pos + R)).b > 0.5f && pixel(receive(pos + R)).a < 0.5f)
    ) {
        // Near water, freeze to sand.
        float value = 0.75f + 0.25f * noiseB(pos);
        return  to_float4(value, 0.0f, 0.0f, 0.0f);
    }
    return self;
}

__KERNEL__ void FallingPiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    int2 pos = to_int2(fragCoord);
    if (iMouse.z > 0.5f && distance(swi2(iMouse,x,y), fragCoord) < 12.0f) {
        // Left mouse button pressed.
        float value = noise(pos);
        //if (texelFetch(iChannel2, to_int2(87, 0), 0).r > 0.0f) {
        if (texture(iChannel2, (to_float2(87, 0)+0.5f)/iResolution).x > 0.0f) {
            // W pressed, add water.
            value = float(value > 0.5f);
            fragColor = value * to_float4(1.0f, 0.0f, 1.0f, 0.0f);
            //return;
        }
        else
        if (texelFetch(iChannel2, to_int2(76, 0), 0).r > 0.0f) {
            // L pressed, add lava.
            value = float(value > 0.5f);
            fragColor = value * to_float4(1.0f, 0.0f, 1.0f, 1.0f);
            //return;
        }
        else
        if (texelFetch(iChannel2, to_int2(66, 0), 0).r > 0.0f) {
            // B pressed, add bedrock.
            value = 0.75f + value * 0.25f;
            fragColor = to_float4_aw(_fmaxf(0.75f, value), 1.0f, 0.0f, 0.0f);
            //return;
        }
        else
        if (texelFetch(iChannel2, to_int2(88, 0), 0).r > 0.0f) {
            // X pressed, erase.
            fragColor = to_float4_aw(0);
            //return;
        }
        // No key, add sand.
        value = float(value > 0.5f) * (0.5f + 0.5f * value);
        fragColor = to_float4(value, 0.0f, 0.0f, 0.0f);
        return;
    }
    // Receive particle.
    int2 receivePos = receive(pos);
    fragColor = pixel(receivePos);
    if (fragColor.w > 0.5f) {
        // Received lava, freeze if near water.
        fragColor = freeze(pos, fragColor);
    }
    if (fragColor.z > 0.5f && fragColor.w < 0.5f) {
        // Received water, boil if near lava.
        fragColor = boil(pos, fragColor);
    }
    if (fragColor.x > 0.5f && fragColor.y < 0.5f && fragColor.z < 0.5f && fragColor.w < 0.5f) {
        // Received sand, melt if near lava.
        fragColor = melt(pos, fragColor);
    }
    if (receivePos == pos) {
        // Didn't receive from neighbor.
        if (fragColor.x > 0.5f) {
            // Move.
            fragColor = move(to_int2(fragCoord), fragColor);
        }
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Small' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0


// = Falling Pixels =
// © 2020 Jacob Lindberg
//-----------------------
// Main/image buffer:
// - Performs shading of the state from Buffer B.

__KERNEL__ void FallingPiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    int2 pos = to_int2(fragCoord);
    //vec4 pixel = texelFetch(iChannel0, pos, 0);
    float4 pixel = texture(iChannel0, (to_float2(pos)+0.5f)/iResolution);
    
    
    if (pixel.x < 0.5f) {
        // Empty.
        fragColor = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }
    if (pixel.w > 0.5f) {
        // Lava.
        float2 noiseCoordA = fragCoord * to_float2(0.2f, 1.0f) / 64.0f;
        noiseCoordA.x += float(iFrame) * 0.001f;
        float2 noiseCoordB = fragCoord * to_float2(0.2f, 1.0f) / 64.0f;
        noiseCoordB -= to_float2(float(iFrame) * 0.001f, 0.5f);
        float noiseA = _tex2DVecN(iChannel1,noiseCoordA.x,noiseCoordA.y,15).x;
        float noiseB = _tex2DVecN(iChannel1,noiseCoordB.x,noiseCoordB.y,15).x;
        float noise = noiseA * noiseB;
        fragColor = to_float4(0.8f, 0.4f, 0.0f, 1.0f);
        fragColor += to_float4(0.1f, 0.4f, 0.2f, 0.0f) * noise;
        return;
    }
    if (pixel.z > 0.5f) {
        // Water.
        int2 noiseCoordA = pos + to_int2(iFrame * 23, 0);
        int2 noiseCoordB = pos + to_int2(0, iFrame * 14 + 1);
        //float noiseA = texelFetch(iChannel1, noiseCoordA % 64, 0).r;
        float noiseA = texture(iChannel1, (to_float2(noiseCoordA % 64)+0.5f)/iResolution).x;
        
        //float noiseB = texelFetch(iChannel1, noiseCoordB % 64, 0).r;
        float noiseB = texture(iChannel1, (to_float2(noiseCoordB % 64)+0.5f)/iResolution).x;
        
        float noise = noiseA * noiseB;
        fragColor = to_float4(0.2f, 0.6f, 1.0f, 1.0f);
        if (noise > 0.9f) {
            // Sparkle!
            fragColor += to_float4(0.2f, 0.2f, 0.0f, 0.0f) * noise;
        }
        return;
    }
    if (pixel.y > 0.5f) {
        // Bedrock.
        float2 noiseCoord = fragCoord * to_float2(0.25f, 1.0f) / 64.0f;
        float noise = _tex2DVecN(iChannel1,noiseCoord.x,noiseCoord.y,15).x;
        noise = 0.25f + noise * 0.1f + pixel.x * 0.2f;
        fragColor = to_float4_aw(to_float3(1) * noise, 1.0f);
        return;
    }
    // Sand.
    fragColor = to_float4(to_float3(1.0f, 0.8f, 0.4f) * pixel.x, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}