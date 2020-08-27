// Ivan Bystrov
// 30 July 2020
//
// Functions used by both blur_cpu and blur_gpu

#include <stdio.h>
#include <math.h>
#include "blur_helpers.h"

#define RADIUS 3


/**
 * Calculates the values for all the elements of the 1D gaussian convolution kernel (values are normalized)
 * @param [output] gaussian_kernel : the 1D kernel to fill the values with
 * @param gaussian_kernel_len : length of the gaussian kernel in pixels
 * @param std_dev : the standard deviation of the gaussian filter
 */
void calculate_kernel(float **gaussian_kernel, unsigned gaussian_kernel_len, unsigned std_dev) {
	// Calculate some constants used in gaussian blur calculation
	unsigned offset = RADIUS * std_dev;
	float exponent_denominator = 2 * std_dev * std_dev;
	
	// Process gaussian value of x = 0 so it doesn't get processed twice in the loop
	(*gaussian_kernel)[offset] = 1;
	float sum = 1;
	
	// Calculate value of gaussian value of x component of corresponding kernel index
	// loop over half the kernel (not including x = 0) because its symmetrical	
	unsigned half_kernel = gaussian_kernel_len / 2;
	for (unsigned i = 0; i < half_kernel; ++i) {
		int gaussian_x = i - offset; // Actual x coordinate in gaussian function of kernel index 
		float exponent = -((gaussian_x * gaussian_x) / exponent_denominator); // -(x^2 / 2 * sigma^2)
		float gauss_val = exp(exponent); // e^(exponent) = value of gaussian function at the x coordinate

		(*gaussian_kernel)[i] = gauss_val; // Store the value of gaussian function in the correct index left of 0 
		(*gaussian_kernel)[gaussian_kernel_len - i - 1] = gauss_val; // Store the value of gaussian function in index right of 0
		
		sum += gauss_val * 2;
	}
	
	// Normalize kernel by dividing each componenent by the calculated sum
	(*gaussian_kernel)[offset] = (*gaussian_kernel)[offset] / sum;
	for (unsigned i = 0; i < half_kernel; ++i) {
		(*gaussian_kernel)[i] = (*gaussian_kernel)[i] / sum;
		(*gaussian_kernel)[gaussian_kernel_len - i - 1] = (*gaussian_kernel)[gaussian_kernel_len - i - 1] / sum;
	}
}

/**
 * Prints out the gaussian kernel to be used in the program
 * @param gaussian_kernel : pointer to the kernel to be output
 * @param gaussian_kernel_len : the length of the kernel in pixels
 */
void print_kernel(float *gaussian_kernel, unsigned gaussian_kernel_len) {
	printf("Normalized Gaussian Blur Kernel (1 Dimensional): \n[ ");
	
	// Print out each element of the kernel
	float sum = 0;
	for (unsigned i = 0; i < gaussian_kernel_len; ++i) {
		float val = gaussian_kernel[i];
		printf("%lf ", val);
		sum += val;
	}
	
	printf("]\nLength: %u, Sum: %f\n\n", gaussian_kernel_len, sum);
}
