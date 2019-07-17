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
    else if (MATCH("beacon", "text"))
    {
        pconfig->beaconText = (char*)malloc(strlen(value) + 1);
        strcpy(pconfig->beaconText, value);
    }
    else if (MATCH("adc0", "scale"))
    {
        pconfig->adc[0].scale = atof(value);
    }
    else if (MATCH("adc1", "scale"))
    {
        pconfig->adc[1].scale = atof(value);
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
    else return 0;

    return 1;
}