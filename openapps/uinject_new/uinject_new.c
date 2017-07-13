#include "opendefs.h"
#include "uinject_new.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"

#include "IEEE802154E.h"
#include "IEEE802154.h"
#include "idmanager.h"

#include "gpio.h"
#include <headers/hw_memmap.h>
#include "leds.h"
#include "neighbors.h"
#include "icmpv6rpl.h"
#include "my_common.h"


extern usaki_vars_t usaki_vars;
uinject_new_vars_t uinject_new_vars;
uint8_t PC2_status = 0;
uint8_t PC2_alarm_on;
opentimer_id_t buzz_timer;

static const uint8_t uinject_new_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 


//=========================== prototypes ======================================
void uinject_new_receive(OpenQueueEntry_t* pkt);
void uinject_new_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uinject_new_timer_cb(opentimer_id_t id);
void uinject_new_task_cb(void);
void usaki_buzz_timer_cb(opentimer_id_t id);
void usaki_buzz_task_cb(void);
// //=========================== public ==========================================
bool isAck(){
  
  if (uinject_new_vars.rtnCounter == uinject_new_vars.counter)
    return TRUE;
  else
    return FALSE;
}

void alarm_on(void){
   //opentimers_stop(buzz_timer);

   opentimers_restart(buzz_timer);
}

void alarm_off(void){
  
  //opentimers_stop(buzz_timer);
  GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, ALARM_PC2_OFF);
  
}

void uinject_new_init(){   
   // clear local variables
   memset(&uinject_new_vars,0,sizeof(uinject_new_vars_t));
   // initial some variables
   uinject_new_vars.needAck = FALSE;
   uinject_new_vars.reTxNum = 0;
   uinject_new_vars.counter = 1;
   uinject_new_vars.rtnCounter = 0;
   uinject_new_vars.uinject_period_time = UPLOAD_PERIOD_TIME_45MS;
   uinject_new_vars.uinject_period_time_code = UINJECT_SET_ULTIME_45_ABS;

   uinject_new_vars.uinject_period_time = UINJECT_NEW_PERIOD;
   
   // start periodic timer
      uinject_new_vars.timerId                    = opentimers_start(
      uinject_new_vars.uinject_period_time,
      TIMER_PERIODIC,TIME_MS,
      uinject_new_timer_cb
   );
   alarm_off();

   PC2_alarm_on = 0;

   buzz_timer = opentimers_start(
      100,
      TIMER_PERIODIC,TIME_MS,
      usaki_buzz_timer_cb
   );

   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);
   GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, GPIO_PIN_2);

   alarm_off();

}


void uinject_new_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
void usaki_buzz_timer_cb(opentimer_id_t id){

   scheduler_push_task(usaki_buzz_task_cb,TASKPRIO_COAP);
}

void usaki_buzz_task_cb(){

  // periodically low/high to PC2
  if (PC2_alarm_on == 1){
    if (PC2_status){
      // set high
      GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, ALARM_PC2_ON);
      PC2_status = 0;
    }else{
      // set low
      GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, ALARM_PC2_OFF);
      PC2_status = 1;
    }
  }else{
    GPIOPinWrite(GPIO_C_BASE, GPIO_PIN_2, ALARM_PC2_OFF);
  }
}

