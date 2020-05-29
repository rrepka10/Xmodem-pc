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

/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout); // msec timeout
     void _outbyte(int c);

 */

/* Needed for memcpy() and memeset() */
#include <string.h>
#include <time.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __TURBOC__
#include <dos.h>
#endif
#include "Serial.h"
#include "xmodem.h"


/***********************************************************************************************************************
 * Character constants
 **********************************************************************************************************************/
#define SOH   0x01
#define STX   0x02
#define EOT   0x04
#define ACK   0x06
#define NAK   0x15
#define CAN   0x18
#define CTRLZ 0x1A

#ifdef XMODEM_1K
/* 1024 for XModem 1k + 3 head chars + 2 crc */
#define XBUF_SIZE (1024 + 3 + 2)
#else
/* 128 for XModem + 3 head chars + 2 crc */
#define XBUF_SIZE (128 + 3 + 2)
#endif

#ifndef HAVE_CRC16
/***********************************************************************************************************************
 * Calculate the CCITT-CRC-16 value of a given buffer
 **********************************************************************************************************************/
static unsigned short crc16_ccitt(
  /* Pointer to the byte buffer */
  const unsigned char *buffer,
  /* length of the byte buffer */
  int length)
{
  unsigned short crc16 = 0;                          
  //XMODEMDEBUG(int len = length;)
  //XMODEMDEBUG(clock_t start = clock();)
  while(length != 0) {
    crc16  = (unsigned char)(crc16 >> 8) | (crc16 << 8);
    crc16 ^= *buffer;
    crc16 ^= (unsigned char)(crc16 & 0xff) >> 4;
    crc16 ^= (crc16 << 8) << 4;
    crc16 ^= ((crc16 & 0xff) << 4) << 1;
    buffer++;
    length--;
  }
  //XMODEMDEBUG(start = clock() - start;)
  //XMODEMDEBUG(printf("CRC time %ld crc=0x%0x\n", (long)start*1000l/(long)CLOCKS_PER_SEC, crc16);)

  return crc16;
}
#endif

/***********************************************************************************************************************
 * Check block   0.17 sec on a pc-xt
 **********************************************************************************************************************/
static int check(int crc, const unsigned char *buf, int sz)
{
  if (crc) {
    unsigned short crc = crc16_ccitt(buf, sz);
    unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
    if (crc == tcrc)
      return 1;
  }
  else {
    int i;
    unsigned char cks = 0;
    for (i = 0; i < sz; ++i) {
      cks += buf[i];
    }
    if (cks == buf[sz])
      return 1;
  }

  return 0;
}
#ifndef __TURBOC__
extern Serial *serialP;
#else
extern int port;
extern int serialControl;
#endif


/***********************************************************************************************************************
 * Flush input
 **********************************************************************************************************************/
static void flushinput(void)
{
 XMODEMDEBUG(printf("Flushing input\n");)
 while (_inbyte(((PACKET_DLY)*3)>>1) != -2)    ;
}

/***********************************************************************************************************************
 * XMODEM Receive
  
 StoreChunkType storeChunk - Function pointer for storing the received chunks or NULL    
 void *ctx                 - If storeChunk is NULL, pointer to the buffer to store the received data, 
                             else function context pointer to pass to storeChunk()   
 int_32 destsz             - Number of bytes to receive   
 int crc                   - Checksum mode to request: 0 - arithmetic, 1 - CRC16, 2 - YMODEM-G (CRC16 and no ACK)   
 int mode                  - Receive mode: 0 - normal, nonzero - receive YMODEM control packet 
 returns                   - bytes received
 **********************************************************************************************************************/
