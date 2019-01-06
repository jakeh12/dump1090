all:
	gcc -o dump1090 dump1090.c
	./dump1090

clean:
	rm -f dump1090
	