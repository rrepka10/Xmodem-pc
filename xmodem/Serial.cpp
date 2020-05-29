/** Serial.cpp
 *
 * A very simple serial port control class that does NOT require MFC/AFX.
 *
 * @author Hans de Ruiter
 *
 http://hdrlab.org.nz/articles/windows-development/a-c-class-for-controlling-a-comm-port-in-windows/
 
  PC-XT UART is a simple  INS8250  serial chip
 * @version 0.1 -- 28 October 2008
 
https://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming

 sample inerrupt code http://wearcam.org/seatsale/programs/www.beyondlogic.org/serial/buff1024.c
**/

#include <string.h>
#include "Serial.h"

#ifdef __TURBOC__
#include <dos.h>
int port;
int comIRQ;
int comIRQ_mask;
int comINT_vec;
int serialControl;

// Implements a circular buffer
volatile int bufferin = 0;
volatile int bufferout = 0;
volatile char buffer[CIRC_BUFF_SIZE+1];
void interrupt (*oldport1isr)(...);


/*---------------------------------------------------------------------------   
  Description: This sets the PC-XT modem hardware baud rate with 8-n-1 status
   There is NO error checking
   
  int port - hex PC-XT id number
  int rate - standard baud rates,  300, 600... 19200
  
    UART Registers - w/ DLAB control state
    Addr DLAB I/O Abbrv. Name
      +0   0   W   THR   Transmitter Holding Buffer
      +0   0   R   RBR   Receiver Buffer
      +0   1  R/W  DLL   Divisor Latch Low Byte
      +1   0  R/W  IER   Interrupt Enable Register
      +1   1  R/W  DLH   Divisor Latch High Byte
      +2   x   R   IIR   Interrupt Identification Register
      +2   x   W   FCR   FIFO Control Register
      +3   x  R/W  LCR   Line Control Register
      +4   x  R/W  MCR   Modem Control Register
      +5   x   R   LSR   Line Status Register
      +6   x   R   MSR   Modem Status Register
      +7   x  R/W  SR    Scratch Register
---------------------------------------------------------------------------*/
void setBaudRate(const char *serialPort, int rate) {
    char DLLB;
    // set critical port info
   if      (stricmp(serialPort, "com4") == 0) { 
      port        = COM4;
      comIRQ      = COM4_IRQ;
      comIRQ_mask = COM4_IRQ_MASK;
      comINT_vec  = COM4_INT_VEC;
      } 
   else if (stricmp(serialPort, "com3") == 0) { 
      port = COM3;
      comIRQ      = COM3_IRQ;
      comIRQ_mask = COM3_IRQ_MASK;
      comINT_vec  = COM3_INT_VEC;
      } 
   else if (stricmp(serialPort, "com2") == 0) { 
      port = COM2;
      comIRQ      = COM2_IRQ;
      comIRQ_mask = COM2_IRQ_MASK;
      comINT_vec  = COM2_INT_VEC;
      } 
   else { 
      port = COM1;
      comIRQ      = COM1_IRQ;
      comIRQ_mask = COM1_IRQ_MASK;
      comINT_vec  = COM1_INT_VEC;
      } 
   
   // Generate the control port ID
   serialControl = port + SERIAL_CONTROL;
   
   // Turn off interrupts 
   outportb(port + 1, 0);

   // Save any old Interrupt Vector
   oldport1isr = getvect(comINT_vec); 

   // Set our Interrupt handler 
   setvect(comINT_vec, serialIntHndlr);     
   
    // set DLAB on so we can set the baud rate
    outportb(port + 3, 0x80); /* SET DLAB ON */
    if      (rate == 300)   {DLLB = 0x80;} // 300
    else if (rate == 600)   {DLLB = 0xc0;} // 600
    else if (rate == 1200)  {DLLB = 0x60;} // 1200
    else if (rate == 2400)  {DLLB = 0x30;} // 2400
    else if (rate == 4800)  {DLLB = 0x18;} // 4800
    else if (rate == 9600)  {DLLB = 0x0C;} // 9600
    else if (rate == 19200) {DLLB = 0x06;} // 19200
    else {DLLB = 0x0c;} //9600
    
    // Set Baud rate - Divisor Latch Low Byte 
    outportb(port + 0, DLLB); 
    
    // Set the high byte divisor, only for 300 baud, 0 otherwise
    if (rate == 300) {outportb(port + 1, 0x01);}
    else             {outportb(port + 1, 0x00);}
    

   /*-----------------------------------------------------------------------
   Line Control Register (LCR) +3
     Bit  Notes
     7    Divisor Latch Access Bit
     6    Set Break Enable
     345  Parity Select
     000    No Parity
     001    Odd Parity
     011    Even Parity
     101    Mark
     111    Space
     2     0  1 Stop Bit
           1  1.5 Stop Bits or 2 Stop Bits
     01  Word Length
     00    5 Bits
     01    6 Bits
     10    7 Bits
     11    8 Bits
   -----------------------------------------------------------------------*/
    outportb(port + 3, 0x03); /* 0x03 - 0000 0011 - 8 Bits, No Parity, 1 Stop Bit, dlab off */
    
   /*-----------------------------------------------------------------------
     fifo control register +2
      67 Interrupt Trigger Level (16 byte)	Trigger Level (64 byte)
      00    1 Byte	1 Byte
      01    4 Bytes	16 Bytes
      10    8 Bytes	32 Bytes
      11    14 Bytes	56 Bytes
      5  Enable 64 Byte FIFO (16750)
      
      4  Reserved
      3  DMA Mode Select
      2  Clear Transmit FIFO
      1  Clear Receive FIFO
      0  Enable FIFOs
    -----------------------------------------------------------------------*/
    outportb(port + 2, 0xC7); /*0xc7 = 1100 0111 enable fifo, clear xmit, clear receive,  14 byte interrupt level */
    
    /*----------------------------------------------------------------------- 
      Modem Control Register (MCR) +4
       Bit Notes
       7   Reserved
       6   Reserved
       5   Autoflow Control Enabled (16750)
       4   Loopback Mode
       3   Auxiliary Output 2
       2   Auxiliary Output 1
       1   Request To Send
       0   Data Terminal Ready
    -----------------------------------------------------------------------*/
    outportb(port + 4, 0x0B); /* 0x0b = 0000 1011 = Turn on DTR, RTS, and OUT2 */

    // Set Programmable Interrupt Controller 
    outportb(SLAVE_PIC,(inportb(SLAVE_PIC) & comIRQ));
 
   // Interrupt when data received 
   outportb(port + 1 , 0x01);  
   return;
} // setBaudRate


