#include "opendefs.h"
//#include "uinject.h"
#include "openudp.h"
#include "openqueue.h"
#include "opentimers.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "neighbors.h"
#include "leds.h"
#include "my_common.h"
#include "gpio.h"
#include <headers/hw_memmap.h>
//=========================== variables =======================================

extern usaki_vars_t usaki_vars;
uinject_vars_t uinject_vars;
uint8_t PC2_status = 0;

static const uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

/// code (1B), MyInfo (10 or 12B), Nbr1 (12B), Nbr2 (12B), Nbr3 (12B)
#define UHURRICANEPAYLOADLEN      49

#define USAKI_CODE_LOC_ULTIME 0
#define UINJECT_CODE_LOC_ULTIME 2
#define UINJECT_CODE_MASK_SHOWPOWER  4 
#define UINJECT_CODE_MASK_NEEDACK    5
#define UINJECT_CODE_MASK_WITHRSSI   6
#define USE_YYS_TOPOLOGY

//=========================== prototypes ======================================

void uinject_timer_cb(opentimer_id_t id);
void uinject_task_cb(void);
void usaki_buzz_timer_cb(opentimer_id_t id);
void usaki_buzz_task_cb(void);
uint8_t PC2_alarm_on;

//=========================== public ==========================================

opentimer_id_t buzz_timer;

bool isAck(void){
  
  if (uinject_vars.rtnCounter == uinject_vars.counter)
    return TRUE;
  else
    return FALSE;
}

void alarm_on(){
   //opentimers_stop(buzz_timer);

   opentimers_restart(buzz_timer);
}

void alarm_off(){
  
  opentimers_stop(buzz_timer);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  
}

void uinject_init() {
   
   // clear local variables
   memset(&uinject_vars,0,sizeof(uinject_vars_t));

   // initial some variables
   uinject_vars.needAck = FALSE;
   uinject_vars.reTxNum = 0;
   uinject_vars.counter = 1;
   uinject_vars.rtnCounter = 0;
   uinject_vars.uinject_period_time = UPLOAD_PERIOD_TIME_5MS;
   uinject_vars.uinject_period_time_code = UINJECT_SET_ULTIME_5_ABS;
   
   // start periodic timer
   uinject_vars.timerId                    = opentimers_start(
      uinject_vars.uinject_period_time,
      TIMER_PERIODIC,TIME_MS,
      uinject_timer_cb
   );

   PC2_alarm_on = 0;

   buzz_timer = opentimers_start(
      100,
      TIMER_PERIODIC,TIME_MS,
      usaki_buzz_timer_cb
   );

   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);

   //alarm_off();

}

void usaki_buzz_timer_cb(opentimer_id_t id){

   scheduler_push_task(usaki_buzz_task_cb,TASKPRIO_COAP);
}

void usaki_buzz_task_cb(){

  // periodically low/high to PC2
  if (PC2_alarm_on == 1){
    if (PC2_status){
      // set high
      GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
      PC2_status = 0;
    }else{
      // set low
      GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, 0);
      PC2_status = 1;
    }
  }else{
    GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
  }
}


void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

int uinject_change_upload_time(uint8_t new_time_code, opentimer_id_t id){
  uint32_t new_duration;

  if ((new_time_code > UINJECT_SET_ULTIME_90_ABS)||(new_time_code < UINJECT_SET_ULTIME_5_ABS)){
    return -1;
  }

  switch(new_time_code){
    case UINJECT_SET_ULTIME_5:
    case UINJECT_SET_ULTIME_5_ABS:
      new_duration = UPLOAD_PERIOD_TIME_5MS;
    break;
    case UINJECT_SET_ULTIME_15:
      new_duration = UPLOAD_PERIOD_TIME_15MS;
    break;
    case UINJECT_SET_ULTIME_20:
    case UINJECT_SET_ULTIME_20_ABS:
      new_duration = UPLOAD_PERIOD_TIME_20MS;
    break;
    case UINJECT_SET_ULTIME_30:
      new_duration = UPLOAD_PERIOD_TIME_30MS;
    break;
    case UINJECT_SET_ULTIME_45:
    case UINJECT_SET_ULTIME_45_ABS:
      new_duration = UPLOAD_PERIOD_TIME_45MS;
    break;
    case UINJECT_SET_ULTIME_60:
      new_duration = UPLOAD_PERIOD_TIME_60MS;
    break;
    case UINJECT_SET_ULTIME_90_ABS:
      new_duration = UPLOAD_PERIOD_TIME_90MS;
    break;
    case UINJECT_SET_ULTIME_180:
      new_duration = UPLOAD_PERIOD_TIME_180MS;
    break;
    case UINJECT_SET_ULTIME_DEF:
      new_duration = UPLOAD_PERIOD_TIME_45MS;
    break;
    default:
      new_duration = UPLOAD_PERIOD_TIME_45MS;
  }

  opentimers_setPeriod(id, TIME_MS, new_duration);

  return 1;
}