MYINT32 XmodemReceive(StoreChunkType storeChunk, void *ctx, MYINT32 destsz, int crc, int mode)
{
  unsigned char xbuff[XBUF_SIZE];
  unsigned char *p;
  MYINT32 bufsz, bufszEnd;
  unsigned char trychar = (crc == 2) ? 'G' : (crc ? 'C' : NAK);
  unsigned char packetno = mode ? 0 : 1;
  int i, c, msdelay;
  MYINT32 len = 0;
  int retry, retrans = MAXRETRANS;
  MYINT32 count;
  XMODEMDEBUG(printf("CRC mode 0x%0x\n", trychar);)
  for(;;) {
    for( retry = 0; retry < 16; ++retry) {
     // Check for an esc character 
     if (kbhit() && (27 == getch())) { return -2;}
     
       XMODEMDEBUG(printf("Retry %d\n", retry);)
      // send a request
        if (trychar) {
            XMODEMDEBUG(printf("received try char 0x%0x \n", trychar);)
            _outbyte(trychar);
        }

      // process commands from the transmitter
      if ((c = _inbyte(10000)) != -2) {
        switch (c) {
          case SOH:
            XMODEMDEBUG(printf("SOH-start of heading 128 block\n");)
            bufsz = 128;
            goto start_recv;
#ifdef XMODEM_1K
          case STX:
            XMODEMDEBUG(printf("STX-start of text 1024 block\n");)
            bufsz = 1024;
            goto start_recv;
#endif
          case EOT:
            XMODEMDEBUG(printf("EOT-end of transmission\n");)
            _outbyte(ACK);
            return len; /* normal end */

          case CAN:
            XMODEMDEBUG(printf("CAN-cancel\n");)
            if ((c = _inbyte(PACKET_DLY)) == CAN) {
              flushinput();
              _outbyte(ACK);
              return -1; /* canceled by remote */
            }
            break;

          default:
            XMODEMDEBUG(printf("default - ignore 0x%0x\n", c);)
            break;
        } // end switch
      } // end if inbyte
    } // End retry
 
    if (trychar == 'G') { 
        // G is 0x47
        trychar = 'C'; 
        crc = CRC_16; 
        continue;
    }
    else if (trychar == 'C') { 
         // C is 0x43
        trychar = NAK; 
        crc = CRC_ARITHMETIC; 
        continue; 
    }

    XMODEMDEBUG(printf("3-CAN\n");)
    flushinput();
    _outbyte(CAN);   // CAN = 0x18
    _outbyte(CAN);
    _outbyte(CAN);
    return -2; /* sync error */

    /*--------------------------------------------------------------------
      Start receiving
    --------------------------------------------------------------------*/
    start_recv:
    trychar = 0;
    p = xbuff;
    *p++ = c;

    // Read the data in
    bufszEnd = bufsz+(crc?1:0)+3;
   SERIALDEBUG(printf(" block Inbyte start ");) 

    for (i = 0;  i < bufszEnd; ++i) {
     // Check for an esc character 
     if (kbhit() && (27 == getch())) { return -2;}
    if ((c = _inbyte(PACKET_DLY)) == -2) {
         goto reject;
      }
      *p++ = c;
    } // End for i
    
    // Make sure the we receive the right packet
    if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
        (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
        check(crc, &xbuff[3], bufsz)) {
        XMODEMDEBUG(printf("block %ld  len %ld  destsz %ld\n",
               packetno, len, destsz);)

      //  Is this the right packet
      if (xbuff[1] == packetno)  {
        count = destsz - len;
        if (count > bufsz) count = bufsz;
        if (count > 0) {
         printf("Len %ld \r", len);
          if(storeChunk) {
            XMODEMDEBUG(printf("calling store chunk %d\n", count);)
            storeChunk(ctx, &xbuff[3], count);
          }
          else {
            XMODEMDEBUG(printf("storing chunk in ram %d\n",count);)
            memcpy (&((unsigned char *)ctx)[len], &xbuff[3], count);
          }
          len += count;
        }
        ++packetno;
        retrans = MAXRETRANS+1;
      }  // end if right packet

      if (--retrans <= 0) {
         XMODEMDEBUG(printf("Excessive retries\n");)
         flushinput();
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        return -3; /* too many retry error */
      }  // end if retrans

      if(crc != 2) _outbyte(ACK);
      if(mode) return len; /* YMODEM control block received */
      continue;
    } // end if right bits

    reject:
    XMODEMDEBUG(printf("Reject\n");)
    flushinput();
    _outbyte(NAK);
  } // end for(;;;)
}

/***********************************************************************************************************************
 * XMODEM Transmit
   FetchChunkType fetchChunk - Function pointer for fetching the data chunks to be sent or NULL
   void *ctx                 - If fetchChunk is NULL, pointer to the buffer to be sent
                               else function context pointer to pass to fetchChunk() 
   int srcsz                 - Number of bytes to send 
   int onek                  - If nonzero 1024 byte blocks are used (XMODEM-1K) 
   int mode                  - Transfer mode: 0 - normal, nonzero - transmit YMODEM control packet 
 **********************************************************************************************************************/
