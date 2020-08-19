// Ivan Bystrov
// 18 August 2020
//
// All the code that actually performs the gpu blur on the input image

#ifndef BLUR_GPU_SEEN
#define BLUR_GPU_SEEN

#include "process_png.h"


/**
 * Performs gpu blur (using OpenCL) on the input image and stores it in the new image space
 * @param img_datap : struct storing all the info of the input image
 * @param std_dev : desired standard deviation of the gaussian_blur
 * @param type : 1 for discrete blur, 2 for bilinear blur
 */
void blur_gpu(struct Img_Data *img_datap, unsigned std_dev, unsigned type); 

#endif /* BLUR_GPU_SEEN */
