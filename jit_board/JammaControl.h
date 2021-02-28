// Copyright 2020 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#if !defined(__JammaControl_H__)
# define __JammaControl_H__

#include <stdint.h>

// PA0: 1P up
// PA1: 1P down
// PA2: 1P left
// PA3: 1P right
// PA6: Service

// PB0: 1P Coin
// PB1: 1P Start
// PB2: 1P button1
// PB3: 1P button2
// PB4: 1P button3
// PB5: 1P button4
// PB6: 1P button5
// PB7: 1P button6

// PC0: 2P Coin
// ...
// PC7: 2P button6

// PD3: TEST
// PD4: 2P up
// PD5: 2P down
// PD6: 2P left
// PD7: 2P right
class JammaControl final {
 public:
  JammaControl();

  // player # (0 origin)
  void InsertCoin(uint8_t player);

  void DriveCoin(bool p1, bool p2);

  // TEST     | - ...
  // 1P Start | Service  | 1P up    | 1P down  | 1P left | 1P right | 1P push1 | 1P push2
  // 1P push3 | 1P push4 | 1P push5 | 1P push6 | -       | -        | -        | -
  // 2P Start | -        | 2P up    | 2P down  | 2P left | 2P right | 2P push1 | 2P push2
  // 2P push3 | 2P push4 | 2P push5 | 2P push6 | -       | -        | -        | -
  void Update(const uint8_t bytes[5]);
};

#endif  // __JammaControl_H__