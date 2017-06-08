//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "my_spi.h"
//#include "hw_memmap.h"
#include <headers/hw_ints.h>
#include <headers/hw_ssi.h>
#include <headers/hw_memmap.h>
#include "hw_ioc.h"
#include "gpio.h"
#include "ioc.h"
#include "ssi.h"
#include "uart.h"
#include "sys_ctrl.h"

//*****************************************************************************
#define PIN_SSI_CLK             GPIO_PIN_2 
#define PIN_SSI_FSS             GPIO_PIN_1
#define PIN_SSI_RX              GPIO_PIN_5
#define PIN_SSI_TX              GPIO_PIN_4
#define GPIO_SSI_BASE           GPIO_D_BASE
#define GPIO_SSI_RST_BASE       GPIO_B_BASE
#define PIN_SSI_RST             GPIO_PIN_1

#define GPIO_SSI_INT_BASE       GPIO_A_BASE
#define GPIO_SSI_INT_0          GPIO_PIN_6
//#define GPIO_SSI_INT_1          GPIO_PIN_7

#define CC1200_WRITE_BIT        0x00
#define CC1200_READ_BIT         0x80
#define CC1200_BURST_BIT                0x40
#define CC1200_EXTENDED_WRITE_CMD       (0x2F | CC1200_WRITE_BIT)
#define CC1200_EXTENDED_BURST_WRITE_CMD \
  (0x2F | CC1200_BURST_BIT | CC1200_WRITE_BIT)
#define CC1200_EXTENDED_READ_CMD        (0x2F | CC1200_READ_BIT)
#define CC1200_EXTENDED_BURST_READ_CMD \
  (0x2F | CC1200_BURST_BIT | CC1200_READ_BIT)
/****************************************************************/
#define CC1200_IS_EXTENDED_ADDR(x)      (x & 0x2F00)
/****************************************************************/
// Macro for asserting accelerometer CSn (set low)
#define CC1200_SPI_BEGIN()         GPIOPinWrite(GPIO_SSI_BASE, PIN_SSI_FSS, 0);

// Macro for deasserting accelerometer CSn (set high)
#define CC1200_SPI_END()           GPIOPinWrite(GPIO_SSI_BASE, PIN_SSI_FSS, PIN_SSI_FSS);
/****************************************************************/
/*
 * We have to prevent duplicate SPI access.
 * We therefore LOCK SPI in order to prevent the rx interrupt to
 * interfere.
 */
static volatile uint8_t spi_locked = 0;
#define LOCK_SPI()                      do { spi_locked++; } while(0)
#define SPI_IS_LOCKED()                 (spi_locked != 0)
#define RELEASE_SPI()                   do { spi_locked--; } while(0)

void CC1200WriteReg(uint8_t ui8Addr, const uint8_t *pui8Buf, uint8_t ui8Len);
/*****************************************************************************/

//#define CC1200_RF_TESTMODE 3

/* The RF configuration to be used. */
#ifdef CC1200_CONF_RF_CFG
#define CC1200_RF_CFG                   CC1200_CONF_RF_CFG
#else
#define CC1200_RF_CFG                   cc1200_802154g_863_870_fsk_50kbps
#endif

#if CC1200_RF_TESTMODE
#undef CC1200_RF_CFG
#if CC1200_RF_TESTMODE == 1
#define CC1200_RF_CFG                   cc1200_802154g_863_870_fsk_50kbps
#elif CC1200_RF_TESTMODE == 2
#define CC1200_RF_CFG                   cc1200_802154g_863_870_fsk_50kbps
#elif CC1200_RF_TESTMODE == 3
#define CC1200_RF_CFG                   cc1200_802154g_863_870_fsk_50kbps
#endif
#endif


extern cc1200_rf_cfg_t cc1200_802154g_863_870_fsk_50kbps;

/*****************************************************************************/

//*****************************************************************************
//
// Configure SSI0 in master Freescale (SPI) mode.  This example will send out
// 3 bytes of data, then wait for 3 bytes of data to come in.  This will all be
// done using the polling method.
//
//*****************************************************************************
int8_t txpower;

