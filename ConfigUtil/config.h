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
    char* fileName;
} AdcStruct;

typedef struct
{
    char* text;
} BeaconStruct;

typedef struct
{
    PttConfig ptt[3];
    BeaconStruct beacons[8];
    AdcStruct adc[4];
    char tempUnit;
    char* tempFile;
    unsigned int tempPrecision;
    char* myCall;
} ConfigStruct;

extern char* configFile;
extern ConfigStruct config;

int configHandler(void* user, const char* section, const char* name, const char* value);