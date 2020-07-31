// Ivan Bystrov
// 30 July 2020
//
// Functions used by both blur_cpu and blur_gpu

#ifndef BLUR_HEADERS_SEEN
#define BLUR_HEADERS_SEEN

#define RADIUS 3


/**
 * Calculates the values for all the elements of the 1D gaussian convolution kernel (values are normalized)
 * @param [output] gaussian_kernel : the 1D kernel to fill the values with
 * @param std_dev : the standard deviation of the gaussian filter
 */
void calculate_kernel(double **gaussian_kernel, unsigned gaussian_kernel_len, unsigned std_dev);

/**
 * Prints out the gaussian kernel to be used in the program
 * @param gaussian_kernel : pointer to the kernel to be output
 * @param gaussian_kernel_len : the length of the kernel in pixels
 */
void print_kernel(double *gaussian_kernel, unsigned gaussian_kernel_len);

#endif /* BLUR_HELPERS_SEEN */
