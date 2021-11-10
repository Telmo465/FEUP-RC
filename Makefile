CC=gcc
CFLAGS=-Wall
DEPS = writenoncanonical.c noncanonical.c alarme.c
OBJ = writenoncanonical.o noncanonical.o alarm.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

app: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

.PHONY: clean

clean: 
	rm -f ./*.o
	rm -f ./app