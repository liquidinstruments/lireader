Convert Moku binary logging .li files to .csv or .mat

Compile with

    cc *.c -o liconvert -Ofast -lmz -std=c99

Convert files to CSV with

    ./liconvert myfile.li

Convert multiple files to MAT-file or CSV with

    ./liconvert myfile1.li myfile2.li --mat myfile3.li myfile4.li --csv myfile5.li

Includes material from [c-capnproto](https://github.com/opensourcerouting/c-capnproto).  See COPYING-c-capnproto.

