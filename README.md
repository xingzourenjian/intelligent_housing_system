# intelligent_housing_system
 基于端边云架构与AI大模型的智能家居系统的设计与实现

使用外设资源总结：

定时器：系统节拍SysTick，延时TIM1，舵机TIM4、ESP01STIM3、卧室灯TIM2、电机TIM4

串口：ESP01S串口3、蓝牙串口2、语音模块串口1

中断：定时器、串口、GPIO、上下任务切换、独立看门狗

ADC：光敏传感器、MQ2、MQ7

PWM：舵机、卧室灯、电机

独立看门狗