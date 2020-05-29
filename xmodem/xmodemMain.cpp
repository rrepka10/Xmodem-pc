// xmodem.cpp : This file contains the 'main' function. Program execution begins and ends there.
// This program was tested wiht Visual Studio 2017 on Windows 10 64 bit and should run on most
// current windows machines.  This code will also compile and run under Turboc++ on PC DOS 3.3 
// The intent of this code is to provide a simple 2 way (very old)PC to (new)PC connection over 
// the serial port


#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>

#include "xmodem.h"
#include "Serial.h"
#ifndef __TURBOC__
#include <iostream>
// Used to simulate readbyte() and writebyte) functions
Serial *serialP;
#endif

#define REALTIME_DIVISOR    (396)       // bytes per second

void recieveBlock(void *ctx, char *buffer, int count);
void transmitBlock(void *ctx, char *buffer, int count);


/*---------------------------------------------------------------------------
  xmodem    comport speed  t | r file
---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
   MYINT32 st;
   int speed = 19200;
   const char *port = "com1";
   int estTime;
   FILE *handle;
   struct stat fileStat;
   clock_t start; 

   // Check command line syntax  we allow 2 formats xmodem direction file  ]
   // or xmodem direction file port speed
   if ((argc != 5) && (argc !=3) || (strcmp(argv [1],"t") && strcmp(argv [1], "r")) ||
       ((argc == 5) && (strncmp(argv [3], "com", 3) != 0))) {
      printf("This program uses a xmodem-1K protocol to send/received data between\n");
      printf("IBM PC-XT DOS computers and/or Windows computers over the serial port.\n");
      printf("Syntax: %s  t | r  fileName  [comPort  speed]\n", argv[0]);
      printf("  t | r    - transmit or receive\n");
      printf("  fileName - file name\n");
      printf("  comPort  - optional com port, default is com1\n");
      printf("  speed    - optional speed, default is 19200.\n");
      printf("e.g. %s t test.txt           - send test.txt over com1 at 19200\n", argv[0]);
      printf("     %s r test.txt com2 6900 - receive test.txt over com2 at 9600\n\n", argv[0]);
      exit(99);      
   }
   
   // use optional parameters
   if (argc == 5) { 
       speed = atoi(argv[4]);
       port = argv[3];
   }

   // verify the speed
   if ((speed != 110) && (speed != 300) && (speed != 600) && (speed != 9600) && (speed != 19200)) {
      printf("Error: speed must be: 110, 300, 600, 9600 or 19200\n");
      exit(98);
   }

   // Syntax must be good
   #ifdef __TURBOC__
   setBaudRate(port, speed);   
   #else
   tstring commPortName(port);
   // open the named com port aka com1, save a pointer to the data structure to inbyte and outbyte
   Serial serial(commPortName, speed);
   serialP = &serial;
   
   #endif
   
   /*------------------------------------------------------------------------
     Read or write data?
   ------------------------------------------------------------------------*/
   if (argv[1][0] == 'r') {
   
      // Make sure the file does not already exist
      if (stat(argv[2], &fileStat) == 0) {
         printf("Error, file %s exists\n", argv[2]);
         xmodemExit(95);
      }
   
      // Receive a file
      #ifdef __TURBOC__
      handle = fopen(argv[2], "wb");
      #else
      fopen_s(&handle, argv[2], "wb");
      #endif                           
      if (handle == NULL) {
         printf("Error, could not open %s\n", argv[2]);
         xmodemExit(99);
      }
     
      /*-----------------------------------------------------------------------
      StoreChunkType storeChunk - Function pointer for storing the received chunks or NULL    
      void *ctx                 - If storeChunk is NULL, pointer to the buffer to store the received data, 
                                  else function context pointer to pass to storeChunk()   
      int destsz                - Number of bytes to receive   
      int crc                   - Checksum mode to request: 0 - arithmetic, 1 - CRC16, 2 - YMODEM-G (CRC16 and no ACK)   
      int mode                  - Receive mode: 0 - normal, nonzero - receive YMODEM control packet 
      -----------------------------------------------------------------------*/
      printf("Receiving, <esc> to quit\n");
      // Wait to give the other app time to start
      Sleep(STARTUP_DLY);
      start = clock();
      st = XmodemReceive((StoreChunkType)recieveBlock, handle, MAX_BUFF, 1, 0);
      if (st < 0) {
         printf("Xmodem receive error: status: %ld\n", st);
      }
      else {
          start = clock() - start;
         printf("Xmodem received %ld bytes in %0ld sec\n", 
                   st, (long)start/(long)CLOCKS_PER_SEC);
         fclose(handle);
      }
      
   }  // end read
   else {
      // Send a file
      #ifdef __TURBOC__
      handle= fopen(argv[2], "rb");
      #else
      fopen_s(&handle, argv[2], "rb");
      #endif
      if (handle == NULL) {
         printf("Error, could not open %s\n", argv[2]);
         xmodemExit(99);
      }
     
      // Find out the file size
      stat(argv[2], &fileStat);
      if (fileStat.st_size <= 0)   {
         printf("Error, could not stat %s\n", argv[2]);
         xmodemExit(99);
      }
   
      /*-----------------------------------------------------------------------
      FetchChunkType fetchChunk - Function pointer for fetching the data chunks to be sent or NULL
      void *ctx                 - If fetchChunk is NULL, pointer to the buffer to be sent
                           else function context pointer to pass to fetchChunk()
      int srcsz                 - Number of bytes to send
      int onek                  - If nonzero 1024 byte blocks are used (XMODEM-1K)
      int mode                  - Transfer mode: 0 - normal, nonzero - transmit YMODEM control packet
      -----------------------------------------------------------------------*/
      estTime = (long)fileStat.st_size / (long)(REALTIME_DIVISOR * (long)60);
      if (estTime < 1) { estTime = 1; }
      printf("Sending %ld bytes (est %d min), <esc> to quit\n", 
                                            fileStat.st_size, estTime);
      // Wait to give the other app time to start
      Sleep(STARTUP_DLY);

      start = clock();
      st = XmodemTransmit((FetchChunkType)transmitBlock, handle, fileStat.st_size, 1, 0);
      if (st < 0) {
         printf("Xmodem transmit error: status: %ld\n", st);
      }
      else {
          start = clock() - start;
         printf("Xmodem successfully transmitted %ld bytes in %ld sec\n",
             st, (long)start / (long)CLOCKS_PER_SEC); 
      }
    
      fclose(handle);
   } // end transmit
xmodemExit(0);
}

// Read and transmit block handlers
void recieveBlock(void *ctx, char *buffer, int count){
   fwrite(buffer, 1, count, (FILE *)ctx);
}

void transmitBlock(void *ctx, char *buffer, int count){
   fread(buffer, 1, count, (FILE *)ctx); 
}

