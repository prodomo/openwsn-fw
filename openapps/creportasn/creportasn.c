/**
\brief An report ASN CoAP application.
*/

#include "opendefs.h"
#include "creportasn.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
//#include "ADC_Channel.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "IEEE802154.h"

#include "neighbors.h"
#include "icmpv6rpl.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define CREPORTASNPERIOD  20000
#define PAYLOADLEN      20

const uint8_t creportasn_path0[] = "reportasn";

//=========================== variables =======================================

creportasn_vars_t creportasn_vars;

//=========================== prototypes ======================================

owerror_t creportasn_receive(OpenQueueEntry_t* msg,
                    coap_header_iht*  coap_header,
                    coap_option_iht*  coap_options);
void    creportasn_timer_cb(opentimer_id_t id);
void    creportasn_task_cb(void);
void    creportasn_sendDone(OpenQueueEntry_t* msg,
                       owerror_t error);

//=========================== public ==========================================

void creportasn_init() {
   
   // prepare the resource descriptor for the /ex path
   creportasn_vars.desc.path0len             = sizeof(creportasn_path0)-1;
   creportasn_vars.desc.path0val             = (uint8_t*)(&creportasn_path0);
   creportasn_vars.desc.path1len             = 0;
   creportasn_vars.desc.path1val             = NULL;
   creportasn_vars.desc.componentID          = COMPONENT_CREPORTASN;
   creportasn_vars.desc.discoverable         = TRUE;
   creportasn_vars.desc.callbackRx           = &creportasn_receive;
   creportasn_vars.desc.callbackSendDone     = &creportasn_sendDone;
   
   creportasn_vars.creportasn_sequence = 0;
   creportasn_vars.lastSuccessLeft = 0;
   creportasn_vars.errorCounter = 0;
   
   opencoap_register(&creportasn_vars.desc);
   creportasn_vars.timerId    = opentimers_start(CREPORTASNPERIOD,
                                                TIMER_PERIODIC,TIME_MS,
                                                creportasn_timer_cb);
}

//=========================== private =========================================

owerror_t creportasn_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   return E_FAIL;
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void creportasn_timer_cb(opentimer_id_t id){
   scheduler_push_task(creportasn_task_cb,TASKPRIO_COAP);
}

void creportasn_task_cb() {
   OpenQueueEntry_t*    pkt;
   owerror_t            outcome;
   
   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      //opentimers_stop(creportasn_vars.timerId);
      openserial_printError(123,66,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      return;
   }

   openqueue_removeAllCreatedBy(COMPONENT_CREPORTASN);

   // create a CoAP RD packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_CREPORTASN);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_CREPORTASN,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_CREPORTASN;
   pkt->owner                     = COMPONENT_CREPORTASN;
   // CoAP payload
   packetfunctions_reserveHeaderSize(pkt,PAYLOADLEN);
   // speacial flag
   pkt->payload[0] = 0x54;
   pkt->payload[1] = 0x66;

   // append asn
   uint8_t* pointer = &pkt->payload[2];
   ieee154e_getAsn(pointer);

   uint8_t numDeSync;
   ieee154e_getNumDesync(&numDeSync);

   pkt->payload[12] = numDeSync;

   uint16_t myRank = icmpv6rpl_getMyDAGrank();

   memcpy(&pkt->payload[13], &myRank, sizeof(myRank));

   uint8_t parentIndex;


   icmpv6rpl_getPreferredParentIndex(&parentIndex);

   neighborRow_t parentRow;
   uint8_t parentTx;
   uint8_t parentTxACK;
   neighbors_getParentTxTxACK(&parentTx, &parentTxACK, parentIndex);
   
   pkt->payload[15] = parentTx;
   pkt->payload[16] = parentTxACK;

   pkt->payload[17] = creportasn_vars.lastSuccessLeft;
   pkt->payload[18] = creportasn_vars.errorCounter;

   creportasn_vars.creportasn_sequence++;

   pkt->payload[19] = creportasn_vars.creportasn_sequence;
   
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_PAYLOAD_MARKER;
   

   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                = (COAP_OPTION_NUM_CONTENTFORMAT - COAP_OPTION_NUM_URIPATH) << 4
                                    | 1;
   pkt->payload[1]                = COAP_MEDTYPE_APPOCTETSTREAM;
   // location-path option

   packetfunctions_reserveHeaderSize(pkt,sizeof(creportasn_path0)-1);
   memcpy(&pkt->payload[0],creportasn_path0,sizeof(creportasn_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                = ((COAP_OPTION_NUM_URIPATH) << 4) | (sizeof(creportasn_path0)-1);
   
   // metadata
   pkt->l4_destination_port       = WKP_UDP_COAP;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_ringmaster,16);
   
   pkt->l2_frameType = IEEE154_TYPE_SENSED_DATA;

   // send
   outcome = opencoap_send(
      pkt,
      COAP_TYPE_NON,
      COAP_CODE_REQ_PUT,
      1,
      &creportasn_vars.desc
   );
   
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
   return;
}

void creportasn_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   if(error == E_FAIL){
      creportasn_vars.errorCounter++;
   }
   creportasn_vars.lastSuccessLeft = msg->l2_retriesLeft;
   openqueue_freePacketBuffer(msg);
}