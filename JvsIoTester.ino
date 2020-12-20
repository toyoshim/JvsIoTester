// An Arduino example for Nano to implement JVS HOST with jvsio library.
// The host mode support in the jvsio library is still experimental.
#include "jvsio/JVSIO.cpp"

#define NANO

#if defined(NANO)
# include "jvsio/clients/NanoClient.h"
// A0: JVS SENSE IN
// RX: JVS D+
// D2: JVS D-
NanoHostSenseClient nano_sense;
JVSIO::SenseClient& sense = nano_sense;
NanoDataClient data;
JVSIO::LedClient led;
#else
# error "not supported device."
#endif

void setup() {
  delay(2000);
  Serial.begin(115200);
}

void loop() {
  static JVSIO io(&data, &sense, &led);
  io.boot();
  Serial.println("ready");
  const uint8_t kRequestIoId[] = { 0x01, 0x02, JVSIO::kCmdIoId };
  const uint8_t kRequestCommandRev[] = { 0x01, 0x02, JVSIO::kCmdCommandRev };
  const uint8_t kRequestJvRev[] = { 0x01, 0x02, JVSIO::kCmdJvRev };
  const uint8_t kRequestProtocolRev[] = { 0x01, 0x02, JVSIO::kCmdProtocolVer };
  const uint8_t kRequestFunctionCheck[] = { 0x01, 0x02, JVSIO::kCmdFunctionCheck };
  uint8_t requestSwInput[] = { 0x01, 0x04, JVSIO::kCmdSwInput, 0, 0 };
  uint8_t requestCoinInput[] = { 0x01, 0x03, JVSIO::kCmdCoinInput, 0 };
  uint8_t* ack;
  uint8_t ack_len;
  bool result = io.sendAndReceive(kRequestIoId, &ack, &ack_len);
  if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("I/O ID: ");
  Serial.println((char*)&ack[2]);

  result = io.sendAndReceive(kRequestCommandRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("Command Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestJvRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("JV Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestProtocolRev, &ack, &ack_len);
  if (!result || ack_len != 3 || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("Protocol Rev: ");
  Serial.println(ack[2], HEX);

  result = io.sendAndReceive(kRequestFunctionCheck, &ack, &ack_len);
  if (!result || ack_len < 3 || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.println("Functions:");
  uint8_t players = 0;
  uint8_t buttons = 0;
  uint8_t coin_slots = 0;
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
  requestSwInput[3] = players;
  requestSwInput[4] = (buttons + 7) >> 3;
  result = io.sendAndReceive(requestSwInput, &ack, &ack_len);
  uint8_t button_bytes = requestSwInput[3] * requestSwInput[4] + 1;
  if (!result || ack_len != 2 + button_bytes || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("SwInput:");
  for (uint8_t i = 0; i < button_bytes; ++i) {
    Serial.print(" ");
    if (ack[2 + i] < 10)
      Serial.print("0");
    Serial.print(ack[2 + i], HEX);
  }
  Serial.println("");

  requestCoinInput[3] = coin_slots;
  uint8_t coin_bytes = coin_slots * 2;
  result = io.sendAndReceive(requestCoinInput, &ack, &ack_len);
  if (!result || ack_len != 2 + coin_bytes || ack[0] != 1 || ack[1] != 1)
    return;
  Serial.print("CoinInput:");
  for (uint8_t i = 0; i < coin_bytes; ++i) {
    Serial.print(" ");
    if (ack[2 + i] < 10)
      Serial.print("0");
    Serial.print(ack[2 + i], HEX);
  }
  Serial.println("");
}
