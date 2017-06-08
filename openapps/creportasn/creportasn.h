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
   opentimer_id_t       timerId;
   uint8_t              creportasn_sequence;
   uint8_t              lastSuccessLeft;
   uint8_t              errorCounter;
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t             int_temp;
   uint16_t             ext_temp;
   uint16_t             ext_pyra;
   uint16_t             int_volt;
   uint16_t				   gpio_pulse;
} creportasn_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void creportasn_init(void);

/**
\}
\}
*/

#endif