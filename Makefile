batchJobExecuter: batchJobExecuter.c parse.o execute.o
	gcc batchJobExecuter.c parse.o execute.o -o batchJobExecuter
parse.o: parse.c
	gcc -c parse.c
execute.o: execute.c
	gcc -c execute.c