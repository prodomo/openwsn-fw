#include "opendefs.h"
#include "usaki.h"
#include "openudp.h"
#include "openqueue.h"
#include "opentimers.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "IEEE802154.h"
#include "idmanager.h"
#include "adc_sensor.h"

#include "my_common.h"
#include "my_spi.h"
#include <headers/hw_memmap.h>
//=========================== variables =======================================
#define USAKI_PERIOD  10000
// #define PAYLOADLEN 19
usaki_vars_t usaki_vars;
uint16_t usaki_pulse_cnt=0;

static const uint8_t usaki_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

//=========================== prototypes ======================================

void usaki_timer_cb(opentimer_id_t id);
void usaki_task_cb(void);

//=========================== public ==========================================

void usaki_init(){
   
   // clear local variables
   memset(&usaki_vars,0,sizeof(usaki_vars_t));

   usaki_vars.usaki_period_time = USAKI_PERIOD;
   usaki_vars.usaki_period_time_code = USAKI_SET_ULTIME_5_ABS;
   
   // start periodic timer
      usaki_vars.timerId                    = opentimers_start(
      usaki_vars.usaki_period_time,
      TIMER_PERIODIC,TIME_MS,
      usaki_timer_cb
   );
}

void usaki_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void usaki_receive(OpenQueueEntry_t* pkt) {
   
   openqueue_freePacketBuffer(pkt);
   
   openserial_printError(
      COMPONENT_USAKI,
      ERR_RCVD_ECHO_REPLY,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
}

//=========================== private =========================================

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void usaki_timer_cb(opentimer_id_t id){
   
   scheduler_push_task(usaki_task_cb,TASKPRIO_COAP);
}

void usaki_task_cb() {
   OpenQueueEntry_t*    pkt;
   uint8_t send_buf[10];

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(usaki_vars.timerId);
      return;
   }
   
   // if you get here, send a packet

   // edit my own data
   // show pyranometer value
   usaki_vars.ext_pyra = adc_sens_read_temperature_PA0();
   // show external temp
   usaki_vars.ext_temp = adc_sens_read_temperature_PA1();
   // show inner temp
   usaki_vars.int_temp = adc_sens_read_temperature();
   // show voltage
   usaki_vars.int_volt = adc_sens_read_VDD_voltage();
   // get pulse count on gpio
   usaki_vars.gpio_pulse = usaki_pulse_cnt;
   
   
   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_USAKI);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_USAKI,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   pkt->owner                         = COMPONENT_USAKI;
   pkt->creator                       = COMPONENT_USAKI;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_SAKI;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_SAKI;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   pkt->l2_frameType                  = IEEE154_TYPE_SENSED_DATA;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],usaki_dst_addr,16);
   
   // packetfunctions_reserveHeaderSize(pkt,PAYLOADLEN);
   // // speacial flag
   // pkt->payload[0] = 0x54;
   // pkt->payload[1] = 0x66;

   // // append asn
   // uint8_t* pointer = &pkt->payload[2];
   // ieee154e_getAsn(pointer);

   packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t)*6);
   *((uint16_t*)&pkt->payload[0]) = usaki_vars.counter++;
   *((uint16_t*)&pkt->payload[2]) = usaki_vars.int_temp;
   *((uint16_t*)&pkt->payload[4]) = usaki_vars.ext_temp;
   *((uint16_t*)&pkt->payload[6]) = usaki_vars.ext_pyra;
   *((uint16_t*)&pkt->payload[8]) = usaki_vars.int_volt;
   *((uint16_t*)&pkt->payload[10]) = usaki_vars.gpio_pulse;
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
/*
   memset(send_buf,0,10);
   
   if(usaki_vars.counter == 1){
     send_buf[5] = 0x06;
     my_SPI_send(CC120X_IOCFG2, send_buf[5]);
     //CC1200WriteReg(CC120X_IOCFG2, &(send_buf[5]), 1);
   }
   else if(usaki_vars.counter == 2)  
     send_buf[3] = my_SPI_recv(CC120X_IOCFG2); 
   else
     send_buf[3] = my_SPI_recv(CC120X_IOCFG2); 
        
   send_buf[0]=0x11;
   send_buf[1]=0x11;
   send_buf[2]=0x11;
   send_buf[4]=usaki_vars.counter;

   my_openserial_printStatus(0x14, send_buf, 5);
*/
}
