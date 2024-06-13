# cobfus
A simple C-obfuscator written in C. 

## Quickstart

```console
$ cc -o cobfus src/cobfus.c
```

## Usage

```console
$ ./cobfus <input_file> [-o <output_file>] [-l <target_ln_len>] [-i ...]
```

Flags:

- `-o`: output file (default is `<input_file>.obfus`)
- `-l`: target line len (in chars) - the obfuscator will try to keep this line len
- `-i`: a list of symbols that won't be renamed (some are automatically ignored like `main`, `printf`, `malloc`, `free` ...) 

## Known bugs

Because `stb_c_lexer` isn't yet fully implemented, some inputs may not work or produce undesired results.
