## Missingno

Corrupts files. On purpose.

```bash  
$ ./missingno -i <input> [OPTIONS] -o <output>  
```

## Usage:
```bash
$ ./missingno -i photo.bmp -o out.bmp
```
Runs bitflip at 5% intensity, random positions. (default behaviour)

```bash
$ ./missingno -i photo.bmp -t bitflip -a 0.3 -o out.bmp  
```
Flips bits across 30% of the file.


```bash
$ ./missingno -i photo.bmp -t shift -a 0.1 -o out.bmp  
```
Shifts bytes in 64-byte (size_t) chunks at 10% intensity.

```bash
$ ./missingno -i photo.bmp -t random -s 10 -o out.bmp
```
Random glitch type, minimum 10 bytes between each corruption.

```bash
$ ./missingno -i photo.bmp -t reverse -s 40 -c 16 -v -o out.bmp
```
Corrupts 16-byte runs every 40 bytes. Prints every hit.

## Build:

```C
$ make
```

## Note:

Audio is experimental.