/* Update TX power */
static void
update_txpower(int8_t txpower_dbm)
{

  uint8_t reg = my_SPI_recv(CC120X_PA_CFG1);

  reg &= ~0x3F;
  /* Up to now we don't handle the special power levels PA_POWER_RAMP < 3 */
  reg |= ((((txpower_dbm + 18) * 2) - 1) & 0x3F);
  my_SPI_send(CC120X_PA_CFG1, reg);

  txpower = txpower_dbm;

}

/* Calculate FREQ register from channel */
#define FREQ_DIVIDER                    625
#define FREQ_MULTIPLIER                 4096
static uint32_t
calculate_freq(uint8_t channel)
{

  uint32_t freq;

  freq = CC1200_RF_CFG.chan_center_freq0 + channel * CC1200_RF_CFG.chan_spacing;
  freq *= FREQ_MULTIPLIER;
  freq /= FREQ_DIVIDER;

  return freq;

}

void cc1200_reg_configure(void){
    int8_t rssi;
    int i;
    const registerSetting_t * tmp_reg_p;

    leds_sync_on();
    // reset 
    trxSpiCmdStrobe(CC120X_SRES,1);

#if CC1200_RF_TESTMODE 
    uint32_t freq=0;
#endif

    // set prefix registers
    i = CC1200_RF_CFG.size_of_register_settings / sizeof(registerSetting_t);
    if(CC1200_RF_CFG.register_settings != NULL){
      tmp_reg_p = CC1200_RF_CFG.register_settings;
      while(i--){
        //CC1200WriteReg(tmp_reg_p->addr, (uint8_t*)&(tmp_reg_p->val), 1);
        my_SPI_send(tmp_reg_p->addr, tmp_reg_p->val);
        tmp_reg_p++;
      }
    }
    // RSSI offset
    rssi = (-81);//CC1200_RSSI_OFFSET;
    CC1200WriteReg(CC120X_AGC_GAIN_ADJUST, (int8_t*)&rssi, 1);

#if (CC1200_RF_TESTMODE == 1) || (CC1200_RF_TESTMODE == 2)
  trxSpiCmdStrobe(CC120X_SFTX,1);
  //single_write(CC1200_TXFIRST, 0);
  my_SPI_send(CC120X_TXFIRST,0);
  //single_write(CC1200_TXLAST, 0xFF);
  my_SPI_send(CC120X_TXLAST,0xFF);
  update_txpower(CC120X_CONST_TX_POWER_MAX);
  my_SPI_send(CC120X_PKT_CFG2, 0x02);
  freq = calculate_freq(CC1200_DEFAULT_CHANNEL - CC1200_RF_CFG.min_channel);
  my_SPI_send(CC120X_FREQ0, ((uint8_t *)&freq)[0]);
  my_SPI_send(CC120X_FREQ1, ((uint8_t *)&freq)[1]);
  my_SPI_send(CC120X_FREQ2, ((uint8_t *)&freq)[2]);

  //printf("RF: Freq0 0x%02x\n",  ((uint8_t *)&freq)[0]);
  //printf("RF: Freq1 0x%02x\n",  ((uint8_t *)&freq)[1]);
  //printf("RF: Freq2 0x%02x\n",  ((uint8_t *)&freq)[2]);
  //my_openserial_printStatus(0x19, (uint8_t*)&freq, 4);

#if (CC1200_RF_TESTMODE == 1)
  my_SPI_send(CC120X_SYNC_CFG1, 0xE8);
  my_SPI_send(CC120X_PREAMBLE_CFG1, 0x00);
  my_SPI_send(CC120X_MDMCFG1, 0x46);
  my_SPI_send(CC120X_PKT_CFG0, 0x40);
  my_SPI_send(CC120X_FS_DIG1, 0x07);
  my_SPI_send(CC120X_FS_DIG0, 0xAA);
  my_SPI_send(CC120X_FS_DVC1, 0xFF);
  my_SPI_send(CC120X_FS_DVC0, 0x17);
#endif
#if (CC1200_RF_TESTMODE == 2)
  my_SPI_send(CC120X_SYNC_CFG1, 0xE8);
  my_SPI_send(CC120X_PREAMBLE_CFG1, 0x00);
  my_SPI_send(CC120X_MDMCFG1, 0x06);
  my_SPI_send(CC120X_PA_CFG1, 0x3F);
  my_SPI_send(CC120X_MDMCFG2, 0x03);
  my_SPI_send(CC120X_FS_DIG1, 0x07);
  my_SPI_send(CC120X_FS_DIG0, 0xAA);
  my_SPI_send(CC120X_FS_DVC0, 0x17);
  my_SPI_send(CC120X_SERIAL_STATUS, 0x08);
#endif

  trxSpiCmdStrobe(CC120X_STX,1);
  
  leds_sync_off();

  while(1) {
    SysCtrlDelay(SysCtrlClockGet() / 500 / 3);
  }  

#endif // end of MODE1/MODE2

#if (CC1200_RF_TESTMODE == 3)
  /* CS on GPIO3 */
  my_SPI_send(CC120X_IOCFG3, 17); // CC120X_IOCFG_CARRIER_SENSE);
  //my_SPI_send(CC1200_IOCFG2, CC1200_IOCFG_SERIAL_CLK);
  //my_SPI_send(CC1200_IOCFG0, CC1200_IOCFG_SERIAL_RX);
  // update cca threashold
  my_SPI_send(CC120X_AGC_CS_THR, CC1200_RF_CFG.cca_threshold);
  freq = calculate_freq(CC1200_DEFAULT_CHANNEL - CC1200_RF_CFG.min_channel);
  my_SPI_send(CC120X_FREQ0, ((uint8_t *)&freq)[0]);
  my_SPI_send(CC120X_FREQ1, ((uint8_t *)&freq)[1]);
  my_SPI_send(CC120X_FREQ2, ((uint8_t *)&freq)[2]);
  trxSpiCmdStrobe(CC120X_SRX, 1);

  GPIOPinTypeGPIOInput(GPIO_SSI_BASE, GPIO_PIN_3);

  while(1){
    if(GPIOPinRead(GPIO_SSI_BASE, GPIO_PIN_3) != 0)
      leds_sync_on();
    else
      leds_sync_off();      
  }
#endif // end of MODE3
}

