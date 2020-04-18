#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "util.h"
#include "mcu.h"

#define SEQFILE "/tmp/orangegate.seq"
#define BCNFILE "/tmp/orangegate.bcn"

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

int pickBeacon()
{
    /* Decide which beacon to send based on the last time they were sent, and frequency setting */
    time_t bcnTimes[8], now;
    int i, beacon = 0;
    float oldest, thisOne;
    size_t result;

    time(&now);

    /* Open the temp file if it exists */
    FILE* tempFile = fopen(BCNFILE, "rb");
    if (tempFile != NULL)
    {
        /* Read the last beacon times */
        for (i = 0; i < 8; i++)
        {
            result = fread(&bcnTimes[i], sizeof(time_t), 1, tempFile);
            if (result != 1) bcnTimes[i] = 0;
        }

        fclose(tempFile);
    }
    else
    {
        /* Set them all to 0 if the temp file wasn't there */
        for (i = 0; i < 8; i++) bcnTimes[i] = 0;
    }

    for (i = 7; i >= 0; i--)
    {
        /* Pick the next beacon, go backwards through the list to give lower numbered beacons priority */
        /* Skip if frequency is set to zero (probably not initialized) */
        if (config.beacons[i].frequency == 0) continue;

        thisOne = (float)(now - bcnTimes[i]) / (float)config.beacons[i].frequency;
        if (thisOne >= oldest)
        {
            oldest = thisOne;
            beacon = i;
        }
    }
    /* Set the beacon time for the one we picked */
    bcnTimes[beacon] = now;

    /* Write out the new values to the temp file */
    tempFile = fopen(BCNFILE, "wb");
    if (tempFile == NULL)
    {
        fprintf(stderr, "Could not open %s for writing!", BCNFILE);
        exit(1);
    }

    for (i = 0; i < 8; i++)
    {
        fwrite(&bcnTimes[i], sizeof(time_t), 1, tempFile);
    }
    fclose(tempFile);

    return beacon;
}

void doBeacon()
{
    /* Generate a beacon string */
    unsigned int num = pickBeacon();
    char* text = config.beacons[num].text;
    int textSz = strlen(text);

    char* zulu = (char*)calloc(1, 8);
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(zulu, 8, "%d%H%Mz", timeinfo);

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
                    printf(config.myCall);
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

                case 's':   // Sequence number
                    printf("%03d", getSeqNum());
                    i++;
                    break;

                case 't':   // Temperature value
                    printf("%g", fround(read_temp(), config.tempPrecision));
                    i++;
                    break;

                case 'z':   // Timestamp
                    printf(zulu);
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