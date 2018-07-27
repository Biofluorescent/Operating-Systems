/*
 * Author: Tanner Quesenberry
 * Date: 8/18/17
 * References:
 *	https://www.tutorialspoint.com/c_standard_library/c_function_atoi.htm
 *	https://stackoverflow.com/questions/523237/how-to-read-in-numbers-as-command-arguments
 *	https://stackoverflow.com/questions/822323/how-to-generate-a-random-number-in-c
 *	www.asciitable.com
 *	https://stackoverflow.com/questions/6660145/convert-ascii-number-to-ascii-character-in-c
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *args[ ]){

    // Possible random chars
    char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

   // Check for correct argument count
   if(argc != 2){
       fprintf(stderr, "Incorrect argument count.\n", 27);
   }else{

        // Get user specified key length from command line argument
        // Use a long for increased user input size
        long keylength = atoi(args[1]);

        // Seed rand generator
        srand(time(NULL));
    
        int i;
        int r;

        // Generate random key
        for(i = 0; i < keylength; i++){
            r = rand() % 27;
            printf("%c", chars[r]);
        }

        // Print necessary newline per assignment
        printf("\n");

    }

    return 0;
}
