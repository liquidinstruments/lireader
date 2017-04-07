//
//  liconvert.c
//  liquidreader
//
//  Created by Antony Searle on 19/10/16.
//  Copyright © 2016 Antony Searle. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "litocsv.h"
#include "litomat.h"

char* li_change_extension(char* filename, char* extension)
{
    long n = strlen(filename);
    long m = strlen(extension);
    char* q = strrchr(filename, '.'); // Find the last '.' if it exists
    ptrdiff_t i = q ? (q - filename) : n;
    size_t j = i + 1 + m + 1; // Prefix + '.' + extension + '\0'
    char* newname = malloc(j);
    if (newname) {
        memcpy(newname, filename, i);
        newname[i] = '.';
        memcpy(newname + i + 1, extension, m + 1);
    }
    return newname;
}

static void help()
{
    printf("Convert Liquid Instruments binary log files (.li) to Comma Separated Value (.csv)\n");
    printf("or MATLAB 5.0 MAT-file.  (C) Liquid Instruments 2016\n");
    printf("\n");
    printf("usage:   liconvert [--mat] [--stdin] [file ...]\n");
    printf("\n");
    printf("example: liconvert file              Write file.csv\n");
    printf("         liconvert file1 file2       Write file1.csv and file2.csv\n");
    printf("         liconvert --mat f1 f2       Write f1.mat and f2.mat\n");
    printf("         liconvert --stdin file      Accept binary data from stdin and write to file.csv\n");
    
}

int main(int argc, char** argv) {
    if (argc == 1)
        help();
    enum {
        csv, mat
    } kind = csv;
    bool use_stdin = false;

    while (*++argv)
        if (**argv == '-') { // Process a flag
            if (!strcmp(*argv, "--csv")) {
                kind = csv;
            } else if (!strcmp(*argv, "--mat")) {
                kind = mat;
            } else if (!strcmp(*argv, "--stdin")) {
                use_stdin = true;
            } else {
                if (!strcmp(*argv, "--help"))
                    printf("Unrecognized option \"%s\"\n\n", *argv);
                help();
            }
        } else { // Convert a file
            FILE* infile;
            if (use_stdin)
                infile = stdin;
            else
                infile = fopen(*argv, "rb");

            if (!infile) {
                fprintf(stderr, "Could not open \"%s\"\n", *argv);
                continue;
            }
            char* outname = NULL;
            switch (kind) {
                case csv:
                    outname = li_change_extension(*argv, "csv");
                    break;
                case mat:
                    outname = li_change_extension(*argv, "mat");
                    break;
            }
            
            //printf("%s -> %s\n", *argv, outname);
            
            FILE* outfile = fopen(outname, "w+b");
            if (!outfile) {
                fprintf(stderr, "Could not open \"%s\" for output\n", outname);
                goto cleanup;
            }
            li_status result;
            switch (kind) {
                case csv:
                    result = li_to_csv(infile, outfile);
                    break;
                case mat:
                    result = li_to_mat(infile, outfile);
                    break;
            }
            if (result)
                printf("%s error converting \"%s\"\n", li_status_string(result), *argv);
            fclose(outfile);
        cleanup:
            free(outname);
            fclose(infile);
        }
    return EXIT_SUCCESS;
}
