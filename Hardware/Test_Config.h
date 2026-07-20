#ifndef __TEST_CONFIG_H
#define __TEST_CONFIG_H

/* ======================== 用户只需修改本文件 ======================== */

/* 电机类型 */
#define TEST_MOTOR_M2006        2006
#define TEST_MOTOR_M3508        3508
#define TEST_MOTOR_GM6020       6020
#define TEST_MOTOR_TYPE         TEST_MOTOR_GM6020

/* GM6020 控制方式：默认使用兼容性更好的电压控制。
 * 电流控制要求 GM6020 固件 >= 1.0.11.2，并在 RoboMaster Assistant 中打开电流环。
 */
#define GM6020_CONTROL_VOLTAGE  0u
#define GM6020_CONTROL_CURRENT  1u
#define GM6020_CONTROL_MODE     GM6020_CONTROL_VOLTAGE

/* 使用 C 板的 CAN1 或 CAN2 接口 */
#define TEST_CAN_BUS            1u

/* 被测设备 ID：M2006/M3508 支持 1~8，GM6020 支持 1~7 */
#define TEST_MOTOR_ID           1u

/* VOFA/串口参数：USART1，PA9(TX)、PB7(RX)，115200-8-N-1 */
#define VOFA_BAUDRATE           115200u
#define VOFA_SEND_PERIOD_MS     50u

/* 控制与保护参数 */
#define MOTOR_CONTROL_PERIOD_MS 10u
#define FEEDBACK_TIMEOUT_MS     100u
#define MANUAL_TIMEOUT_MS       30000u
#define OVER_TEMPERATURE_C      85u

/* 自动测试流程：静止 -> 正转 -> 静止 -> 反转 -> 结束 */
#define AUTO_PREPARE_MS         1000u
#define AUTO_FORWARD_MS         2500u
#define AUTO_ZERO_MS            1000u
#define AUTO_REVERSE_MS         2500u
#define AUTO_FINISH_MS          500u
#define AUTO_MIN_SPEED_RPM      30

#if TEST_MOTOR_TYPE == TEST_MOTOR_M2006
    #define TEST_MOTOR_NAME             "M2006+C610"
    #define MOTOR_MAX_ID                8u
    #define MOTOR_COMMAND_ID_1_TO_4     0x200u
    #define MOTOR_COMMAND_ID_5_TO_8     0x1FFu
    #define MOTOR_FEEDBACK_BASE_ID      0x200u

    #define MOTOR_COMMAND_FULL_RAW      10000
    #define MOTOR_COMMAND_DISPLAY_FULL  10.0f
    #define MOTOR_COMMAND_DISPLAY_UNIT  "A"
    #define MOTOR_COMMAND_IS_CURRENT    1

    #define MOTOR_FEEDBACK_FULL_RAW     10000
    #define MOTOR_FEEDBACK_FULL_AMP     10.0f
    #define MOTOR_SAFE_MAX_RAW          3000
    #define MOTOR_DEFAULT_COMMAND_RAW   1200
    #define MOTOR_AUTO_COMMAND_RAW      1500
    #define MOTOR_COMMAND_STEP_RAW      200
    #define MOTOR_HAS_TEMPERATURE       0

#elif TEST_MOTOR_TYPE == TEST_MOTOR_M3508
    #define TEST_MOTOR_NAME             "M3508+C620"
    #define MOTOR_MAX_ID                8u
    #define MOTOR_COMMAND_ID_1_TO_4     0x200u
    #define MOTOR_COMMAND_ID_5_TO_8     0x1FFu
    #define MOTOR_FEEDBACK_BASE_ID      0x200u

    #define MOTOR_COMMAND_FULL_RAW      16384
    #define MOTOR_COMMAND_DISPLAY_FULL  20.0f
    #define MOTOR_COMMAND_DISPLAY_UNIT  "A"
    #define MOTOR_COMMAND_IS_CURRENT    1

    #define MOTOR_FEEDBACK_FULL_RAW     16384
    #define MOTOR_FEEDBACK_FULL_AMP     20.0f
    #define MOTOR_SAFE_MAX_RAW          4000
    #define MOTOR_DEFAULT_COMMAND_RAW   2000
    #define MOTOR_AUTO_COMMAND_RAW      2500
    #define MOTOR_COMMAND_STEP_RAW      400
    #define MOTOR_HAS_TEMPERATURE       1

#elif TEST_MOTOR_TYPE == TEST_MOTOR_GM6020
    #define MOTOR_MAX_ID                7u
    #define MOTOR_FEEDBACK_BASE_ID      0x204u
    #define MOTOR_FEEDBACK_FULL_RAW     16384
    #define MOTOR_FEEDBACK_FULL_AMP     3.0f
    #define MOTOR_HAS_TEMPERATURE       1

    #if GM6020_CONTROL_MODE == GM6020_CONTROL_VOLTAGE
        #define TEST_MOTOR_NAME             "GM6020(integrated driver,voltage)"
        #define MOTOR_COMMAND_ID_1_TO_4     0x1FFu
        #define MOTOR_COMMAND_ID_5_TO_8     0x2FFu
        #define MOTOR_COMMAND_FULL_RAW      25000
        #define MOTOR_COMMAND_DISPLAY_FULL  100.0f
        #define MOTOR_COMMAND_DISPLAY_UNIT  "%"
        #define MOTOR_COMMAND_IS_CURRENT    0
        #define MOTOR_SAFE_MAX_RAW          6000
        #define MOTOR_DEFAULT_COMMAND_RAW   2500
        #define MOTOR_AUTO_COMMAND_RAW      3500
        #define MOTOR_COMMAND_STEP_RAW      500
    #elif GM6020_CONTROL_MODE == GM6020_CONTROL_CURRENT
        #define TEST_MOTOR_NAME             "GM6020(integrated driver,current)"
        #define MOTOR_COMMAND_ID_1_TO_4     0x1FEu
        #define MOTOR_COMMAND_ID_5_TO_8     0x2FEu
        #define MOTOR_COMMAND_FULL_RAW      16384
        #define MOTOR_COMMAND_DISPLAY_FULL  3.0f
        #define MOTOR_COMMAND_DISPLAY_UNIT  "A"
        #define MOTOR_COMMAND_IS_CURRENT    1
        #define MOTOR_SAFE_MAX_RAW          3000
        #define MOTOR_DEFAULT_COMMAND_RAW   1200
        #define MOTOR_AUTO_COMMAND_RAW      1800
        #define MOTOR_COMMAND_STEP_RAW      200
    #else
        #error "GM6020_CONTROL_MODE must be GM6020_CONTROL_VOLTAGE or GM6020_CONTROL_CURRENT"
    #endif

#else
    #error "TEST_MOTOR_TYPE must be TEST_MOTOR_M2006, TEST_MOTOR_M3508 or TEST_MOTOR_GM6020"
#endif

#if (TEST_MOTOR_ID < 1u) || (TEST_MOTOR_ID > MOTOR_MAX_ID)
    #error "TEST_MOTOR_ID is outside the valid range for the selected motor"
#endif

#if (TEST_CAN_BUS != 1u) && (TEST_CAN_BUS != 2u)
    #error "TEST_CAN_BUS must be 1 or 2"
#endif

#endif
