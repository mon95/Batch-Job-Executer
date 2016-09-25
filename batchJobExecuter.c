/*
Name    :   Sreehari S
ID      :   2013A7P126G
*/

/*
    This is the driver function that takes a batch file as input from the command line 
    and executes the commands line-by-line (as specified).

    Assumptions:

    1. We assume that the single line comments are marked as '# ' (i.e. # followed by a space. Also, multi-line comments are ignored)
    2. #INTERSTART and #INTERSTOP are not being handled as of now
    3. We assume that built-in shell are not part of the input file. (i.e., these are not handled)
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


// for open() and dup2()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "parse.h"
#include "execute.h"

int main(int argc, char **argv) {
    
    FILE *fp;
    char* line = NULL;
    size_t linesize;

    char **args;

    int i,j,k;

    int beginflag = 0;  
    int endflag = 0;

    int commentbegin = 0;
    int commnentend = 0;


    if(argc != 2) {
        printf("Usage: ./executeBatchJobs <file-to-be-executed>\n");
        return 0;
    }

    printf("Batch file being executed: %s\n\n", argv[1]);

    // Readying the OUTPUT.txt using O_TRUNC
    int fd = open("OUTPUT.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP |S_IWUSR); // user - r and w permissions. 
    close(fd);

    fp = fopen(argv[1], "r");

    if (fp == NULL)
        exit(EXIT_FAILURE);


    while(getline(&line, &linesize, fp) != -1) {       

        
        if(strcmp(line, "%BEGIN\n") == 0) {
            if(beginflag == 0) {
                beginflag = 1;  // seen begin. Now, we can start processing from the next line.
                // printf("\nBeginning batch file execution...\n\n");
                continue;
            }

            else 
                continue; // ignore all other begins
        }

        if(beginflag == 0 && strcmp(line, "%END\n") == 0) continue; // %END before seeing a %BEGIN is not valid

        if(beginflag == 1 && endflag == 0) {
            if(strcmp(line, "%END") == 0 || strcmp(line, "%END\n") == 0) {
                endflag = 1;   
                // return 0;
                
                // reset both flags
                beginflag = 0;
                endflag = 0;
            }
        }

        if(beginflag == 1) {
            args = parseLine(line);
            
            
            // printf("Calling 'execute' with the following as args: ");
            // for(j = 0; args[j]!=NULL; j++) printf("%s ", args[j]); 
            // printf("\n");

            execute(args);

            printf("\n\n");

            free(args);
        }

    }



    fclose(fp);
    
    if(beginflag == 1 && endflag == 0)
        printf("\n\nUnable to find matching %%END statement!\n\n");
    
    return 0;
}   
