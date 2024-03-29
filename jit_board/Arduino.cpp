// Copyright 2020 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "Arduino.h"

SerialLibrary Serial;

namespace {

char U4ToHex(uint8_t val) {
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

void delayU8x1usec(uint8_t x1us) {
  // At 16MHz, 1us = 16tau
  asm("mov r18, %0\n"  // 1t
      "1:\n"           // LOOP 1: 16t
      "ldi r19, 4\n"   //  1t
      "2:\n"           //  LOOP 2: 11t
      "dec r19\n"      //   1t
      "brne 2b\n"      //   2t (1t for not taken)
      "nop\n"          // 1t
      "dec r18\n"      // 1t
      "brne 1b\n"      // 2t (1t for not taken)
      ::"r"(x1us)
      : "r18", "r19");
}

}  // namespace

// Use PD1(TXD) for serial debug logging.
SerialLibrary::SerialLibrary() {
  // U2X, 117.647Kbps [+2.1%]
  UBRRH = 0;
  UBRRL = 16;
  UCSRA = 0x02;
  // TX enabled
  UCSRB = (UCSRB & 0x90) | 0x08;
  // 8-bits, non-parity, 1 stop-bit
  UCSRC = 0x86;

#if !defined(NO_DEBUG)
  // Output, High
  PORTD |= 0x02;
  DDRD |= 0x02;
#endif
}

void SerialLibrary::print(uint8_t val) {
#if !defined(NO_DEBUG)
  while (!(UCSRA & (1 << UDRE)))
    ;
  UDR = val;
#endif
}

void SerialLibrary::print(uint8_t val, enum Type type) {
  if (type == BIN) {
    for (int i = 0x80; i; i >>= 1)
      print((val & i) ? '1' : '0');
  } else if (type == DEC) {
    if (val >= 100)
      print(U4ToHex(val / 100));
    if (val >= 10)
      print(U4ToHex((val % 100) / 10));
    print(U4ToHex(val % 10));
  } else if (type == HEX) {
    char hex[3];
    int i = 0;
    if (16 <= val)
      hex[i++] = U4ToHex(val >> 4);
    hex[i++] = U4ToHex(val & 0x0f);
    hex[i++] = 0;
    print(hex);
  }
}

void SerialLibrary::print(const char* val) {
  while (*val)
    print(*val++);
}

void SerialLibrary::println(const char* val) {
  print(val);
  print("\r\n");
}

void SerialLibrary::println(uint8_t val, enum Type type) {
  print(val, type);
  print("\r\n");
}

void delayMicroseconds(uint32_t us) {
  if (us > 255) {
    if (us > 65535) {
      Serial.println("delayMicroseconds > 255: not supported");
      return;
    }
    uint8_t loop = us >> 8;
    for (uint8_t i = 0; i < loop; ++i)
      delayU8x1usec(255);
    // Then, delay for the remaining 8-bits time below.
  }
  delayU8x1usec(us & 0xff);
}

void delay(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i)
    delayMicroseconds(1000);
}
