 /* Copyright 2022 Bogdan Pilyugin
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
 *   File name: common_types.h
 *     Project: ChargePointMainboard
 *  Created on: 2022-07-21
 *      Author: Bogdan Pilyugin
 * Description:	
 */

#ifndef MAIN_INCLUDE_COMMON_TYPES_H_
#define MAIN_INCLUDE_COMMON_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned long LONG;

typedef union
{
    uint8_t Val;
    struct __attribute__((packed))
    {
        uint8_t b0 :1;
        uint8_t b1 :1;
        uint8_t b2 :1;
        uint8_t b3 :1;
        uint8_t b4 :1;
        uint8_t b5 :1;
        uint8_t b6 :1;
        uint8_t b7 :1;
    } bits;
} TCPIP_UINT8_VAL, TCPIP_UINT8_BITS, UINT8_VAL;
typedef union
{
    uint16_t Val;
    uint8_t v[2];
    struct __attribute__((packed))
    {
        uint8_t LB;
        uint8_t HB;
    } byte;
    struct __attribute__((packed))
    {
        uint8_t b0 :1;
        uint8_t b1 :1;
        uint8_t b2 :1;
        uint8_t b3 :1;
        uint8_t b4 :1;
        uint8_t b5 :1;
        uint8_t b6 :1;
        uint8_t b7 :1;
        uint8_t b8 :1;
        uint8_t b9 :1;
        uint8_t b10 :1;
        uint8_t b11 :1;
        uint8_t b12 :1;
        uint8_t b13 :1;
        uint8_t b14 :1;
        uint8_t b15 :1;
    } bits;
} TCPIP_UINT16_VAL, TCPIP_UINT16_BITS, UINT16_VAL;

typedef union
{
    uint32_t Val;
    uint16_t w[2] __attribute__((packed));
    uint8_t v[4];

    struct __attribute__((packed))
    {
        uint16_t LW;
        uint16_t HW;
    } word;

    struct __attribute__((packed))
    {
        uint8_t LB;
        uint8_t HB;
        uint8_t UB;
        uint8_t MB;
    } byte;

    struct __attribute__((packed))
    {
        TCPIP_UINT16_VAL low;
        TCPIP_UINT16_VAL high;
    } wordUnion;

    struct __attribute__((packed))
    {
        uint8_t b0 :1;
        uint8_t b1 :1;
        uint8_t b2 :1;
        uint8_t b3 :1;
        uint8_t b4 :1;
        uint8_t b5 :1;
        uint8_t b6 :1;
        uint8_t b7 :1;
        uint8_t b8 :1;
        uint8_t b9 :1;
        uint8_t b10 :1;
        uint8_t b11 :1;
        uint8_t b12 :1;
        uint8_t b13 :1;
        uint8_t b14 :1;
        uint8_t b15 :1;
        uint8_t b16 :1;
        uint8_t b17 :1;
        uint8_t b18 :1;
        uint8_t b19 :1;
        uint8_t b20 :1;
        uint8_t b21 :1;
        uint8_t b22 :1;
        uint8_t b23 :1;
        uint8_t b24 :1;
        uint8_t b25 :1;
        uint8_t b26 :1;
        uint8_t b27 :1;
        uint8_t b28 :1;
        uint8_t b29 :1;
        uint8_t b30 :1;
        uint8_t b31 :1;
    } bits;
} TCPIP_UINT32_VAL, UINT32_VAL;

#endif /* MAIN_INCLUDE_COMMON_TYPES_H_ */