static void GPIO_Ap6_Handler(void) {
    /* Disable the interrupts */
    GPIOPinIntClear(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0);
    // real process

}

void my_SPI_init(void)
{   
    uint32_t ui32Dummy;

    LOCK_SPI();

    // configure the Reset pin for CC1200, keep it stay high
    GPIOPinTypeGPIOOutput(GPIO_SSI_RST_BASE, PIN_SSI_RST);
    //IOCPadConfigSet(GPIO_SSI_BASE, PIN_SSI_RST, IOC_OVERRIDE_OE);
    GPIOPinWrite(GPIO_SSI_RST_BASE, PIN_SSI_RST, PIN_SSI_RST);   

    //
    // Enable the SSI Peripherals
    //
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_SSI0);
    //SysCtrlPeripheralSleepEnable(SYS_CTRL_PERIPH_SSI0);

    // Reset peripheral previous to configuring it
    SysCtrlPeripheralReset(SYS_CTRL_PERIPH_SSI0);

    // 
    // Disable SSI function before configuring module
    //
    SSIDisable(SSI0_BASE);
    //HWREG(SSI0_BASE + SSI_O_CR1) = 0;

    // set the system clock   
    SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_PIOSC);

    //Configure the SSI pins. 
    IOCPinConfigPeriphOutput(GPIO_SSI_BASE, PIN_SSI_CLK, 
                             IOC_MUX_OUT_SEL_SSI0_CLKOUT);    
    
    IOCPinConfigPeriphOutput(GPIO_SSI_BASE, PIN_SSI_TX, 
                             IOC_MUX_OUT_SEL_SSI0_TXD);    
    
    IOCPinConfigPeriphInput(GPIO_SSI_BASE, PIN_SSI_RX, 
                            IOC_SSIRXD_SSI0);    
            

    // configure pins for use by SSI
    GPIOPinTypeSSI(GPIO_SSI_BASE, (PIN_SSI_CLK |PIN_SSI_RX |PIN_SSI_TX));

    // Configure SSI module to Motorola/Freescale SPI mode 3:
    // Polarity  = 1, SCK steady state is high
    // Phase     = 1, Data changed on first and captured on second clock edge
    // Word size = 8 bits
    //
    SSIConfigSetExpClk(SSI0_BASE, SysCtrlIOClockGet(), SSI_FRF_MOTO_MODE_3,
                       SSI_MODE_MASTER, 8000000UL, 8);
    
    //
    // Enable the SSI0 module.
    //
    SSIEnable(SSI0_BASE);

    while(SSIDataGetNonBlocking(SSI0_BASE, &ui32Dummy)){}

    // Initialze CSn, write to high
    GPIOPinTypeGPIOOutput(GPIO_SSI_BASE, PIN_SSI_FSS);
    GPIOPinWrite(GPIO_SSI_BASE, PIN_SSI_FSS, 1);  

    // Delay ~2ms (3 CPU cycles per arg. value)
    SysCtrlDelay(SysCtrlClockGet() / 500 / 3);

    // reset 
    trxSpiCmdStrobe(CC120X_SRES,1);

    // configure GPIO for use by CC1200
    GPIOPinTypeGPIOInput(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0);
    //GPIOPinTypeGPIOInput(GPIO_SSI_INT_BASE, GPIO_SSI_INT_1);

    /* PA6 on falling edge */
    GPIOIntTypeSet(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0, GPIO_FALLING_EDGE);
    /* PA7 on rising edge */
    //GPIOIntTypeSet(GPIO_SSI_INT_BASE, GPIO_SSI_INT_1, GPIO_RISING_EDGE);

    /* Register the interrupt */
    GPIOPortIntRegister(GPIO_SSI_INT_BASE, GPIO_Ap6_Handler);

    /* Clear and enable the interrupt from PA6*/
    GPIOPinIntClear(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0);
    //GPIOPinIntEnable(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0);
    GPIOPinIntDisable(GPIO_SSI_INT_BASE, GPIO_SSI_INT_0);

    // wait for MISO to go low
    //while( (GPIOPinRead(GPIO_SSI_BASE, PIN_SSI_RX) == 1) );

    //configure the 
    cc1200_reg_configure();

    RELEASE_SPI();
}


