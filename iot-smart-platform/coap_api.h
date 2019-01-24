#ifndef __COAP_API_H__
#define __COAP_API_H__

#include <mbed.h>
#include <sn_coap_protocol.h>
#include <sn_coap_header.h>

int8_t coap_init(uint8_t* _u8RecvBuf);
void* coap_malloc(uint16_t size);
void coap_free(void* addr);
uint8_t coap_tx_cb(uint8_t *a, uint16_t b, sn_nsdl_addr_s *c, void *d);
int8_t coap_rx_cb(sn_coap_hdr_s *a, sn_nsdl_addr_s *b, void *c);
int8_t coap_post(const char* _coap_uri_path, const char* _coap_payload);
int8_t coap_get(const char* _coap_uri_path);
sn_coap_hdr_s* coap_get_parser_obj(uint8_t *_pu8RecvBuf, uint16_t _u16Len);
void coap_get_recvcnt(uint16_t *_pu16Len, unsigned int *_puiCnt);
void coap_inc_recvcnt(void);
void coap_dec_recvcnt(void);
void print_function(const char *format, ...);

#endif // End of __COAP_API_H__
