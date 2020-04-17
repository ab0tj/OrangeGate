#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "util.h"
#include "mcu.h"

extern ConfigStruct config;

void doBeacon(unsigned int num)
{
    char* text = config.beacons[num].text;
    int textSz = strlen(text);

    char* zulu = (char*)calloc(1, 8);
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(zulu, 8, "%d%H%Mz", timeinfo);


    for (int i=0; i<textSz; i++)
    {
        if (text[i] == '|') continue;   // APRS spec says comments cannot contain ~ or |
        if (text[i] == '~')
        {
            int p = text[i+2] - '0';
            switch (text[i+1])
            {
                case 'a':   // Scaled ADC value
                    printf("%g", fround(read_adc(p, 1), config.adc[p].precision));
                    i += 2;
                    break;

                case 'r':   // Raw ADC value
                    printf("%.0f", read_adc(p, 0));
                    i += 2;
                    break;

                case 't':   // Temperature value
                    printf("%g%c", fround(read_temp(), config.tempPrecision), config.tempUnit);
                    i += 1;
                    break;

                case 'z':   // Timestamp
                    printf(zulu);
                    i += 1;
                    break;

                default:
                    break;
            }
        }
        else putchar(text[i]);
    }
    putchar('\n');

    free(zulu);
}