all: npshells.c
	g++ npshells.c -o npshell
	g++ npsimple.c -o npsimple
	g++ np_server2.c -o npserver2

noop: src/noop.cpp
	g++ src/noop.cpp -o bin/noop
	
clean:
	rm -f npshell