# OpenCL Gaussian Blur
This is a gaussian blur implementation written in *C* that can run multithreaded on your CPU using *pthreads* or on your GPU using *OpenCL*

Showcase Video:

Bencharking Data: [here](https://docs.google.com/spreadsheets/d/e/2PACX-1vRb-oeR40DNBxupItV7g4kWo4vkKOww2DULMerxg2vj0lUWFjKT4EWCNUbjnS-LTIyDPZfXg3vYYcDA/pubhtml)

All benchmarking was performed on a 4912 x 3264, 8 bit, RGBA, PNG image, with no transparent pixels.

## General
This program only works on PNG images that have an 8 bit colour depth, and RGBA colour type.
This program uses OpenCL 1.2 so it should work fine on Nvidia GPUs, although I haven't been able to test that because I only have an AMD GPU.
You must have an OpenCL platform installed to compile the program. You can test if you have a platform installed with `clinfo` on Linux.
You must also have `libpng` installed to compile, because thats what this program uses to read and write the PNG images.

I wrote this code using AMD's ROCM OpenCL implementation, on Ubuntu 20.04.
This means if you have a different OpenCL platform and/or operating system installed, you will probably need to change the `Makefile` a bit, so the compiler and linker know where to look for the OpenCL headers and binary files.
You will probably not need to change any of the actual source code, although I have only tested this on one system so I could be wrong.

If everything is working correctly you should notice that for any given input, running on the GPU is several (dozens to hundreds) times faster than running on the CPU 
(even under full multithreaded load). And this ratio should increase as the size of the input image and standard deviation increase.

## Usage
After cloning or downloading the source code, compile with `make`.
If your input image is `.../input.png` the program will output a blurred `.../input_gb.png` without modifying `.../input.png` at all.

`input.png` is the first argument to the program and should be a file path to the (8 bit, RGBA) PNG file you want blurred. 

`standard_deviation` argument specifies the standard deviation of the gaussian convolution kernel used for the blur.
The larger you make the standard deviation the longer the length of the convolution kernel will be, which will result in a stronger blur but take more time.
You should be able to see a significant blur on input images with width and height dimensions in the thousands with a standard deviation of less than 20.

`device` represents the device that you want to perform the blur. 'c' means it will be performed on your CPU and 'g' means it will be performed on your GPU.

`threads` is an optional argument only used when the device is 'c' which specifies how many cpu threads the program should use for the blur.
Number of threads defaults to 1 if no `threads` argument is passed. It is an error to pass a `threads` argument if device is 'g.'

```
Usage: ./blur input.png standard_deviation device [threads]
	input.png = png image to be blurred
	standard_deviation = 'pos_int'
	device = 'c' for running on cpu, device = 'g' for running on gpu
	if device = 'c', threads = number of threads (no threads specified means 1)
````

## Algorithm
Both the CPU and GPU version use essentially the same algorithm to perform the blur.

First, a 1D gaussian convolution kernel is created based on the input value of the standard deviation.
The length of the gaussian kernel is always *(6 * standard deviation + 1)*, and it is always normalized (using floats) before the blur begins.
The kernel can be 1D because the gaussian function is seperable, but this requires two blurring passes to perform the full blur.
This increases performance by a lot, because to blur any pixel the algorithm only needs to look at *2n* other pixels instead of *n\*n*
(where *n* is the length of the convolution kernel) to achieve the same exact blur.

When multithreading the CPU will break the image into the same number of horizontal bands, as user threads requested, and each thread performs the blur on its own band.
On the GPU side I tried a few optimizations, namely regarding breaking the work items into custom work groups to read the global image and gaussian kernel memory into faster local memory.
Sadly this actually proved slower than just letting OpenCL decide how to choose the work groups and have everything be read from global device memory.
I'm not sure why transfering data to local memory didn't prove a lot faster and I will definetly investigate this, and other optimizations (such as mapping host memory instead of reading/writing) further.

If the pixel to be blurred is near an edge so that the convolution kernel extends past the edge of the image,
both algorithms will still loop over the whole kernel.
The CPU algorithm will simply ignore any kernel values out of bounds of the image,
while the GPU version will pretend that the image pixel out of bounds has the same values as the closest pixel on the edge of the image.

## Structure
`main.c` : entry point for the program, processes input arguments and dispatches instructions to all other code

`process_png.c` : responsible for reading input PNG images, and writing output PNG images, uses `libpng`

`error.c` : outputs error messages and exits, can be called by any other code

`blur_cpu.c` : does the actual blur if requested to be done on CPU

`blur_gpu.c` : does the actual blur if requested to be done on GPU, is the host program for the kernels running on the gpu

`blur_helpers.c` : called by both `blur_cpu.c` and `blur_gpu.c` to create the convolution kernel based on the standard deviation value

`kernels.cl` : is the OpenCL kernel code that actually runs on the GPU

## Memory Leak
Although the program works correctly, there is quite a large memory leak when you run it on the gpu device.
You can test this for yourself by running the program with `Valgrind`.
In addition `Valgrind` shows errors when the `clReleaseProgram(program)` line in `blur_gpu.c` is present.
You can see my detailed question about it on Stackoverflow [here](https://stackoverflow.com/questions/63586015/opencl-memory-leaks-and-errors-even-after-releasing-everything-with-clrelease).
For the Stackoverflow question, I reproduced the error with a much simpler OpenCL program, but the main program here has the exact same problem.
This memory leak keeps me up at night so I would be eternally grateful to anybody who can figure out what's wrong and release me from this everlasting torment :)



