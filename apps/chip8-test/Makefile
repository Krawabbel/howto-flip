CC = gcc 

CFLAGS = -g -Wall -Werror -Wextra -O0 -std=c99

demo: demo.c vm.o test.o  
	$(CC) $(CFLAGS) -o demo demo.c test.o vm.o

test.o: test.c vm.o
	$(CC) $(CFLAGS) -c test.c -o test.o

vm.o: ../chip8/vm.c vm.o
	$(CC) $(CFLAGS) -c ../chip8/vm.c -o vm.o

clean:
	rm -f vm.o demo
