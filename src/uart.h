#pragma once

//Make sure to use power of 2 sizes
//#define UART_BUFFER_SIZE    64

#define UART1_RX_BUFFER_SIZE    256
#define UART1_TX_BUFFER_SIZE    16
#define UART2_RX_BUFFER_SIZE    64
#define UART2_TX_BUFFER_SIZE    64
#define UART3_RX_BUFFER_SIZE    64
#define UART3_TX_BUFFER_SIZE    64

typedef struct {
    uint32_t baudRate;
    volatile uint8_t *rxBuffer;
    volatile uint8_t *txBuffer;

    DMA_Channel_TypeDef *rxDMAChannel;
    DMA_Channel_TypeDef *txDMAChannel;
    USART_TypeDef *USARTx;

    uint16_t rxDMAPos;
    uint16_t rxBufferSize;
    uint16_t txBufferSize;
    uint16_t rxBufferHead;
    uint16_t rxBufferTail;
    uint16_t txBufferHead;
    uint16_t txBufferTail;
    //Last count given to dma controller
    uint16_t txDMACount;
} serialPort_t;

extern serialPort_t serialPort1;
extern serialPort_t serialPort2;
extern serialPort_t serialPort3;

serialPort_t *serialOpen(USART_TypeDef * USARTx, uint32_t baudRate);
uint8_t uartAvailable(serialPort_t * s);
uint8_t uartRead(serialPort_t * s);
//Will freeze the thread if the buffer is nearing it's limit
void uartWrite(serialPort_t * s, uint8_t ch);
