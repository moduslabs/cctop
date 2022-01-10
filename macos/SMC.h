//
// Created by Michael Schwartz on 12/30/21.
//

#ifndef CCTOP_SMC_H
#define CCTOP_SMC_H

#include <IOKit/IOKitLib.h>

#define VERSION "0.01"

static const uint8_t KERNEL_INDEX_SMC = 2;

static const uint8_t SMC_CMD_READ_BYTES = 5;
static const uint8_t SMC_CMD_WRITE_BYTES = 6;
static const uint8_t SMC_CMD_READ_INDEX = 8;
static const uint8_t SMC_CMD_READ_KEYINFO = 9;
static const uint8_t SMC_CMD_READ_PLIMIT = 11;
static const uint8_t SMC_CMD_READ_VERS = 12;

static const char *DATATYPE_FPE2 = "fpe2";
static const char *DATATYPE_FLT = "flt ";
static const char *DATATYPE_UINT8 = "ui8 ";
static const char *DATATYPE_UINT16 = "ui16";
static const char *DATATYPE_UINT32 = "ui32";
static const char *DATATYPE_SP78 = "sp78";

// key values
static const char *SMC_KEY_CPU_TEMP = "TC0P";
static const char *SMC_KEY_GPU_TEMP = "TG0P";
static const char *SMC_KEY_BATTERY_TEMP = "TB0T";
static const char *SMC_KEY_FAN0_RPM_CUR = "F0Ac";

typedef struct {
    char major;
    char minor;
    char build;
    char reserved[1];
    UInt16 release;
} SMCKeyData_vers_t;

typedef struct {
    UInt16 version;
    UInt16 length;
    UInt32 cpuPLimit;
    UInt32 gpuPLimit;
    UInt32 memPLimit;
} SMCKeyData_pLimitData_t;

typedef struct {
    UInt32 dataSize;
    UInt32 dataType;
    char dataAttributes;
} SMCKeyData_keyInfo_t;

typedef char SMCBytes_t[32];

typedef struct {
    UInt32 key;
    SMCKeyData_vers_t vers;
    SMCKeyData_pLimitData_t pLimitData;
    SMCKeyData_keyInfo_t keyInfo;
    char result;
    char status;
    char data8;
    UInt32 data32;
    SMCBytes_t bytes;
} SMCKeyData_t;

typedef char UInt32Char_t[5];

typedef struct {
    UInt32Char_t key;
    UInt32 dataSize;
    UInt32Char_t dataType;
    SMCBytes_t bytes;
} SMCVal_t;

class SMC {
public:
    SMC();

    ~SMC();

public:
    bool Open();

    bool Close();

    kern_return_t Call(int index, SMCKeyData_t *inputStructure, SMCKeyData_t *outputStructure);

    kern_return_t ReadKey(const UInt32Char_t key, SMCVal_t *val);

    double GetTemperature(const char *key);

    double GetFanSpeed(const char *key);

    float GetFanRPM(const char *key);

    void ReadAndPrintCpuTemp(char scale);

    void ReadAndPrintGpuTemp(char scale);

    void ReadAndPrintBatteryTemp(char scale);

    int ReadAndPrintFanRPMs();

protected:
    io_connect_t conn{};
};

#endif //CCTOP_SMC_H
