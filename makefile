all: npshells.c
	g++ npshells.c -o npshell
	g++ np_simple.c -o np_simple
	g++ np_single_proc.c -o np_single_proc
	g++ np_multi_proc.c -o np_multi_proc

noop: src/noop.cpp
	g++ src/noop.cpp -o bin/noop
	
clean:
	rm -f npshell
	rm -f np_simple
	rm -f np_single_proc
	rm -f np_multi_proc