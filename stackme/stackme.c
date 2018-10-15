#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void some_function()
{
    execve("/bin/sh", 0, 0);
    exit(0);
}

int copy_name(char *name)
{
    char name_buffer[32];
    strcpy(name_buffer, name);
    printf("Welcome %s!\n", name_buffer);
    return 0;
}


int main(int argc, char **argv)
{
    puts("Welcome to level 0\n");
    puts("Please enter your name:\n");

    char temp[100];
    fgets(temp, 100, stdin);
    copy_name(temp);

    printf("Goodbye!\n");
    return 0;
}
