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

static const char *TAG = "artnet-esp32";
static art_net_t ArtNetHandle;

void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data,  in_addr_t remoteIP);
void (*artSyncCallback)(in_addr_t remoteIP);

void setArtDmxCallback(void (*fptr)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, in_addr_t remoteIP))
{
  artDmxCallback = fptr;
}

void setArtSyncCallback(void (*fptr)(in_addr_t remoteIP))
{
  artSyncCallback = fptr;
}

static uint16_t receive_handler(struct sockaddr_in *addr, uint8_t *artnetPacket, int len)
{
    if (len <= 0)
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
            (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, addr->sin_addr.s_addr);
        return ART_DMX;
    }
    if (opcode == ART_POLL)
    {

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

    art->adr.sin_addr.s_addr = htonl(INADDR_ANY);
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

        setsockopt(art->sock, SOL_SOCKET, SO_RCVTIMEO, &art->timeout, sizeof(art->timeout));

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
                receive_handler((&source_addr), art->buf, len);
                int err = sendto(art->sock, art->buf, len, 0, (struct sockaddr*) &source_addr, sizeof(source_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
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

esp_err_t artnet_init(artnet_mode_t mode)
{
    ArtNetHandle.mode = mode;
    if (mode < ARTNET_MODE_SLAVE_MONITOR)
        xTaskCreate(art_net_master_task, "udp_client", 4096, (void*) &ArtNetHandle, 5, &ArtNetHandle.task);
    else if (mode < ARTNET_MODE_DISABLED)
        xTaskCreate(art_net_slave_task, "udp_client", 4096, (void*) &ArtNetHandle, 5, &ArtNetHandle.task);
    return ESP_OK;
}

