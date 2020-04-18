#include <string.h>
#include <stdlib.h>
#include "config.h"

int configHandler(void* user, const char* section, const char* name, const char* value)
{
    ConfigStruct* pconfig = (ConfigStruct*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    #define BOOL(v) (strcmp(v, "true") == 0)

    if (MATCH("ptt0", "enabled"))
    {
        pconfig->ptt[0].enabled = BOOL(value);
    }
    else if (MATCH("ptt0", "timeout"))
    {
        pconfig->ptt[0].timeout = atoi(value);
    }
    else if (MATCH("ptt1", "enabled"))
    {
        pconfig->ptt[1].enabled = BOOL(value);
    }
    else if (MATCH("ptt1", "timeout"))
    {
        pconfig->ptt[1].timeout = atoi(value);
    }
    else if (MATCH("ptt2", "enabled"))
    {
        pconfig->ptt[2].enabled = BOOL(value);
    }
    else if (MATCH("ptt2", "timeout"))
    {
        pconfig->ptt[2].timeout = atoi(value);
    }
    else if (MATCH("beacon0", "text"))
    {
        pconfig->beacons[0].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[0].text, value);
    }
    else if (MATCH("beacon0", "frequency"))
    {
        pconfig->beacons[0].frequency = atoi(value);
    }
    else if (MATCH("beacon1", "text"))
    {
        pconfig->beacons[1].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[1].text, value);
    }
    else if (MATCH("beacon1", "frequency"))
    {
        pconfig->beacons[1].frequency = atoi(value);
    }
    else if (MATCH("beacon2", "text"))
    {
        pconfig->beacons[2].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[2].text, value);
    }
    else if (MATCH("beacon2", "frequency"))
    {
        pconfig->beacons[2].frequency = atoi(value);
    }
    else if (MATCH("beacon3", "text"))
    {
        pconfig->beacons[3].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[3].text, value);
    }
    else if (MATCH("beacon3", "frequency"))
    {
        pconfig->beacons[3].frequency = atoi(value);
    }
    else if (MATCH("beacon4", "text"))
    {
        pconfig->beacons[4].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[4].text, value);
    }
    else if (MATCH("beacon4", "frequency"))
    {
        pconfig->beacons[4].frequency = atoi(value);
    }
    else if (MATCH("beacon5", "text"))
    {
        pconfig->beacons[5].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[5].text, value);
    }
    else if (MATCH("beacon5", "frequency"))
    {
        pconfig->beacons[5].frequency = atoi(value);
    }
    else if (MATCH("beacon6", "text"))
    {
        pconfig->beacons[6].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[6].text, value);
    }
    else if (MATCH("beacon6", "frequency"))
    {
        pconfig->beacons[6].frequency = atoi(value);
    }
    else if (MATCH("beacon7", "text"))
    {
        pconfig->beacons[7].text = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beacons[7].text, value);
    }
    else if (MATCH("beacon7", "frequency"))
    {
        pconfig->beacons[7].frequency = atoi(value);
    }
    else if (MATCH("adc0", "scale"))
    {
        pconfig->adc[0].scale = atof(value);
    }
    else if (MATCH("adc0", "precision"))
    {
        pconfig->adc[0].precision = atoi(value);
    }
    else if (MATCH("adc0", "offset"))
    {
        pconfig->adc[0].offset = atof(value);
    }
    else if (MATCH("adc1", "scale"))
    {
        pconfig->adc[1].scale = atof(value);
    }
    else if (MATCH("adc1", "precision"))
    {
        pconfig->adc[1].precision = atoi(value);
    }
    else if (MATCH("adc1", "offset"))
    {
        pconfig->adc[1].offset = atof(value);
    }
    else if (MATCH("temp", "unit"))
    {
        pconfig->tempUnit = value[0];
    }
    else if (MATCH("temp", "file"))
    {
        pconfig->tempFile = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->tempFile, value);
    }
    else if (MATCH("temp", "precision"))
    {
        pconfig->tempPrecision = atoi(value);
    }
    else if (MATCH("station", "mycall"))
    {
        pconfig->myCall = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->myCall, value);
    }
    else return 0;

    return 1;
}