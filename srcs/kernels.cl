// Ivan Bystrov
// 20 August 2020
//
// OpenCL kernels compiled and used by blur_gpu.c


__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

/*
/ Kernel blurs all pixels in the first blurring pass
/ @param in_img : the original input image
/ @param out_img : the output image after the first blurring pass
/ @param gaussian_kernel : pointer to the gaussian kernel that will perform the blur
/ @param gaussian_kernel_len : pointer to a variable storing the length of a gaussian kernel
*/
__kernel void first_pass(read_only image2d_t in_img, write_only image2d_t out_img, __global float *gaussian_kernel, __global uint *gaussian_kernel_len) {
	// Coordinates of the pixel this work item is blurring
	int2 coord = (int2) (get_global_id(0), get_global_id(1));
	
	// Store the gaussian kernel data in private memory
	float *gaussian_kernel;
	uint gk_len = *gaussian_kernel_len;
	
	// Initialize the sum values stores
	float sum_r = 0;
	float sum_g = 0;
	float sum_b = 0;

	// Loop over the gaussian kernel
	for (uint i = 0; i < gaussian_kernel 
}
