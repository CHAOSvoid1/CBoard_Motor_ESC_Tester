#include "Motor.h"
#include "MyCAN.h"
#include "Test_Config.h"
#include "Usart.h"
#include <stdlib.h>

static Motor_Test_Mode_t test_mode = MOTOR_TEST_STOPPED;
static Motor_Test_Result_t test_result = MOTOR_RESULT_NOT_RUN;
static int16_t manual_current_raw = MOTOR_DEFAULT_CURRENT_RAW;
static int16_t output_current_raw = 0;
static uint32_t mode_start_ms = 0u;
static uint32_t last_manual_command_ms = 0u;
static uint8_t auto_phase = 0u;
static int16_t forward_peak_rpm = 0;
static int16_t reverse_peak_rpm = 0;
static uint8_t auto_seen_feedback = 0u;

static int16_t ClampCurrent(int32_t current)
{
    if (current > MOTOR_SAFE_MAX_RAW) {
        current = MOTOR_SAFE_MAX_RAW;
    } else if (current < -MOTOR_SAFE_MAX_RAW) {
        current = -MOTOR_SAFE_MAX_RAW;
    }
    return (int16_t)current;
}

static float RawToAmp(int16_t raw)
{
    return ((float)raw * MOTOR_COMMAND_FULL_AMP) / (float)MOTOR_COMMAND_FULL_RAW;
}

static int32_t RawToMilliAmp(int16_t raw)
{
    return ((int32_t)raw * (int32_t)(MOTOR_COMMAND_FULL_AMP * 1000.0f)) /
           MOTOR_COMMAND_FULL_RAW;
}

static void StopMotor(Motor_Test_Result_t result, uint32_t now_ms)
{
    output_current_raw = 0;
    test_mode = (result == MOTOR_RESULT_ABORTED || result == MOTOR_RESULT_NOT_RUN) ?
                MOTOR_TEST_STOPPED : MOTOR_TEST_DONE;
    test_result = result;
    mode_start_ms = now_ms;
    auto_phase = 0u;
}

void Motor_Test_Init(uint32_t now_ms)
{
    test_mode = MOTOR_TEST_STOPPED;
    test_result = MOTOR_RESULT_NOT_RUN;
    manual_current_raw = MOTOR_DEFAULT_CURRENT_RAW;
    output_current_raw = 0;
    mode_start_ms = now_ms;
    last_manual_command_ms = now_ms;
    auto_phase = 0u;
    forward_peak_rpm = 0;
    reverse_peak_rpm = 0;
    auto_seen_feedback = 0u;
}

