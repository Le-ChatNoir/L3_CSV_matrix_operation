prog:fonc.o main.o
	cc fonc.o main.o -lm -o prog -lpthread

fonc.o: fonc.c fonc.h
	cc -c fonc.c -lpthread

main.o: main.c fonc.h
	cc -c main.c -lpthread
