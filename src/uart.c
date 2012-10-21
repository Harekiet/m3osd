/*
    DMA UART routines idea lifted from AutoQuad
    Copyright © 2011  Bill Nesbitt
*/
#include "board.h"
#include "uart.h"

static serialPort_t serialPort1;
static serialPort_t serialPort2;
static serialPort_t serialPort3;

// USART1 - GPS (RX - DMA, TX - IRQ)
serialPort_t *serialUSART1(uint32_t baudRate)
{
    serialPort_t *s;
    static volatile uint8_t rx1Buffer[UART1_RX_BUFFER_SIZE];
    static volatile uint8_t tx1Buffer[UART1_TX_BUFFER_SIZE];
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    s = &serialPort1;
    s->rxBufferSize = UART1_RX_BUFFER_SIZE;
    s->txBufferSize = UART1_TX_BUFFER_SIZE;
    s->rxBuffer = rx1Buffer;
    s->txBuffer = tx1Buffer;
    s->rxDMAChannel = DMA1_Channel5;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    // USART1_TX    PA9
    // USART1_RX    PA10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // TX interrupt, doesn't need high performance
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

// USART2 - Telemetry (RX + TX by DMA)
serialPort_t *serialUSART2(uint32_t baudRate)
{
    serialPort_t *s;
    static volatile uint8_t rx2Buffer[UART2_RX_BUFFER_SIZE];
    static volatile uint8_t tx2Buffer[UART2_TX_BUFFER_SIZE];
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    s = &serialPort2;
    s->baudRate = baudRate;
    s->rxBufferSize = UART2_RX_BUFFER_SIZE;
    s->txBufferSize = UART2_TX_BUFFER_SIZE;
    s->rxBuffer = rx2Buffer;
    s->txBuffer = tx2Buffer;
    s->rxDMAChannel = DMA1_Channel6;
    s->txDMAChannel = DMA1_Channel7;
    s->USARTx = USART2;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    // USART2_TX    PA2
    // USART2_RX    PA3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // DMA TX Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

// USART3 - FlexiPort (RX + TX by IRQ)
serialPort_t *serialUSART3(uint32_t baudRate)
{
    serialPort_t *s;
    static volatile uint8_t rx3Buffer[UART3_RX_BUFFER_SIZE];
    static volatile uint8_t tx3Buffer[UART3_TX_BUFFER_SIZE];
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    s = &serialPort3;
    s->baudRate = baudRate;
    s->rxBufferSize = UART3_RX_BUFFER_SIZE;
    s->txBufferSize = UART3_TX_BUFFER_SIZE;
    s->rxBuffer = rx3Buffer;
    s->txBuffer = tx3Buffer;
    s->USARTx = USART3;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    // USART3_TX    PB10
    // USART3_RX    PB11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // RX/TX interrupt, doesn't need high performance
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

serialPort_t *serialOpen(USART_TypeDef * USARTx, uint32_t baudRate)
{
    DMA_InitTypeDef DMA_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    serialPort_t *s = NULL;

    if (USARTx == USART1)
        s = serialUSART1(baudRate);
    if (USARTx == USART2)
        s = serialUSART2(baudRate);
    if (USARTx == USART3)
        s = serialUSART3(baudRate);

    s->USARTx = USARTx;
    s->rxBufferHead = s->rxBufferTail = 0;
    s->txBufferHead = s->txBufferTail = 0;

    USART_InitStructure.USART_BaudRate = baudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USARTx, &USART_InitStructure);
    USART_Cmd(USARTx, ENABLE);

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & USARTx->DR;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;

    // Receive DMA into a circular buffer
    if (s->rxDMAChannel) {
        DMA_InitStructure.DMA_BufferSize = s->rxBufferSize;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) s->rxBuffer;

        DMA_DeInit(s->rxDMAChannel);
        DMA_Init(s->rxDMAChannel, &DMA_InitStructure);
        DMA_Cmd(s->rxDMAChannel, ENABLE);
        USART_DMACmd(USARTx, USART_DMAReq_Rx, ENABLE);
        s->rxDMAPos = DMA_GetCurrDataCounter(s->rxDMAChannel);
    } else {
        USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    }

    // Transmit DMA
    if (s->txDMAChannel) {
        DMA_InitStructure.DMA_BufferSize = s->txBufferSize;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_DeInit(s->txDMAChannel);
        DMA_Init(s->txDMAChannel, &DMA_InitStructure);
        DMA_ITConfig(s->txDMAChannel, DMA_IT_TC, ENABLE);
        DMA_SetCurrDataCounter(s->txDMAChannel, 0);
        s->txDMAChannel->CNDTR = 0;
        USART_DMACmd(USARTx, USART_DMAReq_Tx, ENABLE);
    } else {
        USART_ITConfig(USARTx, USART_IT_TXE, ENABLE);
    }

    return s;
}

static void uartStartTxDMA(serialPort_t * s)
{
    s->txDMAChannel->CMAR = (uint32_t) & s->txBuffer[s->txBufferTail];
    if (s->txBufferHead > s->txBufferTail) {
        s->txDMAChannel->CNDTR = s->txBufferHead - s->txBufferTail;
        s->txBufferTail = s->txBufferHead;
    } else {
        s->txDMAChannel->CNDTR = s->txBufferSize - s->txBufferTail;
        s->txBufferTail = 0;
    }

    DMA_Cmd(s->txDMAChannel, ENABLE);
}

uint8_t uartAvailable(serialPort_t * s)
{
    if (s->rxDMAChannel)
        return s->rxDMAChannel->CNDTR != s->rxDMAPos;
    else
        return s->rxBufferTail != s->rxBufferHead;
}

uint8_t uartRead(serialPort_t * s)
{
    uint8_t ch;

    if (s->rxDMAChannel) {
        ch = s->rxBuffer[s->rxBufferSize - s->rxDMAPos];
        if (--s->rxDMAPos == 0)
            s->rxDMAPos = s->rxBufferSize;
    } else {
        ch = s->rxBuffer[s->rxBufferTail];
        s->rxBufferTail = (s->rxBufferTail + 1) % s->rxBufferSize;
    }

    return ch;
}

void uartWrite(serialPort_t * s, uint8_t ch)
{
    s->txBuffer[s->txBufferHead] = ch;
    s->txBufferHead = (s->txBufferHead + 1) % s->txBufferSize;

    if (s->txDMAChannel) {
        if (!(s->txDMAChannel->CCR & 1))
            uartStartTxDMA(s);
    } else {
        USART_ITConfig(s->USARTx, USART_IT_TXE, ENABLE);
    }
}













// Handlers

// USART1 Tx IRQ Handler
void USART1_IRQHandler(void)
{
    serialPort_t *s = &serialPort1;
    uint16_t SR = s->USARTx->SR;

    if (SR & USART_FLAG_TXE) {
        if (s->txBufferTail != s->txBufferHead) {
            s->USARTx->DR = s->txBuffer[s->txBufferTail];
            s->txBufferTail = (s->txBufferTail + 1) % s->txBufferSize;
        } else {
            USART_ITConfig(s->USARTx, USART_IT_TXE, DISABLE);
        }
    }
}

// USART2 Tx DMA Handler
void DMA1_Channel7_IRQHandler(void)
{
    serialPort_t *s = &serialPort2;
    DMA_ClearITPendingBit(DMA1_IT_TC7);
    DMA_Cmd(s->txDMAChannel, DISABLE);

    if (s->txBufferHead != s->txBufferTail)
        uartStartTxDMA(s);
}

// USART3 Rx/Tx IRQ Handler
void USART3_IRQHandler(void)
{
    serialPort_t *s = &serialPort3;
    uint16_t SR = s->USARTx->SR;

    if (SR & USART_FLAG_RXNE) {
        s->rxBuffer[s->rxBufferHead] = s->USARTx->DR;
        s->rxBufferHead = (s->rxBufferHead + 1) % s->rxBufferSize;
    }
    if (SR & USART_FLAG_TXE) {
        if (s->txBufferTail != s->txBufferHead) {
            s->USARTx->DR = s->txBuffer[s->txBufferTail];
            s->txBufferTail = (s->txBufferTail + 1) % s->txBufferSize;
        } else {
            USART_ITConfig(s->USARTx, USART_IT_TXE, DISABLE);
        }
    }
}