void Motor_Test_Update(uint32_t now_ms, const Motor_Feedback_t *feedback)
{
    uint32_t elapsed;

    if (feedback == 0) {
        output_current_raw = 0;
        return;
    }

#if MOTOR_HAS_TEMPERATURE
    if (feedback->online && feedback->temperature_c >= OVER_TEMPERATURE_C) {
        output_current_raw = 0;
        if (test_result != MOTOR_RESULT_FAIL_OVERTEMP) {
            Serial_Printf("event:over_temperature_stop,temp=%u\r\n",
                          (unsigned int)feedback->temperature_c);
        }
        test_mode = MOTOR_TEST_FAULT;
        test_result = MOTOR_RESULT_FAIL_OVERTEMP;
        return;
    }
#endif

    if (!feedback->online) {
        output_current_raw = 0;

        /* 运行过程中反馈丢失后不自动恢复输出，避免重连瞬间突然转动。 */
        if (test_mode == MOTOR_TEST_MANUAL) {
            StopMotor(MOTOR_RESULT_ABORTED, now_ms);
            Serial_Printf("event:feedback_lost_stop\r\n");
        } else if (test_mode == MOTOR_TEST_AUTO &&
                   (uint32_t)(now_ms - mode_start_ms) > AUTO_PREPARE_MS) {
            StopMotor(MOTOR_RESULT_FAIL_OFFLINE, now_ms);
            Serial_Printf("event:auto_done,result=FAIL_OFFLINE\r\n");
        }
        return;
    }

    if (test_mode == MOTOR_TEST_MANUAL) {
        if ((uint32_t)(now_ms - last_manual_command_ms) >= MANUAL_TIMEOUT_MS) {
            StopMotor(MOTOR_RESULT_ABORTED, now_ms);
            Serial_Printf("event:manual_timeout_stop\r\n");
        } else {
            output_current_raw = manual_current_raw;
        }
        return;
    }

    if (test_mode != MOTOR_TEST_AUTO) {
        output_current_raw = 0;
        return;
    }

    auto_seen_feedback = 1u;
    elapsed = (uint32_t)(now_ms - mode_start_ms);

    if (elapsed < AUTO_PREPARE_MS) {
        auto_phase = 0u;
        output_current_raw = 0;
    } else if (elapsed < AUTO_PREPARE_MS + AUTO_FORWARD_MS) {
        auto_phase = 1u;
        output_current_raw = MOTOR_AUTO_CURRENT_RAW;
        if (abs(feedback->speed_rpm) > abs(forward_peak_rpm)) {
            forward_peak_rpm = feedback->speed_rpm;
        }
    } else if (elapsed < AUTO_PREPARE_MS + AUTO_FORWARD_MS + AUTO_ZERO_MS) {
        auto_phase = 2u;
        output_current_raw = 0;
    } else if (elapsed < AUTO_PREPARE_MS + AUTO_FORWARD_MS + AUTO_ZERO_MS + AUTO_REVERSE_MS) {
        auto_phase = 3u;
        output_current_raw = -MOTOR_AUTO_CURRENT_RAW;
        if (abs(feedback->speed_rpm) > abs(reverse_peak_rpm)) {
            reverse_peak_rpm = feedback->speed_rpm;
        }
    } else if (elapsed < AUTO_PREPARE_MS + AUTO_FORWARD_MS + AUTO_ZERO_MS +
                         AUTO_REVERSE_MS + AUTO_FINISH_MS) {
        auto_phase = 4u;
        output_current_raw = 0;
    } else {
        output_current_raw = 0;
        test_mode = MOTOR_TEST_DONE;
        auto_phase = 5u;

        if (!auto_seen_feedback) {
            test_result = MOTOR_RESULT_FAIL_OFFLINE;
        } else if (abs(forward_peak_rpm) < AUTO_MIN_SPEED_RPM) {
            test_result = MOTOR_RESULT_FAIL_FORWARD;
        } else if (abs(reverse_peak_rpm) < AUTO_MIN_SPEED_RPM) {
            test_result = MOTOR_RESULT_FAIL_REVERSE;
        } else if ((int32_t)forward_peak_rpm * (int32_t)reverse_peak_rpm >= 0) {
            test_result = MOTOR_RESULT_FAIL_DIRECTION;
        } else {
            test_result = MOTOR_RESULT_PASS;
        }

        Serial_Printf("event:auto_done,result=%s,forward_peak=%d,reverse_peak=%d\r\n",
                      Motor_Test_ResultName(test_result), forward_peak_rpm, reverse_peak_rpm);
    }
}

void Motor_Test_HandleCommand(uint8_t command, uint32_t now_ms, const Motor_Feedback_t *feedback)
{
    switch (command) {
        case 'a':
        case 'A':
            test_mode = MOTOR_TEST_AUTO;
            test_result = MOTOR_RESULT_RUNNING;
            mode_start_ms = now_ms;
            auto_phase = 0u;
            forward_peak_rpm = 0;
            reverse_peak_rpm = 0;
            auto_seen_feedback = (feedback != 0 && feedback->online) ? 1u : 0u;
            output_current_raw = 0;
            Serial_Printf("event:auto_start,current_raw=%d\r\n", MOTOR_AUTO_CURRENT_RAW);
            break;

        case 's':
        case 'S':
            test_mode = MOTOR_TEST_MANUAL;
            test_result = MOTOR_RESULT_RUNNING;
            mode_start_ms = now_ms;
            last_manual_command_ms = now_ms;
            output_current_raw = (feedback != 0 && feedback->online) ? manual_current_raw : 0;
            Serial_Printf("event:manual_start,current_raw=%d\r\n", manual_current_raw);
            break;

        case 'x':
        case 'X':
        case '0':
            StopMotor(MOTOR_RESULT_ABORTED, now_ms);
            Serial_Printf("event:stop\r\n");
            break;

        case '+':
            if (manual_current_raw >= 0) {
                manual_current_raw = ClampCurrent((int32_t)manual_current_raw + MOTOR_CURRENT_STEP_RAW);
            } else {
                manual_current_raw = ClampCurrent((int32_t)manual_current_raw - MOTOR_CURRENT_STEP_RAW);
            }
            last_manual_command_ms = now_ms;
            Serial_Printf("event:set_manual_current,raw=%d,current_mA=%ld\r\n",
                          manual_current_raw, (long)RawToMilliAmp(manual_current_raw));
            break;

        case '-':
            if (manual_current_raw > 0) {
                manual_current_raw = ClampCurrent((int32_t)manual_current_raw - MOTOR_CURRENT_STEP_RAW);
                if (manual_current_raw < 0) manual_current_raw = 0;
            } else if (manual_current_raw < 0) {
                manual_current_raw = ClampCurrent((int32_t)manual_current_raw + MOTOR_CURRENT_STEP_RAW);
                if (manual_current_raw > 0) manual_current_raw = 0;
            }
            last_manual_command_ms = now_ms;
            Serial_Printf("event:set_manual_current,raw=%d,current_mA=%ld\r\n",
                          manual_current_raw, (long)RawToMilliAmp(manual_current_raw));
            break;

        case 'r':
        case 'R':
            manual_current_raw = (int16_t)-manual_current_raw;
            last_manual_command_ms = now_ms;
            Serial_Printf("event:reverse,raw=%d\r\n", manual_current_raw);
            break;

        case 'f':
        case 'F':
            if (manual_current_raw < 0) manual_current_raw = (int16_t)-manual_current_raw;
            last_manual_command_ms = now_ms;
            Serial_Printf("event:forward,raw=%d\r\n", manual_current_raw);
            break;

        case 'b':
        case 'B':
            if (manual_current_raw > 0) manual_current_raw = (int16_t)-manual_current_raw;
            last_manual_command_ms = now_ms;
            Serial_Printf("event:backward,raw=%d\r\n", manual_current_raw);
            break;

        case '?':
        case 'h':
        case 'H':
            Motor_Test_PrintHelp();
            break;

        case '\r':
        case '\n':
        case ' ':
            break;

        default:
            Serial_Printf("event:unknown_command,ascii=%u\r\n", command);
            break;
    }
}

