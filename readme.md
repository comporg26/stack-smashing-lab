# changing a function's return address

we already saw that the x86 `call` instruction calls a function, and `ret` returns from it.

but how does `ret` know where to return?

`call` puts a **return address** on the stack. this is the address of the instruction immediately after `call`.

normally we do not change that address. in this lab, we will change it on purpose.

our program says:

```c
x = 0;
function(1, 2, 3);
x = 1;
printf("%d\n", x);
```

when you run it, it prints:

```
1
```

**your task:** change only `function`, so the program skips `x = 1` and prints:

```
0
```

do not delete or comment out `x = 1` in `main`.

## 1. a little bit about pointers

in C, a pointer is a variable that holds a memory address.

for example:

```c
int x = 5;
int *p = &x;
```

`&x` means "the address of x".

`p` now holds that address. and `*p` means "the value at that address".

so:

```c
*p = 9;
```

changes `x` to 9.

this is what we need: we want to find the memory location that contains the return address, and change the value stored there.

in our program, `ret` is a pointer:

```c
uintptr_t *ret;
```

`uintptr_t` is an integer type big enough to hold an address. on our x86-64 server it is 8 bytes. we cannot safely use `int` for a 64-bit return address: `int` is only 4 bytes here.

there are two different operations in the code:

```c
ret = ret + 0;     /* move the pointer: which stack location? */
*ret = *ret + 0;   /* change the value at that location: where to return? */
```

**important:** `ret + 1` moves by one `uintptr_t`, which is **8 bytes** on this server. but `*ret + 1` adds **1 byte** to the address stored in that location. not 8.

for now, both numbers are `0`, so the program should still print `1`.

## 2. compile and run

```
make
./test
```

look at the `make` output. it first compiles `test.c` into an object file:

```
cc ... -c test.c -o test.o
```

(`...` represents the other compiler options in the makefile.)

`-c` means **compile, but do not link yet**. it produces `test.o`. the next command links `test.o` into an executable called `test`.

now disassemble the object file:

```
objdump -D test.o | less
```

find `<main>:` in the output. you only need its instructions, in the `.text` section. `-D` also prints other sections, which you can ignore. another, shorter command is:

```
objdump -d test.o | less
```

## 3. which instructions do we want to skip?

inside `main`, find the `call` to `function`, followed by the instructions for:

```c
x = 1;
printf("%d\n", x);
```

look at the addresses **on the left** of the disassembly.

find these two addresses:

- **A:** the instruction **immediately after** `call`. this is where `function` would normally return.
- **B:** the first instruction **after** the instruction(s) for `x = 1`. this is where we want to return instead.

then calculate:

```
number of bytes to skip = B - A
```

these addresses are hexadecimal. for example, if two addresses were `0x40` and `0x49`, the difference would be `0x9`, or 9 bytes. **these are just example addresses, not your answer.**

look at the machine-code bytes as well. an assembly instruction can be several bytes long! we are counting **bytes**, not assembly lines.

note: in the `.o` file the `call` target might look odd because the linker has not yet filled in its final address. this is normal. to see the real call target in the linked executable, try:

```
objdump -d test | less
```

## 4. where is the return address stored?

we compile with a **frame pointer**. on x86-64, the register `rbp` helps us find a function's stack frame.

when `function` starts, the relevant part of its stack frame looks like this:

```
        higher memory addresses

        rbp + 8   return address       <-- we want to change this value
        rbp       saved old rbp
        rbp - ... local variables, etc.

        lower memory addresses
```

remember: the stack grows towards lower memory addresses.

in `test.c` we already give you a starting point:

```c
ret = (uintptr_t *)__builtin_frame_address(0);
```

this is a GCC-specific way to get the current frame pointer. it gives us the address represented by `rbp`. the cast tells C to treat it as a pointer to `uintptr_t` values.

so `ret` initially points at the **saved old `rbp`**, not at the return address. how many 8-byte locations should you move it to point at the return address?

change this line:

```c
ret = ret + 0;  /* task 1 */
```

## 5. look at it in gdb

try this before changing the code:

```
gdb ./test
```

then type the following inside gdb:

```
break function
run
info registers rbp rsp
x/4gx $rbp
info frame
```

`x/4gx $rbp` shows four 8-byte words of memory, starting at `rbp`. `info frame` also tells you where gdb found the saved return address.

compare the stack contents with the diagram above. which word looks like a **code address** (an address inside your program)?

some other useful commands:

```
disassemble main
si
continue
quit
```

`si` executes one machine instruction. `continue` resumes the program.

if you want to read about looking at the stack in gdb, here is a useful introduction:

https://jvns.ca/blog/2021/05/17/how-to-look-at-the-stack-in-gdb/

## 6. now change where the function returns

once `ret` points at the location containing the return address, change the second `0`:

```c
*ret = *ret + 0;  /* task 2: use the number of bytes from step 3 */
```

recompile and try again:

```
make
./test
```

if it prints `0`, you have done it.

if it crashes, check the two things separately:

1. does `ret` point to the **location where the return address is stored**? (not the return address itself.)
2. did you add the correct number of **bytes** to the address stored there?

## what to submit

submit your modified `test.c`, and write down three things:

- the address **A** (instruction after `call`);
- the address **B** (where you want to return);
- the number of bytes `B - A`.

one or two lines of explanation are enough.

## a few notes

- this lab is for **our x86-64 Linux server, using GCC and the provided makefile**. it is not portable C, and with different compiler options the instructions and stack layout may change.
- `-O0` disables optimization, so we can recognize the C statements in the assembly. `-fno-omit-frame-pointer` keeps the frame pointer. we also use `-g` to make gdb more helpful.
- `-fno-stack-protector` disables stack-canary instrumentation. canaries are an important defense against some kinds of stack corruption, but they are **not** what makes this exercise possible, and they do not automatically prevent every way of changing a return address.
- on x86-64, the first integer arguments to `function` normally arrive in registers, not on the stack. don't assume `a`, `b`, and `c` are always next to the return address.

we're deliberately doing something that normal C programs should not do. we are using it to see how a call, a stack frame, a return address, and machine instructions fit together.

have fun.
