// Ivan Bystrov
// 21 July 2020
//
// Main file for the gaussian blur program


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "hdrs/process_png.h"
#include "hdrs/blur_png.h"
#include "hdrs/error.h"

#define OUTPUT_MODIFIER "_blrd"
#define OUTPUT_MODIFIER_LEN 5


/**
 * Command line input parameters to the program
 * filename : filename of the input image
 * std_dev : standard deviation of the gaussian blur (must be pos int)
 * device : device to run this program on (must be 'c' for cpu or 'g' for gpu)
 * config : device settings for the device (if device 'c' config = num_threads, if device 'g' config = 1 for discrete sampling or config = 2 for bilinear sampling)
 */
struct Input_Pars {
	char *filename;
	unsigned std_dev;
	char device;
	unsigned config;
}; 


/**
 * Outputs usage message for the program
 * @param program_name : name of this program
 */
void usage_msg(char *program_name) {
	fprintf(stderr, "Usage: %s input.png standard_deviation device config\n", program_name);
	fprintf(stderr, "	input.png = png image to be blurred\n");
	fprintf(stderr, "	standard_deviation = 'pos_int'\n");
	fprintf(stderr, "	device = 'c' for running on cpu, device = 'g' for running on gpu\n");
	fprintf(stderr, "	if device = 'c', config = 'num_threads'\n");
	fprintf(stderr, "	if device = 'g', config = '1' for discrete sampling, config = '2' for bilinear sampling\n\n");
}

/**
 * Outputs the way the program is configured to run, from the input arguments
 * @param input_parameters : struct for input parameters from command line
 * Should be called after parse_input_args() so that input_parameters members are all valid
 */
void print_input_args(struct Input_Pars *input_parameters) {
	fprintf(stdout, "Input Image: %s\n", input_parameters->filename);
	fprintf(stdout, "Standard Deviation: %u\n", input_parameters->std_dev);
	
	if (input_parameters->device == 'c') {
		fprintf(stdout, "Device: cpu\n");
		fprintf(stdout, "Num Threads: %u\n", input_parameters->config);

	} else {
		fprintf(stdout, "Device: gpu\n");

		if (input_parameters->config == 1) {
			fprintf(stdout, "Sampling: discrete\n");
	
		} else {
			fprintf(stdout, "Sampling: bilinear\n");
		}
	}

	fprintf(stdout, "\n");
}

/**
 * Check if input string is a positive integer
 * @param input : the input string to check
 * @return true if input is a positive integer, false otherwise
 */
bool is_pos_int(char *input) {
	unsigned zero_counter = 0;
	for (unsigned i = 0; i < strlen(input); ++i) {
		if (input[i] == '0') { zero_counter ++; }

		if (!isdigit(input[i])) { return false; }
	}
	
	return !(zero_counter == strlen(input));
}

/** 
 * Parse command line arguments to the program
 * @param [output] input_parameters : struct for input paramters from command line, only set if all valid
 * @param argc : num command line arguments
 * @param argv : command line arguments
 */
void parse_input_args(struct Input_Pars *input_parameters, int argc, char **argv) {
	// Print usage message if there are not exactly 4 command line arguments or if -help was input
	if (argc != 5 || !strcmp(argv[1], "-help")) {
		usage_msg(argv[0]);
		exit(1);
	}

	// Print usage message if filename doesn't end in .png
	char *extension = argv[1] + strlen(argv[1]) - 4;
	if (strcmp(extension, ".png")) {
		usage_msg(argv[0]);
		exit(1);
	}
	
	// Print usage message if standard deviation is not a positive integer
	if (!is_pos_int(argv[2])) {
		usage_msg(argv[0]);
		exit(1);
	}
	
	// Print usage message if device isn't either 'c' or 'g'
	if (strlen(argv[3]) != 1 || (argv[3][0] != 'c' && argv[3][0] != 'g')) {
		usage_msg(argv[0]);
		exit(1);
	}

	// Print usage message if config is not a positive integer
	if (!is_pos_int(argv[4])) {
		usage_msg(argv[0]);
		exit(1);
	}
		
	// Print usage message if device is 'g' and config is not (1 or 2)
	if (argv[3][0] == 'g' && argv[4][0] != '1' && argv[4][0] != '2') {
		usage_msg(argv[0]);
		exit(1);
	}

	// Set the input parameters now that we have confirmed they are valid
	input_parameters->filename = argv[1];
	input_parameters->std_dev = strtol(argv[2], NULL, 10);
	input_parameters->device = argv[3][0];
	input_parameters->config = strtol(argv[4], NULL, 10);
}

