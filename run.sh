gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c mm.c -o mm.o
gcc -g -c test_app.c -o testapp.o
gcc -g mm.o gluethread/glthread.o testapp.o -o exe

