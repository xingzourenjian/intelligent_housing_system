#ifndef __OLED_H__
#define __OLED_H__

#include "stm32f10x.h"

void OLED_Init(void);

void OLED_Clear(void);

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char); // 单个字符 行位置，范围：1~4 列位置，范围：1~16

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String); // 字符串

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length); // 无符号十进制

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length); // 有符号十进制

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length); // 16进制

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length); // 二进制

#endif
