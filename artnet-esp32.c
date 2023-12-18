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
 *   File name: artnet-esp32.c
 *     Project: T3HS_E2DMX
 *  Created on: 2023-12-11
 *      Author: bogd
 * Description:	
 */

#include "artnet-esp32.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_types.h"
#include "common_types.h"

#define ART_NET_DEBUG_LEVEL 1


static const char *TAG = "artnet-esp32";
static art_net_t ArtNetHandle;

void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, in_addr_t remoteIP);
void (*artSyncCallback)(in_addr_t remoteIP);

void setArtDmxCallback(
        void (*fptr)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, in_addr_t remoteIP))
{
    artDmxCallback = fptr;
}

void setArtSyncCallback(void (*fptr)(in_addr_t remoteIP))
{
    artSyncCallback = fptr;
}

static uint16_t receive_handler(art_net_t *art, struct sockaddr_in *addr, uint8_t *artnetPacket, int *len)
{
    if (*len <= 0)
        return 0;
    // Check that packetID is "Art-Net" else ignore
    if (memcmp(artnetPacket, ART_NET_ID, sizeof(ART_NET_ID)) != 0)
    {
        return 0;
    }
    uint16_t opcode = artnetPacket[8] | artnetPacket[9] << 8;

    if (opcode == ART_DMX)
    {
        uint8_t sequence = artnetPacket[12];
        uint16_t incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
        uint16_t dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;
        if (artDmxCallback)
            (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START,
                              addr->sin_addr.s_addr);
        return ART_DMX;
    }

    if (opcode == ART_POLL)
    {
        uint8_t node_ip_address[4];
        uint8_t id[8];
        struct artnet_reply_s ArtPollReply;

        UINT32_VAL ip;
        ip.Val = art->ownip;
        node_ip_address[0] = ip.v[0];
        node_ip_address[1] = ip.v[1];
        node_ip_address[2] = ip.v[2];
        node_ip_address[3] = ip.v[3];

        sprintf((char*) id, "Art-Net");
        memcpy(ArtPollReply.id, id, sizeof(ArtPollReply.id));
        memcpy(ArtPollReply.ip, node_ip_address, sizeof(ArtPollReply.ip));
        ArtPollReply.opCode = ART_POLL_REPLY;

        ArtPollReply.port = ART_NET_PORT;
        memset(ArtPollReply.goodinput, 0x00, 4);
        memset(ArtPollReply.goodoutput, 0x80, 4);
        memset(ArtPollReply.porttypes, 0x80, 4);

        uint8_t shortname[18];
        uint8_t longname[64];

        sprintf((char*) shortname, "DMX512_OUT_PORT");
        sprintf((char*) longname, "'T3HS-E2DMX' Two DMX512 output ports node");
        memcpy(ArtPollReply.shortname, shortname, sizeof(shortname));
        memcpy(ArtPollReply.longname, longname, sizeof(longname));

        ArtPollReply.etsaman[0] = 0;
        ArtPollReply.etsaman[1] = 0;
        ArtPollReply.verH = 1;
        ArtPollReply.ver = 0;
        ArtPollReply.subH = 0;
        ArtPollReply.sub = 0;
        ArtPollReply.oemH = 0;
        ArtPollReply.oem = 0xFF;
        ArtPollReply.ubea = 0;
        ArtPollReply.status = 0xd2;
        ArtPollReply.swvideo = 0;
        ArtPollReply.swmacro = 0;
        ArtPollReply.swremote = 0;
        ArtPollReply.style = 0;

        ArtPollReply.numbportsH = 0;
        ArtPollReply.numbports = 2;
        ArtPollReply.status2 = 0x08;

        ArtPollReply.bindip[0] = node_ip_address[0];
        ArtPollReply.bindip[1] = node_ip_address[1];
        ArtPollReply.bindip[2] = node_ip_address[2];
        ArtPollReply.bindip[3] = node_ip_address[3];

        uint8_t swin[4] = { 0x01, 0x02, 0x03, 0x04 };
        uint8_t swout[4] = { 0x03, 0x04, 0x00, 0x00 };
        for (uint8_t i = 0; i < 2; i++)
        {
            ArtPollReply.swout[i] = swout[i];
            ArtPollReply.swin[i] = swin[i];
        }
        memcpy(artnetPacket, &ArtPollReply, sizeof(ArtPollReply));
        *len = sizeof(ArtPollReply);
        return ART_POLL;
    }

    return 0;
}

