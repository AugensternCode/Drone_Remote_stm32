#ifndef _sendpacket_h_
#define _sendpacket_h_

#include "stm32f10x.h"

typedef struct
{
    uint16_t thr;
    uint8_t pit;
    uint8_t rol;
    uint8_t yaw;
    uint8_t key;
    
}RemoteData;


extern RemoteData tx;
void NrfRxPacket(void);
void NrfInvalidateRx(void);
/* Stage-0 diagnostics, updated only by the DMA send path. */
extern volatile uint32_t g_radio_tx_ok;
extern volatile uint32_t g_radio_tx_fail;
extern volatile uint32_t g_radio_tx_timeout;
extern volatile uint8_t g_radio_last_result;
extern volatile uint32_t lastValidAckTick;
void OledDisplayPairStatus(void);

#endif

