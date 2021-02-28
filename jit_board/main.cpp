// Copyright 2020 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "Arduino.h"
#include "JammaControl.h"
#include "JVSIOClient.h"

namespace {
JVSIODataClient data;
JVSIOSenseClient sense;
JVSIO::LedClient led;
JVSIO io(&data, &sense, &led);
JammaControl jamma;
}

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
  for (int i = 0; i < 5; ++i)
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

bool SmokeTest() {
  const uint8_t kRequestIoId[] = { 0x01, 0x02, JVSIO::kCmdIoId };
  const uint8_t kRequestCommandRev[] = { 0x01, 0x02, JVSIO::kCmdCommandRev };
  const uint8_t kRequestJvRev[] = { 0x01, 0x02, JVSIO::kCmdJvRev };
  const uint8_t kRequestProtocolRev[] = { 0x01, 0x02, JVSIO::kCmdProtocolVer };
  const uint8_t kRequestFunctionCheck[] = { 0x01, 0x02, JVSIO::kCmdFunctionCheck };
  uint8_t* ack;
  uint8_t ack_len;
  bool result = io.sendAndReceive(kRequestIoId, &ack, &ack_len);
  if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
    return false;
  Serial.print("I/O ID: ");
  Serial.println((char*)&ack[2]);

  result = io.sendAndReceive(kRequestCommandRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return false;
  Serial.print("Command Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestJvRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return false;
  Serial.print("JV Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestProtocolRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return false;
  Serial.print("Protocol Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestFunctionCheck, &ack, &ack_len);
  if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
    return false;
  Serial.println("Functions:");
  for (uint8_t i = 2; i < ack_len; i += 4) {
    switch (ack[i]) {
      case 0x00:
        break;
      case 0x01:
        Serial.print(" - SW: ");
        players = ack[i + 1];
        Serial.print(players, DEC);
        Serial.print(" players, ");
        buttons = ack[i + 2];
        Serial.print(buttons, DEC);
        Serial.println(" buttons");
        button_bytes = players * ((buttons + 7) >> 3) + 1;
        break;
      case 0x02:
        Serial.print(" - COIN: ");
        coin_slots = ack[i + 1];
        Serial.print(coin_slots, DEC);
        Serial.println(" slots");
        break;
      case 0x03:
        Serial.print(" - ANALOG: ");
        Serial.print(ack[i + 1], DEC);
        Serial.print(" channels, ");
        Serial.print(ack[i + 2], DEC);
        Serial.println(" bits");
        break;
      case 0x12:
        Serial.print(" - DRIVER: ");
        Serial.print(ack[i + 1], DEC);
        Serial.println(" slots");
        break;
      default:
        Serial.print("Other ");
        Serial.println(ack[i], HEX);
        break;
    }
  }
  uint8_t sws[button_bytes];
  if (!GetButtons(sws))
    return false;
  Serial.print("SwInput:");
  for (uint8_t i = 0; i < button_bytes; ++i) {
    Serial.print(" ");
    if (sws[i] < 10)
      Serial.print("0");
    Serial.print(sws[i], HEX);
  }
  Serial.println("");

  uint16_t p1;
  uint16_t p2;
  if (!GetCoins(p1, p2))
    return false;
  Serial.print("CoinInput: ");
  Serial.print(p1, DEC);
  Serial.print(", ");
  Serial.println(p2, DEC);
  return true;
}

bool CoinTest() {
  uint16_t p1;
  uint16_t p2;
  if (!GetCoins(p1, p2) || p1 != 0 || p2 != 0) {
    Serial.println("CoinTest: initial state error");
    Serial.print("  p1: ");
    Serial.println(p1, DEC);
    Serial.print("  p2: ");
    Serial.println(p2, DEC);
    return false;
  }
  jamma.InsertCoin(0);
  uint8_t sws[button_bytes];
  GetButtons(sws);  // sync for coin signal neg edge detection on IONA-JS
  GetButtons(sws);  // sync for coin signal pos edge detection on IONA-JS
  if (!GetCoins(p1, p2) || p1 != 1 || p2 != 0) {
    Serial.println("CoinTest: P1 increment fail");
    return false;
  }
  if (!SubCoin(0) || !GetCoins(p1, p2) || p1 != 0 || p2 != 0) {
    Serial.println("CoinTest: P1 decrement fail");
    return false;
  }
  jamma.InsertCoin(1);
  GetButtons(sws);
  GetButtons(sws);
  if (!GetCoins(p1, p2) || p1 != 0 || p2 != 1) {
    Serial.println("CoinTest: P2 increment fail");
    return false;
  }
  if (!SubCoin(1) || !GetCoins(p1, p2) || p1 != 0 || p2 != 0) {
    Serial.println("CoinTest: P2 decrement fail");
    return false;
  }
  return true;
}

bool ButtonTest() {
  const uint8_t tests[][5] = {
    { 0x80, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x80, 0x00, 0x00, 0x00 },
    { 0x00, 0x40, 0x00, 0x00, 0x00 },
    { 0x00, 0x20, 0x00, 0x00, 0x00 },
    { 0x00, 0x10, 0x00, 0x00, 0x00 },
    { 0x00, 0x08, 0x00, 0x00, 0x00 },
    { 0x00, 0x04, 0x00, 0x00, 0x00 },
    { 0x00, 0x02, 0x00, 0x00, 0x00 },
    { 0x00, 0x01, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x00, 0x40, 0x00, 0x00 },
    { 0x00, 0x00, 0x20, 0x00, 0x00 },
    { 0x00, 0x00, 0x10, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x00, 0x00, 0x20, 0x00 },
    { 0x00, 0x00, 0x00, 0x10, 0x00 },
    { 0x00, 0x00, 0x00, 0x08, 0x00 },
    { 0x00, 0x00, 0x00, 0x04, 0x00 },
    { 0x00, 0x00, 0x00, 0x02, 0x00 },
    { 0x00, 0x00, 0x00, 0x01, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x80 },
    { 0x00, 0x00, 0x00, 0x00, 0x40 },
    { 0x00, 0x00, 0x00, 0x00, 0x20 },
    { 0x00, 0x00, 0x00, 0x00, 0x10 },
    { 0xff, 0xff, 0xff, 0xff, 0xff }
  };
  uint8_t sws[5];
  uint8_t failure = 0;
  for (int i = 0; ; ++i) {
    if (tests[i][0] == 0xff)
      break;
    jamma.Update(tests[i]);
    delay(1);
    if (!GetButtons(sws))
      return false;
    for (int j = 0; j < 5; ++j) {
      if (tests[i][j] != sws[j]) {
        Serial.print("ButtonTest: fail on ");
        Serial.print(i, DEC);
        Serial.print(", ");
        Serial.print(j, DEC);
        Serial.print("; ");
        Serial.print(tests[i][j], HEX);
        Serial.print(" != ");
        Serial.println(sws[j], HEX);
        failure++;
      }
    }
    Serial.print("ButtonTest: ");
    Serial.print(failure, DEC);
    Serial.println(" failure(s)");
  }
  return 0 == failure;
}

int main() {
  Serial.println("JVS IO Tester - JIT : boot ok");

  io.begin();
  for (;;) {
    io.boot();
    Serial.println("Bus Ready");
    uint8_t error = 0;
    if (!SmokeTest())
      error++;
    if (!CoinTest())
      error++;
    if (!ButtonTest())
      error++;
    if (error) {
      Serial.print("ERROR# ");
      Serial.println(error, DEC);
    }
  }
  return 0;
}

