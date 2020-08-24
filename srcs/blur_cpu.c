// Ivan Bystrov
// 26 July 2020
//
// All the code that actually performs the blur on the input image if using cpu


#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "blur_cpu.h"
#include "blur_helpers.h"
#include "error.h"


/** Struct storing all the information threads will need to perform blur
 * img_datap : pointer to the Img_Data struct that contains all the info
 * start_row : the first row of the input image the thread should operate on
 * end_row : the first row (greater than start_row) the thread should NOT operate on
 * gaussian_kernel : pointer to the gaussian kernel that will perform the blur
 * guassian_kernel_len : length of the gaussian_kernel in pixels
 * offset : the offset into the gaussian_kernel that the target pixel is at
 * pass : 1 = first pass of the blur, 2 = second pass of the blur
 */
struct Thread_Params {
	struct Img_Data *img_datap;
	unsigned start_row;
	unsigned last_row;
	float *gaussian_kernel;
	unsigned gaussian_kernel_len;
	unsigned offset;
	unsigned pass;
};

/**
 * Calculates what the new values for each componenet of the blurred pixel should be and stores those values in the new img (FIRST PASS out of 2) 
 * @param row_pointers : pointer to an array of rows of the input image
 * @param new_row_pointers : pointer to an array of rows of the output image
 * @param row : the row the target pixel is at
 * @param col : the column the target pixel is at
 * @param gaussian_kernel : the 1D convolution kernel that will apply the blur
 * @param gaussian_kernel_len : the length of the kernel
 * @param offset : the index of the target pixel in the gaussian kernel (always RADIUS * std_dev)
 * @param pixel_length : the length of a pixel in bytes
 * @param width : the width of the image in pixels
 * @param height : the height of the image in pixels
 * @param pass : 1 if its the first pass of the blur, 2 if its the second pass (illegal inputs not checked so make sure calling function gives correct pass value)
 */
void blur_pixel(struct Img_Data *img_datap, unsigned row, unsigned col, float *gaussian_kernel, unsigned gaussian_kernel_len, unsigned offset, unsigned pass) {
	// Set the input and output buffers of this blur depending on the pass
	unsigned char *input_arr = img_datap->arrays[pass - 1];
	unsigned char *output_arr = img_datap->arrays[pass];

	// Set the rest of the values in img_data used in this blur
	unsigned pxl_length = img_datap->pixel_length;
	unsigned width = img_datap->width;
	unsigned height = img_datap->height;
	
	// Initialize sum values used in the weighted average calculation of the target pixel
	float sum_r = 0;
	float sum_g = 0;
	float sum_b = 0;
	
	// Loop over the gaussian kernel and muliply with the correct pixel
	for (unsigned i = 0; i < gaussian_kernel_len; ++i) {
		// Get the pixel coordinates of the original image to operate on (pass 1 uses horizontal vector, pass 2 uses vertical vector)
		int cur_pxl_row;
		int cur_pxl_col;
		if (pass == 1) {
			cur_pxl_row = row;
			cur_pxl_col = col - offset + i;
		} else {
			cur_pxl_row = row - offset + i;
			cur_pxl_col = col;
		}

		// Make sure none of the values are out of bounds (otherwise don't include that pixel in the blur for this pixel)
		// The cast to (int) should not cause issues because libpng constrains input image to 1 million pixels
		if (!(cur_pxl_row < 0 || cur_pxl_row >= (int) height || cur_pxl_col < 0 || cur_pxl_col >= (int) width)) {
			// Get a pointer to the pixel being multiplied this iteration
			unsigned char *pxl = input_arr + (cur_pxl_row * width * pxl_length) + (cur_pxl_col * pxl_length);
			
			// Multiply each component of the input pixel with the corresponding element of the gaussian kernel
			sum_r += *(pxl + 0) * gaussian_kernel[i];
			sum_g += *(pxl + 1) * gaussian_kernel[i];
			sum_b += *(pxl + 2) * gaussian_kernel[i];
		}
	}

	// Round the average of each component of the target pixel and store it in the output image array
	unsigned target_pxl = (row * width * pxl_length) + (col * pxl_length);
	output_arr[target_pxl + 0] = (unsigned char) round(sum_r);
	output_arr[target_pxl + 1] = (unsigned char) round(sum_g);
	output_arr[target_pxl + 2] = (unsigned char) round(sum_b);
	output_arr[target_pxl + 3] = input_arr[target_pxl + 3];
}

