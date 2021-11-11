all: alarme write read


alarme: alarme.c
	gcc alarme.c -c

write: writecanonical.c
	gcc -o write writecanonical.c


read: noncanonical.c
	gcc -o read noncanonical.c