int usaki_change_upload_time(uint8_t new_time_code, opentimer_id_t id){
  uint32_t new_duration;

  if ((new_time_code > USAKI_SET_ULTIME_90_ABS)||(new_time_code < USAKI_SET_ULTIME_5_ABS)){
    return -1;
  }

  switch(new_time_code){
    case USAKI_SET_ULTIME_5:
    case USAKI_SET_ULTIME_5_ABS:
      new_duration = UPLOAD_PERIOD_TIME_5MS;
    break;
    case USAKI_SET_ULTIME_15:
      new_duration = UPLOAD_PERIOD_TIME_15MS;
    break;
    case USAKI_SET_ULTIME_20:
    case USAKI_SET_ULTIME_20_ABS:
      new_duration = UPLOAD_PERIOD_TIME_20MS;
    break;
    case USAKI_SET_ULTIME_30:
      new_duration = UPLOAD_PERIOD_TIME_30MS;
    break;
    case USAKI_SET_ULTIME_45:
    case USAKI_SET_ULTIME_45_ABS:
      new_duration = UPLOAD_PERIOD_TIME_45MS;
    break;
    case USAKI_SET_ULTIME_60:
      new_duration = UPLOAD_PERIOD_TIME_60MS;
    break;
    case USAKI_SET_ULTIME_90:
    case USAKI_SET_ULTIME_90_ABS:
      new_duration = UPLOAD_PERIOD_TIME_90MS;
    break;
    case USAKI_SET_ULTIME_180:
      new_duration = UPLOAD_PERIOD_TIME_180MS;
    break;
    case USAKI_SET_ULTIME_DEF:
      new_duration = UPLOAD_PERIOD_TIME_30MS;
    break;
    default:
      new_duration = UPLOAD_PERIOD_TIME_30MS;
  }

  opentimers_setPeriod(id, TIME_MS, new_duration);

  return 1;
}