/**
 * Entry point for the cpu threads to perform the blur
 * @param thread_params : Pointer to Thread_Params struct
 * @return : returns NULL
 */
void *multithreaded_blur(void *thread_params) {
	// Get all the values from thread_params
	struct Thread_Params *tp = (struct Thread_Params *) thread_params;
	struct Img_Data *img_datap = tp->img_datap;
	unsigned start_row = tp->start_row;
	unsigned last_row = tp->last_row;
	float *gaussian_kernel = tp->gaussian_kernel;
	unsigned gaussian_kernel_len = tp->gaussian_kernel_len;
	unsigned offset = tp->offset;
	unsigned pass = tp->pass;

	unsigned counter = 0;
	
	// Loop over every pixel this thread is allowed, and apply correct blur to it depending on the pass
	if (last_row > img_datap->height) { last_row = img_datap->height; }
	for (unsigned row = start_row; row < last_row; ++row) {
		for (unsigned col = 0; col < img_datap->width; ++col) {
			blur_pixel(img_datap, row, col, gaussian_kernel, gaussian_kernel_len, offset, pass);
			counter ++;
		}
	}

	// fprintf(stderr, "sr: %u, lr: %u, pxls: %u\n", start_row, last_row, counter);
	
	return NULL;
}

/**
 * Performs blur on the input image and stores it in new image space
 * @param img_datap : struct storing all the info of the input image
 * @param std_dev : desired standard deviation of the gaussian_blur
 * @param num_threads : number of threads to use for the blur
 */
void blur_cpu(struct Img_Data *img_datap, unsigned std_dev, unsigned num_threads) {
	// Create the 1D Gaussian convolution kernel and output it
	unsigned gaussian_kernel_len = std_dev * RADIUS * 2 + 1;
	float *gaussian_kernel = malloc(sizeof(float) * gaussian_kernel_len);
	calculate_kernel(&gaussian_kernel, gaussian_kernel_len, std_dev);
	print_kernel(gaussian_kernel, gaussian_kernel_len);

	// Start timing the duration of the blur
	printf("Blurring...\n");
	struct timespec start, finish;
	float duration;
	clock_gettime(CLOCK_MONOTONIC, &start);
		
	// Declare the desired number of threads (and their params) and the number of rows they operate on
	pthread_t threads[num_threads];
	struct Thread_Params tps[num_threads];
	unsigned num_rows_per_thread = ceil( (float) img_datap->height / num_threads);

	// Loop over both passes of the blur
	for (unsigned pass = 1; pass < 3; ++pass) {
		// Create all the threads for the current pass
		for (unsigned thread = 0; thread < num_threads; ++thread) {
			// Set all the values of the correct Thread_Params struct
			tps[thread].img_datap = img_datap;
			tps[thread].gaussian_kernel = gaussian_kernel;
			tps[thread].gaussian_kernel_len = gaussian_kernel_len;
			tps[thread].offset = RADIUS * std_dev;
			tps[thread].start_row = thread * num_rows_per_thread;
	       		tps[thread].last_row = (thread + 1) * num_rows_per_thread;
			tps[thread].pass = pass;

			// Create the thread
			pthread_create(&threads[thread], NULL, multithreaded_blur, &tps[thread]);
		}

		// Join up all the threads after their current pass
		for (unsigned thread = 0; thread < num_threads; ++thread) {
			pthread_join(threads[thread], NULL);
		}
	}

	// Output the duration of the blur
	clock_gettime(CLOCK_MONOTONIC, &finish);
	duration = (finish.tv_sec - start.tv_sec);
       	duration += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("Blur Duration: %f seconds\n\n", duration);

	// Free the gaussian kernel
	free(gaussian_kernel);
}
