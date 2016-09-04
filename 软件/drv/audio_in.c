#include "base.h"
#include "audio_in.h"

static void tim_config(u16 rate);
static void adc_config(void);
static void dma_config(s16 *buf, u16 len);

/*-----------------------------------*/

// 音频采集初始化
// 参数：采样率，采样buf，采样样本数
// 返回值：0成功/-1失败
s32 audio_in_init(u16 rate, s16 *buf, u16 len)
{
    if (NULL == buf) {
        return -1;
    }
    
    tim_config(rate);
    adc_config();
    dma_config(buf, len);
    
    return 0;
}

// 获取音频采集状态
// 参数：无
// 返回值：AUDIO_IN_READY_NONE-无有效数据/AUDIO_IN_READY_HEAD-前半buf有效/AUDIO_IN_READY_TAIL-后半buf有效
s32 audio_in_get_ready(void)
{
    /* Test DMA1 HT1 flag */
    if (DMA_GetFlagStatus(DMA1_FLAG_HT1) == SET) {
        /* Clear DMA HT1 flag */
        DMA_ClearFlag(DMA1_FLAG_HT1);
        
        return AUDIO_IN_READY_HEAD;
    }

    /* Test DMA1 TC1 flag */
    if (DMA_GetFlagStatus(DMA1_FLAG_TC1) == SET) {
        /* Clear DMA TC1 flag */
        DMA_ClearFlag(DMA1_FLAG_TC1);
        
        return AUDIO_IN_READY_TAIL;
    }

    return AUDIO_IN_READY_NONE;
}

/*-----------------------------------*/

static void tim_config(u16 rate)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* TIM1 Periph clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

    /* TIM1 DeInit */
    TIM_DeInit(TIM1);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_OCStructInit(&TIM_OCInitStructure);

    DBGMCU_APB2PeriphConfig(DBGMCU_TIM1_STOP, ENABLE);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = (SystemCoreClock / rate - 1);
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* Output Compare PWM Mode configuration */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; /* low edge by default */
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0x01;
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);

    /* TIM1 enable counter */
    TIM_Cmd(TIM1, ENABLE);

    /* Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    
    return;
}

static void adc_config(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* ADC1 DeInit */
    ADC_DeInit(ADC1);

    /* GPIOA Periph clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* ADC1 Periph clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    ADC_ClockModeConfig(ADC1, ADC_ClockMode_SynClkDiv4);

    /* Configure ADC channel 0 as analog input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Initialize ADC structure */
    ADC_StructInit(&ADC_InitStructure);
    /* Configure the ADC1 in continuous mode withe a resolution equal to 12 bits  */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;    
    ADC_InitStructure.ADC_ExternalTrigConv =  ADC_ExternalTrigConv_T1_CC4;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &ADC_InitStructure); 

    /* Convert the ADC1 channel 0 with 239.5 Cycles as sampling time */
    ADC_ChannelConfig(ADC1, ADC_Channel_0, ADC_SampleTime_239_5Cycles);

    /* ADC Calibration */
    ADC_GetCalibrationFactor(ADC1);

    /* ADC DMA request in circular mode */
    ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);

    /* Enable ADC_DMA */
    ADC_DMACmd(ADC1, ENABLE);

    /* Enable the ADC peripheral */
    ADC_Cmd(ADC1, ENABLE);

    /* Wait the ADRDY flag */
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    /* ADC1 regular Software Start Conv */
    ADC_StartOfConversion(ADC1);

    return;
}

static void dma_config(s16 *buf, u16 len)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* DMA1 clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);

    /* DMA1 Channel1 Config */
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (ADC1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = len;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    /* DMA1 Channel1 enable */
    DMA_Cmd(DMA1_Channel1, ENABLE);

    return;
}
