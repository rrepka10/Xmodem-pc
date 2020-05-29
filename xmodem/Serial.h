/** Serial.h
 *
 * A very simple serial port control class that does NOT require MFC/AFX.
 *
 * License: This source code can be used and/or modified without restrictions.
 * It is provided as is and the author disclaims all warranties, expressed 
 * or implied, including, without limitation, the warranties of
 * merchantability and of fitness for any purpose. The user must assume the
 * entire risk of using the Software.
 *
 * @author Hans de Ruiter
 *
 * @version 0.1 -- 28 October 2008
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__
#define   COM1               0x3f8
#define   COM2               0x2f8
#define   COM3               0x3e8
#define   COM4               0x2e8

#define COM1_IRQ             0xEF
#define COM2_IRQ             0xF7
#define COM3_IRQ             0xEF
#define COM4_IRQ             0xF7

#define COM1_IRQ_MASK        0x10
#define COM2_IRQ_MASK        0x08
#define COM3_IRQ_MASK        0x10
#define COM4_IRQ_MASK        0x08

#define COM1_INT_VEC         0x0C
#define COM2_INT_VEC         0x0B
#define COM3_INT_VEC         0x0C
#define COM4_INT_VEC         0x0B

#define MASTER_PIC           0x20
#define SLAVE_PIC            0x21

#define   SERIAL_CONTROL       5
#define   SERIAL_INBYTE_FLAG   0x01
#define   SERIAL_OUTBYTE_FLAG  0x20  // 0010 0000

#define   CIRC_BUFF_SIZE      2048

#define  WRITE_DLY            1000                                
//#define SERIALDEBUG(STRING)  STRING
#define SERIALDEBUG(STRING)

#ifdef __TURBOC__

/*---------------------------------------------------------------------------   
  Description: This sets the PC-XT modem hardware baud rate with 8-n-1 status
   There is NO error checking.  This uses interrupt IO.  
---------------------------------------------------------------------------*/
void setBaudRate(const char *serialPort, int rate);
void removeIntHandler(void);

/*---------------------------------------------------------------------------
  Input Interrupt handler
---------------------------------------------------------------------------*/
void interrupt serialIntHndlr(...);

/*---------------------------------------------------------------------------
  Reads a single byte from a port, return -1 if the delay expired.
---------------------------------------------------------------------------*/
int _inbyte(unsigned msdelay);

/*---------------------------------------------------------------------------
   Writes a single byte to a port
---------------------------------------------------------------------------*/
void _outbyte(char c);

#else                             
#include <string>
#include <windows.h>

typedef std::basic_string<TCHAR> tstring;

#define _inbyte(TIME) serialP->inbyte(TIME)  
#define _outbyte(C)   serialP->outbyte(C)

class Serial
{
private:
   HANDLE commHandle;

public:
   Serial(tstring &commPortName, int bitRate = 9600); //was 115200

   virtual ~Serial();

   /** Writes a NULL terminated string.
    *
    * @param buffer the string to send
    *
    * @return int the number of characters written
    */
   int write(const char buffer[]);

   /** Writes a string of bytes to the serial port.
    *
    * @param buffer pointer to the buffer containing the bytes
    * @param buffLen the number of bytes in the buffer
    *
    * @return int the number of bytes written
    */
   int write(const char *buffer, int buffLen);

   void outbyte(int c);

   /** Reads a string of bytes from the serial port.
    *
    * @param buffer pointer to the buffer to be written to
    * @param buffLen the size of the buffer
    * @param nullTerminate if set to true it will null terminate the string
    *
    * @return int the number of bytes read
    */
   int read(char *buffer, int buffLen, bool nullTerminate = true);
   int inbyte(unsigned short timeout);

   /** Flushes everything from the serial port's read buffer
    */
   void flush();
};

#endif
#endif
