#include "../cctop.h"
#include "SMC.h"
#include <cstdio>
#include <cstdlib>

//static io_connect_t conn;

static UInt32 _strtoul(const char *str, int size, int base) {
    UInt32 total = 0;
    int i;

    for (i = 0; i < size; i++) {
        if (base == 16)
            total += str[i] << (size - 1 - i) * 8;
        else
            total += (unsigned char) (str[i] << (size - 1 - i) * 8);
    }
    return total;
}

static void _ultostr(char *str, UInt32 val) {
    str[0] = '\0';
    sprintf(str, "%c%c%c%c",
            (unsigned int) val >> 24,
            (unsigned int) val >> 16,
            (unsigned int) val >> 8,
            (unsigned int) val);
}

typedef union _data {
    float f;
    char  b[4];
} fltUnion;

static float _flttof(unsigned char *str) {
    fltUnion flt;
    flt.b[0] = str[0];
    flt.b[1] = str[1];
    flt.b[2] = str[2];
    flt.b[3] = str[3];

    return flt.f;
}

SMC::SMC() {
    //
    Open();
}

SMC::~SMC() {
    //
    Close();
}

bool SMC::Open() {
    kern_return_t result;
    io_iterator_t iterator;
    io_object_t device;

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    result = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDictionary, &iterator);
    if (result != kIOReturnSuccess) {
        printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
        return false;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (device == 0) {
        printf("Error: no SMC found\n");
        return false;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, &conn);
    IOObjectRelease(device);
    if (result != kIOReturnSuccess) {
        printf("Error: IOServiceOpen() = %08x\n", result);
        return false;
    }

    return true;
}

bool SMC::Close() {
    return IOServiceClose(conn) == kIOReturnSuccess;
}

kern_return_t SMC::Call(int index, SMCKeyData_t *inputStructure, SMCKeyData_t *outputStructure) {
    size_t structureInputSize;
    size_t structureOutputSize;

    structureInputSize = sizeof(SMCKeyData_t);
    structureOutputSize = sizeof(SMCKeyData_t);

#if MAC_OS_X_VERSION_10_5
    return IOConnectCallStructMethod(conn, index,
                                     inputStructure, structureInputSize, // inputStructure
                                     outputStructure, &structureOutputSize // ouputStructure
    );
#else
    return IOConnectMethodStructureIStructureO(conn, index,
                                               structureInputSize, /* structureInputSize */
                                               &structureOutputSize, /* structureOutputSize */
                                               inputStructure, /* inputStructure */
                                               outputStructure); /* ouputStructure */
#endif
}

kern_return_t SMC::ReadKey(const UInt32Char_t key, SMCVal_t *val) {
    kern_return_t result;
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));
    memset(val, 0, sizeof(SMCVal_t));

    inputStructure.key = _strtoul(key, 4, 16);
    inputStructure.data8 = SMC_CMD_READ_KEYINFO;

    result = Call(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    val->dataSize = outputStructure.keyInfo.dataSize;
    _ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = Call(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}

double SMC::GetTemperature(const char *key) {
    SMCVal_t val;
    kern_return_t result;

    result = ReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        // read succeeded - check returned value
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, DATATYPE_FPE2) == 0) {
                // convert fpe2 value to RPM
                return ntohs(*(UInt16 *) val.bytes) / 4.0;
            }
            else if (strcmp(val.dataType, DATATYPE_FLT) == 0) {
                return _flttof((unsigned char *)val.bytes);
            }
            else if (strcmp(val.dataType, DATATYPE_SP78) == 0) {
                // convert fp78 value to temperature
                int intValue = val.bytes[0] * 256 + (unsigned char) val.bytes[1];
                return intValue / 256.0;
            }
        }
    }
    // read failed
    return 0.0;
}

double SMC::GetFanSpeed(const char *key) {
    SMCVal_t val;
    kern_return_t result;

    result = ReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        // read succeeded - check returned value
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, DATATYPE_FPE2) == 0) {
                // convert fpe2 value to rpm
                int intValue = (unsigned char) val.bytes[0] * 256 + (unsigned char) val.bytes[1];
                return intValue / 4.0;
            }
        }
    }
    // read failed
    return 0.0;
}

