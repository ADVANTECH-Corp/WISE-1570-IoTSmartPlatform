/*
 * Copyright (c) 2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "common_functions.h"
#include "UDPSocket.h"
#include "CellularLog.h"
#include "coap_api.h"
#include "debug_print.h"

// Number of retries /
#define RETRY_COUNT 3

// CellularInterface object
NetworkInterface *iface;

// Socket to talk CoAP over
UDPSocket socket;

// Thread to receive messages over CoAP
Thread recvfromThread;

// CoAP
struct coap_s* coapHandle;
coap_version_e coapVersion = COAP_VERSION_1;
static Mutex g_tRecvMutex;
static unsigned int g_uiRecvCnt = 0;
static uint8_t* g_pu8RecvBuf = NULL;
static uint16_t g_u16RecvLen = 0;

static rtos::Mutex PrintMutex;
static int dot_exit = 0;
Thread dot_thread(osPriorityNormal, 512);

void dot_event()
{
    dot_exit = 1;
    while (dot_exit) {
        ThisThread::sleep_for(4000);
        if (iface && iface->get_connection_status() == NSAPI_STATUS_GLOBAL_UP) {
            break;
        } else {
            PrintMutex.lock();
            printf(".");
            fflush(stdout);
            PrintMutex.unlock();
        }
    }
}

static void coap_set_recv_len(uint16_t _u16Len)
{
    g_tRecvMutex.lock();
    g_u16RecvLen = _u16Len;
    g_tRecvMutex.unlock();    
}

sn_coap_hdr_s* coap_get_parser_obj(uint8_t *_pu8RecvBuf, uint16_t _u16Len)
{
    return sn_coap_parser(coapHandle, _u16Len, _pu8RecvBuf, &coapVersion);
}

void coap_get_recvcnt(uint16_t *_pu16Len, unsigned int *_puiCnt)
{
    g_tRecvMutex.lock();
    *_puiCnt = g_uiRecvCnt;
    *_pu16Len = g_u16RecvLen;
    g_tRecvMutex.unlock();
}

void coap_inc_recvcnt(void)
{
    g_tRecvMutex.lock();
    g_uiRecvCnt++;
    g_tRecvMutex.unlock();
}

void coap_dec_recvcnt(void)
{
    g_tRecvMutex.lock();
    g_uiRecvCnt--;
    g_tRecvMutex.unlock();
}


// CoAP HAL
void* coap_malloc(uint16_t size) 
{
    return malloc(size);
}

void coap_free(void* addr) 
{
    free(addr);
}

// tx_cb and rx_cb are not used in this program
uint8_t coap_tx_cb(uint8_t *a, uint16_t b, sn_nsdl_addr_s *c, void *d) 
{
    print_function("coap tx cb\n");
    return 0;
}

int8_t coap_rx_cb(sn_coap_hdr_s *a, sn_nsdl_addr_s *b, void *c) 
{
    print_function("coap rx cb\n");
    return 0;
}

// Main function for the recvfrom thread
void recvfromMain() 
{
    SocketAddress addr;
    nsapi_size_or_error_t ret;
    
    print_function("Start recv thread. \n\n");

    // Suggested is to keep packet size under 1280 bytes
    while ((ret = socket.recvfrom(&addr, g_pu8RecvBuf, 1280)) >= 0) {
        coap_set_recv_len((uint16_t)ret);
        coap_inc_recvcnt();
    }

    print_function("UDPSocket::recvfrom failed, error code %d. Shutting down receive thread.\n", ret);
}

/**
 * Connects to the Cellular Network
 */
nsapi_error_t do_connect()
{
    nsapi_error_t retcode = NSAPI_ERROR_OK;
    uint8_t retry_counter = 0;

    while (iface->get_connection_status() != NSAPI_STATUS_GLOBAL_UP) {
        retcode = iface->connect();
        if (retcode == NSAPI_ERROR_AUTH_FAILURE) {
            print_function("\n\nAuthentication Failure. Exiting application\n");
        } else if (retcode == NSAPI_ERROR_OK) {
            print_function("\n\nConnection Established.\n");
            dot_exit = 0;
        } else if (retry_counter > RETRY_COUNT) {
            print_function("\n\nFatal connection failure: %d\n", retcode);
        } else {
            print_function("\n\nCouldn't connect: %d, will retry\n", retcode);
            retry_counter++;
            continue;
        }
        break;
    }
    return retcode;
}

