// Ivan Bystrov
// 26 July 2020
//
// Can be used by any src code to output error message and exit program

#include <stdlib.h>
#include <stdio.h>


/**
 * Outputs error message and exits program
 * @param error_msg : the error message to be output, if NULL use perror
 */
void error(char *error_msg) {
	// Uses perror for output if no message supplied
	if (!error_msg) {
		perror("Error");
	
	} else {
		fprintf(stderr, "Error: %s\n", error_msg);
	}

	exit(1);
}

