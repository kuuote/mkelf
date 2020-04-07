.PHONY: run
run: main program
	./main && ./a.out

dump: dump.c
	gcc -o dump dump.c

hello: hello.s
	gcc -nostdlib -static -o hello hello.s

main: main.c
	gcc -o main main.c

program: dump hello
	./dump

.PHONY: clean
clean:
	rm -f a.out dump hello main program
