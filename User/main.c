#include "stm32f4xx.h"
#include "Usart.h"
#include "MyCAN.h"
#include "Motor.h"
#include "Motor_Timer.h"
#include "Vofa.h"
#include "Test_Config.h"

static uint32_t last_control_ms = 0u;
static uint32_t last_vofa_ms = 0u;
static uint32_t last_rx_rate_ms = 0u;
static uint32_t last_rx_count = 0u;
static uint32_t rx_frequency_hz = 0u;

int main(void)
{
    Motor_Feedback_t feedback;
    Motor_Test_Status_t status;
    uint32_t now_ms;
    uint8_t command;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    Serial_Init();
    Motor_Timer_Init();
    MyCAN_Init();
    Vofa_Init();

    now_ms = Motor_Timer_GetTick();
    Motor_Test_Init(now_ms);
    Motor_Test_PrintHelp();

    while (1) {
        now_ms = Motor_Timer_GetTick();
        MyCAN_GetMotorFeedback(&feedback, now_ms);

        while (Serial_ReadByte(&command)) {
            Motor_Test_HandleCommand(command, now_ms, &feedback);
        }

        Motor_Test_Update(now_ms, &feedback);

        if ((uint32_t)(now_ms - last_control_ms) >= MOTOR_CONTROL_PERIOD_MS) {
            last_control_ms = now_ms;
            MyCAN_SendMotorCommand(TEST_MOTOR_ID, Motor_Test_GetCommandRaw());
        }

        if ((uint32_t)(now_ms - last_rx_rate_ms) >= 1000u) {
            uint32_t elapsed = (uint32_t)(now_ms - last_rx_rate_ms);
            uint32_t delta = feedback.rx_count - last_rx_count;
            rx_frequency_hz = (elapsed > 0u) ? (delta * 1000u / elapsed) : 0u;
            last_rx_count = feedback.rx_count;
            last_rx_rate_ms = now_ms;
        }

        if ((uint32_t)(now_ms - last_vofa_ms) >= VOFA_SEND_PERIOD_MS) {
            last_vofa_ms = now_ms;
            Motor_Test_GetStatus(&status, &feedback, now_ms, rx_frequency_hz);
            Vofa_SendMotorStatus(&status);
        }
    }
}
