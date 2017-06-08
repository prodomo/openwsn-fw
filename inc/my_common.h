#ifndef _MY_COMMON_
#define _MY_COMMON_

/**
\addtogroup AppUdp
\{
\addtogroup uinject
\{
*/

#include "opentimers.h"

//=========================== define ==========================================

//#define UINJECT_PERIOD_MS 30000

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t             int_temp;
   uint16_t             ext_temp;
   uint16_t             ext_pyra;
   uint16_t             int_volt;
   uint16_t		gpio_pulse;
} usaki_vars_t;

//=========================== prototypes ======================================

void usaki_init(void);
void usaki_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void usaki_receive(OpenQueueEntry_t* msg);

/**
\addtogroup AppUdp
\{
\addtogroup uinject
\{
*/

//=========================== define ==========================================

//#define UINJECT_PERIOD_MS 40000
#define UINJECT_RETRANSMIT_CNT 2
#define UINJECT_WAIT_RSP_TIME 3000
#define MAX_ALLOW_NEIGHBORS          10

#define UPLOAD_PERIOD_TIME_5MS 5000
#define UPLOAD_PERIOD_TIME_15MS 15000
#define UPLOAD_PERIOD_TIME_20MS 20000
#define UPLOAD_PERIOD_TIME_30MS 30000
#define UPLOAD_PERIOD_TIME_45MS 45000
#define UPLOAD_PERIOD_TIME_60MS 60000
#define UPLOAD_PERIOD_TIME_90MS 90000
#define UPLOAD_PERIOD_TIME_180MS 180000
#define UPLOAD_PERIOD_TIME_DEFAULT UPLOAD_PERIOD_TIME_45MS

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   bool                 needAck;
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   opentimer_id_t       rtnTimerId;  ///< periodic timer which triggers transmission
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t             rtnCounter;
   uint8_t              reTxNum;
   uint16_t             uinject_period_time;
   uint16_t             usaki_period_time;
   uint8_t              uinject_period_time_code;
   uint8_t              usaki_period_time_code;
} uinject_vars_t;

typedef struct {
   uint8_t       cmdType;
   uint8_t       serialNumL;
   uint8_t       serialNumH;
//   union{
//      uint8_t ctrlCmd;
//      uint8_t setParents[16];
//   }
} uinject_recv_t;

typedef enum UINJECT_CMD_TYPE{
  UINJECT_SET_LED = 0,
  UINJECT_UNSET_LED,
  UINJECT_TOGGLE_LED,
  UINJECT_SET_PARENTS,
  UINJECT_SET_RETRANSMIT,
  UINJECT_UNSET_RETRANSMIT,
  UINJECT_GET_INFO,
  UINJECT_RSP_ACK = 10,
  UINJECT_SET_ACK,
  UINJECT_UNSET_ACK,
  UINJECT_SET_UPLOAD_TIME = 20,
  UINJECT_SET_ULTIME_5,
  UINJECT_SET_ULTIME_15,
  UINJECT_SET_ULTIME_20,
  UINJECT_SET_ULTIME_30,
  UINJECT_SET_ULTIME_45,
  UINJECT_SET_ULTIME_60,
  UINJECT_SET_ULTIME_90,
  UINJECT_SET_ULTIME_180,
  UINJECT_SET_ULTIME_DEF,
  USAKI_SET_UPLOAD_TIME = 30,
  USAKI_SET_ULTIME_5,
  USAKI_SET_ULTIME_15,
  USAKI_SET_ULTIME_20,
  USAKI_SET_ULTIME_30,
  USAKI_SET_ULTIME_45,
  USAKI_SET_ULTIME_60,
  USAKI_SET_ULTIME_90,
  USAKI_SET_ULTIME_180,
  USAKI_SET_ULTIME_DEF,
  UALERT_SET_ON = 40,
  UALERT_SET_OFF
}uinject_cmd_type_t;

#define UINJECT_SET_ULTIME_5_ABS 0
#define UINJECT_SET_ULTIME_20_ABS 1
#define UINJECT_SET_ULTIME_45_ABS 2
#define UINJECT_SET_ULTIME_90_ABS 3

#define USAKI_SET_ULTIME_5_ABS 0
#define USAKI_SET_ULTIME_20_ABS 1
#define USAKI_SET_ULTIME_45_ABS 2
#define USAKI_SET_ULTIME_90_ABS 3
//=========================== prototypes ======================================

void uinject_init(void);
void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uinject_receive(OpenQueueEntry_t* request);

#endif
