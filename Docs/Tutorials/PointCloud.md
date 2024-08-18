# Rendering PointClouds

This tutorial will show how to use the rasterizer and the blitter to render pointclouds (in XYZ/ASCII format) with both points and billboard quads.

The XYZ/ASCII format will be gzipped to show how Compushady con decompress and upload to the gpu on the fly (in asynchronous/nonblocking mode).

The tutorial will use HLSL for both the Vertex and Pixel shaders, but at the end of the tutorial you will find the GLSL variant.

## Step0: loading the data in a StructuredBuffer

For this tutorial we are going to use the Denver Union Station (https://en.wikipedia.org/wiki/Denver_Union_Station) pointcloud by Trimble Inc. (https://www.trimble.com/en) that you can download (in gzipped XYZ format) from here: https://github.com/rdeioris/compushady-assets/raw/main/UnionStation.txt.gz

![image](..//Screenshots/POINTCLOUD_000.png)

The file contains one point per line, with each line exposing 6 relevant columns (space separated): X, Y, Z, R, G and B

For loading those data in a GPU StructuredBuffer, we need to instruct the Compushady GZIP ASCII Async loader on how to build the follwing structure:

```c
struct Point
{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};
```

This is pretty straightforward:



## Step1: rendering points

## Step2: moving to quads

## Optional Step 3: storing the shaders in a file

## Optional Step 4: using GLSL
