#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


// for open(), dup() and dup2()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define _GNU_SOURCE 

// for tee and splice error checking 
#include <limits.h>
#include <errno.h>

#include "parse.h"
#include "execute.h"

// returns the length of the array of arguments
int argsLength(char **args) {
  int i = 0;
  while(args[i] != NULL) {
    i++;
  }
  return i;
}

// returns the number of pipes in one parsed line of arguments
int numberOfPipes(char **args) {
  int n = 0; // number of pipes
  int len = argsLength(args);
  int i;
  for(i=0; i<len; i++) {
    if(strcmp(args[i],"|") == 0) n = n+1;
  }

  return n;
}

/*
    Implementation of the main logic to perform the necessary tasks.

    Input is an array of arguments (obtained after parsing a line from the batch file). 
    (Assume that built-in commands like cd, exit are handled externally)

    This function handles "|", ">" and ">>" operators. 

    By default, all final output is redirected to OUTPUT.txt.

    IMPORTANT NOTE: For correct parsing, all input in the batch file is assumed to have correct (non-built in functions only) input, separated by spaces.

    HOW IT WORKS:

    1. Get the length of the arguments array (i.e., no of arguments)
    2. Iterate and get the positions(if present) of the '>', '>>' and the '|' (Irrelevant note: this implementation returns the position of the last '|' found)
    3. Handle on a case by case basis:
      i)    No |, > or >> operator. Redirect output to OUTPUT.txt
      ii)   '>' operator found or '>>' found. Redirect to appropriate file following the operator
      iii)  Case: '|' operator is present. 
  
    Case i)    No |, > or >> operator. Redirect output to OUTPUT.txt:
  
       To redirect the output of exec command to the OUTPUT.txt file,
       dup2() is used. This duplicates the file descriptor.

    Case ii)   '>' operator found or '>>' found. Redirect to appropriate file following the operator:

        Make the argument at position_of('>') = NULL for correct execution.
        Then, use dup2() for redirection.

    Case iii)  Case: '|' operator is present:

        Parse the arguments further and put them into an array of commands.
        Call executePipeCommands function that handles multiple pipes.

*/

