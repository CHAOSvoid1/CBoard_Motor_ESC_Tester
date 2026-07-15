#ifndef __VOFA_H
#define __VOFA_H

#include "Motor.h"

void Vofa_Init(void);
void Vofa_SendData(const float *channels, int count);
void Vofa_SendDataWithLabel(const char *label, const float *channels, int count);
void Vofa_SendText(const char *text);
void Vofa_SendMotorStatus(const Motor_Test_Status_t *status);

#endif
