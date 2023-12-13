/* Copyright 2023 Bogdan Pilyugin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *   File name: artnet-esp32.h
 *     Project: T3HS_E2DMX
 *  Created on: 2023-12-11
 *      Author: bogd
 * Description:	
 */

#ifndef COMPONENTS_ARTNET_ESP32_INCLUDE_ARTNET_ESP32_H_
#define COMPONENTS_ARTNET_ESP32_INCLUDE_ARTNET_ESP32_H_
#include "esp_err.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

typedef enum
{
    ARTNET_MODE_MASTER = 0,
    ARTNET_MODE_MASTER_SELFSLAVE,
    ARTNET_MODE_BRIDGE_OUTPUT,
    ARTNET_MODE_SLAVE_MONITOR,
    ARTNET_MODE_BRIDGE_INPUT,
    ARTNET_MODE_DISABLED,

} artnet_mode_t;

struct artnet_reply_s
{
    uint8_t id[8];
    uint16_t opCode;
    uint8_t ip[4];
    uint16_t port;
    uint8_t verH;
    uint8_t ver;
    uint8_t subH;
    uint8_t sub;
    uint8_t oemH;
    uint8_t oem;
    uint8_t ubea;
    uint8_t status;
    uint8_t etsaman[2];
    uint8_t shortname[18];
    uint8_t longname[64];
    uint8_t nodereport[64];
    uint8_t numbportsH;
    uint8_t numbports;
    uint8_t porttypes[4]; //max of 4 ports per node
    uint8_t goodinput[4];
    uint8_t goodoutput[4];
    uint8_t swin[4];
    uint8_t swout[4];
    uint8_t swvideo;
    uint8_t swmacro;
    uint8_t swremote;
    uint8_t sp1;
    uint8_t sp2;
    uint8_t sp3;
    uint8_t style;
    uint8_t mac[6];
    uint8_t bindip[4];
    uint8_t bindindex;
    uint8_t status2;
    uint8_t filler[26];
} __attribute__((packed));

esp_err_t ArtNetInit(artnet_mode_t mode);

// UDP specific
#define ART_NET_PORT 6454
#define ART_NET_TIMEOUT 10
// Opcodes
#define ART_POLL    0x2000
#define ART_DMX     0x5000
#define ART_SYNC    0x5200

// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net"
#define ART_DMX_START 18

#define HOST_IP_ADDR "192.168.99.255"

typedef struct
{
    uint16_t packetSize;
    uint16_t opcode;
    uint8_t sequence;
    uint16_t incomingUniverse;
    uint16_t dmxDataLength;
} art_net_data_t;

typedef struct
{
    int idx;
    artnet_mode_t mode;
    uint8_t buf[MAX_BUFFER_ARTNET];
    int sock;
    struct sockaddr_in adr;
    struct timeval timeout;
    TaskHandle_t task;
} art_net_t;

#endif /* COMPONENTS_ARTNET_ESP32_INCLUDE_ARTNET_ESP32_H_ */
