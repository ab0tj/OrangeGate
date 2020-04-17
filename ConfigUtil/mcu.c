#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include "config.h"

#define MOSI    12
#define MISO    13
#define CLK     14
#define RST     4

extern int verbose, debug;
extern ConfigStruct config;

void initSpi()
{
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
}

unsigned char spiTrxByte(unsigned char b)
{
    /* Send a byte via SPI while reading another in.
    Using bitbang here as the spidev interface doesn't
    seem to work well for talking to the MCU. */

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

void initMcu()
{
    unsigned int temp;

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