// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// This is an alternative main to implement JVS to Jamma converter. It is
// intended to be connected to JVS based cabinet and pretends as a JVS host,
// then convert input signals to a plain parallel signals for JAMMA board.

#include "Arduino.h"
#include "JammaControl.h"
#include "JVSIOClient.h"

namespace {

const uint8_t kRequestIoId[] = { 0x01, 0x02, JVSIO::kCmdIoId };
const uint8_t kRequestCommandRev[] = { 0x01, 0x02, JVSIO::kCmdCommandRev };
const uint8_t kRequestJvRev[] = { 0x01, 0x02, JVSIO::kCmdJvRev };
const uint8_t kRequestProtocolRev[] = { 0x01, 0x02, JVSIO::kCmdProtocolVer };
const uint8_t kRequestFunctionCheck[] = { 0x01, 0x02, JVSIO::kCmdFunctionCheck };

JVSIODataClient data;
JVSIOSenseClient sense;
JVSIO::LedClient led;
JVSIO io(&data, &sense, &led);
JammaControl jamma;

JVSIO::SenseClient& sense_client = sense;

uint8_t players = 0;
uint8_t buttons = 0;
uint8_t button_bytes = 0;
uint8_t coin_slots = 0;

bool GetButtons(uint8_t sws[5]) {
  uint8_t bytes = (buttons + 7) >> 3;
  uint8_t requestSwInput[] = { 0x01, 0x04, JVSIO::kCmdSwInput, players, bytes };
  uint8_t* ack;
  uint8_t ack_len;
  bool result = io.sendAndReceive(requestSwInput, &ack, &ack_len);
  if (!result || ack_len != 2 + button_bytes || ack[0] != 1 || ack[1] != 1)
    return false;
  for (int i = 0; i < 5 && i < button_bytes; ++i)
    sws[i] = ack[i + 2];
  return true;
}

bool GetCoins(uint16_t& p1, uint16_t& p2) {
  uint8_t requestCoinInput[] = { 0x01, 0x03, JVSIO::kCmdCoinInput, coin_slots };
  uint8_t coin_bytes = coin_slots * 2;
  uint8_t* ack;
  uint8_t ack_len;
  bool result = io.sendAndReceive(requestCoinInput, &ack, &ack_len);
  if (!result || ack_len != 2 + coin_bytes || ack[0] != 1 || ack[1] != 1) {
    Serial.println(" - GetCoins error#1");
    return false;
  }
  if (ack[2] & 0xc0 || ack[4] & 0xc0) {
    Serial.println(" - GetCoins error#2");
    return false;
  }
  p1 = (ack[2] << 8) | ack[3];
  if (coin_slots > 1)
    p2 = (ack[4] << 8) | ack[5];
  return true;
}

// `player` is 0 origin.
bool SubCoin(uint8_t player) {
  uint8_t slot = player + 1;
  uint8_t requestCoinSub[] = { 0x01, 0x05, JVSIO::kCmdCoinSub, slot,  0, 1 };
  uint8_t* ack;
  uint8_t ack_len;
  bool result = io.sendAndReceive(requestCoinSub, &ack, &ack_len);
  if (!result || ack_len != 2 || ack[0] != 1 || ack[1] != 1)
    return false;
  return true;
}

}

int main() {
  uint8_t jammas[5] = { 0, 0, 0, 0, 0 };
  Serial.println("JVS to JAMMA");
  io.begin();
  for (;;) {
    jamma.Update(jammas);
    Serial.println("boot up JVS bus");
    io.boot();
    Serial.println("Bus Ready");

    uint8_t* ack;
    uint8_t ack_len;
    bool result = io.sendAndReceive(kRequestIoId, &ack, &ack_len);
    if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
      continue;
    Serial.print("I/O ID: ");
    Serial.println((char*)&ack[2]);

    result = io.sendAndReceive(kRequestCommandRev, &ack, &ack_len);
    if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
      continue;
    Serial.print("Command Rev: ");
    Serial.println(ack[2], HEX);

    result = io.sendAndReceive(kRequestJvRev, &ack, &ack_len);
    if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
      continue;
    Serial.print("JV Rev: ");
    Serial.println(ack[2], HEX);

    result = io.sendAndReceive(kRequestProtocolRev, &ack, &ack_len);
    if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
      continue;
    Serial.print("Protocol Rev: ");
    Serial.println(ack[2], HEX);

    result = io.sendAndReceive(kRequestFunctionCheck, &ack, &ack_len);
    if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
      continue;
    Serial.println("Functions:");
    for (uint8_t i = 2; i < ack_len; i += 4) {
      switch (ack[i]) {
        case 0x00:
          break;
        case 0x01:
          players = ack[i + 1];
          buttons = ack[i + 2];
          button_bytes = players * ((buttons + 7) >> 3) + 1;
          break;
        case 0x02:
          coin_slots = ack[i + 1];
          break;
        default:
          break;
      }
    }

    for (;;) {
      if (!sense_client.is_connected())
        break;
      GetButtons(jammas);
      jamma.Update(jammas);

      uint16_t p1 = 0;
      uint16_t p2 = 0;
      GetCoins(p1, p2);
      jamma.DriveCoin(p1, p2);
      if (p1)
        SubCoin(0);
      if (p2)
        SubCoin(1);
    }

    for (uint8_t i = 0; i < 5; ++i)
      jammas[i] = 0;
    jamma.Update(jammas);
    jamma.DriveCoin(false, false);
  }
  return 0;
}