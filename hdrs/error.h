// Ivan Bystrov
// 26 July 2020
//
// Can be used by any src code to output error message and exit program

#ifndef ERROR_SEEN
#define ERROR_SEEN

/**
 * Outputs error message and exits program
 * @param error_msg : the error message to be output, if NULL use perror
 */
void error(char *error_msg); 

#endif /* ERROR_SEEN */
