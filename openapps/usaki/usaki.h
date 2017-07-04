#ifndef _USAKI_H_
#define _USAKI_H_
#endif

#include "opentimers.h"


typedef struct {
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   uint16_t             usaki_period_time;
   uint8_t              usaki_period_time_code;
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t             int_temp;
   uint16_t             ext_temp;
   uint16_t             ext_pyra;
   uint16_t             int_volt;
   uint16_t				gpio_pulse;
} usaki_vars_t;