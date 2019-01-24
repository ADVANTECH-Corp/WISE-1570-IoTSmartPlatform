
#include "mbed.h"
#include "hdc1050.h"
#include "debug_print.h"
#include "smart_platform.h"

#define MAIN_RETRY_CNT 3
#define SCHEDULE_TIME_SEC    10

static DigitalOut g_tPower(CB_PWR_ON);

int main()
{
    char aDeviceId[16];
    int iRet, i, iNeedRegister;
    unsigned int uiCnt = 0;
    float fTemperature = 0;
    uint16_t u16Humidity = 0;

    //
    // Initiation for board
    //
    g_tPower = 1;

    //
    // Initiation for hdc1050 sensor
    //
    HDC1050_Init();

    //
    // Initiation for APIs of IoT smart platform
    //
    iRet = SPlat_iInit();
    if(iRet != 0) {
        print_function("Init IoT smart platform failed!\n");
        return -1;
    }

    //
    // Get device ID if we have already done with registration
    //
    memset(aDeviceId, 0, sizeof(aDeviceId));
    iNeedRegister = 0;
    for(i=0; i < MAIN_RETRY_CNT; i++) {
        iRet = SPlat_iGetDeviceId(DEVICE_DIGEST, DEVICE_SN, aDeviceId);
        if(iRet == 0) {
            break;
        }
        if((i+1) >= MAIN_RETRY_CNT) {
            print_function("Not found device ID from cloud or something wrong with networking.\n");
            iNeedRegister = 1;
        }
        wait(SCHEDULE_TIME_SEC);
    }

    //
    // Register to cloud(IoT smart platform of CHT) if it's never done before
    //
    if(iNeedRegister) 
    for(i=0; i < MAIN_RETRY_CNT; i++)
    {
        iRet = SPlat_iRegister(DEVICE_DIGEST, DEVICE_SN);
        if(iRet == 0) {
            print_function("Register to cloud success.\n");
            break;
        }
        if((i+1) >= MAIN_RETRY_CNT) {
            print_function("Register to cloud failed!\n");
            return -1;
        }
        wait(SCHEDULE_TIME_SEC);
    }

    //
    // After registration, retry to get device ID 
    //
    if(iNeedRegister) {
        memset(aDeviceId, 0, sizeof(aDeviceId));
        for(i=0; i < MAIN_RETRY_CNT; i++) {
            iRet = SPlat_iGetDeviceId(DEVICE_DIGEST, DEVICE_SN, aDeviceId);
            if(iRet == 0) {
                break;
            }
            if((i+1) >= MAIN_RETRY_CNT) {
                print_function("Not get device ID and exit the program\n");
                return -1;
            }
            wait(SCHEDULE_TIME_SEC);
        }
    }

    print_function("Get Device ID :%s from cloud\n", aDeviceId);

    //
    // Update sensor data to cloud
    //
    while(1) 
    {
        print_function("========== Cnt:%d, Seconds:%d ==========\n", uiCnt, uiCnt*SCHEDULE_TIME_SEC);
        HDC1050_GetSensorData(&fTemperature, &u16Humidity);
        print_function("Temperature:%.2f, Humidity:%d\n\r", fTemperature, u16Humidity);
        SPlat_iWriteSensorData(aDeviceId, fTemperature, u16Humidity);
        print_function("\n\n");

        wait(SCHEDULE_TIME_SEC);
        uiCnt++;
    }

    return 0;
}

