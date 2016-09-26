Check SpecifiedDesign_BatchJobExecuter.png for the design specifications of the batch job executer program.

List of files:

    1. parse.h
    2. parse.c

    3. execute.h
    4. execute.c

    5. batchJobExecuter.c

    6. bfile (batchfile with various command combinations for testing)
    7. pipetest (batchfile for testing multiple pipes, redirection and pipes in general)
    8. hello.txt (just an input file which is used in few commands in the above batch files)
    9. OUTPUT.txt, newhello.txt (included to show the outputs generated)
    10. Makefile

HOW TO COMPILE AND RUN:

    To compile:
        Run: make

    To run:
        ./batchJobExecuter bfile (or use: ./batchJobExecuter pipetest )

Assumptions:

    1. We assume that the single line comments are marked as '# ' (i.e. # followed by a space. Also, multi-line comments are ignored)
    2. We assume that built-in shell are not part of the input file. (i.e., these are not handled)

Parts that are not handled:

1. #INTERSTART and #INTERSTOP are not being handled as of now (Reason: Using splice() and tee() together to try to do this was resulting in errors)
2. No dynamic compilation of dup/dup2 Vs pipe/tee solution and single solution using both dup/dup2() and pipe() is implemented. (Haven't fully understood exactly how to go about with only either solution completely. Maybe we could maybe use files - one for read and another for write instead of pipes; But, wasn't completely sure about it)


batchJobExecuter.c:

    This is the driver function that takes a batch file as input from the command line 
    and executes the commands line-by-line (as specified).

    IMPORTANT NOTE: 
        For correct functioning: multiline comments, INTERSTART & INTERSTOP shouldn't be used.
        Also, Comments are to be specifies as '# ' (i.e., with a space) if in the beginning of line or 'cmd1 # <comment stuff>' if in between.

execute.c, execute.h:
    
    Contain the main logic for executing the commands in each line.

parse.c parse.h:
    
    To perform the necessary parsing

(All the above files are extensively commented to explain the functioning of each method)

Blogs and websites referred to:
Stephan Brennan's blog and extensions of the same (github)
GNU C Library
Stackoverflow
man pages



    




