#ifndef __CGREEN_H
#define __CGREEN_H

/**
\addtogroup AppCoAP
\{
\addtogroup cgreen
\{
*/
#include "opencoap.h"
//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   opentimer_id_t       timerId;
   uint8_t              creportasn_sequence;
   uint8_t              lastSuccessLeft;
   uint8_t              errorCounter;
} cgreen_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void cgreen_init(void);

/**
\}
\}
*/

#endif
