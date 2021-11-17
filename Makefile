CC = gcc

all: auxiliar write read

auxiliar: auxiliar.c
	$(CC) auxiliar.c -c

write: writenoncanonical.c
	$(CC) -w -o write writenoncanonical.c auxiliar.c

read: noncanonical.c
	$(CC) -w -o read noncanonical.c

clean:
	rm -f *.o *.d read write

