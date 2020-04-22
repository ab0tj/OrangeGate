#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "util.h"
#include "mcu.h"

#define SEQFILE "/tmp/orangegate.seq"

extern ConfigStruct config;

unsigned int getSeqNum()
{
    /* Return the next sequence number, or 0 if we don't know the last one */
    char buffer;
    size_t result;
    unsigned int num = 0;

    /* Open temp file */
    FILE* tempFile = fopen(SEQFILE, "rb");
    if (tempFile != NULL)
    {
        result = fread(&buffer, sizeof(char), 1, tempFile);
        if (result) num = buffer;

        fclose(tempFile);
    }

    /* Save the next number to the temp file */
    tempFile = fopen(SEQFILE, "wb");
    if (tempFile == NULL)
    {
        fprintf(stderr, "Could not open %s for writing!", SEQFILE);
        exit(1);
    }

    /* Increment and write it */
    buffer = num + 1;
    fwrite(&buffer, sizeof(char), 1, tempFile);
    fclose(tempFile);

    return num;
}

void doBeacon(unsigned int num)
{
    /* Generate a beacon string */
    char* text;
    int textSz;

    char* zulu = (char*)calloc(1, 8);
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(zulu, 8, "%d%H%Mz", timeinfo);

    if (config.beacons[num].text == NULL)
    {
        fprintf(stderr, "Beacon %d not defined!\n", num);
        exit(1);
    }
    text = config.beacons[num].text;
    textSz = strlen(text);

    /* Look through the beacon string and make substitutions */
    for (int i=0; i<textSz; i++)
    {
        if (text[i] == '~') /* Aprs spec doesn't allow tilde, so we'll use it as an escape character */
        {
            int param = text[i+2] - '0';
            switch (text[i+1])
            {
                case 'a':   // Scaled ADC value
                    printf("%g", fround(read_adc(param, 1), config.adc[param].precision));
                    i += 2;
                    break;

                case 'A':   // Raw ADC value
                    printf("%.0f", read_adc(param, 0));
                    i += 2;
                    break;

                case 'c':   // Station callsign
                    printf("%s", config.myCall);
                    i++;
                    break;

                case 'C':   // Station callsign, fixed width
                    printf("%-9s", config.myCall);
                    i++;
                    break;

                case 'g':   // GPIO pin value (0 or 1)
                    printf("%d", readGpio(param));
                    i += 2;
                    break;

                case 'p':   // Ping 8.8.8.8 and insert 1 for good, 0 for bad
                    printf("%d", !(system("ping -c1 8.8.8.8 > /dev/null")));
                    i++;
                    break;

                case 's':   // Sequence number
                    printf("%03d", getSeqNum());
                    i++;
                    break;

                case 't':   // Temperature value
                    printf("%g", fround(read_temp(), config.tempPrecision));
                    i++;
                    break;

                case 'z':   // Timestamp
                    printf("%s", zulu);
                    i++;
                    break;

                case '~':   // Literal tilde
                    putchar('~');
                    i++;
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