#ifndef __MYCAN_H
#define __MYCAN_H

#include "stm32f4xx.h"
#include <stdint.h>

typedef struct
{
    uint16_t angle_raw;
    int16_t speed_rpm;
    int16_t current_raw;
    uint8_t temperature_c;
    uint8_t online;
    uint32_t rx_count;
    uint32_t last_rx_ms;
    uint32_t last_std_id;
} Motor_Feedback_t;

void MyCAN_Init(void);
uint8_t MyCAN_SendMotorCommand(uint8_t motor_id, int16_t command_raw);
void MyCAN_GetMotorFeedback(Motor_Feedback_t *feedback, uint32_t now_ms);
uint32_t MyCAN_GetTxOkCount(void);
uint32_t MyCAN_GetTxFailCount(void);
uint32_t MyCAN_GetErrorStatus(void);

#endif
