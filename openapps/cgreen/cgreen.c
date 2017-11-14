/**
\brief An example CoAP application.
*/

#include "opendefs.h"
#include "cgreen.h"
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

//=========================== defines =========================================

const uint8_t cgreen_path0[] = "green";

//=========================== variables =======================================

cgreen_vars_t cgreen_vars;

//=========================== prototypes ======================================

owerror_t cgreen_receive(OpenQueueEntry_t* msg,
                    coap_header_iht*  coap_header,
                    coap_option_iht*  coap_options);
void    cgreen_sendDone(OpenQueueEntry_t* msg,
                       owerror_t error);

//=========================== public ==========================================

void cgreen_init() {
   
   // prepare the resource descriptor for the /g path
   cgreen_vars.desc.path0len             = sizeof(cgreen_path0)-1;
   cgreen_vars.desc.path0val             = (uint8_t*)(&cgreen_path0);
   cgreen_vars.desc.path1len             = 0;
   cgreen_vars.desc.path1val             = NULL;
   cgreen_vars.desc.componentID          = COMPONENT_CEXAMPLE;
   cgreen_vars.desc.discoverable         = TRUE;
   cgreen_vars.desc.callbackRx           = &cgreen_receive;
   cgreen_vars.desc.callbackSendDone     = &cgreen_sendDone;
   
   
   opencoap_register(&cgreen_vars.desc);
}

//=========================== private =========================================

owerror_t cgreen_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   openserial_printInfo(COMPONENT_CGREEN, ERR_BOOTED, 0, 0);
   owerror_t outcome;
   
   switch (coap_header->Code) {
      case COAP_CODE_REQ_GET:
         
         //=== reset packet payload (we will reuse this packetBuffer)
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         //=== prepare  CoAP response
         
         // radio name
         packetfunctions_reserveHeaderSize(msg,sizeof(infoRadioName)-1);
         memcpy(&msg->payload[0],&infoRadioName,sizeof(infoRadioName)-1);
         
         // uC name
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infouCName)-1);
         memcpy(&msg->payload[0],&infouCName,sizeof(infouCName)-1);
         
         // board name
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infoBoardname)-1);
         memcpy(&msg->payload[0],&infoBoardname,sizeof(infoBoardname)-1);
         
         // stack name and version
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infoStackName)-1+5);
         memcpy(&msg->payload[0],&infoStackName,sizeof(infoStackName)-1);
         msg->payload[sizeof(infoStackName)-1+5-5] = '0'+OPENWSN_VERSION_MAJOR;
         msg->payload[sizeof(infoStackName)-1+5-4] = '.';
         msg->payload[sizeof(infoStackName)-1+5-3] = '0'+OPENWSN_VERSION_MINOR;
         msg->payload[sizeof(infoStackName)-1+5-2] = '.';
         msg->payload[sizeof(infoStackName)-1+5-1] = '0'+OPENWSN_VERSION_PATCH;
         
         // payload marker
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = COAP_PAYLOAD_MARKER;
         
         // set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;
         break;
      case COAP_CODE_REQ_POST:
         // try sending data
         // openserial_printInfo(COMPONENT_CGREEN, ERR_BUSY_RECEIVING, 1, 0);
         
         // test if need reset
         if(msg->payload[0] == (1 << 7)){
            schedule_resetAllDistributeCell();
         }
         
         //get entry count;
         uint8_t entryCount = msg->payload[1];
         // openserial_printInfo(COMPONENT_CGREEN, ERR_UNSUPPORTED_COMMAND, entryCount, 0);
         for(int i=0; i<entryCount; i++){
            uint8_t baseOffset = i*11+2;
            // openserial_printInfo(COMPONENT_CGREEN, ERR_MSG_UNKNOWN_TYPE, baseOffset, 0);
            open_addr_t     temp_neighbor;
            memset(&temp_neighbor,0,sizeof(temp_neighbor));
            temp_neighbor.type = ADDR_64B;
            uint8_t slotOffset = msg->payload[baseOffset];
            uint8_t channelOffset = msg->payload[baseOffset+1];
            uint8_t cellType = msg->payload[baseOffset+2];
            for(int j=0; j<8; j++){
               temp_neighbor.addr_64b[j] = msg->payload[baseOffset+3+j];
            }
            
            if(cellType==(1<<6)){  //1 TX 0 RX
               cellType = CELLTYPE_TX;
            }else{
               cellType = CELLTYPE_RX;
            }
            schedule_addActiveSlot(
               slotOffset,                    // slot offset
               cellType,                     // type of slot
               FALSE,                                 // shared?
               channelOffset,                                     // channel offset
               &temp_neighbor                         // neighbor
            );
            //for chunk size 2
            // schedule_addActiveSlot(
            //    slotOffset+1,                    // slot offset
            //    cellType,                     // type of slot
            //    FALSE,                                 // shared?
            //    channelOffset,                                     // channel offset
            //    &temp_neighbor                         // neighbor
            // );
         }
         
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;

         break;
      case COAP_CODE_REQ_DELETE:
         // new method
         // openserial_printInfo(COMPONENT_CGREEN, ERR_INVALIDSERIALFRAME, 0, 0);
         //reset packet
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;


         char messageToSend[] = "Hello from Green.";

         packetfunctions_reserveHeaderSize(msg, sizeof(messageToSend)-1);
         memcpy(&msg->payload[0], &messageToSend, sizeof(messageToSend)-1);
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = COAP_PAYLOAD_MARKER;
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;

         break;
      default:
         // return an error message
         outcome = E_FAIL;
   }
   
   return outcome;
}

void cgreen_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
