// Ivan Bystrov
// 22 July 2020
//
// Reads and writes on .png files


#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "process_png.h"
#include "error.h"


/**
 * Check if the input file is a valid png file
 * @param fp : file pointer (SEEKS to start of file at start and end of function)
 * @return true if file is a valid png, false otherwise
 */
bool is_valid_png(FILE *fp) {
	png_byte header[8];
	
	fseek(fp, 0, SEEK_SET);

	// Read first 8 bytes of the image file
	if (fread(header, 1, 8, fp) != 8) { error(NULL); }
	
	fseek(fp, 0, SEEK_SET);	

	// Return true if the first 8 bytes of the file validate that its a png
	return !png_sig_cmp(header, 0, 7);
}

/**
 * Initialize png structs used for reading
 * @param png_ptr_p : pointer to a png_struct pointer
 * @param info_ptr_p : pointer to a png_info pointer
 */
bool init_read_structs(png_structp *png_ptr_p, png_infop *info_ptr_p) {
	// Allocate and initialize png_struct: png_ptr
	if (!(*png_ptr_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		return 1;
	}

	// Allocate and initilaize png_info: info_ptr
	if (!(*info_ptr_p = png_create_info_struct(*png_ptr_p))) {
		png_destroy_read_struct(png_ptr_p, (png_infopp) NULL, (png_infopp) NULL);
		return 1;
	}

	return 0;
}

/**
 * Copy the image data between img_datap->row_pointers and img_datap->arrays[arr_val]
 * @param img_datap : pointer to img_data struct that stores all image information
 * @param arr_val : index into desired array in img_datap->arrays[arr_val]
 * @param io_to_comp : 1 means copy from row_pointers to arrays[arr_val], 0 means otherwise
 */
void copy_row_pointers_and_arr(struct Img_Data *img_datap, unsigned arr_val, unsigned io_to_comp) {
	// Get the arrays involved in the copy
	png_bytep *row_ptrs = img_datap->row_pointers;
	unsigned char *arr = img_datap->arrays[arr_val];

	// Get the image info
	unsigned width = img_datap->width;
	unsigned height = img_datap->height;
	unsigned pxl_len = img_datap->pixel_length;

	// Loop over each byte of each pixel of row_pointers and copy it to the correct index of arr1
	for (unsigned row = 0; row < height; ++row) {
		for (unsigned col = 0; col < width; ++col) {
			// Get pointers to the pxls involved in the copy between the i/o array and the computation array
			unsigned char *io_pxl = row_ptrs[row] + col * pxl_len;
			unsigned char *comp_pxl = arr + (row * width * pxl_len) + (col * pxl_len);

			// Copy the current pixel between the two arrays in the desired direction
			if (io_to_comp) {
				*(comp_pxl + 0) = *(io_pxl + 0);
				*(comp_pxl + 1) = *(io_pxl + 1);
				*(comp_pxl + 2) = *(io_pxl + 2);
				*(comp_pxl + 3) = *(io_pxl + 3);
			
			} else {
				*(io_pxl + 0) = *(comp_pxl + 0);
				*(io_pxl + 1) = *(comp_pxl + 1);
				*(io_pxl + 2) = *(comp_pxl + 2);
				*(io_pxl + 3) = *(comp_pxl + 3);
			}
		}
	}
}

/**
 * Reads a png image and stores image data at img_p
 * @param [output] img_datap : pointer to struct storing input image data needed for program
 * @param filename : filepath to the input image the program will be blurring
 */
void read_png(struct Img_Data *img_datap, char *filename) {
	// Open input image file
	FILE *fp;
	if (!(fp = fopen(filename, "rb"))) { error(NULL); }
	
	// Check that image file is a valid png
	if (!is_valid_png(fp)) {
		fclose(fp);
		error("input image is not a valid PNG file\n"); 
	}
	
	// Allocate and initialize all the structs used for reading
	png_structp png_ptr;
	png_infop info_ptr;
	if(init_read_structs(&png_ptr, &info_ptr)) { 
		fclose(fp);
		error("failed to initialize structs for reading input PNG\n"); 
	}
	
	// libpng jumps here when it encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		fclose(fp);
		error("libpng failed to process input image\n");
	}

	// Read the png into memory
	png_init_io(png_ptr, fp);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	fclose(fp);
	
	// Save the values in img_data for use by the rest of the program
	img_datap->png_ptr = png_ptr;
	img_datap->info_ptr = info_ptr;
	img_datap->row_pointers = png_get_rows(png_ptr, info_ptr);
	img_datap->width = png_get_image_width(png_ptr, info_ptr);
	img_datap->height = png_get_image_height(png_ptr, info_ptr);
	img_datap->bit_depth = png_get_bit_depth(png_ptr, info_ptr); 
	img_datap->colour_type = png_get_color_type(png_ptr, info_ptr);
	img_datap->pixel_length = 4;

	// Output core image information	
	printf("Image Width: %u, Image Height: %u, Bit Depth: %u, Colour Type: %u\n\n", 
			img_datap->width, img_datap->height, img_datap->bit_depth, img_datap->colour_type);

	// Make sure core image information is acceptable for the program
	if (img_datap->bit_depth != 8 || img_datap->colour_type != 6) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		error("input image colour type is not RGBA with bit depth 8\n");
	}
}

/**
 * Free img_data struct (should only be called after the three buffers have been created)
 * @param img_datap : pointer to the img_data struct to be freed
 */
void free_img_data_struct(struct Img_Data *img_datap) {
	// Free the two read structs from read_png first (this also frees img_data->row_pointers)
	png_destroy_read_struct(&(img_datap->png_ptr), &(img_datap->info_ptr), (png_infopp) NULL);

	// Free all the arrays
	for (unsigned i = 0; i < 2; ++i) {
		free(img_datap->arrays[i]);
	}
	free(img_datap->arrays);
}

/**
 * Writes the blurred png image to another file (<input_filepath>_OUTPUT_MODIFIER.png
 * @param img_datap : pointer to struct storing input and output image data
 * @param filename : filepath of the input image the program blurred
 */
void write_png(struct Img_Data *img_datap, char *filename) {
	// Open output image file
	FILE *fp;
	if(!(fp = fopen(filename, "wb"))) { error(NULL); }

	// Allocate and initialize the png struct used for reading
	png_structp write_png_ptr;
	if(!(write_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		free_img_data_struct(img_datap);
		fclose(fp);
		error("failed to initialize struct for writing output PNG\n");
	}
	
	// libpng jumps here when it encounters an error
	if (setjmp(png_jmpbuf(write_png_ptr))) {
		png_destroy_write_struct(&write_png_ptr, (png_infopp) NULL);
		free_img_data_struct(img_datap);
		fclose(fp);
		error("libpng failed to process output image\n");
	}

	// Write the PNG
	png_init_io(write_png_ptr, fp);
	png_set_rows(write_png_ptr, img_datap->info_ptr, img_datap->row_pointers);
	png_write_png(write_png_ptr, img_datap->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	fclose(fp);

	// Free the write_png_ptr struct
	png_destroy_write_struct(&write_png_ptr, (png_infopp) NULL);
}
