#include "stm32f4xx.h"
#include "Usart.h"
#include "Test_Config.h"
#include <stdarg.h>

#define SERIAL_RX_BUFFER_SIZE 64u

static volatile uint8_t serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
static volatile uint16_t serial_rx_write = 0u;
static volatile uint16_t serial_rx_read = 0u;
static volatile uint16_t serial_rx_overflow = 0u;

static void Serial_SendUnsignedLong(unsigned long value, unsigned int base, uint8_t uppercase)
{
    char digits[16];
    unsigned int count = 0u;
    const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (base < 2u || base > 16u) {
        return;
    }

    do {
        digits[count++] = alphabet[value % base];
        value /= base;
    } while (value != 0u && count < sizeof(digits));

    while (count > 0u) {
        Serial_SendByte((uint8_t)digits[--count]);
    }
}

static void Serial_SendSignedLong(long value)
{
    unsigned long magnitude;

    if (value < 0) {
        Serial_SendByte((uint8_t)'-');
        /* 避免对 LONG_MIN 直接取负导致有符号溢出。 */
        magnitude = (unsigned long)(-(value + 1L)) + 1UL;
    } else {
        magnitude = (unsigned long)value;
    }
    Serial_SendUnsignedLong(magnitude, 10u, 0u);
}

void Serial_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;

    gpio.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOB, &gpio);

    USART_StructInit(&usart);
    usart.USART_BaudRate = VOFA_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_Cmd(USART1, ENABLE);
}

void Serial_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
    }
    USART_SendData(USART1, byte);
}

void Serial_SendArray(const uint8_t *array, uint16_t length)
{
    uint16_t i;
    if (array == 0) {
        return;
    }
    for (i = 0u; i < length; ++i) {
        Serial_SendByte(array[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
    }
}

void Serial_SendString(const char *string)
{
    if (string == 0) {
        return;
    }
    while (*string != '\0') {
        Serial_SendByte((uint8_t)*string++);
    }
}

/*
 * 轻量级串口格式化输出，避免依赖标准库 printf 的浮点/重定向设置。
 * 支持：%s %c %d %i %u %x %X %ld %li %lu %lx %lX %%
 */
void Serial_Printf(const char *format, ...)
{
    va_list args;

    if (format == 0) {
        return;
    }

    va_start(args, format);
    while (*format != '\0') {
        uint8_t long_modifier = 0u;
        char specifier;

        if (*format != '%') {
            Serial_SendByte((uint8_t)*format++);
            continue;
        }

        format++;
        if (*format == '%') {
            Serial_SendByte((uint8_t)'%');
            format++;
            continue;
        }

        if (*format == 'l') {
            long_modifier = 1u;
            format++;
        }

        specifier = *format;
        if (specifier == '\0') {
            break;
        }
        format++;

        switch (specifier) {
            case 's': {
                const char *text = va_arg(args, const char *);
                Serial_SendString(text != 0 ? text : "(null)");
                break;
            }
            case 'c':
                Serial_SendByte((uint8_t)va_arg(args, int));
                break;
            case 'd':
            case 'i':
                if (long_modifier) {
                    Serial_SendSignedLong(va_arg(args, long));
                } else {
                    Serial_SendSignedLong((long)va_arg(args, int));
                }
                break;
            case 'u':
                if (long_modifier) {
                    Serial_SendUnsignedLong(va_arg(args, unsigned long), 10u, 0u);
                } else {
                    Serial_SendUnsignedLong((unsigned long)va_arg(args, unsigned int), 10u, 0u);
                }
                break;
            case 'x':
            case 'X':
                if (long_modifier) {
                    Serial_SendUnsignedLong(va_arg(args, unsigned long), 16u,
                                            (specifier == 'X') ? 1u : 0u);
                } else {
                    Serial_SendUnsignedLong((unsigned long)va_arg(args, unsigned int), 16u,
                                            (specifier == 'X') ? 1u : 0u);
                }
                break;
            default:
                Serial_SendByte((uint8_t)'%');
                if (long_modifier) {
                    Serial_SendByte((uint8_t)'l');
                }
                Serial_SendByte((uint8_t)specifier);
                break;
        }
    }
    va_end(args);
}

uint8_t Serial_ReadByte(uint8_t *byte)
{
    if (byte == 0 || serial_rx_read == serial_rx_write) {
        return 0u;
    }

    *byte = serial_rx_buffer[serial_rx_read];
    serial_rx_read = (uint16_t)((serial_rx_read + 1u) % SERIAL_RX_BUFFER_SIZE);
    return 1u;
}

uint16_t Serial_GetRxOverflowCount(void)
{
    return serial_rx_overflow;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = (uint8_t)USART_ReceiveData(USART1);
        uint16_t next = (uint16_t)((serial_rx_write + 1u) % SERIAL_RX_BUFFER_SIZE);

        if (next != serial_rx_read) {
            serial_rx_buffer[serial_rx_write] = data;
            serial_rx_write = next;
        } else {
            serial_rx_overflow++;
        }
    }

    if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
        volatile uint16_t dummy;
        dummy = USART1->SR;
        dummy = USART1->DR;
        (void)dummy;
    }
}
