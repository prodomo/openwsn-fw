#ifndef _UINJECT_NEW_H_
#define _UINJECT_NEW_H_
#endif

#include "opentimers.h"

//=========================== variables =======================================
#define UINJECT_NEW_PERIOD  40000
#define PAYLOADLEN 19
#define UHURRICANEPAYLOADLEN      49

#define USAKI_CODE_LOC_ULTIME 0
#define UINJECT_CODE_LOC_ULTIME 2
#define UINJECT_CODE_MASK_SHOWPOWER  4 
#define UINJECT_CODE_MASK_NEEDACK    5
#define UINJECT_CODE_MASK_WITHRSSI   6
#define USE_YYS_TOPOLOGY


typedef struct {
   bool                 needAck;
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   opentimer_id_t       rtnTimerId;  ///< periodic timer which triggers transmission
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t             rtnCounter;
   uint8_t              reTxNum;
   uint16_t             uinject_period_time;
   uint16_t             usaki_period_time;
   uint8_t              uinject_period_time_code;
   uint8_t              usaki_period_time_code;

} uinject_new_vars_t;

typedef struct {
   uint8_t       cmdType;
   uint8_t       serialNumL;
   uint8_t       serialNumH;
//   union{
//      uint8_t ctrlCmd;
//      uint8_t setParents[16];
//   }
} uinject_new_recv_t;