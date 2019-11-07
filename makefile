all: npshells.c
	g++ npshells.c -o npshell
	g++ np_simple.c -o np_simple
	g++ np_single_proc.c -o np_single_proc

noop: src/noop.cpp
	g++ src/noop.cpp -o bin/noop
	
clean:
	rm -f npshell