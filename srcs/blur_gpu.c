// Ivan Bystrov
// 18 August 2020
//
// All the code that actually performs the blur on the input image if using gpu


#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "blur_gpu.h"
#include "blur_helpers.h"


/**
 * Performs blur on the input image and stores it in the new image space
 * @param img_datap : struct storing all the info of the input image
 * @param std_dev : desired standard deviation of the gaussian_blur
 * @param blur_type : 1 for discrete blur, 2 for bilinear blur
 */
void blur_gpu(struct Img_Data *img_datap, unsigned std_dev, unsigned blur_type) {
	// Create the 1D Gaussian convolution kernel and output it
	(void) img_datap;
	(void) std_dev;
	(void) blur_type;
	exit(0);
}
