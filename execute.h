// To find the length of the array of arguments
int argsLength(char **args);

/*
    Main logic to perform the necessary tasks.

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
int execute(char** args);

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
void executePipeCommands(char** commands[], int numberOfCommands, int op1, int op2, char* redirectfile);


// returns the number of pipes in one parsed line of arguments
int numberOfPipes(char **args);

/* 
    Helper functions to execute the left-most and right-most commands
    in a series of piped commands

    NOTE: These are currently not being used as they do not function as required. 
*/
void executeLeft(int pipefd[], char** args);
void executeIntermediate(int pipefd[], char** args);
void executeRight(int pipefd[], char** args);