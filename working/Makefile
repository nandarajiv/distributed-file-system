all: ser cli ss

clean:
	rm s c ss

ser: ser.c
	gcc -Wall ser.c -o s -pthread

cli: cli.c
	gcc -Wall cli.c -o c -pthread

ss: ss.c
	gcc -Wall ss.c -o ss -pthread