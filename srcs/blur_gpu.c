// Ivan Bystrov
// 18 August 2020
//
// All the code that actually performs the blur on the input image if using gpu

// Define opencl version 1.2
#define CL_TARGET_OPENCL_VERSION 120

// Name of the .cl file to build program with
#define CL_FILE "srcs/blur_kernel.cl"

// Template for options string to be passed when building OpenCL program for OpenCL version 1.2
#define CL_OPTIONS "-cl-std=CL1.2 -D GAUSSIAN_KERNEL_LEN=%u -D OFFSET=%u -D IMG_WIDTH=%u -D IMG_HEIGHT=%u"

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
 */
void blur_gpu(struct Img_Data *img_datap, unsigned std_dev) {
	// Create the 1D Gaussian convolution kernel and output it
	cl_uint gaussian_kernel_len = std_dev * RADIUS * 2 + 1;
	cl_uint offset = std_dev * RADIUS;
	cl_float *gaussian_kernel = malloc(sizeof(float) * gaussian_kernel_len);
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
	
	// Determine size of kernel source file
	FILE *fp;
	if (!(fp = fopen(CL_FILE, "r"))) { error(NULL); }
	if (fseek(fp, 0, SEEK_END)) { error("could not seek in " CL_FILE "\n"); }
	long int src_size = ftell(fp);
	rewind(fp);
	
	// Read kernel source file into buffer
	char *buf = malloc(sizeof(char) * src_size + 1);
	if (fread(buf, sizeof(char), src_size, fp) != (long unsigned) src_size) { error("could not read " CL_FILE "\n"); }	
	buf[src_size] ='\0';
	if (fclose(fp)) { error("could not close " CL_FILE "\n"); }
		
	// Create the program
	cl_program program = clCreateProgramWithSource(context, 1, (const char **) &buf, 0, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL program\n"); }
	
	// Calculate the number of characters to represent each MACRO to be sent to the kernels
	unsigned size_gaussian_kernel_len = snprintf(NULL, 0, "%u", gaussian_kernel_len);
	unsigned size_offset = snprintf(NULL, 0, "%u", offset);
	unsigned size_img_height = snprintf(NULL, 0, "%u", img_datap->height);
	unsigned size_img_width = snprintf(NULL, 0, "%u", img_datap->width);

	// Create options string for building program
	char options[size_gaussian_kernel_len + size_offset + size_img_width + size_img_height + sizeof(CL_OPTIONS) - 4*2];
	snprintf(options, sizeof(options), CL_OPTIONS, gaussian_kernel_len, offset, img_datap->width, img_datap->height);
	
	// Build the program and output log if it failed
	err = clBuildProgram(program, 1, &device, options, NULL, NULL);
	char *program_log;
	if (err != 0) {
		size_t log_size;
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		program_log = malloc(sizeof(char) * (log_size + 1));
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
		printf("%s\n\n", program_log);
		error("could not build OpenCL program\n");
	}

	// Create the command queue to the gpu
	cl_command_queue command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL command queue on the gpu\n"); }

	// Create the kernel for the first pass of the blur
	const char first_pass_kernel_name[] = "first_pass_blur";
	cl_kernel first_pass_kernel = clCreateKernel(program, first_pass_kernel_name, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL kernel for the first pass of the blur\n"); }

	// Initialize image format struct
	cl_image_format *format = malloc(sizeof(cl_image_format));
       	format->image_channel_order = CL_RGBA;
	format->image_channel_data_type = CL_UNSIGNED_INT8;

	// Initialize image descriptor struct
	cl_image_desc *desc = malloc(sizeof(cl_image_desc));
	desc->image_type = CL_MEM_OBJECT_IMAGE2D;
	desc->image_width = img_datap->width;
	desc->image_height = img_datap->height;
	desc->image_depth = 0;
	desc->image_array_size = 0;
	desc->image_row_pitch = 0;
	desc->image_slice_pitch = 0;
	desc->num_mip_levels = 0;
	desc->num_samples = 0;

	// Create the input and output images
	cl_mem input_img = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (const cl_image_format *) format, (const cl_image_desc *) desc, img_datap->row_pointers, &err);
	if (err) { error("could not create input image for first pass of the blur\n"); }
	cl_mem output_img = clCreateImage(context, CL_MEM_WRITE_ONLY, (const cl_image_format *) format, (const cl_image_desc *) desc, NULL, &err);
	if (err) { error("could not create output image for first pass of the blur\n"); }

	// Create the gaussian kernel memory object
	cl_mem gaussian_kernel_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(gaussian_kernel), gaussian_kernel, &err);
	if (err) { error("could not create Gaussian kernel global memory object\n"); }

	// Set the kernel arguments
	clSetKernelArg(first_pass_kernel, 0, sizeof(input_img), &input_img);
	clSetKernelArg(first_pass_kernel, 1, sizeof(output_img), &output_img);
	clSetKernelArg(first_pass_kernel, 2, sizeof(gaussian_kernel_mem), &gaussian_kernel_mem);
	clSetKernelArg(first_pass_kernel, 3, sizeof(gaussian_kernel), NULL);

	// Release all OpenCL objects
	clReleaseContext(context);
	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseKernel(first_pass_kernel);

	// Free allocated memory
	free(gaussian_kernel);
	free(buf);
	free(program_log);
	free(format);

	exit(0);
}