/*---------------------------------------------------------------------------   
  Description: PC-XT uart interrupt handler remover.  
  You must call this before exiting your code or your pc will crash
---------------------------------------------------------------------------*/
void removeIntHandler(void) {
   // Turn off interrupts 
   outportb(port + 1 , 0);      
   
   // MASK IRQ using PIC 
   outportb(SLAVE_PIC,(inportb(SLAVE_PIC) | 0x10));  

   // Restore old interrupt vector 
   setvect(comINT_vec, oldport1isr); 
   return;
} // end removeIntHandler


/*---------------------------------------------------------------------------   
  Description: PC-XT uart interrupt handler - ISR
---------------------------------------------------------------------------*/
void interrupt serialIntHndlr(...) {
   int c;
   do {
      /* Line Status Register (LSR) +5
       Bit	Notes
       7	Error in Received FIFO
       6	Empty Data Holding Registers
       5	Empty Transmitter Holding Register
       4	Break Interrupt
       3	Framing Error
       2	Parity Error
       1	Overrun Error
       0	Data Ready */
      c = inportb(serialControl);
      
      // Check for data ready
      if (c & 1) {
         buffer[bufferin] = inportb(port);
         bufferin++;
         
         // Circular buffer 
		 if (bufferin == CIRC_BUFF_SIZE) { bufferin = 0; }
         }
      }while (c & 1);
   
   // Reset the master interrupt controller
   outportb(MASTER_PIC, 0x20);
} // end serialIntHndlr