int uinject_change_upload_time(uint8_t new_time_code, opentimer_id_t id){
  uint32_t new_duration;

  if ((new_time_code > UINJECT_SET_ULTIME_90_ABS)||(new_time_code < UINJECT_SET_ULTIME_5_ABS)){
    return -1;
  }

  switch(new_time_code){
    case UINJECT_SET_ULTIME_5:
    case UINJECT_SET_ULTIME_5_ABS:
      new_duration = UPLOAD_PERIOD_TIME_8MS;
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
      new_duration = UPLOAD_PERIOD_TIME_8MS;
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
void uinject_new_receive(OpenQueueEntry_t* msg) {

   uint8_t rcv_cmd;
   uinject_new_recv_t * pkt = (uinject_new_recv_t*)msg->payload;
   uint16_t serialNum;

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
        uinject_new_vars.needAck = TRUE;
      break;
      case UINJECT_UNSET_ACK:
        uinject_new_vars.needAck = FALSE;
      break;
      case UINJECT_RSP_ACK:
        serialNum = pkt->serialNumH;
        serialNum *= 256;
        serialNum += pkt->serialNumL;
        uinject_new_vars.rtnCounter = serialNum;
        if(uinject_new_vars.rtnCounter < 10)
          leds_debug_toggle();
      break;
      case UINJECT_SET_UPLOAD_TIME:
        uinject_change_upload_time(pkt->serialNumL, uinject_new_vars.timerId);
        uinject_new_vars.uinject_period_time_code = pkt->serialNumL;
        leds_debug_toggle();
      break;
      case USAKI_SET_UPLOAD_TIME:
        usaki_change_upload_time(pkt->serialNumL, usaki_vars.timerId);
        uinject_new_vars.usaki_period_time_code = pkt->serialNumL;
        leds_debug_toggle();
      break;
      case UALERT_SET_ON:
      	// PC2_alarm_on = 1;
      	leds_debug_toggle();
      break;
      case UALERT_SET_OFF:
      	// PC2_alarm_on = 0;
      	leds_debug_toggle();
      break;
      default:
      	leds_debug_toggle();
      }

      openqueue_freePacketBuffer(msg);


}

void uinject_new_timer_cb(opentimer_id_t id){
   
   scheduler_push_task(uinject_new_task_cb,TASKPRIO_COAP);
}

void uinject_new_task_cb() {

   OpenQueueEntry_t*    pkt;
   uint8_t              code=0;
   uint8_t              tmp_mask;
   uint8_t              uinjectPayloadLen;
   open_addr_t 			tmp_addr;
   uint8_t 				numTx, numTxAck, numPcnt=0;
   uint16_t 			tmp_num = 255;
   uint8_t              numNeighbor;
   uint8_t 				parentShortAddr[2];
   dagrank_t 			rank;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) 
      return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(uinject_new_vars.timerId);
      return;
   }

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT_NEW);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UINJECT_NEW,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   pkt->owner                         = COMPONENT_UINJECT_NEW;
   pkt->creator                       = COMPONENT_UINJECT_NEW;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_INJECT_NEW;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT_NEW;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],uinject_new_dst_addr,16);

   // find number of neighbors
   numNeighbor = neighbors_getNumNeighbors();
   if(numNeighbor==0) {
      openqueue_freePacketBuffer(pkt);
      return;
   }

   // check re-transmit mechanism
   if(uinject_new_vars.needAck){
      if (isAck()){
         uinject_new_vars.reTxNum = 0;
         leds_debug_toggle();
         uinject_new_vars.counter++;
         openqueue_freePacketBuffer(pkt);
         return;
      }else{
         if (uinject_new_vars.reTxNum > UINJECT_RETRANSMIT_CNT){
            uinject_new_vars.reTxNum = 0;
            uinject_new_vars.counter++;
            openqueue_freePacketBuffer(pkt);
            return;
         }else
            uinject_new_vars.reTxNum++;
            // send previous SN
            packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
            *((uint16_t*)&pkt->payload[0]) = uinject_new_vars.counter;
            uinject_new_vars.rtnTimerId = opentimers_start(
              UINJECT_WAIT_RSP_TIME,
              TIMER_ONESHOT,TIME_MS,
              uinject_new_timer_cb
            );
      }
   }else{
      // send counter value
      packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
      *((uint16_t*)&pkt->payload[0]) = uinject_new_vars.counter;
      uinject_new_vars.counter++; 
   }


   //rank                      = neighbors_getMyDAGrank();
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
   code |= (uint8_t)uinject_new_vars.needAck << UINJECT_CODE_MASK_NEEDACK;

   // update upload time value
   tmp_mask = 0x03 << UINJECT_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_new_vars.uinject_period_time_code << UINJECT_CODE_LOC_ULTIME;
   code |= tmp_mask;

   tmp_mask = 0x03 << USAKI_CODE_LOC_ULTIME;
   code &= ~tmp_mask;
   tmp_mask = uinject_new_vars.usaki_period_time_code << USAKI_CODE_LOC_ULTIME;
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

