# Applying a GameBoy PostProcessing filter

In this tutorial we are going to create a PostProcessing shader (in both GLSL and HLSL) that will be applied to the Game Viewport.

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
layout(binding=0) uniform sampler2D colorInput;

const vec3 shades[4] = vec3[4](
	vec3(15./255., 56./255., 15./255.),
    vec3(48./255., 98./255., 48./255.),
    vec3(139./255., 172./255., 15./255.),
    vec3(155./255., 188./255., 15./255.)
);

void main()
{
    // get output texture size
    const ivec2 size = textureSize(colorInput, 0);
    // 800 is a good compromise for pixel size
    const float ratio = size.x / 800.; 

    // 160x144 is the original gameboy resolution
    const vec2 resolution = vec2(160., 144.) * ratio;
    const vec2 st = floor(uv * resolution) / resolution;
    
    // read the input color
	  const vec3 color = texture(colorInput, st).rgb;
    
    // find the color in the shades array
    const float intensity = (color.r + color.g + color.b) / 3.;
    const int index = int(intensity * 4.);
    
    // write it
    outColor = vec4(shades[index], 1.0);
}
```

## The Shader (HLSL)
