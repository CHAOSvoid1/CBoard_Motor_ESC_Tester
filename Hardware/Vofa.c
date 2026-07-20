#include "Vofa.h"
#include "Usart.h"
#include "Test_Config.h"
#include <stdint.h>
#include <string.h>

#define VOFA_BUFFER_SIZE 320u
#define VOFA_DECIMALS    3u
#define VOFA_SCALE       1000L

static uint16_t AppendChar(char *buffer, uint16_t length, char value)
{
    if (length < (VOFA_BUFFER_SIZE - 1u)) {
        buffer[length++] = value;
    }
    return length;
}

static uint16_t AppendString(char *buffer, uint16_t length, const char *text)
{
    if (text == 0) {
        return length;
    }

    while (*text != '\0' && length < (VOFA_BUFFER_SIZE - 1u)) {
        buffer[length++] = *text++;
    }
    return length;
}

static uint16_t AppendUnsigned(char *buffer, uint16_t length, uint32_t value, uint8_t min_digits)
{
    char digits[10];
    uint8_t count = 0u;

    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && count < sizeof(digits));

    while (count < min_digits && count < sizeof(digits)) {
        digits[count++] = '0';
    }

    while (count > 0u) {
        length = AppendChar(buffer, length, digits[--count]);
    }
    return length;
}

static uint16_t AppendFloatFixed3(char *buffer, uint16_t length, float value)
{
    int32_t scaled;
    uint32_t magnitude;
    uint32_t integer_part;
    uint32_t fraction_part;

    /* 本工程的所有 VOFA 通道都远小于该范围；限幅可避免异常值转换溢出。 */
    if (value > 2147480.0f) {
        value = 2147480.0f;
    } else if (value < -2147480.0f) {
        value = -2147480.0f;
    }

    scaled = (value >= 0.0f) ?
             (int32_t)(value * (float)VOFA_SCALE + 0.5f) :
             (int32_t)(value * (float)VOFA_SCALE - 0.5f);

    if (scaled < 0) {
        length = AppendChar(buffer, length, '-');
        magnitude = (uint32_t)(-(scaled + 1)) + 1u;
    } else {
        magnitude = (uint32_t)scaled;
    }

    integer_part = magnitude / (uint32_t)VOFA_SCALE;
    fraction_part = magnitude % (uint32_t)VOFA_SCALE;

    length = AppendUnsigned(buffer, length, integer_part, 1u);
    length = AppendChar(buffer, length, '.');
    length = AppendUnsigned(buffer, length, fraction_part, VOFA_DECIMALS);
    return length;
}

void Vofa_Init(void)
{
    /* USART1 已由 Serial_Init() 初始化。保留与 C_Board_v1.0 相似的调用方式。 */
}

void Vofa_SendData(const float *channels, int count)
{
    Vofa_SendDataWithLabel("", channels, count);
}

void Vofa_SendDataWithLabel(const char *label, const float *channels, int count)
{
    char buffer[VOFA_BUFFER_SIZE];
    uint16_t length = 0u;
    int i;

    if (channels == 0 || count <= 0) {
        return;
    }

    if (label != 0 && label[0] != '\0') {
        length = AppendString(buffer, length, label);
        length = AppendChar(buffer, length, ':');
    }

    for (i = 0; i < count; ++i) {
        if (i > 0) {
            length = AppendChar(buffer, length, ',');
        }
        length = AppendFloatFixed3(buffer, length, channels[i]);
    }

    length = AppendChar(buffer, length, '\n');
    Serial_SendArray((const uint8_t *)buffer, length);
}

void Vofa_SendText(const char *text)
{
    Serial_SendString(text);
}

void Vofa_SendMotorStatus(const Motor_Test_Status_t *status)
{
    float channels[16];

    if (status == 0) {
        return;
    }

    /*
     * motor_test 通道定义：
     * 0 angle_raw, 1 speed_rpm, 2 feedback_current_raw, 3 feedback_current_A,
     * 4 temperature_C(-1 表示 M2006/C610 无此反馈), 5 command_raw,
     * 6 command_display（M2006/M3508/GM6020电流模式单位A；GM6020电压模式单位%）,
     * 7 online, 8 rx_frequency_Hz, 9 tx_fail_count, 10 CAN_ESR,
     * 11 mode, 12 result, 13 auto_phase, 14 forward_peak_rpm, 15 reverse_peak_rpm
     */
    channels[0] = (float)status->feedback.angle_raw;
    channels[1] = (float)status->feedback.speed_rpm;
    channels[2] = (float)status->feedback.current_raw;
    channels[3] = status->feedback_current_amp;
#if MOTOR_HAS_TEMPERATURE
    channels[4] = (float)status->feedback.temperature_c;
#else
    channels[4] = -1.0f;
#endif
    channels[5] = (float)status->command_raw;
    channels[6] = status->command_display;
    channels[7] = (float)status->feedback.online;
    channels[8] = (float)status->rx_frequency_hz;
    channels[9] = (float)status->tx_fail_count;
    channels[10] = (float)status->can_error_status;
    channels[11] = (float)status->mode;
    channels[12] = (float)status->result;
    channels[13] = (float)status->auto_phase;
    channels[14] = (float)status->forward_peak_rpm;
    channels[15] = (float)status->reverse_peak_rpm;

    Vofa_SendDataWithLabel("motor_test", channels, 16);
}
