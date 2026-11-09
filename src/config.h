#pragma once

// ============================================================
//  Which task to build and upload:
//    1 - no debounce (interrupt on FALLING)
//    2 - time-based debounce (ignore < 50 ms, logic outside ISR)
//    3 - state-based debounce (accept only if still pressed)
//    4 - polling every 5 ms + state machine (no interrupts)
//  Task 5 = tasks 1-4 again with the RC filter: set EXTERNAL_PULLUP to 1
// ============================================================
#ifndef TASK
#define TASK 1
#endif

// Button between this pin and GND
#define BUTTON_GPIO GPIO_NUM_18

// 0 - internal pull-up (tasks 1-4)
// 1 - external 10 kOhm pull-up + RC filter (task 5): internal one is switched off
#define EXTERNAL_PULLUP 0

#if EXTERNAL_PULLUP
#define BUTTON_PULLUP GPIO_PULLUP_DISABLE
#else
#define BUTTON_PULLUP GPIO_PULLUP_ENABLE
#endif
