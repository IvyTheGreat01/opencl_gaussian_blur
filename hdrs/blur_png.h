// Ivan Bystrov
// 26 July 2020
//
// All the code that actually performs the blur on the input image

#ifndef BLUR_PNG_SEEN
#define BLUR_PNG_SEEN

#include "process_png.h"


/**
 * Performs blur on the input image and stores it in new image space
 * @param img_data : struct storing all the info of the input image
 * @param std_dev : desired standard deviation of the gaussian_blur
 * @param config : number of threads to use for the blur
 */
void blur_cpu(struct Img_Data *img_data, unsigned std_dev, unsigned num_threads);

#endif /* BLUR_PNG_SEEN */
