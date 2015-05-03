
CFLAGS := -Wall -std=gnu99 -lpthread -g -DDEBUG

all:
	gcc $(CFLAGS) *.c -o l_list

clean:
	rm *.o l_list
	