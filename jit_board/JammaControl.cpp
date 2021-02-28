// Copyright 2020 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "JammaControl.h"

#include "Arduino.h"

namespace {
void Initialize() {
  DDRA &= ~0x4f;
  PORTA &= ~0x4f;
  DDRB = 0;
  PORTB = 0;
  DDRC = 0;
  PORTC = 0;
  DDRD = (uint8_t)~0xf8;
  PORTD = (uint8_t)~0xf8;
}
}

JammaControl::JammaControl() {
  Initialize();
}

void JammaControl::Update(const uint8_t bytes[5]) {
  Initialize();
  if (bytes[0] & 0x80)
    DDRD |= (1 << 3);

  if (bytes[1] & 0x80)
    DDRB |= (1 << 1);
  if (bytes[1] & 0x40)
    DDRA |= (1 << 6);
  if (bytes[1] & 0x20)
    DDRA |= (1 << 0);
  if (bytes[1] & 0x10)
    DDRA |= (1 << 1);
  if (bytes[1] & 0x08)
    DDRA |= (1 << 2);
  if (bytes[1] & 0x04)
    DDRA |= (1 << 3);
  if (bytes[1] & 0x02)
    DDRB |= (1 << 2);
  if (bytes[1] & 0x01)
    DDRB |= (1 << 3);

  if (bytes[2] & 0x80)
    DDRB |= (1 << 4);
  if (bytes[2] & 0x40)
    DDRB |= (1 << 5);
  if (bytes[2] & 0x20)
    DDRB |= (1 << 6);
  if (bytes[2] & 0x10)
    DDRB |= (1 << 7);

  if (bytes[3] & 0x80)
    DDRC |= (1 << 1);
  if (bytes[3] & 0x20)
    DDRD |= (1 << 4);
  if (bytes[3] & 0x10)
    DDRD |= (1 << 5);
  if (bytes[3] & 0x08)
    DDRD |= (1 << 6);
  if (bytes[3] & 0x04)
    DDRD |= (1 << 7);
  if (bytes[3] & 0x02)
    DDRC |= (1 << 2);
  if (bytes[3] & 0x01)
    DDRC |= (1 << 3);

  if (bytes[4] & 0x80)
    DDRC |= (1 << 4);
  if (bytes[4] & 0x40)
    DDRC |= (1 << 5);
  if (bytes[4] & 0x20)
    DDRC |= (1 << 6);
  if (bytes[4] & 0x10)
    DDRC |= (1 << 7);
}

void JammaControl::InsertCoin(uint8_t player) {
  if (player == 0)
    DDRB |= 0x01;
  else
    DDRC |= 0x01;
  delay(50);
  if (player == 0)
    DDRB &= ~0x01;
  else
    DDRC &= ~0x01;
  delay(50);
}

