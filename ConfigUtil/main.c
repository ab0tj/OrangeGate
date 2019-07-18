#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <wiringPi.h>
#include "ini.h"
#include "config.h"

#define MOSI    12
#define MISO    13
#define CLK     14
#define RST     4

int verbose = 0, debug = 0;
char* configFile;
ConfigStruct config;

unsigned char spiTrxByte(unsigned char b)
{
    /* Send a byte via SPI while reading another in.
    Using bitbang here as the spidev interface doesn't
    seem to work well for programming the MCU. */

    unsigned char c = 0;

    if (debug) printf("SPI out:%02X", b);
    for (int i=0; i<8; i++)
    {
        digitalWrite(MOSI, b & 0x80);
        b <<= 1;
        c <<= 1;
        usleep(1);
        c |= digitalRead(MISO);
        digitalWrite(CLK, 1);
        usleep(1);
        digitalWrite(CLK, 0);
    }

    if (debug) printf(" in:%02X\n", c);
    usleep(1);
    return c;
}

unsigned int spiTrxWord(unsigned int w)
{
    /* Read a 16 bit word from SPI */
    unsigned int val = spiTrxByte(w);
    val |= (unsigned int)spiTrxByte(w>>8) << 8;
    return val;
}

void resetMcu()
{
    digitalWrite(RST, 0);     // Reset the MCU
    usleep(10000);
    digitalWrite(RST, 1);     // Release reset
    usleep(100000);         // Give the MCU some time to be ready
}

void initPtt()
{
    for (int i=0; i<3; i++)
    {
        if (config.ptt[i].enabled)
        {
            spiTrxByte(0x10 | i);
            spiTrxWord(config.ptt[i].timeout);
            usleep(100000);
            if (verbose) printf("Initialized PTT%d\n", i);
        }
    }
}

