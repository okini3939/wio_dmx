/*
 * DMX512, RDM send/recv library
 * Copyright (c) 2021,2017 Hiroshi Suga
 * Released under the MIT License: http://mbed.org/license/mit
 */

/** @file
 * @brief DMX512 send/recv
 */

#include "DMX.h"
#include "SERCOM.h"
#include "SAMD51_TC.h"

DMX *DMX::_inst;
//void UART_IRQHandler ();

DMX::DMX () {
    _inst = this;
    _uart = new SERCOM(SERCOM_DEV);

//    mode_tx = DMX_MODE_BEGIN;
    mode_tx = DMX_MODE_STOP;
    mode_rx = DMX_MODE_BEGIN;
    is_received = 0;
    is_sent    = 0;
    clear();
    time_break = DMX_TIME_BREAK;
    time_mab   = DMX_TIME_MAB;
    time_mbb   = DMX_TIME_MBB;
}

void DMX::init () {

    XMIT_INIT;
    XMIT_RX;
    XMIT2_INIT;
    XMIT2_TX;

    pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
    pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);

    _uart->initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, 250000);
    _uart->initFrame(UART_CHAR_SIZE_8_BITS, LSB_FIRST, SERCOM_NO_PARITY, SERCOM_STOP_BITS_2);
    _uart->initPads(uc_padTX, uc_padRX);
    _uart->enableUART();

    _dmx_attach(1, 0); // (this, &DMX::int_rx, Serial::RxIrq);
	  NVIC_SetPriority(SERCOM3_0_IRQn, 1);
	  NVIC_EnableIRQ(SERCOM3_0_IRQn);

//    timeout01.attach_us(this, &DMX::int_timer, DMX_TIME_BETWEEN);
}

void DMX::put (int addr, int data) {
    if (addr < 0 || addr >= DMX_SIZE) return;
    data_tx[addr] = data;
}

void DMX::put (const unsigned char *buf, int addr, int len) {
    if (addr < 0 || addr >= DMX_SIZE) return;
    if (len > DMX_SIZE - addr) len = DMX_SIZE - addr;
    memcpy(&data_tx[addr], buf, len);
}

int DMX::get (int addr) {
    if (addr < 0 || addr >= DMX_SIZE) return -1;
    return data_rx[addr];
}

void DMX::get (unsigned char *buf, int addr, int len) {
    if (addr < 0 || addr >= DMX_SIZE) return;
    if (len > DMX_SIZE - addr) len = DMX_SIZE - addr;
    memcpy(buf, &data_rx[addr], len);
}

void DMX::int_timer () {

    switch (mode_tx) {
    case DMX_MODE_BEGIN:
        // Break Time
        timeout01_detach();
        while(SERCOM_DEV->USART.SYNCBUSY.bit.ENABLE);
        SERCOM_DEV->USART.CTRLA.bit.ENABLE = 0;
        SERCOM_DEV->USART.CTRLA.reg |= SERCOM_USART_CTRLA_TXINV; // break on
        SERCOM_DEV->USART.CTRLA.bit.ENABLE = 1;
        while(SERCOM_DEV->USART.SYNCBUSY.bit.ENABLE); // is enabled
        mode_tx = DMX_MODE_BREAK;
        timeout01_attach_us(1, time_break); // (this, &DMX::int_timer, time_break);
        break;

    case DMX_MODE_BREAK:
        // Mark After Break
        timeout01_detach();
        while(SERCOM_DEV->USART.SYNCBUSY.bit.ENABLE);
        SERCOM_DEV->USART.CTRLA.bit.ENABLE = 0;
        SERCOM_DEV->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_TXINV; // break off
        SERCOM_DEV->USART.CTRLA.bit.ENABLE = 1;
        while(SERCOM_DEV->USART.SYNCBUSY.bit.ENABLE); // is enabled
        mode_tx = DMX_MODE_MAB;
        timeout01_attach_us(1, time_mab); // (this, &DMX::int_timer, time_mab);
        break;

    case DMX_MODE_MAB:
        // Start code
        timeout01_detach();
        addr_tx = 0;
        mode_tx = DMX_MODE_DATA;
#ifdef DMX_UART_DIRECT
        while (! SERCOM_DEV->USART.INTFLAG.bit.DRE); // is empty
        SERCOM_DEV->USART.DATA.reg = DMX_START_CODE;
#else
        _dmx_putc(DMX_START_CODE);
#endif
        _dmx_attach(1, 1); // (this, &DMX::int_tx, Serial::TxIrq);
        break;
    }
}

void DMX::int_tx () {
    // Data
    if (mode_tx == DMX_MODE_DATA) {
        if (addr_tx < DMX_SIZE) {
            // DMX data
#ifdef DMX_UART_DIRECT
            SERCOM_DEV->USART.DATA.reg = (uint8_t)data_tx[addr_tx];
#else
            _dmx_putc(data_tx[addr_tx]);
#endif
            addr_tx ++;
        } else {
            _dmx_attach(0, 1); // (0, Serial::TxIrq); // disable
            mode_tx = DMX_MODE_BEGIN;
            is_sent = 1;
            timeout01_attach_us(1, time_mbb);
        }
    }
}