int8_t coap_init(uint8_t* _u8RecvBuf)
{
    print_function("\nWISE-1570\n");

    if(_u8RecvBuf != NULL) {
        g_pu8RecvBuf = _u8RecvBuf;
    }
    else {
        print_function("\nCoAP init failed!\n");
        return -1;
    }
       
    print_function("Establishing connection ");

    // sim pin, apn, credentials and possible plmn are taken atuomtically from json when using get_default_instance()
    iface = NetworkInterface::get_default_instance();
    MBED_ASSERT(iface);
        
    /* Attempt to connect to a cellular network */    
#if MBED_CONF_MBED_TRACE_ENABLE
    trace_open();
#else
    dot_thread.start(dot_event);
#endif // #if MBED_CONF_MBED_TRACE_ENABLE

    if (do_connect() != NSAPI_ERROR_OK) {
        print_function("\n\nFailure. Exiting \n");    
        return -1;
    }

    nsapi_error_t err = socket.open(iface);
    print_function("Open socket return: %d \n\r", err);

    // Initialize the CoAP protocol handle, pointing to local implementations on malloc/free/tx/rx functions
    coapHandle = sn_coap_protocol_init(&coap_malloc, &coap_free, &coap_tx_cb, &coap_rx_cb);
    if(coapHandle == NULL) {
        print_function("Init CoAP failed!\n");    
        return -1;
    }

    // UDPSocket::recvfrom is blocking, so run it in a separate RTOS thread
    recvfromThread.start(&recvfromMain);

    return 0;
}

int8_t coap_post(const char* _coap_uri_path, const char* _coap_payload) 
{
    uint16_t message_len;
    uint8_t* message_ptr;
    int scount;

    // See ns_coap_header.h
    sn_coap_hdr_s *coap_res_ptr = (sn_coap_hdr_s*)calloc(sizeof(sn_coap_hdr_s), 1);
    coap_res_ptr->uri_path_ptr = (uint8_t*)_coap_uri_path;        // Path
    coap_res_ptr->uri_path_len = strlen(_coap_uri_path);   
    coap_res_ptr->msg_code = COAP_MSG_CODE_REQUEST_POST;        // CoAP method
    coap_res_ptr->payload_len = strlen(_coap_payload);          // Body length
    coap_res_ptr->payload_ptr = (uint8_t*)_coap_payload;          // Body pointer
    coap_res_ptr->content_format = COAP_CT_TEXT_PLAIN;          // CoAP content type
    coap_res_ptr->options_list_ptr = 0;                         // Optional: options list
    
    // Message ID is used to track request->response patterns, because we're using UDP (so everything is unconfirmed).
    // See the receive code to verify that we get the same message ID back
    coap_res_ptr->msg_id = 7;

    // Calculate the CoAP message size, allocate the memory and build the message
    message_len = sn_coap_builder_calc_needed_packet_data_size(coap_res_ptr);
#if COAP_API_DEBUG
    print_function("Calculated message length: %d bytes\n\r", message_len);
#endif // COAP_API_DEBUG

    message_ptr = (uint8_t*)malloc(message_len);
    
    sn_coap_builder(message_ptr, coap_res_ptr);
#if COAP_API_DEBUG
    print_function("payload: %s\n\r", _coap_payload);
#endif // COAP_API_DEBUG

    scount = socket.sendto(SERVER_IP_ADDR, UDP_SOCKET_PORT, message_ptr, message_len);
#if COAP_API_DEBUG
    print_function("Sent %d bytes to coap server\n\r", scount);
#endif // COAP_API_DEBUG

    free(coap_res_ptr);
    free(message_ptr);

    return 0;
}

int8_t coap_get(const char* _coap_uri_path) 
{
    uint16_t message_len;
    uint8_t* message_ptr;
    int scount;

    // See ns_coap_header.h
    sn_coap_hdr_s *coap_res_ptr = (sn_coap_hdr_s*)calloc(sizeof(sn_coap_hdr_s), 1);
    coap_res_ptr->uri_path_ptr = (uint8_t*)_coap_uri_path;      // Path
    coap_res_ptr->uri_path_len = strlen(_coap_uri_path);   
    coap_res_ptr->msg_code = COAP_MSG_CODE_REQUEST_GET;         // CoAP method
    coap_res_ptr->payload_len = 0;                               // Body length
    coap_res_ptr->payload_ptr = 0;                                 // Body pointer
    coap_res_ptr->content_format = COAP_CT_TEXT_PLAIN;          // CoAP content type
    coap_res_ptr->options_list_ptr = 0;                         // Optional: options list
    // Message ID is used to track request->response patterns, because we're using UDP (so everything is unconfirmed).
    // See the receive code to verify that we get the same message ID back
    coap_res_ptr->msg_id = 9;

    // Calculate the CoAP message size, allocate the memory and build the message
    message_len = sn_coap_builder_calc_needed_packet_data_size(coap_res_ptr);
    print_function("Calculated message length: %d bytes\n\r", message_len);
    message_ptr = (uint8_t*)malloc(message_len);

#if COAP_API_RAW_DEBUG
    // Uncomment to see the raw buffer that will be sent...
    print_function("Message is: ");
    for (size_t ix = 0; ix < message_len; ix++) {
         print_function("%02x ", message_ptr[ix]);
    }
     print_function("\n\r");
#endif // COAP_API_RAW_DEBUG

    sn_coap_builder(message_ptr, coap_res_ptr);
    scount = socket.sendto(SERVER_IP_ADDR, UDP_SOCKET_PORT, message_ptr, message_len);
#if COAP_API_DEBUG
    print_function("Sent %d bytes to coap server\n\r", scount);
#endif // COAP_API_DEBUG

    free(coap_res_ptr);
    free(message_ptr);

    return 0;
}

