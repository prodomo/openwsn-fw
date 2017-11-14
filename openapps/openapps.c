/**
\brief Applications running on top of the OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, September 2014.
*/

#include "opendefs.h"

// CoAP
#include "opencoap.h"
#include "c6t.h"
#include "cinfo.h"
#include "cleds.h"
#include "cwellknown.h"
#include "rrt.h"
#include "cgreen.h"
#include "creportasn.h"
#include "cexample.h"
#include "cstorm.h"
#include "emergency.h"
// UDP
#include "uecho.h"
#include "uinject.h"
#include "userialbridge.h"
#include "uexpiration.h"
#include "uexpiration_monitor.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

//=========================== private =========================================

void openapps_init(void) {
   //-- 04-TRAN
   opencoap_init();     // initialize before any of the CoAP applications
   
   // CoAP
   c6t_init();
   cinfo_init();
   cgreen_init();
   //cexample_init();
   cleds__init();
   cwellknown_init();
   rrt_init();

   creportasn_init();
   emergency_init();
   
   // UDP
   uecho_init();
   uinject_init();
   userialbridge_init();
   uexpiration_init();
   umonitor_init();
}
