#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ini.h"
#include "config.h"
#include "mcu.h"
#include "beacon.h"
#include "util.h"

int verbose = 0, debug = 0;
char* configFile;
ConfigStruct config;

void show_help(const char* cmdline)
{
    printf("Usage: %s [-abcdisv]\n", cmdline);
    printf("  -a <adc>\tRead ADC 0 or 1\n");
    printf("  -b <num>\tGenerate APRS beacon string\n");
    printf("  -c\t\tSpecify config file (default is /etc/orangegate.conf)\n");
    printf("  -d\t\tDebug: Print SPI transfers\n");
    printf("  -i\t\tInitialize the MCU\n");
    printf("  -r\t\tRaw ADC output\n");
    printf("  -s <ptt>\tPrint PTT status (0-2)\n");
    printf("  -t\t\tPrint temperature\n");
    printf("  -x\t\tReset the MCU\n");
    printf("  -v\t\tBe verbose\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    int opt, do_init = 0, adc = -1, stat = -1, printTemp = 0, do_beacon = -1, scaled = 1, reset = 0;

    if (argc == 1)
    {
        show_help(argv[0]);
        return 1;
    }
    while((opt = getopt(argc, argv, ":vidb:trxc:s:a:")) != -1)
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
                reset = 1;
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
                do_beacon = atoi(optarg);
                break;

            case 'c':   // Use a different config file
                configFile = optarg;
                break;

            case 'r':   // Raw ADC output
                scaled = 0;
                break;

            case 'x':   // Just reset the MCU
                reset = 1;
                break;

            default:
                show_help(argv[0]);
                return 1;
        }
    }

    initSpi();

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

    if (reset) resetMcu();

    initMcu();

    if (do_init) initPtt();

    usleep(100000); // Let MCU's SPI counter reset
    if (do_beacon != -1) doBeacon(do_beacon);
    if (stat != -1) get_ptt_status(stat);
    if (adc != -1) printf("%g\n", scaled ? fround(read_adc(adc, 1), config.adc[adc].precision) : read_adc(adc, 0));
    if (printTemp) printf("%g%c\n", fround(read_temp(), config.tempPrecision), config.tempUnit);
    return 0;
}
