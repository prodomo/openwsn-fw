#ifndef __CREPORTASN_H
#define __CREPORTASN_H

/**
\addtogroup AppUdp
\{
\addtogroup creportasn
\{
*/
#include "opencoap.h"
//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   opentimers_id_t      timerId;
   bool                 isEmergency;
   uint8_t              creportasn_sequence;
   uint8_t              lastSuccessLeft;
   uint8_t              errorCounter;
   uint8_t              lastCallbackSequence;

} creportasn_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void creportasn_init(void);

/**
\}
\}
*/

#endif
