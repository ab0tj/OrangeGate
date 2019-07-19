const float version = 1.00;

typedef struct
{
    int enabled;
    unsigned int timeout;
} PttConfig;

typedef struct
{
    float scale;
    uint precision;
} AdcStruct;

typedef struct
{
    PttConfig ptt[3];
    char* beaconText;
    AdcStruct adc[2];
    char tempUnit;
    char* tempFile;
    uint tempPrecision;
} ConfigStruct;

extern char* configFile;
extern ConfigStruct config;

int configHandler(void* user, const char* section, const char* name, const char* value);