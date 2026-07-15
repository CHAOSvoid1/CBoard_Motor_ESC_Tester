#ifndef __MOTOR_H
#define __MOTOR_H

#include "MyCAN.h"
#include <stdint.h>

typedef enum
{
    MOTOR_TEST_STOPPED = 0,
    MOTOR_TEST_MANUAL = 1,
    MOTOR_TEST_AUTO = 2,
    MOTOR_TEST_DONE = 3,
    MOTOR_TEST_FAULT = 4
} Motor_Test_Mode_t;

typedef enum
{
    MOTOR_RESULT_NOT_RUN = 0,
    MOTOR_RESULT_RUNNING = 1,
    MOTOR_RESULT_PASS = 2,
    MOTOR_RESULT_FAIL_OFFLINE = 3,
    MOTOR_RESULT_FAIL_FORWARD = 4,
    MOTOR_RESULT_FAIL_REVERSE = 5,
    MOTOR_RESULT_FAIL_DIRECTION = 6,
    MOTOR_RESULT_FAIL_OVERTEMP = 7,
    MOTOR_RESULT_ABORTED = 8
} Motor_Test_Result_t;

typedef struct
{
    Motor_Feedback_t feedback;
    int16_t command_raw;
    float command_amp;
    float feedback_current_amp;
    uint32_t rx_frequency_hz;
    uint32_t tx_ok_count;
    uint32_t tx_fail_count;
    uint32_t can_error_status;
    uint32_t mode_elapsed_ms;
    int16_t forward_peak_rpm;
    int16_t reverse_peak_rpm;
    Motor_Test_Mode_t mode;
    Motor_Test_Result_t result;
    uint8_t auto_phase;
} Motor_Test_Status_t;

void Motor_Test_Init(uint32_t now_ms);
void Motor_Test_Update(uint32_t now_ms, const Motor_Feedback_t *feedback);
void Motor_Test_HandleCommand(uint8_t command, uint32_t now_ms, const Motor_Feedback_t *feedback);
int16_t Motor_Test_GetCommandRaw(void);
void Motor_Test_GetStatus(Motor_Test_Status_t *status, const Motor_Feedback_t *feedback,
                          uint32_t now_ms, uint32_t rx_frequency_hz);
const char *Motor_Test_ModeName(Motor_Test_Mode_t mode);
const char *Motor_Test_ResultName(Motor_Test_Result_t result);
void Motor_Test_PrintHelp(void);

#endif
