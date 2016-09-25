#define tok_delimiters " \t\n\r"
#define tok_size 128

/*
    Function to parse a space separated line containing a command and its corresponding args.
*/
char** parseLine(char *line);