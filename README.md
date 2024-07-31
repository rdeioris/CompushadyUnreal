# CompushadyUnreal
Compushady for Unreal Engine 5

## Intro

Compushady is an Unreal Engine 5 plugin aimed at easily (and quickly) executing GPU shaders. 

The plugin exposes features like runtime shaders loading (from strings, files or assets), conversion and compilation (HLSL and GLSL are supported, but if you are brave you can assemble from SPIRV too), reflection, fast copies (for both Textures and Buffers) and integration with various Unreal features (from postprocessing to raytracing) and assets (like MediaTextures, Curves, DataTables...).

The common use case is to optimize highly parallelizable problems using compute shaders, but you can integrate runtime shaders programming to generate motion graphics and even audio (yes, you can pipe the the audio output to a shader or generate waveforms from a shader!). The shadertoy website is always a good source for amazing idea: https://www.shadertoy.com/
 
Currently Windows (D3D12 and Vulkan), Linux and Android (Vulkan) are supported. Mac and iOS (Metal) are currently in development.

Like the homonym python module (this plugin is a porting of its APIs), it makes heavy use of the DirectXShaderCompiler project (https://github.com/Microsoft/DirectXShaderCompiler) as well as the various Khronos projects for SPIRV management.

Join the Discord server for support: https://discord.gg/2WvdpkYXHW

## Quickstart

Let's start with a glossary:

* `HLSL`: a high level shading language from microsoft (you can write your shaders in this language)
* `GLSL`: a high level shading language from Khronos (you can write your shaders in this language)
* `SPIRV`: a low level shading language from Khronos (you can write your shaders in this language but it is very unpractical, higher level languages can compile to SPIRV)
* `DXIL`: a low level shading language from Microsoft (limited support on Compushady, higher level languages can compile to DXIL)
* `CPU memory`: your system RAM, both your programm and the GPU can access it (slower access from the GPU)
* `GPU memory`: the memory directly installed on the GPU (if available, very quick access) or a portion of the system ram dedcated to it (in this case the access will be slower)
* `CBV`: Constant Buffer View, it represents a tiny block (generally no more than 4096 bytes) of constantly changing data accessible by a shader. Those blocks are generally mapped to the CPU memory.
* `SRV`: Shader Resource View, it represents potentially big readonly data in the form of buffers (raw bytes) or textures.
* `UAV`: Unordered Access View, it represents potentially big read/write data in the form of buffers (raw bytes) or textures.
* `Samplers`: blocks of configuration defining the filtering and addressing mode when reading pixels from textures.
* `RTV`: Render Target View, a texture to which the Rasterizer (see below) can write to
* `DSV`: Depth Stencil View, a texture containing the depth and the stencil buffer. The Rasterizer can optionally write to it.
* `Compute`: A compute shader, composed by a shader and a set of 0 or more CBV, SRV, UAV or samplers. You use Compute for running generic task on a GPU
* `Rasterizer`: A vertex + pixel shader or mesh + pixel shader (where supported) with a set of 0 or more RTV, CBV, SRV, UAV or samplers and 0 or 1 DSV. You use a Rasterizer for drawing triangles, lines and points using the GPU.
* `Blitter`: a Compushady subsystem for quickly drawing textures on the screen or applying post processing effects

We can now write our first shader (we will use HLSL) to generate a simple texture with a color gradient

As we need to write to a texture, our shader will require access to the UAV representing the texture (we can create UAVs from blueprints or C++)


```hlsl
RWTexture2D<float4> OutputTexture;

[numthreads(1, 1, 1)]
void main()
{
}
```


## Tutorials

* PostProcessing

## The Blitter

## API
