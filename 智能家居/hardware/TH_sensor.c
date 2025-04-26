#include "TH_sensor.h"

// PB1 DHT11数据引脚
void DHT_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_SetBits(GPIOB, GPIO_Pin_1); // DHT11上电后需要等待1s以越过不稳定状态，总线空闲状态为高电平
}

/*
功能：
    改变GPIO口的输入输出模式
参数：
    mode：指定输入或输出模式
返回值：
    void
*/
static void change_DHT_GPIO_mode(GPIOMode_TypeDef mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = mode;  // 通过参数形式来控制GPIO的模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*
功能：
    DHT11模块起始信号
参数：
    void
返回值：
    1 标志起动信号成功
    0，不成功
*/
static uint8_t DHT_start_signal(void)
{
    // 主机拉低总线 >18ms，通知DHT11开始通信
	change_DHT_GPIO_mode(GPIO_Mode_Out_PP); // 推挽输出模式
	GPIO_ResetBits(GPIOB, GPIO_Pin_1);      // 主机控制单总线输出20ms低电平
	vTaskDelay(pdMS_TO_TICKS(20));

    // 释放总线，输出高电平, 总线由上拉电阻拉高
    GPIO_SetBits(GPIOB, GPIO_Pin_1);        // 主机释放总线
	change_DHT_GPIO_mode(GPIO_Mode_Out_OD); // 配置为开漏输出模式
	delay_us(20);                           // 延时等待20-40us后, 读取DHT11的响应信号

    // 等待应答
	if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1))         // 判断DHT11是否进入应答模式
	{
		while(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)); // DHT11 发送80us低电平响应信号
        while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1));  // DHT11 再把总线拉高80us, 准备发送数据
		return 1; // 返回1表示应答成功
	}
	return 0;     // 返回0表示应答失败
}

/*
功能：
    接收DHT11发送来1字节数据
参数：
    void
返回值：
    返回接收到的8位数据
*/
static uint8_t DHT_get_byte_data(void)
{
	uint8_t temp;

    // 每一bit数据都以50us低电平开始,高电平的长短决定了数据位是0 (26-28us) 还是 1 (70us)
	for(int i = 0; i < 8; i++)
	{
		temp <<= 1; // 高位先出
		while(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)); // 主机等待50~58μs的低电平结束
		delay_us(28);  // 等待26-28μs的高电平结束
		GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) ? (temp |= 0x01) : (temp &= ~0x01);
		while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1));  // DHT11拉低50us，1bit 发送结束
	}
	return temp;
}

/*
功能：
    获取DHT11的温度湿度数据
参数：
    humidity：湿度数据指针
	temperature：温度数据指针
返回值：
    1：数据校验正确
    0：失败
*/
uint8_t DHT_get_temp_humi_data(float *humidity, float *temperature)
{
	uint8_t data_buffer[5] = {0};

    // 从模式下,DHT11接收到开始信号触发一次温湿度采集
    // 湿度整数(8bit) 湿度小数(8bit) 温度整数(8bit) 温度小数(8bit) 校验和(8bit)，高位先出
	if(DHT_start_signal()) // if判断DHT11从机是否应答
	{
		data_buffer[0] = DHT_get_byte_data();  // 湿度的整数，十进制
		data_buffer[1] = DHT_get_byte_data();  // 湿度的小数
		data_buffer[2] = DHT_get_byte_data();  // 温度的整数
		data_buffer[3] = DHT_get_byte_data();  // 温度的小数
		data_buffer[4] = DHT_get_byte_data();  // 校验数据

		// 湿度数据
		if(data_buffer[1] < 10)
			*humidity = data_buffer[0] + data_buffer[1] * 0.1;
		else if(data_buffer[1] >= 10)
			*humidity = data_buffer[0] + data_buffer[1] * 0.01;

		// 温度数据
		if(data_buffer[3] < 10)
			*temperature = data_buffer[2] + data_buffer[3] * 0.1;
		else if(data_buffer[3] >= 10)
			*temperature = data_buffer[2] + data_buffer[3] * 0.01;

		return (data_buffer[0]+data_buffer[1]+data_buffer[2]+data_buffer[3] == data_buffer[4]) ? 1 : 0; // 校验数据是否传输正确
	}
	else
		return 0; // DHT11从机没有应答
}

void show_DHT_sensor_value_OLED(uint8_t line, uint8_t column)
{
	float humidity = 0.0, temperature = 0.0;

	DHT_get_temp_humi_data(&humidity, &temperature); // 获取温湿度数据
	OLED_ShowString(line, column, "H:");
	OLED_ShowNum(line, column+2, (uint32_t)humidity, 2); // 显示湿度整数部分
	OLED_ShowChar(line, column+4, '.');
	OLED_ShowNum(line, column+5, (uint32_t)(humidity * 10) % 10, 1); // 显示湿度小数部分
	OLED_ShowString(line, column+6, "%"); // 显示百分号

	OLED_ShowString(line, column+8, "T:");
	OLED_ShowNum(line, column+10, (uint32_t)temperature, 2); // 显示温度整数部分
	OLED_ShowChar(line, column+12, '.');
	OLED_ShowNum(line, column+13, (uint32_t)(temperature * 10) % 10, 1); // 显示温度小数部分
	OLED_ShowString(line, column+14, "C"); // 显示摄氏度符号
}
