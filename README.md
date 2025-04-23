# Vanilla C Compiler

Vanilla C Compiler is a practice for compiler construction.  
It supports a Vanilla-C language and generates x86_64 [NASM](https://github.com/netwide-assembler/nasm) assembly code for **macOS**.  
Vanilla-C is a plain, simple language which is C subset with restriction and limited features.

## Features

- Basic C syntax support
- x86_64 assembly output (Mach-O format)
- Simple main program structure and no subprograms (function or procedure)
- Basic data types: `int`, `char`, `bool`, `string`
- Control flow: `if`, `while`
- Basic I/O operations `read()`, `print()`, `println()`
- Logical and bitwise operations

## Build

Run make to build:
```shell
$ make clean
$ make
```

To run the tests:
```shell
$ make test
```

## Usage

```shell
$ ./vcc <input-file>.vc
```

You will get the generated x86_64 assembly output file: `a.asm`  
The output can be assembled and linked into a valid executable: `a.out`
```shell
$ nasm -fmacho64 a.asm
$ gcc a.o
$ ./a.out
```

## Example

```c
// hello.vc
main() {
    println("Hello, World!");
}
```

Compile and run:
```shell
$ ./vcc hello.vc
$ nasm -fmacho64 a.asm
$ gcc a.o
$ ./a.out
```

## License

VCC is freely redistributable under the BSD 2 clause license.
