const float version = 1.02;

typedef struct
{
    int enabled;
    unsigned int timeout;
} PttConfig;

typedef struct
{
    float scale;
    float offset;
    unsigned int precision;
} AdcStruct;

typedef struct
{
    char* text;
    unsigned int frequency;
} BeaconStruct;

typedef struct
{
    PttConfig ptt[3];
    BeaconStruct beacons[8];
    AdcStruct adc[2];
    char tempUnit;
    char* tempFile;
    unsigned int tempPrecision;
    char* myCall;
} ConfigStruct;

extern char* configFile;
extern ConfigStruct config;

int configHandler(void* user, const char* section, const char* name, const char* value);