int execute(char** args) {

    pid_t pid, wpid;

    int status; 

    int len = 0;    // length of the argument list (including |, > and >> operators)

    int i = 0;      
    int io_counter = 0;

    int op1 = 0;      // to mark pos of ">"
    int op2 = 0;     // to mark pos of ">>"
    int opPipe = 0;        // to mark pos of "|"

    int out_fd;
    int out;

    len = argsLength(args);

    for(i=0;i<len;i++) {
        if(strcmp(args[i], ">") == 0) { op1=i; }       // pos of ">"
        else if(strcmp(args[i], ">>") == 0) { op2=i; }   // pos of ">>"
        else if(strcmp(args[i], "|") == 0) { opPipe=i; }       // pos of "|"
    }

    //Case: No |, > or >> operator. Redirect output to OUTPUT.txt
    if(op1 == 0 && op2 == 0 && opPipe == 0) {
        
      if((pid = fork()) < 0) 
        perror("Fork error");

      else if(pid == 0) {
        // Child. Execute command here

        // To redirect the output of exec command to the OUTPUT.txt file,
        // dup(2) is used. This duplicates the file descriptor.
        int fd = open("OUTPUT.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP |S_IWUSR); // user - r and w permissions. 
        

        dup2(fd, 1);    // to make stdout go to file. (1 is the file descriptor for stdout)
        dup2(fd, 2);    // to make stderr go to file. (2 is the file descriptor for stderr)
        close(fd); 

        printf("\n\n");
        execvp(args[0], args);
        printf("Couldn't execute this command\n");
      }

      else {
        // wait for the child in parent process
        do {
          wpid = waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));

      }

    }

    // Case: '>' operator found or '>>' found. Redirect to appropriate file following the operator
    else if( (op1 != 0 || op2 != 0) && opPipe == 0) {

      if((pid = fork()) < 0) 
        perror("Fork error");

      else if(pid == 0) {
        //child
        if (op1 != 0) {

          // '>' operator
          args[op1] = NULL; // must make that pos NULL so that command can execute correctly
          /*
          // Use the out file descriptor to redirect the output
          // It must be write-only, 
          // Has to be created if it doesn't exist,
          // Must be truncated each time it's opened for writing
          // Give appropriate permissions
          */
          out_fd = open(args[op1 + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP |S_IWUSR);
        
        }

        else {
          // '>>' operator
          args[op2] = NULL;
          out_fd = open(args[op2 + 1], O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        }


        // replace stdout with this out_fd
        dup2(out_fd, 1);  // 1 equivalent to STDOUT_FILENO
        close(out_fd);

        printf("\n\n");
        execvp(args[0], args);
        printf("Couldn't execute this command\n");
      }

      else {
        //parent
        do {
          wpid = waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
      }

    }

    // Case: '|' operator is present. 
    // Now, modified to handle multiple pipes using executePipeCommands function.
    else if(opPipe != 0) {

      int n = numberOfPipes(args);

      // // DEBUG
      // printf("numberOfPipes = %d\n", n);

      char **commands[n+1];   // An array of commands after splitting on '|' operator

      int j;
      int curoffset = 0;
      int k = 0;

      // Parse the commands separated by pipes and add them to the commands array
      while (k<n+1) {

          commands[k] = (char**)malloc(len * sizeof(char*));
          if(!commands[k]) exit(EXIT_FAILURE);

          j = 0;
          while(curoffset < len && strcmp(args[curoffset],"|") != 0 && strcmp(args[curoffset], ">") != 0 && strcmp(args[curoffset], ">>") != 0) {
            commands[k][j] = args[curoffset];
            curoffset = curoffset + 1;
            j = j + 1;
          }
          
          commands[k][j] = NULL;
          
          // // DEBUG
          // int newlen = argsLength(commands[k]);
          // for(i=0; i<newlen; i++) {
          //   printf("%s ", commands[k][i]);
          // }

          // printf("\n");

          k = k + 1;
          curoffset = curoffset + 1;

      }
      
      if(op1 || op2)
        {
          // printf("1\n");
          executePipeCommands(commands, n+1, op1, op2, args[len - 1]);
        }
      else
        executePipeCommands(commands, n+1, op1, op2, NULL);


      // ---- BEGIN OF COMMENT --- (DOUBT: Check why this does not work. Some problem with using the same pipe? Unable to pin point the error in the previous code)

      // Pasted the code in notes.txt

      // --- END OF COMMENT ----------------

    }

    return 1;

}

/*
    Function that handles multiple pipes (with or without redirection at the end).
    commands[]      :   Array of commands which are part of the whole command on the line
    numberOfCommands:   number of commands in commands[]
    op1             :   pos of '>' ; indicates presence of '>'
    op2             :   pos of '>>' ; indicates presence of '>>'
    redirectfile    :   filename to be used in case of redirection

    HOW IT WORKS:

    Iterate through all commands from left to right and use pipe()
    and dup2() to redirect input and output.

    fin is descriptor for input
    fout is descriptor for output

    According to the commands, we modify fin and fout.

    To execute each command, we use a fork() call and child executes the command.
    
*/
void executePipeCommands(char **commands[], int n, int op1, int op2, char *redirectfile) {

  int fin, fout;  
  int pid, wpid;
  int status;
  int len;
  fin = dup(0); // initially fin is a copy of stdin.

  int i,j,k;

  // DEBUG
  // printf("Inside executePipeCommands:\n");
  // for(i=0; i<n; i++)
  // {
  //   len = argsLength(commands[i]);
  //   // for(j = 0; j<len; j++) printf("%s ", commands[i][j]);
  //   //   printf("\n");
  //   printf("%s (%s)\n", commands[i][len - 2], commands[i][len - 1]);
  // }
  // if(op1)
  //   printf("RedirectFile: %s ",redirectfile);

  for(i=0; i<n; i++) {

    dup2(fin, 0);
    close(fin);

    if(i == n-1) {
      // If it's the last command, check where the OUTPUT must be redirected
 
      if(op1 != 0) {
        fout = open(redirectfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      }
      else if(op2 != 0) {
        fout = open(redirectfile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      }
      else {
        // No redirectio. So, use OUTPUT.txt
        fout = open("OUTPUT.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP |S_IWUSR); 
      }
    }

    else {
      // Use pipe for everything in between. 
      int pipefd[2];
      pipe(pipefd);
      fin = pipefd[0];
      fout = pipefd[1];
    }

    dup2(fout, 1);
    close(fout);

    if((pid = fork()) < 0) 
      perror("Fork error");

    else if(pid == 0) {
      // child
      execvp(commands[i][0], commands[i]);
      printf("Couldn't execute this command\n");
    }

    else {
      // parent
      do {
          wpid = waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));

    }

  }

}




// Currently none of the following functions are being used as there seems to be some problem 
// with using pipes in the intended manner using executeIntermediate(). See commented portion 
// in execute.

/*

  Function to execute the left-most command in a series of piped commands
  
  Here reading is still from stdin. 
  Output is written to the write-end of the pipe.

*/

void executeLeft(int pipefd[], char **args) {
  int pid, wpid;
  int status;
  
  if((pid = fork()) < 0) 
    perror("Fork error");

  else if(pid == 0) {
    //child
    dup2(pipefd[1], 1); // replace stdout with write end of pipe
    close(pipefd[1]);

    execvp(args[0], args);
    printf("Couldn't execute this command\n");
  }

  else {
    close(pipefd[1]); // close the write end of the pipe
    //parent
        do {
          wpid = waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }

}
/*
  Function to execute the intermediate commands in a series of piped commands
  
  Here reading is from read-end of the pipe
  Output is written to the write-end of the pipe.
*/
void executeIntermediate(int pipefd[], char** args) {
  int pid, wpid;
  int status;

  if((pid = fork()) < 0) 
    perror("Fork error");

  else if(pid == 0) {
    // child
    dup2(pipefd[0], 0); // replace stdin with read end of pipe
    close(pipefd[0]);
    // close(pipefd[1]);

    dup2(pipefd[1], 1); // replace stdout with write end of pipe
    close(pipefd[1]);
    // close(pipefd[0]);

    execvp(args[0], args);
    printf("Couldn't execute this command\n");
  }

  else {
    // check this
    // close(pipefd[0]);
    close(pipefd[1]);
    do {
          wpid = waitpid(pid, &status, 0);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));

    printf("We're in parent intermediate\n");
    int i;
    char buf[256];
    if((i = read(pipefd[0], buf, sizeof(buf))) > 0) {
            printf("%s ", buf);
            printf("So, this probably works and I'm not seeing that!\n");
    }
    buf[i] = 0;

  }

}


/*
  Function to execute the right-most command in a series of piped commands
  
  Here reading is from read-end of the pipe
  Output is written to stdout(for now - testing) (i.e., change that to OUTPUT.txt later)
*/
void executeRight(int pipefd[], char** args) {
  int pid, wpid;
  int len;
  int i,j;
  int op1 = 0;
  int op2 = 0;
  int opPipe = 0;
  int status;

  int out_fd;

  len = argsLength(args);

  for(i = 0; i<len; i++) {
    if(strcmp(args[i], ">") == 0) { op1=i; }       // pos of ">"
    else if(strcmp(args[i], ">>") == 0) { op2=i; }   // pos of ">>"
      // Check for this later
     // else if(strcmp(args[i], "|") == 0) { opPipe=i; }       // pos of "|"
  }

  // Case: No '>' or '>>'
  if(op1 == 0 && op2 == 0) {
    
    // printf("Getting ready to execute right side of pipe command\n");

    if((pid = fork()) < 0) 
      perror("Fork error");

    else if(pid == 0) {
      //child
      dup2(pipefd[0], 0); // replace stdin with pipe's read end
      
      // DEBUG
      int fd = open("OUTPUT.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP |S_IWUSR); // user - r and w permissions. 

      dup2(fd, 1);    // to make stdout go to file. (1 is the file descriptor for stdout)
      dup2(fd, 2);    // to make stderr go to file. (2 is the file descriptor for stderr)

      close(fd); 
      close(pipefd[0]);


      printf("\n\n");
      execvp(args[0], args);
      printf("Couldn't execute this command\n");
    }

    else {
      //parent
      do {
        wpid = waitpid(pid, &status, 0);
      } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

  }

  // case: one of '>' or '>>' is present and 
  // output must be redirected to the corresponding file
  else if(op1 != 0 || op2 != 0) {

    if((pid = fork()) < 0) 
      perror("Fork error");

    else if(pid == 0) {
      //child
      if(op1 != 0) {
        args[op1] = NULL;
        out_fd = open(args[op1+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      }

      else {
        args[op2] = NULL;
        out_fd = open(args[op2+1], O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      }

      
      dup2(out_fd, 1);  // replace stdout with out_fd
      close(out_fd);    

      dup2(pipefd[0], 0); //replace stdin with read end of pipe
      close(pipefd[0]);

      execvp(args[0], args);
      printf("Couldn't execute this command\n");
    }

    else {
      // parent
      do {
        wpid = waitpid(pid, &status, 0);
      } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }


  }


}   // end of executeRight
