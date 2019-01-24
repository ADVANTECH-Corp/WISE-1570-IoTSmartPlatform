
#include "mbed.h"
#include <coap_api.h>
#include <debug_print.h>
#include <smart_platform.h>
#include <string>

static char g_cUriBuf[URI_BUF_SIZE];
static char g_cJsonBuf[JSON_BUF_SIZE];
static uint8_t g_u8RecvBuf[RECV_BUF_SIZE];

int SPlat_iInit(void)
{
    return coap_init(g_u8RecvBuf);   
}

int SPlat_iRegister(const char *_strDigest, const char *_strSN)
{
    int iRet;
    unsigned int uiSize;
    TRecvResponse tResponse;
    
    memset(g_cJsonBuf, 0, JSON_BUF_SIZE); 
    uiSize = snprintf(g_cJsonBuf, JSON_BUF_SIZE, JSON_CMD_REGISTER, _strDigest);
    if(uiSize >= JSON_BUF_SIZE) {
        print_function("Maybe buffer size of json too small!\n\r");
        return -1;
    }

    memset(g_cUriBuf, 0, URI_BUF_SIZE);
    uiSize = snprintf(g_cUriBuf, URI_BUF_SIZE, RESTFUL_API_REGISTER, API_KEY, _strSN);
    if(uiSize >= URI_BUF_SIZE) {
        print_function("Maybe buffer size of URI too small!\n\r");
        return -1;
    }
    
    coap_post(g_cUriBuf, g_cJsonBuf);

    memset(g_cJsonBuf, 0, JSON_BUF_SIZE);
    tResponse.u16PayloadLen = JSON_BUF_SIZE;
    tResponse.pu8Payload = (uint8_t *)g_cJsonBuf;
    iRet = SPlat_iRecvResponse(&tResponse);
    if(iRet != 0 || tResponse.u16MsgCode != 69) {
        return -1;
    }
    
    return 0;
}

int SPlat_iGetDeviceId(const char *_strDigest, const char *_strSN, char *_strDeviceId)
{
    int iRet;
    unsigned int uiSize;
    TRecvResponse tResponse;
    char *pcChar;
    char *pcTmp1;

    memset(&tResponse, 0, sizeof(TRecvResponse));
    memset(g_cJsonBuf, 0, JSON_BUF_SIZE);
    memset(g_cUriBuf, 0, URI_BUF_SIZE);

    uiSize = snprintf(g_cUriBuf, 
                        URI_BUF_SIZE, 
                        RESTFUL_API_GET_ALL_THINGS, 
                        API_KEY, 
                        _strSN, 
                        _strDigest);
    if(uiSize >= URI_BUF_SIZE) {
        print_function("Maybe buffer size of URI too small!\n\r");
        return -1;
    }
    
    coap_get(g_cUriBuf);

    tResponse.u16PayloadLen = JSON_BUF_SIZE;
    tResponse.pu8Payload = (uint8_t *)g_cJsonBuf;
    iRet = SPlat_iRecvResponse(&tResponse);
    if(iRet != 0 || tResponse.u16MsgCode != 69) {
        print_function("Response failed!\n");
        return -1;
    }
#if SPLAT_RAW_DEBUG
    else {
        print_function("Response success and payload as below\n");
        print_function("%s\n", g_cJsonBuf);
    }
#endif

    pcChar = strstr(g_cJsonBuf, "deviceId");
    if(pcChar != NULL) {
        pcTmp1 = strpbrk(pcChar+11, "\"");
        //print_function("Device ID length: %d\n\r", pcTmp1 - (pcChar+11));
        strncpy(_strDeviceId, (pcChar+11), pcTmp1 - (pcChar+11));
        return 0;
    }

    print_function("Parse deviceId failed!\n");
    return -1;
}

int SPlat_iWriteSensorData(char *_strDeviceId, float _fTempData, uint16_t _u16HumiData)
{
    unsigned int uiSize;
    TRecvResponse tResponse;
    
    memset(g_cJsonBuf, 0, JSON_BUF_SIZE); 
    uiSize = snprintf(g_cJsonBuf, 
                        JSON_BUF_SIZE, 
                        JSON_CMD_WRITE_SENSRO_DATA, 
                        ID_STRING_TEMPERATURE,
                        _fTempData,
                        ID_STRING_HUMIDITY,
                        _u16HumiData);
    if(uiSize >= JSON_BUF_SIZE) {
        print_function("Maybe buffer size of json too small!\n\r");
        return -1;
    }

    memset(g_cUriBuf, 0, URI_BUF_SIZE);
    uiSize = snprintf(g_cUriBuf, 
                        URI_BUF_SIZE, 
                        RESTFUL_API_WRITE_SENSRO_DATA, 
                        API_KEY, 
                        _strDeviceId);
    if(uiSize >= URI_BUF_SIZE) {
        print_function("Maybe buffer size of URI too small!\n\r");
        return -1;
    }
    
    coap_post(g_cUriBuf, g_cJsonBuf);

    memset(g_cJsonBuf, 0, JSON_BUF_SIZE);
    tResponse.u16PayloadLen = JSON_BUF_SIZE;
    tResponse.pu8Payload = (uint8_t *)g_cJsonBuf;
    SPlat_iRecvResponse(&tResponse);

    return 0;   
}