/**
 * Create the two arrays output array used in the blur filter
 * @param img_data : struct stores input image information, new_row_pointers value updated at return
 * @return 0 on success, 1 on failure
 */
int create_new_img_arrays(struct Img_Data *img_datap) {
	// Create an array of pointers to each row of the new image (first pass and second pass)
	png_bytep *row_ptrs_p1 = malloc(sizeof(png_bytep) * img_datap->height);
	png_bytep *row_ptrs_p2 = malloc(sizeof(png_bytep) * img_datap->height);

	// Loop over every row of the new images and allocate enough space for all pixels on that row
	unsigned num_actual_bytes_per_row = sizeof(png_byte) * img_datap->pixel_length * img_datap->width;
	for (unsigned row = 0; row < img_datap->height; ++row) {
		if (!(row_ptrs_p1[row] = calloc(num_actual_bytes_per_row, 1))) { return 1; }
		if (!(row_ptrs_p2[row] = calloc(num_actual_bytes_per_row, 1))) { return 1; }
	}
	
	// Save pointers to the new arrays in img_data
	img_datap->row_ptrs_p1 = row_ptrs_p1;
	img_datap->row_ptrs_p2 = row_ptrs_p2;

	return 0;
}

/**
 * Constructs the output filename of the blurred image
 * @param input_filename : the filename of the input image
 * @param [output] output_filename : pointer to where the filename of the output image is stored
 */
void get_output_filename(char *input_filename, char *output_filename) {
	output_filename[0] = '\0';
	strncat(output_filename, input_filename, strlen(input_filename) - 4);
	output_filename[strlen(input_filename) - 3] = '\0';
	strncat(output_filename, OUTPUT_MODIFIER, OUTPUT_MODIFIER_LEN + 1);
	strncat(output_filename, input_filename + strlen(input_filename) - 4, 5);
}

/**
 * Starting point of the gaussian blur program
 * @param argc : num command line arguments
 * @param argv : command line arguments
 */
int main(int argc, char **argv) {
	// Parse and store command line arguments in input_parameters struct
	struct Input_Pars input_parameters;
	parse_input_args(&input_parameters, argc, argv);
	print_input_args(&input_parameters);

	// Read and store png file in img_data and output some core information
	struct Img_Data img_data;
	read_png(&img_data, input_parameters.filename);
	printf("Image Width: %u, Image Height: %u, Bit Depth: %u, Colour Type: %u\n\n", img_data.width, img_data.height, img_data.bit_depth, img_data.colour_type);

	// Allocate space to store new modified image
	if (create_new_img_arrays(&img_data)) { error("could not allocate enough space in memory for output image\n"); }
	
	// Call correct blur function depending on device
	if (input_parameters.device == 'c') {
		blur_cpu(&img_data, input_parameters.std_dev, input_parameters.config);
	}

	// Write the blurred image to the output file
	char output_filename[strlen(input_parameters.filename) + OUTPUT_MODIFIER_LEN + 1];
	get_output_filename(input_parameters.filename, output_filename);
	write_png(&img_data, output_filename);

	// Free the img_data struct and the filename array
	free_img_data_struct(&img_data);

	// Output the output image filename
	printf("Output Image: %s\n", output_filename);

	return 0;
}