void my_SPI0_select(void){
    
    GPIOPinWrite(GPIO_SSI_BASE, PIN_SSI_FSS, 0x0000); //chip select--low
    // wait until MISO goes low, low exit
    while( !(GPIOPinRead(GPIO_SSI_BASE, PIN_SSI_RX) == 0) );
}

void my_SPI0_deselect(void){
    
    GPIOPinWrite(GPIO_SSI_BASE, PIN_SSI_FSS, PIN_SSI_FSS); 
    // wait until MISO goes low, low exit
    //while( !(GPIOPinRead(GPIO_SSI_BASE, PIN_SSI_RX) == 0) );
}

uint8_t my_SPI_send(uint16_t addr, uint8_t data){
    uint32_t rtn_data =0;
    uint32_t byte1st=0;
    uint32_t byte2nd=0;
    uint32_t ext_addr=0x2F;

         byte1st = addr;
         byte2nd = data;
         //GPIOPinWrite(GPIO_D_BASE, PIN_SSI_FSS, 0x0000); //chip select--low
         my_SPI0_select();

         if(CC1200_IS_EXTENDED_ADDR(addr)){
           SSIDataPut(SSI0_BASE, ext_addr);  
           while(!SSIBusy(SSI0_BASE));
           SSIDataGet(SSI0_BASE, &rtn_data);  
           SSIDataPut(SSI0_BASE, byte1st&0xff); 
           while(!SSIBusy(SSI0_BASE));
           SSIDataGet(SSI0_BASE, &rtn_data);   
         }else{
           SSIDataPut(SSI0_BASE, byte1st);  
           while(!SSIBusy(SSI0_BASE));
           SSIDataGet(SSI0_BASE, &rtn_data);    
         }
         SSIDataPut(SSI0_BASE, byte2nd);  
         while(!SSIBusy(SSI0_BASE));
         SSIDataGet(SSI0_BASE, &rtn_data); 

         //GPIOPinWrite(GPIO_D_BASE, PIN_SSI_FSS, PIN_SSI_FSS); 
         my_SPI0_deselect();
         //my_openserial_printStatus(0x16, (uint8_t*)&rtn_data, 4);

         return (uint8_t)rtn_data;
}