int16_t Motor_Test_GetCommandRaw(void)
{
    return ClampCurrent(output_current_raw);
}

void Motor_Test_GetStatus(Motor_Test_Status_t *status, const Motor_Feedback_t *feedback,
                          uint32_t now_ms, uint32_t rx_frequency_hz)
{
    if (status == 0 || feedback == 0) {
        return;
    }

    status->feedback = *feedback;
    status->command_raw = Motor_Test_GetCommandRaw();
    status->command_amp = RawToAmp(status->command_raw);
    status->feedback_current_amp = RawToAmp(feedback->current_raw);
    status->rx_frequency_hz = rx_frequency_hz;
    status->tx_ok_count = MyCAN_GetTxOkCount();
    status->tx_fail_count = MyCAN_GetTxFailCount();
    status->can_error_status = MyCAN_GetErrorStatus();
    status->mode_elapsed_ms = (uint32_t)(now_ms - mode_start_ms);
    status->forward_peak_rpm = forward_peak_rpm;
    status->reverse_peak_rpm = reverse_peak_rpm;
    status->mode = test_mode;
    status->result = test_result;
    status->auto_phase = auto_phase;
}

const char *Motor_Test_ModeName(Motor_Test_Mode_t mode)
{
    switch (mode) {
        case MOTOR_TEST_STOPPED: return "STOPPED";
        case MOTOR_TEST_MANUAL: return "MANUAL";
        case MOTOR_TEST_AUTO: return "AUTO";
        case MOTOR_TEST_DONE: return "DONE";
        case MOTOR_TEST_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

const char *Motor_Test_ResultName(Motor_Test_Result_t result)
{
    switch (result) {
        case MOTOR_RESULT_NOT_RUN: return "NOT_RUN";
        case MOTOR_RESULT_RUNNING: return "RUNNING";
        case MOTOR_RESULT_PASS: return "PASS";
        case MOTOR_RESULT_FAIL_OFFLINE: return "FAIL_OFFLINE";
        case MOTOR_RESULT_FAIL_FORWARD: return "FAIL_FORWARD";
        case MOTOR_RESULT_FAIL_REVERSE: return "FAIL_REVERSE";
        case MOTOR_RESULT_FAIL_DIRECTION: return "FAIL_DIRECTION";
        case MOTOR_RESULT_FAIL_OVERTEMP: return "FAIL_OVERTEMP";
        case MOTOR_RESULT_ABORTED: return "ABORTED";
        default: return "UNKNOWN";
    }
}

void Motor_Test_PrintHelp(void)
{
    Serial_Printf("\r\n=== C Board RM Motor/ESC Tester ===\r\n");
    Serial_Printf("Motor=%s, CAN%u, ESC_ID=%u, safe_limit=%d raw\r\n",
                  TEST_MOTOR_NAME, (unsigned int)TEST_CAN_BUS,
                  (unsigned int)TEST_ESC_ID, MOTOR_SAFE_MAX_RAW);
    Serial_Printf("A: automatic forward/reverse test\r\n");
    Serial_Printf("S: manual start    X or 0: emergency stop\r\n");
    Serial_Printf("+: increase current    -: decrease current\r\n");
    Serial_Printf("R: reverse direction   F: forward   B: backward\r\n");
    Serial_Printf("H or ?: show help\r\n");
    Serial_Printf("VOFA FireWater line: motor_test:...\r\n\r\n");
}