/*---------------------------------------------------------------------------
  This reads a byte from the circular buffer
---------------------------------------------------------------------------*/
long int waitCount = 0;
int _inbyte(unsigned msdelay) {
   int c;
   SERIALDEBUG(printf("Inbyte start ");)
  
   // read from the circular buffer 
   do {
      if (bufferin != bufferout){
		 c = buffer[bufferout];
		 c &= 0x00ff;

         SERIALDEBUG(printf("0x%0x received\n", c);)
         bufferout++;
         
         // circular 
		 if (bufferout == CIRC_BUFF_SIZE) { bufferout = 0; }
         
         // return the data
		 return(c);
      }  // end if
      else {
         // wait
         msdelay--;
		 delay(1);
      }
    } while (msdelay);
   
   // time out
   SERIALDEBUG(printf("inbyte timeout\n");)
   return(-2);
}

/*---------------------------------------------------------------------------
---------------------------------------------------------------------------*/
void _outbyte(char c) {
  char data;
  unsigned msdelay = WRITE_DLY;
  SERIALDEBUG(printf("outbyte start ");)
  do {
   data = inportb(port + SERIAL_CONTROL);
   if (data & SERIAL_OUTBYTE_FLAG) {
	  SERIALDEBUG(printf("0x%0x sent\n", c);)
	  outportb(port, c);
	  return;
	  }
   delay(10);   // 300 baud is ~33ms/byte
  } while (msdelay--);
  SERIALDEBUG(printf("outbyte timeout\n");)
  return;
}
#else                             

extern Serial *serialP;

// Open a named serial port 
Serial::Serial(tstring &commPortName, int bitRate)
{
   commHandle = CreateFile(commPortName.c_str(), GENERIC_READ|GENERIC_WRITE, 0,NULL, OPEN_EXISTING, 
      0, NULL);

   if(commHandle == INVALID_HANDLE_VALUE) 
   {
      printf("ERROR: Could not open com port\n");
      exit(99);
   }
   else 
   {
      // set timeouts
      COMMTIMEOUTS cto = { MAXDWORD, 0, 0, 0, 0};
      DCB dcb;
      if(!SetCommTimeouts(commHandle,&cto))
      {
         Serial::~Serial();
         throw("ERROR: Could not set com port time-outs");
      }

#if 0
      typedef struct _DCB {
          DWORD DCBlength;      /* sizeof(DCB)                     */
          DWORD BaudRate;       /* Baudrate at which running       */
          DWORD fBinary : 1;     /* Binary Mode (skip EOF check)    */
          DWORD fParity : 1;     /* Enable parity checking          */
          DWORD fOutxCtsFlow : 1; /* CTS handshaking on output       */
          DWORD fOutxDsrFlow : 1; /* DSR handshaking on output       */
          DWORD fDtrControl : 2;  /* DTR Flow control                */
          DWORD fDsrSensitivity : 1; /* DSR Sensitivity              */
          DWORD fTXContinueOnXoff : 1; /* Continue TX when Xoff sent */
          DWORD fOutX : 1;       /* Enable output X-ON/X-OFF        */
          DWORD fInX : 1;        /* Enable input X-ON/X-OFF         */
          DWORD fErrorChar : 1;  /* Enable Err Replacement          */
          DWORD fNull : 1;       /* Enable Null stripping           */
          DWORD fRtsControl : 2;  /* Rts Flow control                */
          DWORD fAbortOnError : 1; /* Abort all reads and writes on Error */
          DWORD fDummy2 : 17;     /* Reserved                        */
          WORD wReserved;       /* Not currently used              */
          WORD XonLim;          /* Transmit X-ON threshold         */
          WORD XoffLim;         /* Transmit X-OFF threshold        */
          BYTE ByteSize;        /* Number of bits/byte, 4-8        */
          BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
          BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
          char XonChar;         /* Tx and Rx X-ON character        */
          char XoffChar;        /* Tx and Rx X-OFF character       */
          char ErrorChar;       /* Error replacement char          */
          char EofChar;         /* End of Input character          */
          char EvtChar;         /* Received Event character        */
          WORD wReserved1;      /* Fill for now.                   */
      } DCB, *LPDCB;
#endif 
      // set DCB

      memset(&dcb,0,sizeof(dcb));
      dcb.DCBlength = sizeof(dcb);
      dcb.BaudRate = bitRate;
      dcb.fBinary = 1;

      dcb.Parity = NOPARITY;
      dcb.StopBits = ONESTOPBIT;
      dcb.ByteSize = 8;

      if(!SetCommState(commHandle,&dcb))
      {
         Serial::~Serial();
         throw("ERROR: Could not set com port parameters");
      }
   }
}


