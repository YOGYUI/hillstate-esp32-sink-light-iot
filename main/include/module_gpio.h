#ifndef _MODULE_GPIO_H_
#define _MODULE_GPIO_H_
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool initialize_gpio();
bool set_ctrl_level(uint32_t level);
bool toggle_ctrl_signal();
int get_state_level();

#ifdef __cplusplus
}
#endif
#endif