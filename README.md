# Xmodem-pc
A PC DOS and Windows PC command line serial port binary file transfer utility and PC BASICA uudecode application.
The binaries-extras directory contains the Win32 and DOS 16 built application.  The directory also 
contains a uuencoded version of the DOS 16 application suitable for transfer via text COM port usage.
The uudecode.bas program is a PC DOS BASICA application which can convert the uuencoded file back to binary
on the old computer.  
Summary:  
1) use com redirection to send the xmodem16.uu and uudecode.bas programs to the old pc.  
2) Use the uudecode.bas program on the old pc to convert xmodem16.uu back to xmodem16.exe
3) Use xmodem16.exe on the old computer to communicate with xmodem32.exe on the new computer.
This program as tested with Visual Studio 2017 on Windows 10 64 bit and should run on most
current windows machines.  This code will also compile and run under Turboc++ on PC DOS 3.3.
The intent of this code is to provide a simple 2 way (very old)PC to (new)PC connection over 
the serial port