uint8_t my_SPI_recv(uint16_t addr){
    uint32_t rtn_data =0;
    uint32_t byte1st;
         byte1st = addr;
         byte1st |= CC1200_READ_BIT;
         //GPIOPinWrite(GPIO_D_BASE, PIN_SSI_FSS, 0x0000); //chip select--low
         my_SPI0_select();

         SSIDataPut(SSI0_BASE, byte1st);      
         //wait until Tx start to send
	 while(!SSIBusy(SSI0_BASE));
         SSIDataGet(SSI0_BASE, &rtn_data);    // block read data from Rx FIFO

         SSIDataPut(SSI0_BASE, 0);
         while(!SSIBusy(SSI0_BASE));
         SSIDataGet(SSI0_BASE, &rtn_data);    // read data from Rx FIFO
         //GPIOPinWrite(GPIO_D_BASE, PIN_SSI_FSS, PIN_SSI_FSS); 
         my_SPI0_deselect();

         //my_openserial_printStatus(0x17, (uint8_t*)&rtn_data, 4);

         return (uint8_t)rtn_data;
}

void trxSpiCmdStrobe(uint8_t ui8Addr, uint8_t ui8Len)
{
    uint32_t ui32Dummy;

    //
    // Wait for ongoing transfers to complete before pulling CSn low
    //
    while(SSIBusy(SSI0_BASE))
    {
    }

    while(ui8Len--)
    {
        //
        // Set CSn active
        //
        CC1200_SPI_BEGIN();

        //
        // Send address byte to SSI FIFO and increment address
        //
        SSIDataPut(SSI0_BASE, ui8Addr++);

        //
        // Wait for data to be clocked out before pulling CSn high
        //
        while(SSIBusy(SSI0_BASE))
        {
        }

        //
        // Clear CSn
        //
        CC1200_SPI_END();
    }

    //
    // Empty SSI IN FIFO
    //
    while(SSIDataGetNonBlocking(SSI0_BASE, &ui32Dummy));
}

/**************************************************************************//**
* @brief    This function writes 1200 registers. 
*(multiple writes with CSn low)
*thus CSn is pulled high between each address-data pair.
*
* @param    ui8Addr     is the register start address.
* @param    pui8Buf     is the pointer to source buffer.
* @param    ui8Len      is the number of registers to write.
*
* @return   None
******************************************************************************/
void CC1200WriteReg(uint8_t ui8Addr, const uint8_t *pui8Buf, uint8_t ui8Len)
{
    uint32_t ui32Dummy;

    //
    // Wait for ongoing transfers to complete before pulling CSn low
    //
    while(SSIBusy(SSI0_BASE))
    {
    }

    while(ui8Len--)
    {
        //
        // Set CSn active
        //
        CC1200_SPI_BEGIN();

        //
        // Send address byte to SSI FIFO and increment address
        //
        SSIDataPut(SSI0_BASE, ui8Addr++);

        //
        // Send value byte to SSI FIFO and increment buffer pointer
        //
        SSIDataPut(SSI0_BASE, *pui8Buf++);

        //
        // Wait for data to be clocked out before pulling CSn high
        //
        while(SSIBusy(SSI0_BASE))
        {
        }

        //
        // Clear CSn
        //
        CC1200_SPI_END();
    }

    //
    // Empty SSI IN FIFO
    //
    while(SSIDataGetNonBlocking(SSI0_BASE, &ui32Dummy));
}
