all: diffdir379

diffdir379: diffdir379.c
	gcc -std=c99 -D_BSD_SOURCE diffdir379.c -o diffdir379 

clean:
	rm -rf *.o