float read_adc(unsigned char a, int scale)
{
    unsigned int val;

    if (a > 1)
    {
        fprintf(stderr, "ADC can only be 0 or 1.\n");
        exit(1);
    }

    do
    {
        spiTrxByte(0x80 | a);   // Start ADC conversion
        usleep(250);            // Allow time for the conversion
        val = spiTrxWord(0xFFFF);
    } while (val > 0x3ff);      // Filter invalid results

    if (scale) return val * config.adc[a].scale;
    return val;
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

void get_ptt_status(unsigned char p)
{
    unsigned char val;

    if (p > 2)
    {
        fprintf(stderr, "PTT can only be 0-2");
        exit(1);
    }

    spiTrxByte(0x20 | p);
    val = spiTrxByte(0xff);

    printf("PTT%d is %sinitialized\n", p, val & 0x01 ? "" : "not ");
    printf("PTT%d is %sactive\n", p, val & 0x02 ? "" : "not ");
    printf("PTT%d has %stimed out\n", p, val & 0x04 ? "" : "not ");
}

void doBeacon()
{
    char* text = config.beaconText;
    char* temp = (char*)calloc(1, 2);
    int textSz = strlen(text);

    char* zulu = (char*)calloc(1, 8);
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(zulu, 8, "%d%H%Mz", timeinfo);


    for (int i=0; i<textSz; i++)
    {
        if (text[i] == '{')
        {
            switch (text[i+1])
            {
                case 'a':   // Scaled ADC value
                    temp[0] = text[i+2];
                    printf("%.2f", read_adc(atoi(temp), 1));
                    i += 3;
                    break;

                case 'r':   // Raw ADC value
                    temp[0] = text[i+2];
                    printf("%.0f", read_adc(atoi(temp), 0));
                    i += 3;
                    break;

                case 't':   // Temperature value
                    printf("%.*f%c", config.tempPrecision, read_temp(), config.tempUnit);
                    i += 2;
                    break;

                case 'z':   // Timestamp
                    printf(zulu);
                    i += 2;
                    break;

                default:
                    putchar('{');
                    break;
            }
        }
        else putchar(text[i]);
    }
    putchar('\n');

    free(zulu);
    free(temp);
}

void show_help(const char* cmdline)
{
    printf("Usage: %s [-abcdisv]\n", cmdline);
    printf("  -a <adc>\tRead ADC 0 or 1\n");
    printf("  -b\t\tGenerate APRS beacon string\n");
    printf("  -c\t\tSpecify config file (default is /etc/orangegate.conf)\n");
    printf("  -d\t\tDebug: Print SPI transfers\n");
    printf("  -i\t\tInitialize the MCU\n");
    printf("  -r\t\tRaw ADC output\n");
    printf("  -s <ptt>\tPrint PTT status (0-2)\n");
    printf("  -t\t\tPrint temperature\n");
    printf("  -v\t\tBe verbose\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    int opt, do_init = 0, adc = -1, stat = -1, printTemp = 0, do_beacon = 0, scaled = 1;
    unsigned int temp;

    if (argc == 1)
    {
        show_help(argv[0]);
        return 1;
    }
    while((opt = getopt(argc, argv, ":vidbtrc:s:a:")) != -1)
    {
        switch(opt)
        {
            case 'v':   // Verbose
                printf("OrangeGate Utility v%.02f\n\n", version);
                verbose = 1;
                break;

            case 'd':   // Debug
                debug++;
                break;

            case 'i':   // Init
                do_init = 1;
                break;

            case 'a':   // Read ADC
                adc = atoi(optarg);
                break;

            case 's':   // Get PTT status
                stat = atoi(optarg);
                break;

            case 't':   // Print temperature
                printTemp = 1;
                break;

            case 'b':   // Generate an APRS status beacon string
                do_beacon = 1;
                break;

            case 'c':   // Use a different config file
                configFile = optarg;
                break;

            case 'r':   // Raw ADC output
                scaled = 0;
                break;

            default:
                show_help(argv[0]);
                return 1;
        }
    }

    wiringPiSetup();
    pinMode(MOSI, OUTPUT);
    pullUpDnControl(MOSI, PUD_OFF);
    digitalWrite(MOSI, 0);
    pinMode(MISO, INPUT);
    pullUpDnControl(MISO, PUD_OFF);
    pinMode(CLK, OUTPUT);
    pullUpDnControl(CLK, PUD_OFF);
    digitalWrite(CLK, 0);
    pinMode(RST, OUTPUT);
    pullUpDnControl(4, PUD_OFF);
    digitalWrite(RST, 1);

    if (configFile == NULL)
    {
        configFile = (char*)malloc(20);
        strcpy(configFile, "/etc/orangegate.ini");
    }

    if (ini_parse(configFile, configHandler, &config) < 0)
    {
        fprintf(stderr, "Could not load %s\n", configFile);
        return 1;
    }
    if (config.tempUnit != 'F' && config.tempUnit != 'K') config.tempUnit = 'C';

    if (do_init) resetMcu();

    spiTrxByte(0x02);           // Check for MCU presence
    temp = spiTrxWord(0xFFFF);
    if (temp != 0xAA55)
    {
        fprintf(stderr, "Error: Invalid magic number (Expected 0xAA55 but read 0x%04X)\n", temp);
        exit(1);
    }
    
    if (verbose)
    {
        spiTrxByte(0x01);
        temp = spiTrxWord(0xFFFF);
        printf("Found MCU firmware revision %d\n", temp);
    }

    if (do_init) initPtt();

    usleep(100000); // Let MCU's SPI counter reset
    if (do_beacon) doBeacon();
    if (stat != -1) get_ptt_status(stat);
    if (adc != -1) printf("%.*f\n", scaled ? 2 : 0, read_adc(adc, scaled));
    if (printTemp) printf("%.*f%c\n", config.tempPrecision, read_temp(), config.tempUnit);
    return 0;
}