int SPlat_iRecvResponse(TRecvResponse *_ptResponse)
{
    uint16_t u16Len;
    unsigned int uiRecvCnt, uiTimeoutCnt;

    uiTimeoutCnt = 0;
    while(1) {
        coap_get_recvcnt(&u16Len, &uiRecvCnt);
        if(uiRecvCnt > 0) {
            print_function("Recv packet cnt:%d, len:%d\n", uiRecvCnt, u16Len);
            coap_dec_recvcnt();
            break;
        }

        wait(1);
        uiTimeoutCnt++;
        if(uiTimeoutCnt >= TIMEOUT_SEC) {
            print_function("Timeout and no response from cloud!\n");
            return -1;
        }
    }

#if SPLAT_RAW_DEBUG
    print_function("Received a message of length '%d'\n", u16Len);
    for (size_t ix = 0; ix < u16Len; ix++) {
        print_function("%02x ", g_u8RecvBuf[ix]);
       }
    print_function("\n\r");
#endif // SPLAT_DEBUG

    sn_coap_hdr_s* parsed = coap_get_parser_obj(g_u8RecvBuf, u16Len);

    // We know the payload is going to be a string
    std::string payload((const char*)parsed->payload_ptr, parsed->payload_len);

#if SPLAT_DEBUG
    print_function("Response >>>>>>>>>>>>\n\r");
    print_function("\tmsg_id:           %d\n\r", parsed->msg_id);
    print_function("\tmsg_code:         %d\n\r", parsed->msg_code);
    print_function("\tcontent_format:   %d\n\r", parsed->content_format);
    print_function("\tpayload_len:      %d\n\r", parsed->payload_len);
    print_function("\tpayload:          %s\n\r", payload.c_str());
    print_function("\toptions_list_ptr: %p\n\r", parsed->options_list_ptr);
#endif // SPLAT_DEBUG

    //
    // Copy payload if payload size is expected
    //
    if(_ptResponse->u16PayloadLen >= parsed->payload_len) {
        _ptResponse->u16PayloadLen = parsed->payload_len;
        _ptResponse->u16MsgId = parsed->msg_id;
        _ptResponse->u16MsgCode = parsed->msg_code;
        strcpy((char*)_ptResponse->pu8Payload, payload.c_str());
        free(parsed);
        return 0;
    }
    else {
        print_function("Payload size is too smaller! input:%d, response:%d\n", 
                    _ptResponse->u16PayloadLen, parsed->payload_len);
        free(parsed);
        return -1;
    }
}

int SPlat_iGetSensorData(const char *_strDeviceId, const char *_strSensorId)
{
    int iRet, i;
    unsigned int uiSize;
    TRecvResponse tResponse;

    memset(&tResponse, 0, sizeof(TRecvResponse));
    memset(g_cJsonBuf, 0, JSON_BUF_SIZE);
    memset(g_cUriBuf, 0, URI_BUF_SIZE);

    uiSize = snprintf(g_cUriBuf, 
                        URI_BUF_SIZE, 
                        RESTFUL_API_GET_SENSOR_DATA, 
                        API_KEY, 
                        _strDeviceId, 
                        _strSensorId);
    if(uiSize >= URI_BUF_SIZE) {
        print_function("Maybe buffer size of URI too small!\n\r");
        return -1;
    }
    
    for(i=0; i<3; i++) {
        coap_get(g_cUriBuf);
        tResponse.u16PayloadLen = JSON_BUF_SIZE;
        tResponse.pu8Payload = (uint8_t *)g_cJsonBuf;
        iRet = SPlat_iRecvResponse(&tResponse);
        if(iRet == 0 && tResponse.u16MsgId == 9)
            break;
        print_function("[%d] Re-send packet to get data\n\r", i);
    }

    if(iRet != 0 || tResponse.u16MsgCode != 69) {
        print_function("Response failed!\n");
        return -1;
    }
    
#if SPLAT_DEBUG
    print_function("Response success and payload as below\n");
    print_function("%s\n", g_cJsonBuf);
#endif // SPLAT_DEBUG

    return 0;
}

