/**
\brief An report ASN CoAP application.
*/

#include "opendefs.h"
#include "uinject_coap.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"

#include "idmanager.h"
#include "IEEE802154E.h"
#include "IEEE802154.h"

#include "gpio.h"
#include <headers/hw_memmap.h>
#include "leds.h"
#include "neighbors.h"
#include "icmpv6rpl.h"
#include "my_common.h"

//=========================== defines =========================================

// inter-packet period (in ms)
// #define UINJECT_COAPPERIOD  		40000
// #define PAYLOADLEN      			20

// #define UHURRICANEPAYLOADLEN      	49

#define USAKI_CODE_LOC_ULTIME 		 0
#define UINJECT_CODE_LOC_ULTIME 	 2
#define UINJECT_CODE_MASK_SHOWPOWER  4 
#define UINJECT_CODE_MASK_NEEDACK    5
#define UINJECT_CODE_MASK_WITHRSSI   6
#define USE_YYS_TOPOLOGY

const uint8_t uinject_coap_path0[] = "uinject_coap";

//=========================== variables =======================================

uinject_coap_vars_t 	uinject_coap_vars;
uint8_t					PC2_alarm_on;
opentimer_id_t 			buzz_timer;
//=========================== prototypes ======================================

owerror_t uinject_coap_receive(OpenQueueEntry_t* msg);
void uinject_coap_timer_cb(opentimer_id_t id);
void uinject_coap_task_cb(void);
void uinject_coap_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void usaki_buzz_timer_cb(opentimer_id_t id);
void usaki_buzz_task_cb(void);

//=========================== public ==========================================
bool isAck(void){
  
  if (uinject_coap_vars.rtnCounter == uinject_coap_vars.counter)
    return TRUE;
  else
    return FALSE;
}

// void alarm_on(){
//    //opentimers_stop(buzz_timer);

//    opentimers_restart(buzz_timer);
// }

// void alarm_off(){
  
//   opentimers_stop(buzz_timer);
//   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  
// }

void uinject_coap_init()
{
	   // clear local variables
   memset(&uinject_coap_vars,0,sizeof(uinject_coap_vars_t));

	   // prepare the resource descriptor for the /ex path
   uinject_coap_vars.desc.path0len             = sizeof(uinject_coap_path0)-1;
   uinject_coap_vars.desc.path0val             = (uint8_t*)(&uinject_coap_path0);
   uinject_coap_vars.desc.path1len             = 0;
   uinject_coap_vars.desc.path1val             = NULL;
   uinject_coap_vars.desc.componentID          = COMPONENT_UINJECT_COAP;
   uinject_coap_vars.desc.discoverable         = TRUE;
   uinject_coap_vars.desc.callbackRx           = &uinject_coap_receive;
   uinject_coap_vars.desc.callbackSendDone     = &uinject_coap_sendDone;

   // initial some variables
   uinject_coap_vars.needAck = FALSE;
   uinject_coap_vars.reTxNum = 0;
   uinject_coap_vars.counter = 1;
   uinject_coap_vars.rtnCounter = 0;
   uinject_coap_vars.uinject_period_time = UPLOAD_PERIOD_TIME_5MS;
   uinject_coap_vars.uinject_period_time_code = UINJECT_SET_ULTIME_5_ABS;

   opencoap_register(&uinject_coap_vars.desc);
   uinject_coap_vars.timerId    = opentimers_start(uinject_coap_vars.uinject_period_time,
                                                TIMER_PERIODIC,TIME_MS,
                                                uinject_coap_timer_cb);

   PC2_alarm_on = 0;

   // buzz_timer = opentimers_start(
   //    100,
   //    TIMER_PERIODIC,TIME_MS,
   //    usaki_buzz_timer_cb);

   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);

   //alarm_off();
}


//=========================== private =========================================