// Close the serial port
Serial::~Serial()
{
   CloseHandle(commHandle);
}


   /** Writes a NULL terminated string.
    *
    * @param buffer the string to send
    *
    * @return int the number of characters written
    */
int Serial::write(const char *buffer)
{
   DWORD numWritten;

   WriteFile(commHandle, buffer, strlen(buffer), &numWritten, NULL); 
   return(numWritten);
}

// Write binary data to the port
   /** Writes a string of bytes to the serial port.
    *
    * @param buffer pointer to the buffer containing the bytes
    * @param buffLen the number of bytes in the buffer
    *
    * @return int the number of bytes written
    */
int Serial::write(const char *buffer, int buffLen)
{
   DWORD numWritten;
   WriteFile(commHandle, buffer, buffLen, &numWritten, NULL); 

   return numWritten;
}

void Serial::outbyte(int c)
{
                           
	char buffer = c & 0x00ff;
    DWORD numWritten;
    SERIALDEBUG(printf("outbyte start ");)
       
    WriteFile(commHandle, &buffer, 1, &numWritten, NULL);
    SERIALDEBUG(printf("0x%0x sent\n", c);)
    return;
}

/** Reads a string of bytes from the serial port.
 *
 * @param buffer pointer to the buffer to be written to
 * @param buffLen the size of the buffer
 * @param nullTerminate if set to true it will null terminate the string
 *
 * @return int the number of bytes read
 */
int Serial::read(char *buffer, int buffLen, bool nullTerminate)
{
   DWORD numRead;
   if(nullTerminate)
   {
      --buffLen;
   }

   BOOL ret = ReadFile(commHandle, buffer, buffLen, &numRead, NULL);

   if(!ret)
   {
      return 0;
   }

   if(nullTerminate)
   {
      buffer[numRead] = '\0';
   }

   return numRead;
}

//                  delay in milliseconds
int Serial::inbyte(unsigned short timeout)
{
   char buffer;
   int numBytes;
   int delay = ((int) timeout) << 5;
   SERIALDEBUG(printf("inbyte start "); )
   while ((numBytes = read(&buffer, 1, false)) == 0) {
          // in ms
         Sleep(30);     /* 30 us * 32 = 960 us (~ 1 ms) */
         if (timeout) {
             if (--delay == 0) {
                 SERIALDEBUG(printf("inbyte timeout \n"); )
                 return -2;
             }
         }
      }  
SERIALDEBUG(printf("received 0x%0X ", buffer & 0x00ff);)
return(buffer & 0x00ff);
}


#define FLUSH_BUFFSIZE 10

/** Flushes everything from the serial port's read buffer
 */
void Serial::flush()
{
   char buffer[FLUSH_BUFFSIZE];
   int numBytes = read(buffer, FLUSH_BUFFSIZE, false);
   while(numBytes != 0)
   {
      numBytes = read(buffer, FLUSH_BUFFSIZE, false);
   }
}
#endif