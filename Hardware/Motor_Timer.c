#include "stm32f4xx.h"
#include "Motor_Timer.h"

static volatile uint32_t motor_test_ms_tick = 0u;

void Motor_Timer_Init(void)
{
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef nvic;

    /* APB1=42 MHz，TIM2 时钟=84 MHz。84 MHz/(8400*10)=1 kHz，即 1 ms。 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 8400u - 1u;
    timer.TIM_Period = 10u - 1u;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &timer);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM2, ENABLE);
}

uint32_t Motor_Timer_GetTick(void)
{
    return motor_test_ms_tick;
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        motor_test_ms_tick++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