owerror_t uinject_coap_receive(OpenQueueEntry_t* msg) {
// //   uint16_t          temp_l4_destination_port;
// //   OpenQueueEntry_t* reply;
//    uint8_t rcv_cmd;
//    uinject_coap_recv_t * pkt = request->payload;
//    const uint8_t uinject_info[] = "ITRI MOTE";
//    uint16_t serialNum;
// /*
//    reply = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
//    if (reply==NULL) {
//       openserial_printError(
//          COMPONENT_UINJECT,
//          ERR_NO_FREE_PACKET_BUFFER,
//          (errorparameter_t)0,
//          (errorparameter_t)0
//       );
//       openqueue_freePacketBuffer(request); //clear the request packet as well
//       return;
//    }

//    reply->owner                         = COMPONENT_UINJECT;

//    // reply with the same OpenQueueEntry_t
//    reply->creator                       = COMPONENT_UINJECT;
//    reply->l4_protocol                   = IANA_UDP;
//    temp_l4_destination_port           = request->l4_destination_port;
//    reply->l4_destination_port           = request->l4_sourcePortORicmpv6Type;
//    reply->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
//    reply->l3_destinationAdd.type        = ADDR_128B;
   
//    // copy source to destination to echo.
//    memcpy(&reply->l3_destinationAdd.addr_128b[0],&request->l3_sourceAdd.addr_128b[0],16);

//    packetfunctions_reserveHeaderSize(reply,request->length);
//    //memcpy(&reply->payload[0],&request->payload[0],request->length);

//    if ((openudp_send(reply))==E_FAIL) {
//       openqueue_freePacketBuffer(reply);
//    }
// */
//    rcv_cmd = pkt->cmdType;

//    switch(rcv_cmd){
//       case UINJECT_GET_INFO:
//       	break;
//       case UINJECT_SET_PARENTS:
//       	break;
//       case UINJECT_SET_LED:
//         leds_debug_on();
//       	break;
//       case UINJECT_UNSET_LED:
//         leds_debug_off();
//       	break;
//       case UINJECT_TOGGLE_LED:
//         leds_debug_toggle();
//       	break;
//       case UINJECT_SET_ACK:
//         uinject_vars.needAck = TRUE;
//       	break;
//       case UINJECT_UNSET_ACK:
//         uinject_vars.needAck = FALSE;
//       	break;
//       case UINJECT_RSP_ACK:
//         serialNum = pkt->serialNumH;
//         serialNum *= 256;
//         serialNum += pkt->serialNumL;
//         uinject_vars.rtnCounter = serialNum;
//         if(uinject_vars.rtnCounter < 10)
//           leds_debug_toggle();
//       	break;
//       case UINJECT_SET_UPLOAD_TIME:
//         uinject_change_upload_time(pkt->serialNumL, uinject_vars.timerId);
//         uinject_vars.uinject_period_time_code = pkt->serialNumL;
//         leds_debug_toggle();
//       	break;
//       case USAKI_SET_UPLOAD_TIME:
//         usaki_change_upload_time(pkt->serialNumL, usaki_vars.timerId);
//         uinject_vars.usaki_period_time_code = pkt->serialNumL;
//         leds_debug_toggle();
//       	break;
//       case UALERT_SET_ON:
//         //alarm_on();
// 		PC2_alarm_on = 1;	
//       	break;
//       case UALERT_SET_OFF:
//         //alarm_off();
// 		PC2_alarm_on = 0;	
//      	break;
//       default:
//         leds_debug_toggle();
//    }

//    //openserial_printData(rcv_dataa, 3);

//    openqueue_freePacketBuffer(request);


//    // openqueue_freePacketBuffer(pkt);
   
//    // openserial_printError(
//    //    COMPONENT_UINJECT,
//    //    ERR_RCVD_ECHO_REPLY,
//    //    (errorparameter_t)0,
//    //    (errorparameter_t)0
//    // );
    return E_FAIL;
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void uinject_coap_timer_cb(opentimer_id_t id){
   scheduler_push_task(uinject_coap_task_cb,TASKPRIO_COAP);
}

void uinject_coap_task_cb() {
   OpenQueueEntry_t*    pkt;
   owerror_t            outcome;

   uint8_t              code=0;
   uint8_t              tmp_mask;
   uint8_t              uinjectPayloadLen;
   open_addr_t 			tmp_addr;
   uint8_t 				   numTx, numTxAck, numPcnt=0;
   uint16_t 			   tmp_num = 255;
   uint8_t              numNeighbor;
   uint8_t 				   parentShortAddr[2];
   dagrank_t 			   rank;
   
   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(uinject_coap_vars.timerId);
      openserial_printError(123,66,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      return;
   }

   openqueue_removeAllCreatedBy(COMPONENT_UINJECT_COAP);

   // create a CoAP RD packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT_COAP);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UINJECT_COAP,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      // openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_UINJECT_COAP;
   pkt->owner                     = COMPONENT_UINJECT_COAP;

   // find number of neighbors
   numNeighbor = neighbors_getNumNeighbors();
   if(numNeighbor==0) {
      openqueue_freePacketBuffer(pkt);
      return;
   }

   if (numNeighbor < MAX_ALLOW_NEIGHBORS)
      uinjectPayloadLen = 9 + numNeighbor*3;
   else
      uinjectPayloadLen = 9 + MAX_ALLOW_NEIGHBORS*3;

   // CoAP payload
   packetfunctions_reserveHeaderSize(pkt,uinjectPayloadLen);

	// check re-transmit mechanism
   if(uinject_coap_vars.needAck){
      if (isAck()){
         uinject_coap_vars.reTxNum = 0;
         leds_debug_toggle();
         uinject_coap_vars.counter++;
         openqueue_freePacketBuffer(pkt);
         return;
      }else{
         if (uinject_coap_vars.reTxNum > UINJECT_RETRANSMIT_CNT){
            uinject_coap_vars.reTxNum = 0;
            uinject_coap_vars.counter++;
            openqueue_freePacketBuffer(pkt);
            return;
         }else
            uinject_coap_vars.reTxNum++;
            // send previous SN
            packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
            *((uint16_t*)&pkt->payload[2]) = uinject_coap_vars.counter;
            uinject_coap_vars.rtnTimerId = opentimers_start(
              UINJECT_WAIT_RSP_TIME,
              TIMER_ONESHOT,TIME_MS,
              uinject_coap_timer_cb
            );
      }
   }else{
      // send counter value
      packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
      *((uint16_t*)&pkt->payload[2]) = uinject_coap_vars.counter;
      uinject_coap_vars.counter++; 
   }

   rank = icmpv6rpl_getMyDAGrank();

   // get parent 64b addr
   //neighbors_getPreferredParentEui64(&tmp_addr);
   icmpv6rpl_getPreferredParentEui64(&tmp_addr);
   
   // assign parent short addr
   memset(parentShortAddr,0,2);
   if(tmp_addr.type == ADDR_64B)
      memcpy(parentShortAddr, &tmp_addr.addr_64b[6], 2);

   // find PDR
   if(my_neighbors_getTxTxAck(&tmp_addr, &numTx, &numTxAck)){
     if ((numTx !=0)&&(numTx >= numTxAck)){
        tmp_num *= numTxAck;
        tmp_num /=numTx;
        numPcnt = (uint8_t)(tmp_num & 0xff);
     }
   }

   // find code
   code |= 1 << UINJECT_CODE_MASK_SHOWPOWER;
   code |= 1 << UINJECT_CODE_MASK_WITHRSSI;
   
   tmp_mask = 1 << UINJECT_CODE_MASK_NEEDACK;
   code &= ~tmp_mask;
   code |= (uint8_t)uinject_coap_vars.needAck << UINJECT_CODE_MASK_NEEDACK;

   // update upload time value
   tmp_mask = 0x03 << UINJECT_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_coap_vars.uinject_period_time_code << UINJECT_CODE_LOC_ULTIME;
   code |= tmp_mask;

   tmp_mask = 0x03 << USAKI_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_coap_vars.usaki_period_time_code << USAKI_CODE_LOC_ULTIME;
   code |= tmp_mask;

   //payload info
   *((uint8_t*)&pkt->payload[4]) = code;
   *((uint8_t*)&pkt->payload[5]) = numPcnt;

   memcpy(&pkt->payload[6],parentShortAddr,2);

   memcpy(&(pkt->payload[8]),&rank,sizeof(rank));

   *((uint8_t*)&pkt->payload[10]) = numNeighbor;

   neighbors_getNshortAddrnRSSI(&(pkt->payload[11]));

   //payload info end

   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_PAYLOAD_MARKER;
   

   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                = (COAP_OPTION_NUM_CONTENTFORMAT - COAP_OPTION_NUM_URIPATH) << 4
                                    | 1;
   pkt->payload[1]                = COAP_MEDTYPE_APPOCTETSTREAM;
   // location-path option

   packetfunctions_reserveHeaderSize(pkt,sizeof(uinject_coap_path0)-1);
   memcpy(&pkt->payload[0],uinject_coap_path0,sizeof(uinject_coap_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                = ((COAP_OPTION_NUM_URIPATH) << 4) | (sizeof(uinject_coap_path0)-1);
   
   // metadata
   //pkt->l4_destination_port       = WKP_UDP_COAP;
   pkt->l4_destination_port       = WKP_UDP_INJECT_COAP;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_ringmaster,16);
   
   pkt->l2_frameType = IEEE154_TYPE_SENSED_DATA;

   // send
   outcome = opencoap_send(
      pkt,
      COAP_TYPE_NON,
      COAP_CODE_REQ_PUT,
      1,
      &uinject_coap_vars.desc
   );
   
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
   return;
}

void uinject_coap_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

// int uinject_coap_change_upload_time(uint8_t new_time_code, opentimer_id_t id){
//   uint32_t new_duration;

//   if ((new_time_code > UINJECT_SET_ULTIME_90_ABS)||(new_time_code < UINJECT_SET_ULTIME_5_ABS)){
//     return -1;
//   }

//   switch(new_time_code){
//     case UINJECT_SET_ULTIME_5:
//     case UINJECT_SET_ULTIME_5_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_5MS;
//     break;
//     case UINJECT_SET_ULTIME_15:
//       new_duration = UPLOAD_PERIOD_TIME_15MS;
//     break;
//     case UINJECT_SET_ULTIME_20:
//     case UINJECT_SET_ULTIME_20_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_20MS;
//     break;
//     case UINJECT_SET_ULTIME_30:
//       new_duration = UPLOAD_PERIOD_TIME_30MS;
//     break;
//     case UINJECT_SET_ULTIME_45:
//     case UINJECT_SET_ULTIME_45_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_45MS;
//     break;
//     case UINJECT_SET_ULTIME_60:
//       new_duration = UPLOAD_PERIOD_TIME_60MS;
//     break;
//     case UINJECT_SET_ULTIME_90_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_90MS;
//     break;
//     case UINJECT_SET_ULTIME_180:
//       new_duration = UPLOAD_PERIOD_TIME_180MS;
//     break;
//     case UINJECT_SET_ULTIME_DEF:
//       new_duration = UPLOAD_PERIOD_TIME_45MS;
//     break;
//     default:
//       new_duration = UPLOAD_PERIOD_TIME_45MS;
//   }

//   opentimers_setPeriod(id, TIME_MS, new_duration);

//   return 1;
// }


//===========================uaski_buzz===============================

// void usaki_buzz_timer_cb(opentimer_id_t id){

//    scheduler_push_task(usaki_buzz_task_cb,TASKPRIO_COAP);
// }

// void usaki_buzz_task_cb(){

// 	// periodically low/high to PC2
//   if (PC2_alarm_on == 1){
//     if (PC2_status){
//       // set high
//       GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//       PC2_status = 0;
//     }else{
//       // set low
//       GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, 0);
//       PC2_status = 1;
//     }
//   }else{
//     GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
//   }
// }

// int usaki_change_upload_time(uint8_t new_time_code, opentimer_id_t id){
//   uint32_t new_duration;

//   if ((new_time_code > USAKI_SET_ULTIME_90_ABS)||(new_time_code < USAKI_SET_ULTIME_5_ABS)){
//     return -1;
//   }

//   switch(new_time_code){
//     case USAKI_SET_ULTIME_5:
//     case USAKI_SET_ULTIME_5_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_5MS;
//     break;
//     case USAKI_SET_ULTIME_15:
//       new_duration = UPLOAD_PERIOD_TIME_15MS;
//     break;
//     case USAKI_SET_ULTIME_20:
//     case USAKI_SET_ULTIME_20_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_20MS;
//     break;
//     case USAKI_SET_ULTIME_30:
//       new_duration = UPLOAD_PERIOD_TIME_30MS;
//     break;
//     case USAKI_SET_ULTIME_45:
//     case USAKI_SET_ULTIME_45_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_45MS;
//     break;
//     case USAKI_SET_ULTIME_60:
//       new_duration = UPLOAD_PERIOD_TIME_60MS;
//     break;
//     case USAKI_SET_ULTIME_90:
//     case USAKI_SET_ULTIME_90_ABS:
//       new_duration = UPLOAD_PERIOD_TIME_90MS;
//     break;
//     case USAKI_SET_ULTIME_180:
//       new_duration = UPLOAD_PERIOD_TIME_180MS;
//     break;
//     case USAKI_SET_ULTIME_DEF:
//       new_duration = UPLOAD_PERIOD_TIME_30MS;
//     break;
//     default:
//       new_duration = UPLOAD_PERIOD_TIME_30MS;
//   }

//   opentimers_setPeriod(id, TIME_MS, new_duration);

//   return 1;
// }