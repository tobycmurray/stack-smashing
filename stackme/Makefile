CC=gcc
CFLAGS=-m32 -z execstack -fno-stack-protector -no-pie -g

stackme: stackme.c
	$(CC) $(CFLAGS)  $< -o $@


clean:
	rm -f stackme
