/*
 * DMX512, RDM send/recv library
 * Copyright (c) 2021,2017 Hiroshi Suga
 * Released under the MIT License: http://mbed.org/license/mit
 */

/** @file
 * @brief DMX512 send/recv
 */
 
#ifndef __DMX_H__
#define __DMX_H__

#ifndef ARDUINO_WIO_TERMINAL
#error please select Wio Terminal
#endif

#include "Arduino.h"
#include "wiring_private.h"
#include <stdint.h>
#include <string.h>

#define SERCOM_DEV SERCOM3
#define uc_pinRX PIN_WIRE_SCL  // BCM3 PA16/I2C/I0/SERCOM1.0+3.1/SCL
#define uc_pinTX PIN_WIRE_SDA  // BCM2 PA17/I2C/I1/SERCOM1.1+3.0/SDA
#define uc_padRX SERCOM_RX_PAD_1
#define uc_padTX UART_TX_PAD_0

#define XMIT_INIT pinMode(D1, OUTPUT)
#define XMIT_TX	 digitalWrite(D1, HIGH)
#define XMIT_RX	 digitalWrite(D1, LOW)
#define XMIT2_INIT 
#define XMIT2_TX	 
#define XMIT2_RX	 

#define DMX_SIZE        512
#define DMX_START_CODE  0

#define RDM_START_CODE  E120_SC_RDM
#define RDM_SUB_CODE    E120_SC_SUB_MESSAGE

#define DMX_TIME_BREAK  200 // 100us (88us-1s)
#define DMX_TIME_MAB    50 // 12us (8us-1s)
#define DMX_TIME_MBB    1000 // 10us (0-1s)
#define RDM_TIME_DELAY  200

enum DMX_MODE {
    DMX_MODE_BEGIN,
    DMX_MODE_START,
    DMX_MODE_BREAK,
    DMX_MODE_MAB,
    DMX_MODE_DATA,
    DMX_MODE_ERROR,
    DMX_MODE_STOP,
    DMX_MODE_RDMSUB,
    DMX_MODE_RDM,
};

/** DMX512 class (sender/client)
 */
class DMX {
public:
    /** init DMX class
     * @param p_tx TX serial port (p9, p13, p28)
     * @param p_rx RX serial port (p10, p14, p27)
     * @param p_xmit data enable/~receive enable (rx port)
     * @param p_xmit2 data enable/~receive enable (tx port)
     */
    DMX (); 

    void init ();

    /** Send the data
     * @param addr DMX data address (0-511)
     * @param data DMX data (0-255)
     */
    void put (int addr, int data);
    /** Send the data
     * @param buf DMX data buffer
     * @param addr DMX data address
     * @param len data length
     */
    void put (const unsigned char *buf, int addr = 0, int len = DMX_SIZE);

    /** Send the data
     * @param addr DMX data address (0-511)
     * @return DMX data (0-255)
     */
    int get (int addr);
    /** Send the data
     * @param buf DMX data buffer
     * @param addr DMX data address
     * @param len data length
     */
    void get (unsigned char *buf, int addr = 0, int len = DMX_SIZE);

    /** Start DMX send operation
     */
    void start ();
    /** Stop DMX send operation
     */
    void stop ();
    /** Clear DMX data
     */
    void clear ();

    int isReceived ();
    int isSent ();
    unsigned char *getRxBuffer ();
    unsigned char *getTxBuffer ();
    int setTimingParameters (int breaktime, int mab, int mbb);

    void int_timer ();
    void int_tx ();
    void int_rx ();

		static DMX *_inst;
		int callback[3];

protected:

//    Serial _dmx;
    volatile DMX_MODE mode_tx, mode_rx;
    volatile int addr_tx, addr_rx;
    unsigned char data_tx[DMX_SIZE];
    unsigned char data_rx[DMX_SIZE];
    volatile int is_received, is_sent, is_rdm_received;
    int time_break, time_mab, time_mbb;
    int framerate, time_us;

		void _dmx_putc (int dat);
		int _dmx_getc ();
		void _dmx_attach (int func, int type);
		void timeout01_attach_us (int func, int us);
		void timeout01_detach ();

private:
    SERCOM *_uart;

};

#endif
