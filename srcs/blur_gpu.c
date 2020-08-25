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

// Number of work items in a work group
#define WORK_ITEMS_PER_GROUP 256

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
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
 * Prints out log from building the OpenCL program
 * @param programp : pointer to the program object that was built
 * @param device : device id of the device program was built for
 */
void print_error_build_log(cl_program *programp, cl_device_id device) {
	char *program_log;
	size_t log_size;
	clGetProgramBuildInfo(*programp, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	program_log = malloc(sizeof(char) * (log_size + 1));
	clGetProgramBuildInfo(*programp, device, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
	printf("%s\n\n", program_log);
	free(program_log);
	error("could not build OpenCL program\n");
}


/**
 * Initialize format and descriptor structs for use by OpenCL memory objects
 * @param formatp : pointer to the format struct to initialize
 * @param descp : pointer to the descriptor struct to initialize
 * @param img_datap : pointer to struct that stores all info about image
 */
void initialize_format_and_desc(cl_image_format *formatp, cl_image_desc *descp, struct Img_Data *img_datap) {
	// Initialize image format struct
       	formatp->image_channel_order = CL_RGBA;
	formatp->image_channel_data_type = CL_UNSIGNED_INT8;

	// Initialize image descriptor struct
	descp->image_type = CL_MEM_OBJECT_IMAGE2D;
	descp->image_width = img_datap->width;
	descp->image_height = img_datap->height;
	descp->image_depth = 0;
	descp->image_array_size = 0;
	descp->image_row_pitch = 0;
	descp->image_slice_pitch = 0;
	descp->num_mip_levels = 0;
	descp->num_samples = 0;
	descp->buffer = NULL;
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

	// Start timing the duration of the blur
	printf("Blurring...\n");
	struct timespec start, finish;
	float duration;
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	// Initialize platform id structure (for simplicity detect exactly 1 platform even if there are more)
	cl_int err;
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
	// if (print_platform_and_device_info(platform, device)) { error("could not get some OpenCL platform info\n"); }

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
	char *buf = malloc(sizeof(char) * (src_size + 1));
	if (fread(buf, sizeof(char), src_size, fp) != (long unsigned) src_size) { error("could not read " CL_FILE "\n"); }	
	buf[src_size] ='\0';
	if (fclose(fp)) { error("could not close " CL_FILE "\n"); }

	// Create the program
	cl_program program = clCreateProgramWithSource(context, 1, (const char **) &buf, NULL, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL program\n"); }
	
	// Calculate the number of characters to represent each MACRO to be sent to the kernels
	unsigned size = snprintf(NULL, 0, "%u", gaussian_kernel_len);
	size += snprintf(NULL, 0, "%u", offset);
	size += snprintf(NULL, 0, "%u", img_datap->height);
	size += snprintf(NULL, 0, "%u", img_datap->width);

	// Create options string for building program
	char options[size + sizeof(CL_OPTIONS) - 4 * 2];
	snprintf(options, sizeof(options), CL_OPTIONS, gaussian_kernel_len, offset, img_datap->width, img_datap->height);
	
	// Build the program
	err = clBuildProgram(program, 1, &device, (const char *) options, NULL, NULL);
	if (err) { print_error_build_log(&program, device); }
	
	// Create the command queue to the gpu
	cl_command_queue command_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL command queue on the gpu\n"); }

	// Create the kernel for the first pass of the blur
	const char first_pass_kernel_name[] = "first_pass_blur";
	cl_kernel first_pass_kernel = clCreateKernel(program, first_pass_kernel_name, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL kernel for the first pass of the blur\n"); }

	// Initialize image format and descriptor structs
	cl_image_format format;
	cl_image_desc desc;
	initialize_format_and_desc(&format, &desc, img_datap);
	
	// Create first pass input image / second pass output image
	cl_mem img1 = clCreateImage(context, CL_MEM_READ_WRITE, (const cl_image_format *) &format, (const cl_image_desc *) &desc, NULL, &err);
	if (err) { error("could not create input image buffer object for first pass of the blur\n"); }

	// Create first pass output image / second pass input image
	cl_mem img2 = clCreateImage(context, CL_MEM_READ_WRITE, (const cl_image_format *) &format, (const cl_image_desc *) &desc, NULL, &err);
	if (err) { error("could not create output image buffer object for first pass of the blur\n"); }

	// Create the gaussian kernel buffer memory object
	cl_mem gaussian_kernel_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, gaussian_kernel_len * sizeof(float), NULL, &err);
	if (err) { error("could not create gaussian kernel global memory object\n"); }

	// Write the input image into the input image object
	size_t origin[] = {0, 0, 0};
	size_t region[] = {img_datap->width, img_datap->height, 1};
	err = clEnqueueWriteImage(command_queue, img1, CL_TRUE, origin, region, 0, 0, img_datap->arrays[0], 0, NULL, NULL);
	if (err != CL_SUCCESS) { error("could not write input image for first pass from host to device\n"); }

	// Write the gaussian kernel into the gaussian kernel memory object
	err = clEnqueueWriteBuffer(command_queue, gaussian_kernel_mem, CL_TRUE, 0, gaussian_kernel_len * sizeof(float), gaussian_kernel, 0, NULL, NULL);
	if (err != CL_SUCCESS) { error("could not write gaussian kernel for first pass from host to device\n"); }

	// Set the kernel arguments for the first pass
	if (clSetKernelArg(first_pass_kernel, 0, sizeof(cl_mem), &img1) != CL_SUCCESS) { 
		error("could not set input image OpenCL kernel argument\n"); 
	} else if (clSetKernelArg(first_pass_kernel, 1, sizeof(cl_mem), &img2) != CL_SUCCESS) { 
		error("could not set output image OpenCL kernel argument\n");
	} else if (clSetKernelArg(first_pass_kernel, 2, sizeof(cl_mem), &gaussian_kernel_mem) != CL_SUCCESS) { 
		error("could not set gaussian filter OpenCL kernel argument\n");
	}
	/*
	} else if (clSetKernelArg(first_pass_kernel, 3, sizeof(gaussian_kernel), NULL) != CL_SUCCESS) {
		error("could not set device local gaussian filter OpenCL kernel argument\n");
	}
	*/

	/*
	// Enqueue the first pass kernel for execution
	size_t global_work_size_width = WORK_ITEMS_PER_GROUP * ((img_datap->width + WORK_ITEMS_PER_GROUP - 1) / WORK_ITEMS_PER_GROUP);
	size_t global_work_size[] = {global_work_size_width, img_datap->height};
	size_t local_work_size[] = {WORK_ITEMS_PER_GROUP, 1};
	clEnqueueNDRangeKernel(command_queue, first_pass_kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	*/
	size_t global_work_size[] = {img_datap->width, img_datap->height};
	clEnqueueNDRangeKernel(command_queue, first_pass_kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL); 

	// Create the kernel for the second pass of the blur
	const char second_pass_name[] = "second_pass_blur";
	cl_kernel second_pass_kernel = clCreateKernel(program, second_pass_name, &err);
	if (err != CL_SUCCESS) { error("could not create OpenCL kernel for the second pass of the blur\n"); }

	// Set the kernel arguments for the second pass
	if (clSetKernelArg(second_pass_kernel, 0, sizeof(cl_mem), &img2) != CL_SUCCESS) { 
		error("could not set input image OpenCL kernel argument\n"); 
	} else if (clSetKernelArg(second_pass_kernel, 1, sizeof(cl_mem), &img1) != CL_SUCCESS) { 
		error("could not set output image OpenCL kernel argument\n");
	} else if (clSetKernelArg(second_pass_kernel, 2, sizeof(cl_mem), &gaussian_kernel_mem) != CL_SUCCESS) { 
		error("could not set gaussian filter OpenCL kernel argument\n");
	}

	/*
	} else if (clSetKernelArg(second_pass_kernel, 3, sizeof(gaussian_kernel), NULL) != CL_SUCCESS) {
		error("could not set device local gaussian filter OpenCL kernel argument\n");
	}
	*/

	/*
	// Enqueue the second pass kernel for execution
	size_t global_work_size_height = WORK_ITEMS_PER_GROUP * ((img_datap->height + WORK_ITEMS_PER_GROUP - 1) / WORK_ITEMS_PER_GROUP);
	size_t global_work_size2[] = {img_datap->width, global_work_size_height};
	size_t local_work_size2[] = {1, WORK_ITEMS_PER_GROUP};
	clEnqueueNDRangeKernel(command_queue, second_pass_kernel, 2, NULL, global_work_size2, local_work_size2, 0, NULL, NULL);
	*/
	clEnqueueNDRangeKernel(command_queue, second_pass_kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);

	// Read the processed image back to host memory
	clEnqueueReadImage(command_queue, img1, CL_TRUE, origin, region, 0, 0, img_datap->arrays[2], 0, NULL, NULL);
	
	// Release all OpenCL objects (TODO fix memory leak)
	clReleaseMemObject(gaussian_kernel_mem);
	clReleaseMemObject(img1);
	clReleaseMemObject(img2);
	clReleaseKernel(first_pass_kernel);
	clReleaseKernel(second_pass_kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseProgram(program);
	clReleaseContext(context);

	// Output the duration of the blur
	clock_gettime(CLOCK_MONOTONIC, &finish);
	duration = (finish.tv_sec - start.tv_sec);
       	duration += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("Blur Duration: %f seconds\n\n", duration);
	
	// Free allocated memory
	free(gaussian_kernel);
	free(buf);
}
