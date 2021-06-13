# Adaptive-Replacement-Cache-Algorithm
An implementation of Adaptive-Replacement-Cache-Algorithm in C language.
Tested with attached input (P3.lis) file with various c pages(128 to 1024). Elapsed time ~9s with 1.2 hit ratio.

# For compilation
gcc -o ARC main.c cache.c

# For Execution
ARC.exe 256 P3.lis

Argument 1 - Specifies cache capacity in pages
Argument 2 - Specifies the input file name(For file format refer attached P3.lis file)
