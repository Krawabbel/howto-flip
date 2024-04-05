CC = gcc 

CFLAGS = -g -Wall -Werror -Wextra -O0 -std=c99

demo: vm.o demo.c
	$(CC) $(CFLAGS) -o demo demo.c vm.o

vm.o: ../chip8/vm.c
	$(CC) $(CFLAGS) -c ../chip8/vm.c -o vm.o

clean:
	rm -f vm.o demo
