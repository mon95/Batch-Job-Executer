#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

char** parseLine(char *line) {
    int len = strlen(line);
    int argsize = tok_size;
    char** args = (char**)malloc(argsize * sizeof(char*));
    int pos = 0;

    char* arg;

    if(!args) {
        // Not enough space available. Return failure and exit.
        printf("Memory allocation unsuccessful! Exiting...\n");
        exit(EXIT_FAILURE);     
    }


    // Key parsing logic:
    // Get the first argument. 
    // While the argument is valid (i.e., not null), add it to the list of arguments that will be returned to the main function.
    // Take care of running out of space using realloc.
    arg = strtok(line, tok_delimiters);
    
    while(arg != NULL) {

        if(strcmp(arg, "#") == 0) break;    // Don't parse after the beginning of a commment is detected
        
        args[pos] = arg;
        pos = pos + 1;

        if(pos > argsize) {
            // reallocate memory for args
            argsize = argsize + tok_size;
            args = realloc(args, argsize*sizeof(char*));

            if(!args) {
               // Not enough space available. Return failure and exit.
                printf("Memory allocation unsuccessful! Exiting...\n");
                exit(EXIT_FAILURE);     
            }

        }

        arg = strtok(NULL, tok_delimiters);
    }
    
    args[pos] = NULL;

    return args;
}
