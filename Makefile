all: main

main: obj/main.o 
	g++ -o main obj/main.o -Linclude/lib -lmingw32 -lSDL2main -lSDL2

obj/main.o: src/main.cpp 
	g++ -Iinclude -c src/main.cpp -o obj/main.o

clean:
	rm -f main obj/*.o
