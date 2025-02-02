TARGET:exe
exe:gluethread/glthread.o testapp.o mm.o
	gcc -g mm.o gluethread/glthread.o testapp.o -o exe
testapp.o: test_app.c
	gcc -g -c test_app.c -o testapp.o
mm.o: mm.c
	gcc -g -c mm.c -o mm.o
glthread.o: gluethread/glthread.c
	gcc -g -c gluethread/gluethread.c -o gluethread/glthread.o
clean:
	rm exe
	rm testapp.o
	rm mm.o
	rm gluethread/glthread.o
