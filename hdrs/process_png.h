// Ivan Bystrov
// 12 July 2020
//
// Reads and writes on .png files

#ifndef PROCESS_PNG_SEEN
#define PROCESS_PNG_SEEN

#include <png.h>


/**
 * Struct that stores the information of this image
 * png_ptr : pointer to libpng struct of the input image
 * info_ptr : pointer to libpng info struct of the input image
 * row_pointers : array of pointers to each row of the input image
 * width : width of the input image in pixels
 * height : height of the input image in pixels
 * colour_type : colour type of the input image in png notation (must be 6 (RGBA) for this program to work)
 * bit_depth : bit depth of the input image (must be 8 for this program to work)
 * pixel_length : length of each pixel in bytes (must be 4 for this program to work)
 * row_ptrs_p1 : array of pointers to each row of the output image of the first pass of the blur
 * row_ptrs_p2 : array of pointers to each row of the output image of the second pass of the blur (program output img) 
 */
struct Img_Data {
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	unsigned width;
	unsigned height;
	unsigned colour_type;
	unsigned bit_depth;
	unsigned pixel_length;
	png_bytep *row_ptrs_p1;
	png_bytep *row_ptrs_p2;
};

/**
 * Free img_data struct (should only be called after the two buffers have been created)
 * @param img_datap : pointer to the img_data struct to be freed
 */
void free_img_data_struct(struct Img_Data *img_datap); 

/**
 * Reads a png image and stores image data at img_p
 * @param [output] img_datap : pointer to struct storing input image data needed for program
 * @param input_filepath : filepath to the input image the program will be blurring
 */
void read_png(struct Img_Data *img_datap, char *input_filepath);

/**
 * Writes the blurred png image to another file (<input_filepath>_OUTPUT_MODIFIER.png
 * @param img_datap : pointer to struct storing input and output image data
 * @param filename : filepath of the input image the program blurred
 */
void write_png(struct Img_Data *img_datap, char *filename); 

/**
 * Prints the image information of the input image
 * @param img_datap : pointer to struct storing the input image data needed for program
 */
void print_image_information(struct Img_Data *img_datap); 

#endif /* !PROCESS_PNG_SEEN */
