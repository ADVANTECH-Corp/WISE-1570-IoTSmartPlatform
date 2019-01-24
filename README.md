# WISE-1570-IoTSmartPlatform


## Introduction

This example is demonstrated for connecting to Smart Connection Platform of CHT IoT on WISE-1570.


## Deployment

Please follow the below steps to setup for development:

1. `git clone https://github.com/ADVANTECH-Corp/WISE-1570-IoTSmartPlatform.git`

1. `cd WISE-1570-IoTSmartPlatform`

1. `mbed config root .`

1. `mbed deploy`


## SIM credentials

Modify the file `mbed_app.json` to fill in all information include the pin code for your SIM card, as well as any APN settings if needed. For example:

```json
        "nsapi.default-cellular-plmn": 0,
        "nsapi.default-cellular-sim-pin": "\"1234\"",
        "nsapi.default-cellular-apn": "\"your_apn\"",
        "nsapi.default-cellular-username": 0,
        "nsapi.default-cellular-password": 0
```

## Configurations with Iot Smart Platform

Modify the file `mbed_app.json` and set macros include "API_KEY", "DEVICE_DIGEST" and "DEVICE_SN" for IoT Smart Platform. For the purpose to debug, the user can modify the value from 0 to 1 with macros of XXX_DEBUG.

```json

      "macros": [
        "UDP_SOCKET_PORT=5683",
        "SERVER_IP_ADDR=\"211.20.181.199\"",
        "API_KEY=\"INPUT_YOUR_API_KEY_STRING\"",
        "DEVICE_DIGEST=\"INPUT_YOUR_DIGEST_STRING\"",
        "DEVICE_SN=\"INPUT_YOUR_SERIAL_NUMBER_STRING\"",
        "SPLAT_DEBUG=0",
        "SPLAT_RAW_DEBUG=0",
        "COAP_API_DEBUG=0",
        "COAP_API_RAW_DEBUG=0"
    ],

```

## Compilation

Go into WISE-1570-IoTSmartPlatform directory and run the below script to compile the example.

`./compile.sh`


