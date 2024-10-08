# Applying a GameBoy PostProcessing filter

In this tutorial we are going to create a PostProcessing shader (in both GLSL and HLSL) that will be applied to the Game Viewport.

Video version: https://youtu.be/E2L5MC3e0s0

![image](..//Screenshots/GAMEBOY_000.png)

I am using the TopDown official Epic template for it, but you can use any level you want.

The code is based on this shadertoy shader: https://www.shadertoy.com/view/MlfBR8 (set one of the available videos as iChannel0, like the Van Damme one, and have fun)

## The Shader (GLSL)

```glsl
#version 450

// the uv of the currently processed pixel
layout(location=0) in vec2 uv;

// the output color (UAV)
layout(location=0) out vec4 outColor;

// SceneColorInput (SRV)
layout(binding=0) uniform texture2D colorInput;
layout(binding=1) uniform sampler sampler0;

const vec3 shades[4] = vec3[4](
    vec3(15./255., 56./255., 15./255.),
    vec3(48./255., 98./255., 48./255.),
    vec3(139./255., 172./255., 15./255.),
    vec3(155./255., 188./255., 15./255.)
);

void main()
{
    // get output texture size
    const ivec2 size = textureSize(sampler2D(colorInput, sampler0), 0);
    // 800 is a good compromise for pixel size
    const float ratio = size.x / 800.; 

    // 160x144 is the original gameboy resolution
    const vec2 resolution = vec2(160., 144.) * ratio;
    const vec2 st = floor(uv * resolution) / resolution;
    
    // read the input color
    const vec3 color = texture(sampler2D(colorInput, sampler0), st).rgb;
    
    // find the color in the shades array
    const float intensity = (color.r + color.g + color.b) / 3.;
    const int index = int(intensity * 4.);
    
    // write it
    outColor = vec4(shades[index], 1.0);
}
```

When writing postprocessing shader with Compushady, you need to get the UV of the currently processed pixel (they are available in the location 0 of the fragment/pixel shader) and eventually one of the Scene Textures (Color, GBuffers, Depth, Stencil...) with the related Sampler (the filter for reading the texture values)

Note that while GLSL supports the sampler2D type (texture and sampler as a single object), in Compushady (and generally in Vulkan-friendly GLSL) is better to split them like in HLSL. This simplifies the support for the same shader code on the various platforms.

## Running the PostProcess effect on the Viewport

In this example we need just the ColorInput (this is the current result of the various passes of the renderer) and a Sampler (with point/nearest filtering and clamp addressing mode):

![image](../Screenshots/GAMEBOY_001.png)

Now we can create the Compushady Blendable object (Blendables are pipelines, generally compute or rasterizer, that are executed automatically at every frame and combined with the renderer passes):

Note how we mapped colorInput and sampler0 shader variables/resources to their respective SRV and Sampler. 

![image](../Screenshots/GAMEBOY_002.png)

(Eventually connect the ErrorMessages pin to a PrintString node to get error messages, if any)

The blendable is ready, and we can now attach it to the viewport or to a PostProcess Volume.

Attaching to a Viewport is super easy thanks to The Blitter:

![image](../Screenshots/GAMEBOY_003.png)

You can now play the Level and enjoy.

## Running the PostProcess effect on a PostProcess Volume:

Instead of applying the effect to the whole viewport, we can configure a PostProcess Volume that triggers the effect whenever the current camera enters its volume.

The first step is defining the PostProcess Volume by adding it to the level and defining its Brush settings (the goal here is to activate the effect when the characters moves over the ramp):

![image](../Screenshots/GAMEBOY_004.png)

Then by adding a reference to the PostProcess Volume in the Level Blueprint, we can attach the Compushady Blendable to it:

![image](../Screenshots/GAMEBOY_005.png)

Now the effect will trigger as soon as the mannequin steps over the ramp, and will be turned off when the camera exits the volume.

Note that sometimes the context autocomplete fails to find the "Add or Update Blendable" node, just disable it from the context menu and search it again.

## HLSL Variant

The HLSL code is pretty similar:

```hlsl
Texture2D<float4> colorInput;
SamplerState sampler0;

static const float3 shades[4] = {
    float3(15./255., 56./255., 15./255.),
    float3(48./255., 98./255., 48./255.),
    float3(139./255., 172./255., 15./255.),
    float3(155./255., 188./255., 15./255.)
};

float4 main(const float2 uv : TEXCOORD) : SV_Target0
{
    // get output texture size
    uint width;
    uint height;
    colorInput.GetDimensions(width, height);
    const float2 size = float2(width, height);
    // 800 is a good compromise for pixel size
    const float ratio = size.x / 800.; 

    // 160x144 is the original gameboy resolution
    const float2 resolution = float2(160., 144.) * ratio;
    const float2 st = floor(uv * resolution) / resolution;
    
    // read the input color
    const float3 color = colorInput.Sample(sampler0, st).rgb;
    
    // find the color in the shades array
    const float intensity = (color.r + color.g + color.b) / 3.;
    const int index = int(intensity * 4.);
    
    // write it
   return float4(shades[index], 1.0);
}
```

Instead of locations/layout in HLSL we have semantics: TEXCOORD is the semantic containing the UVs (it it set by the postprocess vertex shader automatically) and SV_Target0 (it means 'write the the first render target/RTV'):

We can now use `CreateCompushadyBlendableByMapFromHLSLString` instead of `CreateCompushadyBlendableByMapFromGLSLString`:

![image](../Screenshots/GAMEBOY_006.png)
