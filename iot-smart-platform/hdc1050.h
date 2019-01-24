#ifndef __HDC1050_H__
#define __HDC1050_H__

#ifdef __cplusplus
extern "C"
{
#endif

// TI HDC1050
//    1. Configure the acquisition parameters in register address 0x02:
//    (a) Set the acquisition mode to measure both temperature and humidity by setting Bit[12] to 1.
//    (b) Set the desired temperature measurement resolution:
//      ¡V Set Bit[10] to 0 for 14 bit resolution.
//         ¡V Set Bit[10] to 1 for 11 bit resolution.
// (c) Set the desired humidity measurement resolution:
//         ¡V Set Bit[9:8] to 00 for 14 bit resolution.
//        ¡V Set Bit[9:8] to 01 for 11 bit resolution.
//         ¡V Set Bit[9:8] to 10 for 8 bit resolution.
//     2. Trigger the measurements by executing a pointer write transaction with the address pointer set to 0x00.
//         Refer to Figure 12.
// 3. Wait for the measurements to complete, based on the conversion time (refer to Electrical Characteristics(1)
//         for the conversion time).

///    Register Map
//    0x00 Temperature 0x0000 Temperature measurement output
//    0x01 Humidity 0x0000 Relative Humidity measurement output
//    0x02 Configuration 0x1000 HDC1050 configuration and status
//    0xFB Serial ID device dependent First 2 bytes of the serial ID of the part
//    0xFC Serial ID device dependent Mid 2 bytes of the serial ID of the part
//    0xFD Serial ID device dependent Last byte bit of the serial ID of the part
//    0xFE Manufacturer ID 0x5449 ID of Texas Instruments
//    0xFF Device ID 0x1050 ID of HDC1050 device

#define TI_HDC1050_DEVICE_ADDR              0X40

#define TI_HDC1050_TEMPERATURE_ADDR         0X00
#define TI_HDC1050_HUMIDITY_ADDR            0X01
#define TI_HDC1050_CONFIGURATION_ADDR       0X02
#define TI_HDC1050_SERIALID0_ADDR           0XFB
#define TI_HDC1050_SERIALID1_ADDR           0XFC
#define TI_HDC1050_MANUFACTURER_ID_ADDR     0XFE
#define TI_HDC1050_DEVICE_ID_ADDR           0XFF

#define TI_HDC1050_MANUFACTURER_ID          0x5449 
#define TI_HDC1050_DEVICE_ID                0x1050


void HDC1050_Init(void);
void HDC1050_UnInit(void);
uint16_t  HDC1050_GetVendorID(void);
void HDC1050_GetSensorData(float *Temperature, uint16_t *Humidity);


#ifdef __cplusplus
}
#endif


#endif // End of __HDC1050_H__
