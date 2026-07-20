#include "stm32f4xx.h"
#include "MyCAN.h"
#include "Motor_Timer.h"
#include "Test_Config.h"

static volatile Motor_Feedback_t motor_feedback;
static volatile uint32_t can_tx_ok_count = 0u;
static volatile uint32_t can_tx_fail_count = 0u;

static CAN_TypeDef *TestCAN(void)
{
#if TEST_CAN_BUS == 1u
    return CAN1;
#else
    return CAN2;
#endif
}

static void MyCAN_GPIO_Config(void)
{
    GPIO_InitTypeDef gpio;

#if TEST_CAN_BUS == 1u
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &gpio);
#else
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
#endif
}

static void MyCAN_Filter_Config(void)
{
    CAN_FilterInitTypeDef filter;

    filter.CAN_FilterIdHigh = 0u;
    filter.CAN_FilterIdLow = 0u;
    filter.CAN_FilterMaskIdHigh = 0u;
    filter.CAN_FilterMaskIdLow = 0u;
    filter.CAN_FilterMode = CAN_FilterMode_IdMask;
    filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterActivation = ENABLE;

#if TEST_CAN_BUS == 1u
    filter.CAN_FilterNumber = 0u;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
#else
    filter.CAN_FilterNumber = 14u;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO1;
#endif
    CAN_FilterInit(&filter);
}

void MyCAN_Init(void)
{
    CAN_InitTypeDef can;
    NVIC_InitTypeDef nvic;

    /* CAN2 依赖 CAN1 时钟，因此两者时钟都打开。 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
#if TEST_CAN_BUS == 2u
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
#endif

    MyCAN_GPIO_Config();

    /* STM32F407 的 CAN1/CAN2 共用过滤器组，14~27 分配给 CAN2。 */
    CAN_SlaveStartBank(14u);
    CAN_DeInit(TestCAN());
    CAN_StructInit(&can);
    can.CAN_TTCM = DISABLE;
    can.CAN_ABOM = ENABLE;
    can.CAN_AWUM = ENABLE;
    can.CAN_NART = DISABLE;
    can.CAN_RFLM = DISABLE;
    can.CAN_TXFP = DISABLE;
    can.CAN_Mode = CAN_Mode_Normal;

    /* APB1=42 MHz：42 MHz/(3*(1+10+3)) = 1 Mbps。 */
    can.CAN_SJW = CAN_SJW_1tq;
    can.CAN_BS1 = CAN_BS1_10tq;
    can.CAN_BS2 = CAN_BS2_3tq;
    can.CAN_Prescaler = 3u;
    CAN_Init(TestCAN(), &can);

    MyCAN_Filter_Config();

#if TEST_CAN_BUS == 1u
    nvic.NVIC_IRQChannel = CAN1_RX0_IRQn;
#else
    nvic.NVIC_IRQChannel = CAN2_RX1_IRQn;
#endif
    nvic.NVIC_IRQChannelPreemptionPriority = 0u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

#if TEST_CAN_BUS == 1u
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
#else
    CAN_ITConfig(CAN2, CAN_IT_FMP1, ENABLE);
#endif
}

uint8_t MyCAN_SendMotorCommand(uint8_t motor_id, int16_t command_raw)
{
    CanTxMsg tx;
    uint8_t mailbox;
    uint8_t status;
    uint32_t timeout;
    uint8_t offset;
    uint8_t i;

    if (motor_id < 1u || motor_id > MOTOR_MAX_ID) {
        can_tx_fail_count++;
        return 0u;
    }

    tx.StdId = (motor_id <= 4u) ? MOTOR_COMMAND_ID_1_TO_4 : MOTOR_COMMAND_ID_5_TO_8;
    tx.ExtId = 0u;
    tx.IDE = CAN_Id_Standard;
    tx.RTR = CAN_RTR_Data;
    tx.DLC = 8u;
    for (i = 0u; i < 8u; ++i) {
        tx.Data[i] = 0u;
    }

    offset = (uint8_t)(((motor_id - 1u) % 4u) * 2u);
    tx.Data[offset] = (uint8_t)((uint16_t)command_raw >> 8);
    tx.Data[offset + 1u] = (uint8_t)command_raw;

    mailbox = CAN_Transmit(TestCAN(), &tx);
    if (mailbox == CAN_TxStatus_NoMailBox) {
        can_tx_fail_count++;
        return 0u;
    }

    for (timeout = 0u; timeout < 10000u; ++timeout) {
        status = CAN_TransmitStatus(TestCAN(), mailbox);
        if (status == CAN_TxStatus_Ok) {
            can_tx_ok_count++;
            return 1u;
        }
        if (status == CAN_TxStatus_Failed) {
            break;
        }
    }

    can_tx_fail_count++;
    return 0u;
}

static void MyCAN_ParseFeedback(const CanRxMsg *rx)
{
    const uint32_t expected_id = MOTOR_FEEDBACK_BASE_ID + TEST_MOTOR_ID;

    if (rx == 0 || rx->IDE != CAN_Id_Standard || rx->RTR != CAN_RTR_Data ||
        rx->DLC < 7u || rx->StdId != expected_id) {
        return;
    }

    motor_feedback.angle_raw = (uint16_t)(((uint16_t)rx->Data[0] << 8) | rx->Data[1]);
    motor_feedback.speed_rpm = (int16_t)(((uint16_t)rx->Data[2] << 8) | rx->Data[3]);
    motor_feedback.current_raw = (int16_t)(((uint16_t)rx->Data[4] << 8) | rx->Data[5]);
#if MOTOR_HAS_TEMPERATURE
    motor_feedback.temperature_c = rx->Data[6];
#else
    motor_feedback.temperature_c = 0u;
#endif
    motor_feedback.rx_count++;
    motor_feedback.last_rx_ms = Motor_Timer_GetTick();
    motor_feedback.last_std_id = rx->StdId;
}

void MyCAN_GetMotorFeedback(Motor_Feedback_t *feedback, uint32_t now_ms)
{
    uint32_t primask;

    if (feedback == 0) {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    *feedback = motor_feedback;
    if (primask == 0u) {
        __enable_irq();
    }

    feedback->online = (feedback->rx_count > 0u &&
                        (uint32_t)(now_ms - feedback->last_rx_ms) <= FEEDBACK_TIMEOUT_MS) ? 1u : 0u;
}

uint32_t MyCAN_GetTxOkCount(void)
{
    return can_tx_ok_count;
}

uint32_t MyCAN_GetTxFailCount(void)
{
    return can_tx_fail_count;
}

uint32_t MyCAN_GetErrorStatus(void)
{
    return TestCAN()->ESR;
}

void CAN1_RX0_IRQHandler(void)
{
    CanRxMsg rx;
    while (CAN_MessagePending(CAN1, CAN_FIFO0) > 0u) {
        CAN_Receive(CAN1, CAN_FIFO0, &rx);
#if TEST_CAN_BUS == 1u
        MyCAN_ParseFeedback(&rx);
#endif
    }
}

void CAN2_RX1_IRQHandler(void)
{
    CanRxMsg rx;
    while (CAN_MessagePending(CAN2, CAN_FIFO1) > 0u) {
        CAN_Receive(CAN2, CAN_FIFO1, &rx);
#if TEST_CAN_BUS == 2u
        MyCAN_ParseFeedback(&rx);
#endif
    }
}