static void art_net_master_task(void *arg)
{
    art_net_t *art;
    art = (art_net_t*) arg;
    art->adr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    art->adr.sin_family = AF_INET;
    art->adr.sin_port = htons(ART_NET_PORT);

    while (1)
    {
        art->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (art->sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        int bcast = 1;
        setsockopt(art->sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof bcast);
        ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, ART_NET_PORT);

        while (1)
        {
            int err = sendto(art->sock, art->buf, strlen((char*) art->buf), 0, (struct sockaddr*) &art->adr,
                             sizeof(art->adr));
            if (err < 0)
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(art->sock, art->buf, sizeof(art->buf) - 1, 0, (struct sockaddr*) &source_addr, &socklen);

            if (len < 0) // Error occurred during receiving
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }

            else  // Data received
            {
                art->buf[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes:", len);
                ESP_LOGI(TAG, "%s", art->buf);
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        if (art->sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(art->sock, 0);
            close(art->sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    vTaskDelete(NULL);
}

static void art_net_slave_task(void *arg)
{
    art_net_t *art;
    art = (art_net_t*) arg;

    art->adr.sin_addr.s_addr = htonl(IPADDR_ANY);
    art->adr.sin_family = AF_INET;
    art->adr.sin_port = htons(ART_NET_PORT);
    art->timeout.tv_sec = ART_NET_TIMEOUT;
    art->timeout.tv_usec = 0;

    while (1)
    {
        art->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (art->sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int bcast = 1;
        setsockopt(art->sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof bcast);

        int err = bind(art->sock, (struct sockaddr*) &art->adr, sizeof(art->adr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", ART_NET_PORT);

        struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (1)
        {
            int len = recvfrom(art->sock, art->buf, sizeof(art->buf) - 1, 0, (struct sockaddr*) &source_addr,
                               &socklen);
            if (len < 0)    // Error occurred during receiving
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            else  //Data received
            {
#if ART_NET_DEBUG_LEVEL > 0
                char ip[32];
                inet_ntop(AF_INET, &source_addr.sin_addr, ip, sizeof(ip));
                ESP_LOGI(TAG, "Packet length:%d", len);
                ESP_LOGI(TAG, "Sender:%s", ip);
                ESP_LOG_BUFFER_HEX(TAG, art->buf, 64);
#endif
                if (receive_handler(art, &source_addr, art->buf, &len) == ART_POLL)
                {
#if ART_NET_DEBUG_LEVEL > 0
                    char ip2[32];
                    inet_ntop(AF_INET, &source_addr.sin_addr, ip2, sizeof(ip2));
                    ESP_LOGI(TAG, "Packet length:%d", len);
                    ESP_LOGI(TAG, "Receiver:%s", ip2);
                    ESP_LOG_BUFFER_HEX(TAG, art->buf, 64);
#endif
                    int err = sendto(art->sock, art->buf, len, 0, (struct sockaddr*) &source_addr, sizeof(source_addr));
                    if (err < 0)
                    {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }
        }

        if (art->sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(art->sock, 0);
            close(art->sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    vTaskDelete(NULL);
}

esp_err_t artnet_init(artnet_mode_t mode, uint32_t ip, void* appconf)
{
    ArtNetHandle.mode = mode;
    ArtNetHandle.ownip = ip;
    ArtNetHandle.appconf = appconf;
    if (mode < ARTNET_MODE_SLAVE_MONITOR)
        xTaskCreate(art_net_master_task, "udp_client", 4096, (void*) &ArtNetHandle, 5, &ArtNetHandle.task);
    else if (mode < ARTNET_MODE_DISABLED)
        xTaskCreate(art_net_slave_task, "udp_client", 4096, (void*) &ArtNetHandle, 5, &ArtNetHandle.task);
    return ESP_OK;
}