double convertToFahrenheit(double celsius) {
    return (celsius * (9.0 / 5.0)) + 32.0;
}

// Requires SMCOpen()
void SMC::ReadAndPrintCpuTemp(char scale) {
    double temperature = GetTemperature(SMC_KEY_CPU_TEMP);
    if (scale == 'F') {
        temperature = convertToFahrenheit(temperature);
    }

    console.label("CPU: ");
    console.print("%0.1f °%c ", temperature, scale);
}

// Requires SMCOpen()
void SMC::ReadAndPrintGpuTemp(char scale) {
    double temperature = GetTemperature(SMC_KEY_GPU_TEMP);
    if (scale == 'F') {
        temperature = convertToFahrenheit(temperature);
    }

    console.label("  GPU: ");
    console.print("  %0.1f °%c ", temperature, scale);
}

// Requires SMCOpen()
void SMC::ReadAndPrintBatteryTemp(char scale) {
    double temperature = GetTemperature(SMC_KEY_BATTERY_TEMP);
    if (scale == 'F') {
        temperature = convertToFahrenheit(temperature);
    }

    console.label("BATTERY: ");
    console.print("%0.1f °%c ", temperature, scale);
}

float SMC::GetFanRPM(const char *key) {
    SMCVal_t val;
    kern_return_t result;

    result = ReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        // read succeeded - check returned value
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, DATATYPE_FPE2) == 0) {
                // convert fpe2 value to RPM
                return ntohs(*(UInt16 *) val.bytes) / 4.0;
            }
            else if (strcmp(val.dataType, DATATYPE_FLT) == 0) {
                return _flttof((unsigned char *)val.bytes);
            }
            else if (strcmp(val.dataType, DATATYPE_SP78) == 0) {
                // convert fp78 value to temperature
                int intValue = (val.bytes[0] * 256 + val.bytes[1]) >> 2;
                return float(intValue / 64.0);
            }
        }
    }
    // read failed
    return -1.f;
}

// Requires SMCOpen()
int SMC::ReadAndPrintFanRPMs() {
    int count = 0;
    kern_return_t result;
    SMCVal_t val;
    UInt32Char_t key;
    int totalFans, i;

    result = ReadKey("FNum", &val);

    if (result == kIOReturnSuccess) {
        totalFans = _strtoul((char *) val.bytes, val.dataSize, 10);

//        console.println("Num fans: %d", totalFans);
//        count++;
        for (i = 0; i < totalFans; i++) {
            sprintf(key, "F%dID", i);
            result = ReadKey(key, &val);
            if (result != kIOReturnSuccess) {
                continue;
            }
            char *name = val.bytes + 4;

            sprintf(key, "F%dAc", i);
            float actual_speed = GetFanRPM(key);
            if (actual_speed < 0.f) {
                continue;
            }

            sprintf(key, "F%dMn", i);
            float minimum_speed = GetFanRPM(key);
            if (minimum_speed < 0.f) {
                continue;
            }

            sprintf(key, "F%dMx", i);
            float maximum_speed = GetFanRPM(key);
            if (maximum_speed < 0.f) {
                continue;
            }

            float rpm = actual_speed - minimum_speed;
            if (rpm < 0.f) {
                rpm = 0.f;
            }
            float pct = rpm / (maximum_speed - minimum_speed);

            pct *= 100.f;
            console.label("Fan %d: ", i);
            console.print("%4.0f RPM ", rpm);
            count++;

            char *ptr, *endPtr;
            sprintf(key, "F%dSf", i);
            ReadKey(key, &val);
            ptr = val.bytes;
//            console.print("   Safe speed: %.0f", strtof(ptr, &endPtr));
//            count++;
            sprintf(key, "F%dTg", i);
            ReadKey(key, &val);
            ptr = val.bytes;
//            console.print(" Target speed: %.0f", strtof(ptr, &endPtr));
//            count++;
            ReadKey("FS! ", &val);
            if ((_strtoul((char *) val.bytes, 2, 16) & (1 << i)) == 0) {
                console.print("  auto ");
            } else {
                console.print("forced ");
            }
            console.gauge(20, pct);
            console.println(" %3.0f%%", pct);
            count++;
        }
    }
    return count;
}
