# cobfus
A simple C-obfuscator written in C. 

## Example

Obfuscated (donut.c)[https://www.a1k0n.net/2011/07/20/donut-math.html]:

```c
int _0;double sin(),cos();main(){float _1=0,_2=0,_3,_4,_5[1760];char _6[1760];printf("\x1b[2J");for(
;;){memset(_6,32,1760);memset(_5,0,7040);for(_4=0;6.28>_4;_4+=0.07){for(_3=0;6.28>_3;_3+=0.02){float
_7=sin(_3),_8=cos(_4),_9=sin(_1),_10=sin(_4),_11=cos(_1),_12=_8+2,_13=1/(_7*_12*_9+_10*_11+5),_14=
cos(_3),_15=cos(_2),_16=sin(_2),_17=_7*_12*_11-_10*_9;int _18=40+30*_13*(_14*_12*_15-_17*_16),_19=12
+15*_13*(_14*_12*_16+_17*_15),_20=_18+80*_19,_21=8*((_10*_9-_7*_8*_11)*_15-_7*_8*_9-_10*_11-_14*_8*
_16);if(22>_19&&_19>0&&_18>0&&80>_18&&_13>_5[_20]){_5[_20]=_13;_6[_20]=".,-~:;=!*#$@"[_21>0?_21:0];}
}}printf("\x1b[d");for(_0=0;1761>_0;_0++)putchar(_0%80?_6[_0]:10);_1+=0.04;_2+=0.02;}}
```

## Quickstart

```console
$ cc -o cobfus src/cobfus.c
```

## Usage

```console
$ ./cobfus <input_file> [-o <output_file>] [-l <target_ln_len>] [-i ...]
```

### Flags

- `-o`: output file (default is `<input_file>.obfus`)
- `-l`: target line len (in chars) - the obfuscator will try to keep this line len
- `-i`: a list of symbols that won't be renamed (some are automatically ignored like `main`, `printf`, `malloc`, `free` ...) 

## Limitations

The input program must only use primitive data types and specify the functions that should not be renamed (see `ignored_c_funcs`).

## Known bugs

Because `stb_c_lexer` isn't yet fully implemented, some inputs may crash or produce undesired results.