void DMX::int_rx () {
    uint32_t flag, stat, dat;

    flag = SERCOM_DEV->USART.INTFLAG.reg;
    stat = SERCOM_DEV->USART.STATUS.reg;

    dat = SERCOM_DEV->USART.DATA.reg;

    if ((flag & (SERCOM_USART_INTFLAG_ERROR|SERCOM_USART_INTFLAG_RXBRK)) && (stat & (SERCOM_USART_STATUS_FERR))) {
        // Break Time
        SERCOM_DEV->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR|SERCOM_USART_INTFLAG_RXBRK;
        SERCOM_DEV->USART.STATUS.reg = SERCOM_USART_STATUS_FERR;
        if (is_rdm_received) {
            return;
        } else
        if (addr_rx >= 24 && mode_rx == DMX_MODE_DATA) {
            is_received = 1;
        }
        if (dat == 0) {
            mode_rx = DMX_MODE_BREAK;
        } else {
            mode_rx = DMX_MODE_ERROR;
        }
        return;
		}

    if (mode_rx == DMX_MODE_BREAK) {

        // Start Code
        if (dat == DMX_START_CODE) {
            addr_rx = 0;
            mode_rx = DMX_MODE_DATA;
        } else {
            mode_rx = DMX_MODE_ERROR;
        }

    } else
    if (mode_rx == DMX_MODE_DATA) {

        // Data
        data_rx[addr_rx] = dat;
        addr_rx ++;

        if (addr_rx >= DMX_SIZE) {
            is_received = 1;
            mode_rx = DMX_MODE_BEGIN;
        }

    }
}

void DMX::start () {
    if (mode_tx == DMX_MODE_STOP) {
        mode_tx = DMX_MODE_BEGIN;
        is_sent = 0;
        timeout01_attach_us(1, time_mbb); // &DMX::int_timer, time_mbb);
    }
}

void DMX::stop () {
    _dmx_attach(0, 1); // (0, Serial::TxIrq);
    timeout01_detach();
    mode_tx = DMX_MODE_STOP;
}

void DMX::clear () {
    memset(data_tx, 0, sizeof(data_tx));
    memset(data_rx, 0, sizeof(data_rx));
}

int DMX::isReceived (){
    int r = is_received;
    is_received = 0;
    return r;
}

int DMX::isSent () {
    int r = is_sent;
    is_sent = 0;
    return r;
}

unsigned char *DMX::getRxBuffer () {
    return data_rx;
}

unsigned char *DMX::getTxBuffer () {
    return data_tx;
}

int DMX::setTimingParameters (int breaktime, int mab, int mbb) {
    if (breaktime < 88 || breaktime > 1000000) return -1;
    if (mab < 8 || mab > 1000000) return -1;
    if (mbb < 0 || mbb > 1000000) return -1;

    time_break = breaktime;
    time_mab   = mab;
    time_mbb   = mbb;
    return 0;
}

void isrUart () {
  int flg = SERCOM_DEV->USART.INTFLAG.reg;

  if (flg & (SERCOM_USART_INTFLAG_ERROR | SERCOM_USART_INTFLAG_RXBRK | SERCOM_USART_INTFLAG_RXC)) {
    if (DMX::_inst->callback[0] == 1) {
      DMX::_inst->int_rx();
    } else
    if (DMX::_inst->callback[0] == 2) {
    }
  }
  if (flg & SERCOM_USART_INTFLAG_DRE) {
    if (DMX::_inst->callback[1] == 1) {
      DMX::_inst->int_tx();
    }
  }
}

extern "C" {
void SERCOM3_0_Handler () {
  isrUart();
}
void SERCOM3_1_Handler() {
  isrUart();
}
void SERCOM3_2_Handler() {
  isrUart();
}
void SERCOM3_3_Handler() {
  isrUart();
}
} // extern

void DMX::_dmx_putc (int dat) {
	SERCOM_DEV->USART.DATA.reg = dat;
}

int DMX::_dmx_getc () {
	return SERCOM_DEV->USART.DATA.reg;
}

void DMX::_dmx_attach (int func, int type) {
	callback[type] = func;
	if (type == 0) {
		if (func) {
			SERCOM_DEV->USART.INTENSET.reg = (SERCOM_USART_INTENSET_RXC | SERCOM_USART_INTENSET_RXBRK | SERCOM_USART_INTENSET_ERROR); // interrupt RX
		} else {
			SERCOM_DEV->USART.INTENCLR.reg = (SERCOM_USART_INTENSET_RXC | SERCOM_USART_INTENSET_RXBRK | SERCOM_USART_INTENSET_ERROR);
		}
	} else {
		if (func) {
			SERCOM_DEV->USART.INTENSET.reg = (SERCOM_USART_INTENSET_DRE); // interrupt TX
		} else {
			SERCOM_DEV->USART.INTENCLR.reg = (SERCOM_USART_INTENSET_DRE);
		}
	}
}

void isrTimerDmx (void) {
	TimerTC3.stop();
	if (DMX::_inst->callback[2] == 1) {
		DMX::_inst->int_timer();
	}
}

void DMX::timeout01_attach_us (int func, int us) {
	callback[2] = func;
  TimerTC3.initialize(us);
  TimerTC3.attachInterrupt(isrTimerDmx);
}

void DMX::timeout01_detach () {
	callback[2] = 0;
	TimerTC3.stop();
}