void uinject_receive(OpenQueueEntry_t* request) {
//   uint16_t          temp_l4_destination_port;
//   OpenQueueEntry_t* reply;
   uint8_t rcv_cmd;
   uinject_recv_t * pkt = request->payload;
   const uint8_t uinject_info[] = "ITRI MOTE";
   uint16_t serialNum;
/*
   reply = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
   if (reply==NULL) {
      openserial_printError(
         COMPONENT_UINJECT,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(request); //clear the request packet as well
      return;
   }

   reply->owner                         = COMPONENT_UINJECT;

   // reply with the same OpenQueueEntry_t
   reply->creator                       = COMPONENT_UINJECT;
   reply->l4_protocol                   = IANA_UDP;
   temp_l4_destination_port           = request->l4_destination_port;
   reply->l4_destination_port           = request->l4_sourcePortORicmpv6Type;
   reply->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
   reply->l3_destinationAdd.type        = ADDR_128B;
   
   // copy source to destination to echo.
   memcpy(&reply->l3_destinationAdd.addr_128b[0],&request->l3_sourceAdd.addr_128b[0],16);

   packetfunctions_reserveHeaderSize(reply,request->length);
   //memcpy(&reply->payload[0],&request->payload[0],request->length);

   if ((openudp_send(reply))==E_FAIL) {
      openqueue_freePacketBuffer(reply);
   }
*/
   rcv_cmd = pkt->cmdType;

   switch(rcv_cmd){
      case UINJECT_GET_INFO:
      break;
      case UINJECT_SET_PARENTS:
      break;
      case UINJECT_SET_LED:
        leds_debug_on();
      break;
      case UINJECT_UNSET_LED:
        leds_debug_off();
      break;
      case UINJECT_TOGGLE_LED:
        leds_debug_toggle();
      break;
      case UINJECT_SET_ACK:
        uinject_vars.needAck = TRUE;
      break;
      case UINJECT_UNSET_ACK:
        uinject_vars.needAck = FALSE;
      break;
      case UINJECT_RSP_ACK:
        serialNum = pkt->serialNumH;
        serialNum *= 256;
        serialNum += pkt->serialNumL;
        uinject_vars.rtnCounter = serialNum;
        if(uinject_vars.rtnCounter < 10)
          leds_debug_toggle();
      break;
      case UINJECT_SET_UPLOAD_TIME:
        uinject_change_upload_time(pkt->serialNumL, uinject_vars.timerId);
        uinject_vars.uinject_period_time_code = pkt->serialNumL;
        leds_debug_toggle();
      break;
      case USAKI_SET_UPLOAD_TIME:
        usaki_change_upload_time(pkt->serialNumL, usaki_vars.timerId);
        uinject_vars.usaki_period_time_code = pkt->serialNumL;
        leds_debug_toggle();
      break;
      case UALERT_SET_ON:
        //alarm_on();
	PC2_alarm_on = 1;	
      break;
      case UALERT_SET_OFF:
        //alarm_off();
	PC2_alarm_on = 0;	
      break;
      default:
        leds_debug_toggle();
   }

   //openserial_printData(rcv_dataa, 3);

   openqueue_freePacketBuffer(request);

#if 0
   openqueue_freePacketBuffer(pkt);
   
   openserial_printError(
      COMPONENT_UINJECT,
      ERR_RCVD_ECHO_REPLY,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
#endif

}

//=========================== private =========================================

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void uinject_timer_cb(opentimer_id_t id){
   
   scheduler_push_task(uinject_task_cb,TASKPRIO_COAP);
}

void uinject_task_cb() {
   OpenQueueEntry_t*    pkt;
   uint8_t              code=0;
   uint8_t              tmp_mask;
   uint8_t              uinjectPayloadLen;
   open_addr_t tmp_addr;
   uint8_t numTx, numTxAck, numPcnt=0;
   uint16_t tmp_num = 255;
   uint8_t              numNeighbor;
   uint8_t parentShortAddr[2];
   dagrank_t rank;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(uinject_vars.timerId);
      return;
   }

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UINJECT,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   pkt->owner                         = COMPONENT_UINJECT;
   pkt->creator                       = COMPONENT_UINJECT;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_INJECT;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],uinject_dst_addr,16);

   // check re-transmit mechanism
   if(uinject_vars.needAck){
      if (isAck()){
         uinject_vars.reTxNum = 0;
         leds_debug_toggle();
         uinject_vars.counter++;
         openqueue_freePacketBuffer(pkt);
         return;
      }else{
         if (uinject_vars.reTxNum > UINJECT_RETRANSMIT_CNT){
            uinject_vars.reTxNum = 0;
            uinject_vars.counter++;
            openqueue_freePacketBuffer(pkt);
            return;
         }else
            uinject_vars.reTxNum++;
            // send previous SN
            packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
            *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter;
            uinject_vars.rtnTimerId                 = opentimers_start(
              UINJECT_WAIT_RSP_TIME,
              TIMER_ONESHOT,TIME_MS,
              uinject_timer_cb
            );
      }
   }else{
      // send counter value
      packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
      *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter;
      uinject_vars.counter++; 
   }

   // find number of neighbors
   numNeighbor = neighbors_getNumNeighbors();
   if(numNeighbor==0) {
      openqueue_freePacketBuffer(pkt);
      return;
   }

   //rank                      = neighbors_getMyDAGrank();
   rank                      = icmpv6rpl_getMyDAGrank();

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
   code |= (uint8_t)uinject_vars.needAck << UINJECT_CODE_MASK_NEEDACK;

   // update upload time value
   tmp_mask = 0x03 << UINJECT_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_vars.uinject_period_time_code << UINJECT_CODE_LOC_ULTIME;
   code |= tmp_mask;

   tmp_mask = 0x03 << USAKI_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_vars.usaki_period_time_code << USAKI_CODE_LOC_ULTIME;
   code |= tmp_mask;

   // find total packet size, 5 = code/numPcnt/numNeighbor/ParentShortAddr
   if (numNeighbor < MAX_ALLOW_NEIGHBORS)
      uinjectPayloadLen = 7 + numNeighbor*3;
   else
      uinjectPayloadLen = 7 + MAX_ALLOW_NEIGHBORS*3;

   // allocate packet size
   packetfunctions_reserveHeaderSize(pkt,uinjectPayloadLen);

   *((uint8_t*)&pkt->payload[0]) = code;
   *((uint8_t*)&pkt->payload[1]) = numPcnt;

   memcpy(&pkt->payload[2],parentShortAddr,2);

   memcpy(&(pkt->payload[4]),&rank,sizeof(rank));

   *((uint8_t*)&pkt->payload[6]) = numNeighbor;

   neighbors_getNshortAddrnRSSI(&(pkt->payload[7]));

   // send out packet   
   if ((openudp_send(pkt))==E_FAIL) 
      openqueue_freePacketBuffer(pkt);

}
