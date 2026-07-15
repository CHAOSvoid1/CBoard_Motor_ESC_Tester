#ifndef __TEST_CONFIG_H
#define __TEST_CONFIG_H

/* ======================== 用户只需修改本文件 ======================== */

/* 电机类型 */
#define TEST_MOTOR_M2006        2006
#define TEST_MOTOR_M3508        3508
#define TEST_MOTOR_TYPE         TEST_MOTOR_M3508

/* 使用 C 板的 CAN1 或 CAN2 接口 */
#define TEST_CAN_BUS            1u

/* 电调 ID，支持 1~8；默认测试 ID=1 */
#define TEST_ESC_ID             1u

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
    #define MOTOR_COMMAND_FULL_RAW      10000
    #define MOTOR_COMMAND_FULL_AMP      10.0f
    #define MOTOR_SAFE_MAX_RAW          3000
    #define MOTOR_DEFAULT_CURRENT_RAW   1200
    #define MOTOR_AUTO_CURRENT_RAW      1500
    #define MOTOR_CURRENT_STEP_RAW      200
    #define MOTOR_HAS_TEMPERATURE       0
#elif TEST_MOTOR_TYPE == TEST_MOTOR_M3508
    #define TEST_MOTOR_NAME             "M3508+C620"
    #define MOTOR_COMMAND_FULL_RAW      16384
    #define MOTOR_COMMAND_FULL_AMP      20.0f
    #define MOTOR_SAFE_MAX_RAW          4000
    #define MOTOR_DEFAULT_CURRENT_RAW   2000
    #define MOTOR_AUTO_CURRENT_RAW      2500
    #define MOTOR_CURRENT_STEP_RAW      400
    #define MOTOR_HAS_TEMPERATURE       1
#else
    #error "TEST_MOTOR_TYPE must be TEST_MOTOR_M2006 or TEST_MOTOR_M3508"
#endif

#if (TEST_ESC_ID < 1u) || (TEST_ESC_ID > 8u)
    #error "TEST_ESC_ID must be 1~8"
#endif

#if (TEST_CAN_BUS != 1u) && (TEST_CAN_BUS != 2u)
    #error "TEST_CAN_BUS must be 1 or 2"
#endif

#endif
