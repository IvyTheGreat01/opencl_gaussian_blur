// Ivan Bystrov
// 18 August 2020
//
// All the code that actually performs the blur on the input image if using gpu

// Define opencl version 1.2
#define CL_TARGET_OPENCL_VERSION 120

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <CL/cl.h>
#include "blur_gpu.h"
#include "blur_helpers.h"
#include "error.h"

/**
 * Prints the platform name, version and device name
 * @param platform : the platform id who's info this function prints
 * @param device : the device id who's info this function prints
 * @return 0 on success, 1 on error
 */
unsigned print_platform_and_device_info(cl_platform_id platform, cl_device_id device) {
	// Get the platform and device info
	char platform_name[64];
	char platform_version[64];
	char device_name[64];
	char device_vendor[64];
	cl_int pname_err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
	cl_int pversion_err = clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(platform_version), platform_version, NULL);
	cl_int dname_err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
	cl_int dvendor_err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
	
	// Print platform and device info and return
	if (!pname_err && !pversion_err && !dname_err && !dvendor_err) {
		printf("OpenCL Platform Name: %s\n", platform_name);
		printf("OpenCL Platform Version: %s\n", platform_version);
		printf("OpenCL Device Name: %s\n", device_name);
		printf("OpenCL Device Vendor: %s\n\n", device_vendor);
		return 0;
	}
	return 1;
}

/**
 * Performs blur on the input image and stores it in the new image space
 * @param img_datap : struct storing all the info of the input image
 * @param std_dev : desired standard deviation of the gaussian_blur
 * @param blur_type : 1 for discrete blur, 2 for bilinear blur
 */
void blur_gpu(struct Img_Data *img_datap, unsigned std_dev, unsigned blur_type) {
	// Create the 1D Gaussian convolution kernel and output it
	cl_ushort gaussian_kernel_len = std_dev * RADIUS * 2 + 1;
	cl_double *gaussian_kernel = malloc(sizeof(double) * gaussian_kernel_len);
	calculate_kernel(&gaussian_kernel, gaussian_kernel_len, std_dev);
	print_kernel(gaussian_kernel, gaussian_kernel_len);
	
	// Error variable for cl_<function> returns
	cl_int err;

	// Initialize platform id structure (for simplicity detect exactly 1 platform even if there are more)
	cl_platform_id platform;
	cl_uint num_platforms;
	err = clGetPlatformIDs(1, &platform, &num_platforms);
	if (err || num_platforms != 1) { error("did not detect exactly 1 OpenCL platform\n"); }

	// Initialize device id structure for the gpu (for simplicity detect exactly 1 gpu even if there are more)
	cl_device_id device;
	cl_uint num_devices;
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &num_devices);
	if (err || num_devices != 1) { error("did not detect exactly 1 GPU for this OpenCL platform\n"); }
	
	// Print the platform name, version, and device name, vendor
	if (print_platform_and_device_info(platform, device)) { error("could not get some OpenCL platform info\n"); }

	// Initialize a context
	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if (err) { error("could not create OpenCL context\n"); }
	
	cl_uint float_width;
	clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(float_width), &float_width, NULL);
	printf("Float Width Preferred: %d\n", float_width);


	// Create the program
		// Read in .cl file(s)
		// cl_program program = clCreateProgramWithSource(context, ...);
		// clBuildProgram(program, ...);
	

	// Release the context before exiting function
	clReleaseContext(context);

	(void) blur_type;
	(void) img_datap;


	exit(0);
}
