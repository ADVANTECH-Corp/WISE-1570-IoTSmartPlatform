#ifndef __CHT_SMART_PLATFORM_H__
#define __CHT_SMART_PLATFORM_H__

#include <coap_api.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define JSON_BUF_SIZE   256
#define URI_BUF_SIZE    128
#define RECV_BUF_SIZE   1280
#define TIMEOUT_SEC     30


#define JSON_CMD_REGISTER "{\"op\":\"Reconfigure\",\"digest\":\"%s\",\"authority\":\"device\"}"
#define JSON_CMD_WRITE_TEMPERATURE_DATA "[{\"id\":\"temperature\",\"value\":[\"%d\"]}]"
#define JSON_CMD_WRITE_HUMIDITY_DATA "[{\"id\":\"humidity\",\"value\":[\"%d\"]}]"
#define JSON_CMD_WRITE_SENSRO_DATA "[{\"id\":\"%s\",\"value\":[\"%.2f\"]},{\"id\":\"%s\",\"value\":[\"%d\"]}]"

#define RESTFUL_API_REGISTER "/%s/iot/v1/registry/%s"
#define RESTFUL_API_WRITE_SENSRO_DATA "/%s/iot/v1/device/%s/rawdata"

// Orig format: /iot/v1/thing/${sn}?digest=${digest}
#define RESTFUL_API_GET_ALL_THINGS "/%s/iot/v1/thing/%s?digest=%s"

// Orig format: /iot/v1/device/${device_id}/sensor/${sensor_id}/rawdata
// Response as below
// {"id":"gpio","deviceId":"7568574498","time":"2018-08-08T05:40:38.967Z","value":["0"]}
#define RESTFUL_API_GET_SENSOR_DATA "/%s/iot/v1/device/%s/sensor/%s/rawdata"

#define ID_STRING_TEMPERATURE "temperature"
#define ID_STRING_HUMIDITY "humidity"

typedef struct _TRecvResponse{
    uint16_t u16MsgId;
    uint16_t u16MsgCode;
    uint16_t u16PayloadLen;
    uint8_t* pu8Payload;
}TRecvResponse;

int SPlat_iInit(void);
int SPlat_iRegister(const char *_strDigest, const char *_strSN);
int SPlat_iWriteSensorData(char *_strDeviceId, float _fTempData, uint16_t _u16HumiData);
int SPlat_iRecvResponse(TRecvResponse *_ptResponse);
int SPlat_iGetDeviceId(const char *_strDigest, const char *_strSN, char *_strDeviceId);
int SPlat_iGetSensorData(const char *_strDeviceId, const char *_strSensorId);

#ifdef __cplusplus
}
#endif

#endif // End of __CHT_SMART_PLATFORM_H__
