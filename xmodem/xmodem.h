/***********************************************************************************************************************
 * XMODEM implementation with YMODEM support
 ***********************************************************************************************************************
 * Copyright 2001-2019 Georges Menie (www.menie.org)
 * Modified by Thuffir in 2019
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********************************************************************************************************************/

#ifndef XMODEM_H_
#define XMODEM_H_

/***********************************************************************************************************************
 * Config Section
 **********************************************************************************************************************/
/* Define this if you have your own CCITT-CRC-16 implementation */
/* #define HAVE_CRC16 */

/* Define this if you want XMODEM-1K support, it will increase stack usage by 896 bytes */
#define XMODEM_1K

#define PACKET_DLY   4000   // ms Time to wait for packets to arrive
#define STARTUP_DLY  4000   // ms Time to wait before actively processing data 

#define MAXRETRANS 25
/** End Config Section ************************************************************************************************/

//0 - arithmetic, 1 - CRC16, 2 - YMODEM - G(CRC16 and no ACK)
#define CRC_ARITHMETIC  (0)
#define CRC_16          (1)
#define CRC_16_NO_ACK   (2)
#define MAX_BUFF        (1073741823l)

//#define XMODEMDEBUG(STRING)    STRING
#define XMODEMDEBUG(STRING)

#ifdef __TURBOC__
#define   COM1               0x3f8
#define   COM2               0x2f8
#define   COM3               0x3e8
#define   COM4               0x2e8

#define   SERIAL_CONTROL     5
#define   SERIAL_INBYE_FLAG    0x01
#define   SERIAL_OUTBYTE_FLAG  0x20    
#define   MYINT32             long int

#define Sleep(v)            delay(v)
#else
#define   MYINT32             int
#define   kbhit()        _kbhit()
#define   getch()        _getch()
#endif

/***********************************************************************************************************************
 * Function prototype for storing the received chunks
 **********************************************************************************************************************/
typedef void (*StoreChunkType)(
  /* Pointer to the function context (can be used for anything) */
  void *funcCtx,
  /* Pointer to the XMODEM receive buffer (store data from here) */
  void *xmodemBuffer,
  /* Number of bytes received in the XMODEM buffer (and to be stored) */
  int xmodemSize);

/***********************************************************************************************************************
 * XMODEM Receive
 **********************************************************************************************************************/
MYINT32 XmodemReceive(
  /* Function pointer for storing the received chunks or NULL*/
  StoreChunkType storeChunk,
  /* If storeChunk is NULL, pointer to the buffer to store the received data, else function context pointer to pass to
     storeChunk() */
  void *ctx,
  /* Number of bytes to receive */
    MYINT32 destsz,
  /* Checksum mode to request: 0 - arithmetic, 1 - CRC16, 2 - YMODEM-G (CRC16 and no ACK) */
  int crc,
  /* Receive mode: 0 - normal, nonzero - receive YMODEM control packet */
  int mode);

/***********************************************************************************************************************
 * Function removes the serial interrupt handler if necessary and exits
 **********************************************************************************************************************/
void xmodemExit(int cr);

/***********************************************************************************************************************
 * Function shortcut - XMODEM Receive with checksum
 **********************************************************************************************************************/
#define XmodemReceiveCsum(storeChunk, ctx, destsz) XmodemReceive(storeChunk, ctx, destsz, 0, 0)

/***********************************************************************************************************************
 * Function shortcut - XMODEM Receive with CRC-16
 **********************************************************************************************************************/
#define XmodemReceiveCrc(storeChunk, ctx, destsz) XmodemReceive(storeChunk, ctx, destsz, 1, 0)

/***********************************************************************************************************************
 * Function prototype for fetching the data chunks
 **********************************************************************************************************************/
typedef void (*FetchChunkType)(
  /* Pointer to the function context (can be used for anything) */
  void *funcCtx,
  /* Pointer to the XMODEM send buffer (fetch data into here) */
  void *xmodemBuffer,
  /* Number of bytes that should be fetched (and stored into the XMODEM send buffer) */
  int xmodemSize);

/***********************************************************************************************************************
 * XMODEM Transmit
 **********************************************************************************************************************/
MYINT32 XmodemTransmit(
  /* Function pointer for fetching the data chunks to be sent or NULL*/
  FetchChunkType fetchChunk,
  /* If fetchChunk is NULL, pointer to the buffer to be sent, else function context pointer to pass to fetchChunk() */
  void *ctx,
  /* Number of bytes to send */
  MYINT32 srcsz,
  /* If nonzero 1024 byte blocks are used (XMODEM-1K) */
  int onek,
  /* Transfer mode: 0 - normal, nonzero - transmit YMODEM control packet */
  int mode);

/***********************************************************************************************************************
 * YMODEM Header Context
 **********************************************************************************************************************/
typedef struct {
   char *filename;
   int size;
} YmodemHeaderContextType;

/***********************************************************************************************************************
 * YMODEM Transmit (Single file)
 **********************************************************************************************************************/
int YmodemTransmit(
  /* File Name */
  char *fileName,
  /* Function pointer for fetching the data chunks to be sent or NULL*/
  FetchChunkType fetchChunk,
  /* If fetchChunk is NULL, pointer to the buffer to be sent, else function context pointer to pass to fetchChunk() */
  void *ctx,
  /* Number of bytes to send */
  int size);

/***********************************************************************************************************************
 * Function shortcut - XMODEM Transmit with 128 bytes blocks
 **********************************************************************************************************************/
#define XmodemTransmit128b(fetchChunk, ctx, srcsz) XmodemTransmit(fetchChunk, ctx, srcsz, 0, 0)

/***********************************************************************************************************************
 * Function shortcut - XMODEM Transmit with 1K blocks
 **********************************************************************************************************************/
#define XmodemTransmit1K(fetchChunk, ctx, srcsz) XmodemTransmit(fetchChunk, ctx, srcsz, 1, 0)

#endif // XMODEM_H_
