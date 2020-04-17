#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include "config.h"

extern ConfigStruct config;

float fround(float f, uint digits)
{
    if (digits == 0) return round(f);
    return float(int(f * pow(10, digits) + 0.5)) / pow(10, digits);
}

float read_temp()
{
    float val;
    long size;
    char* buff;
    FILE *fp = fopen(config.tempFile, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening temperature file: %d\n", errno);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buff = (char*)calloc(1, size + 1);
    fread(buff, size, 1, fp);
    fclose(fp);

    val = atoi(buff);
    free(buff);

    if (config.tempUnit == 'F') val = (val * (9.0 / 5.0)) + 32000;
    else if (config.tempUnit == 'K') val += 273150;

    return val / 1000;
}