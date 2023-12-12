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

typedef enum
{
    ARTNET_MODE_MASTER = 0,
    ARTNET_MODE_MASTER_SELFSLAVE,
    ARTNET_MODE_BRIDGE_OUTPUT,
    ARTNET_MODE_SLAVE_MONITOR,
    ARTNET_MODE_BRIDGE_INPUT,
    ARTNET_MODE_DISABLED,

} artnet_mode_t;

esp_err_t ArtNetInitMaster();
esp_err_t ArtNetInitSlave();

// UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net"
#define ART_DMX_START 18

#define HOST_IP_ADDR "192.168.99.255"

typedef struct
{
  int idx;
  uint8_t artnetPacket[MAX_BUFFER_ARTNET];


} art_net_t;


#endif /* COMPONENTS_ARTNET_ESP32_INCLUDE_ARTNET_ESP32_H_ */