MYINT32 XmodemTransmit(FetchChunkType fetchChunk, void *ctx, MYINT32 srcsz, int onek, int mode)
{
  unsigned char xbuff[XBUF_SIZE];
  MYINT32 bufsz;
  int crc = -1;
  unsigned char packetno = mode ? 0 : 1;
  int i;
  MYINT32  c;
  MYINT32 len = 0;
  int retry;
  
  memset(xbuff, 0xee, XBUF_SIZE);
  for(;;) {
    for( retry = 0; retry < 16; ++retry) {
     // Check for an esc character 
     if (kbhit() && (27 == getch())) { return -2;}

      // Read a command
      if ((c = _inbyte(PACKET_DLY)) != -2) {
        switch (c) {
          case 'G':
            XMODEMDEBUG(printf("G CRC16 no ack \n");)
            crc = CRC_16_NO_ACK;
            goto start_trans;

          case 'C':
            XMODEMDEBUG(printf("C CRC 16\n");)
            crc = CRC_16;
            goto start_trans;

          case NAK:
            XMODEMDEBUG(printf("NAK \n");)
            crc = CRC_ARITHMETIC;
            goto start_trans;

          case CAN:
            XMODEMDEBUG(printf("CAN by remote\n");)
            if ((c = _inbyte(PACKET_DLY)) == CAN) {
              _outbyte(ACK);
              flushinput();
              return -1; /* canceled by remote */
            }
            break;

          default:
              XMODEMDEBUG(printf("default - ignore 0x%0x\n", c);)
            break;
        } // end switch
      } // end if inbyte
    } // end retry
    
    XMODEMDEBUG(printf("no sync\n");)
    _outbyte(CAN);
    _outbyte(CAN);
    _outbyte(CAN);
    flushinput();
    return -2; /* no sync */

    /*-----------------------------------------------------------------------
      start transmission
    -----------------------------------------------------------------------*/
start_trans:
    for(;;) {
      XMODEMDEBUG(printf("START block %d\n", packetno);)
      c = srcsz - len;
#ifdef XMODEM_1K
      if(onek && (c > 128)) {
        xbuff[0] = STX; bufsz = 1024;
      }
      else
#endif
      {
        xbuff[0] = SOH; bufsz = 128;
      }
      xbuff[1] = packetno;
      xbuff[2] = ~packetno;
      if (c > bufsz) c = bufsz;
      if (c > 0) {
          printf("Len %ld \r", len);

        memset (&xbuff[3], mode ? 0 : CTRLZ, bufsz);
        if(fetchChunk) {
          fetchChunk(ctx, &xbuff[3], c);
        }
        else {
          memcpy (&xbuff[3], &((unsigned char *)ctx)[len], c);
        }

        // add the CRC or check sum to the packets
        if (crc) {
          unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
          xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
          xbuff[bufsz+4] = ccrc & 0xFF;
        }
        else {
          unsigned char ccks = 0;
          for (i = 3; i < bufsz+3; ++i) {
            ccks += xbuff[i];
          }
          xbuff[bufsz+3] = ccks;
        }

        // Actually send the bytes
        for (retry = 0; retry < MAXRETRANS; ++retry) {
            // Check for an esc character 
            if (kbhit() && (27 == getch())) { return -2;}
            XMODEMDEBUG(printf("Sending block %d\n", packetno);)

         for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
           _outbyte(xbuff[i]);
         } // End for i

          XMODEMDEBUG(printf("  Block %d sent \n", packetno);)

          // Check status from the receiver
          XMODEMDEBUG(printf("Waiting for ACK\n");)
          c = (crc == CRC_16_NO_ACK) ? ACK : _inbyte(PACKET_DLY);
          if (c >= 0 ) {
            switch (c) {
              case ACK:
                  XMODEMDEBUG(printf("  ACK received\n");)
                ++packetno;
                len += bufsz;
                goto start_trans;

              case CAN:
                  XMODEMDEBUG(printf("  CAN received\n");)

                if ((c = _inbyte(PACKET_DLY)) == CAN) {
                  _outbyte(ACK);
                  flushinput();
                  return -1; /* canceled by remote */
                }
                break;

              case NAK:
                  XMODEMDEBUG(printf("  NAK received\n");)

              default:
                  XMODEMDEBUG(printf("  unknown received\n");)

                break;
            } // End switch
          }
        } // end for retry
        XMODEMDEBUG(printf("Sending cancels\n");)
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        flushinput();
        return -4; /* xmit error */
      }
      else if(mode) {
        return len; /* YMODEM control block sent */
      }
      else {
        for (retry = 0; retry < 10; ++retry) {
          _outbyte(EOT);
          if ((c = _inbyte((PACKET_DLY)<<1)) == ACK) break;
        }
        if(c == ACK) {
          return len; /* Normal exit */
        }
        else {
          XMODEMDEBUG(printf("Flush input, no ACK after EOT\n");)
          flushinput();
          return -5; /* No ACK after EOT */
        }
      }
    }
  }
}

void xmodemExit(int rc) {

#ifdef __TURBOC__
    removeIntHandler();
#endif
    exit(rc);
}


#if 0    // Ymodem not supported
/***********************************************************************************************************************
 * Transmit minimal YMODEM Header (filename and size)
 **********************************************************************************************************************/
static void YmodemTransmitHeader(void *funcCtx, void *xmodemBuffer, int xmodemSize)
{
  YmodemHeaderContextType *ctx = (YmodemHeaderContextType *) funcCtx;
  snprintf((char *)xmodemBuffer, xmodemSize, "%s%c%u", ctx->filename, 0, ctx->size);
}

/***********************************************************************************************************************
 * Do nothing (leave packet zero)
 **********************************************************************************************************************/
static void YmodemDoNothing(void *funcCtx, void *xmodemBuffer, int xmodemSize)
{
}

/***********************************************************************************************************************
 * YMODEM Transmit (Single file)
 **********************************************************************************************************************/
int YmodemTransmit(char *fileName, FetchChunkType fetchChunk, void *ctx, int size)
{
  YmodemHeaderContextType header = { fileName, size };
  int res, tmp;

  /* Transmit YMODEM header */
  tmp = XmodemTransmit(YmodemTransmitHeader, &header, 128, 0, 1);
  if( tmp < 0) {
    return tmp;
  }

  /* Transmit Data */
  res = XmodemTransmit(fetchChunk, ctx, size, 1, 0);
  if( res < 0) {
    return res;
  }

  /* Transmit End Block*/
  tmp = XmodemTransmit(YmodemDoNothing, NULL, 128, 0, 1);
  if( tmp < 0) {
    return tmp;
  }

  return res;
}
#endif
