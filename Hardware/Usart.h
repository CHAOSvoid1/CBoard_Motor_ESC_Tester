#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include <stdint.h>

void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendArray(const uint8_t *array, uint16_t length);
void Serial_SendString(const char *string);
void Serial_Printf(const char *format, ...);

/* 非阻塞读取。返回 1 表示读到一个字节，返回 0 表示接收缓冲区为空。 */
uint8_t Serial_ReadByte(uint8_t *byte);
uint16_t Serial_GetRxOverflowCount(void);

#endif
