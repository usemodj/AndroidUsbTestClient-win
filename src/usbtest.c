/*
 * usbtest.c
 * This file is part of OsciPrime
 *
 * Copyright (C) 2011 - Manuel Di Cerbo
 *
 * OsciPrime is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * OsciPrime is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OsciPrime; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

	

#include <stdio.h>
#include <usb.h>
#include <libusb.h>
#include <string.h>
#include <unistd.h>

//#define IN 0x85
//#define OUT 0x07
#define IN 0x81
#define OUT 0x02

// lsusb -v 
//#define VID 0x18d1
//#define PID 0x4E22
#define VID 0x18D1 	// idVendor
#define PID 0x4ee2 	// idProduct

#define ACCESSORY_PID 0x2d01
#define ACCESSORY_PID_ALT 0x2D00

#define LEN 2
#define BUFFER 1024
/*
If you are on Ubuntu you will require libusb as well as the headers...
We installed the headers with "apt-get source libusb"
gcc usbtest.c -I/usr/include/ -o usbtest -lusb-1.0 -I/usr/include/ -I/usr/include/libusb-1.0

Tested for Nexus S with Gingerbread 2.3.4

 Properties > C/C++ Build > Settings> GCC Linker > Libraries: usb-1.0

 Debug> sudo ./UsbTestClient
*/

static int mainPhase();
static int init(void);
static int deInit(void);
static void error(int code);
static void status(int code);
static int isUsbAccessory();
static int setupAccessory(
	const char* manufacturer,
	const char* modelName,
	const char* description,
	const char* version,
	const char* uri,
	const char* serialNumber);

//static
static struct libusb_device_handle* handle;
static char stop;
static char success = 0;

int main (int argc, char *argv[]){
	if(isUsbAccessory() < 0){
		if(init() < 0){
			deInit();
			return 0;
		}

		//doTransfer();
		if(setupAccessory(
			"Nexus-Computing GmbH",
			"osciprime",
			"Description",
			"antilope",
			"http://neuxs-computing.ch",
			"2254711SerialNo.") < 0){
			fprintf(stdout, "Error setting up accessory\n");
			deInit();
			return -1;
		};
	}

	if(mainPhase() < 0){
		fprintf(stdout, "Error during main phase\n");
		deInit();
		return -1;
	}	
	deInit();
	fprintf(stdout, "Done, no errors\n");
	return 0;
}

static int mainPhase(){
	unsigned char buffer[BUFFER];
	int response = 0;
	static int transferred;


	 while ((response = libusb_bulk_transfer(handle, IN, buffer, BUFFER, &transferred, 0)) == 0) {
		 	buffer[transferred]= '\0';
		 	fprintf(stdout, "%s\n", buffer);
		 	libusb_bulk_transfer(handle, OUT, buffer, transferred+1, &transferred, 0);
	 }
	 if(response < 0){error(response);return -1;}
	 return 0;
}


static int init(){
	//TODO: RESET process
	if(handle != NULL){
		libusb_release_interface(handle, 0);
		libusb_close(handle);
	}
	libusb_init(NULL);

	if((handle = libusb_open_device_with_vid_pid(NULL, VID, PID)) == NULL){
		fprintf(stdout, "Problem acquiring handle\n");
		return -1;
	}
	libusb_claim_interface(handle, 0);
	return 0;
}

static int deInit(){
	//TODO free all transfers individually...
	//if(ctrlTransfer != NULL)
	//	libusb_free_transfer(ctrlTransfer);
	if(handle != NULL){
		libusb_release_interface(handle, 0);
		libusb_close(handle);
	}
	libusb_exit(NULL);
	return 0;
}
static int isUsbAccessory() {
  int res;
  libusb_init(NULL);
  if((handle = libusb_open_device_with_vid_pid(NULL, VID,  ACCESSORY_PID)) == NULL) {
    fprintf(stdout, "Device is not USB Accessory Mode\n");
    res = -1;
  } else {
    // already usb accessory mode
    fprintf(stdout, "Device is already USB Accessory Mode\n");
    libusb_claim_interface(handle, 0);
    res = 0;
  }
  return res;
}

static int setupAccessory(
	const char* manufacturer,
	const char* modelName,
	const char* description,
	const char* version,
	const char* uri,
	const char* serialNumber){

	unsigned char ioBuffer[2];
	int devVersion;
	int response;
	int tries = 5;

	// Values to check for AOA capability
	response = libusb_control_transfer(
		handle, //handle
		0xC0, //bmRequestType(USB_DIRECTION_IN and USB_TYPE_VENDOR)
		51, //bRequest(0x33)
		0, //wValue
		0, //wIndex
		ioBuffer, //data
		2, //wLength
        0 //timeout
	);

	if(response <= 0){error(response);return-1;}

	devVersion = ioBuffer[1] << 8 | ioBuffer[0];
	fprintf(stdout,"Version Code Device: %d\n", devVersion);
	
	usleep(1000);//sometimes hangs on the next transfer :(
	/*
	 *  send identifying string information to the device.
	 *
	 * Characteristics for control requests
	 *  Request type: USB_DIR_OUT and USB_TYPE_VENDOR
	 *  Request: 52(ox34)
	 *  Value: 0
	 *  Index: String ID
	 *  Data: Zero terminated('\0') UTF8 string sent from accessory to device
	 *  �� String ID ��0�� means that the following data is for �쐌anufacturer name.��
	 *  �� String ID ��1�� means it is for �쐌odel name.��
	 *  �� String ID ��2�� means it is for �쐂escription.��
	 *  �� String ID ��3�� means it is for �쐖ersion.��
	 *  �� String ID ��4�� means it is for �쏹RI.��
	 *  �� String ID ��5�� means it is for �쐓erial number.��
	 */
	response = libusb_control_transfer(handle,0x40,52,0,0,(char*)manufacturer,strlen(manufacturer)+1,0);
	if(response < 0){error(response);return -1;}
	response = libusb_control_transfer(handle,0x40,52,0,1,(char*)modelName,strlen(modelName)+1,0);
	if(response < 0){error(response);return -1;}
	response = libusb_control_transfer(handle,0x40,52,0,2,(char*)description,strlen(description)+1,0);
	if(response < 0){error(response);return -1;}
	response = libusb_control_transfer(handle,0x40,52,0,3,(char*)version,strlen(version)+1,0);
	if(response < 0){error(response);return -1;}
	response = libusb_control_transfer(handle,0x40,52,0,4,(char*)uri,strlen(uri)+1,0);
	if(response < 0){error(response);return -1;}
	response = libusb_control_transfer(handle,0x40,52,0,5,(char*)serialNumber,strlen(serialNumber)+1,0);
	if(response < 0){error(response);return -1;}

	fprintf(stdout,"Accessory Identification sent: %d\n", devVersion);

	/*
	 * request the device to start up in accessory mode.
	 *
	 * Characteristics of a control vendor request on endpoint 0 to the android device
	 *  Request type: USB_DIR_OUT and USB_TYPE_VENDOR
	 *  Request: 53(0x35)
	 *  Value: 0
	 *  Index: 0
	 *  Data: None
	 */
	response = libusb_control_transfer(handle,0x40,53,0,0,NULL,0,0);
	if(response < 0){error(response);return -1;}

	fprintf(stdout,"Attempted to put device into accessory mode: %d\n", devVersion);

	if(handle != NULL)
		libusb_release_interface(handle, 0);

	int pid = PID;
	for(;;){//attempt to connect to new PID, if that doesn't work try ACCESSORY_PID_ALT
		tries--;
		fprintf(stdout, " tries %d PID: %x", tries, pid);
		if((handle = libusb_open_device_with_vid_pid(NULL, VID, pid)) == NULL){
			pid = ACCESSORY_PID;
			if(tries < 0){
				return -1;
			}
		}else{
			break;
		}
		sleep(1);
	}
	libusb_claim_interface(handle, 0);
	fprintf(stdout, "Interface claimed, ready to transfer data\n");
	return 0;
}

static void error(int code){
	fprintf(stdout,"\n");

	switch(code){
	case LIBUSB_ERROR_IO:
		fprintf(stdout,"Error: LIBUSB_ERROR_IO\nInput/output error.\n");
		break;
	case LIBUSB_ERROR_INVALID_PARAM:
		fprintf(stdout,"Error: LIBUSB_ERROR_INVALID_PARAM\nInvalid parameter.\n");
		break;
	case LIBUSB_ERROR_ACCESS:
		fprintf(stdout,"Error: LIBUSB_ERROR_ACCESS\nAccess denied (insufficient permissions).\n");
		break;
	case LIBUSB_ERROR_NO_DEVICE:
		fprintf(stdout,"Error: LIBUSB_ERROR_NO_DEVICE\nNo such device (it may have been disconnected).\n");
		break;
	case LIBUSB_ERROR_NOT_FOUND:
		fprintf(stdout,"Error: LIBUSB_ERROR_NOT_FOUND\nEntity not found.\n");
		break;
	case LIBUSB_ERROR_BUSY:
		fprintf(stdout,"Error: LIBUSB_ERROR_BUSY\nResource busy.\n");
		break;
	case LIBUSB_ERROR_TIMEOUT:
		fprintf(stdout,"Error: LIBUSB_ERROR_TIMEOUT\nOperation timed out.\n");
		break;
	case LIBUSB_ERROR_OVERFLOW:
		fprintf(stdout,"Error: LIBUSB_ERROR_OVERFLOW\nOverflow.\n");
		break;
	case LIBUSB_ERROR_PIPE:
		fprintf(stdout,"Error: LIBUSB_ERROR_PIPE\nPipe error.\n");
		break;
	case LIBUSB_ERROR_INTERRUPTED:
		fprintf(stdout,"Error:LIBUSB_ERROR_INTERRUPTED\nSystem call interrupted (perhaps due to signal).\n");
		break;
	case LIBUSB_ERROR_NO_MEM:
		fprintf(stdout,"Error: LIBUSB_ERROR_NO_MEM\nInsufficient memory.\n");
		break;
	case LIBUSB_ERROR_NOT_SUPPORTED:
		fprintf(stdout,"Error: LIBUSB_ERROR_NOT_SUPPORTED\nOperation not supported or unimplemented on this platform.\n");
		break;
	case LIBUSB_ERROR_OTHER:
		fprintf(stdout,"Error: LIBUSB_ERROR_OTHER\nOther error.\n");
		break;
	default:
		fprintf(stdout, "Error: unknown error\n");
		break;
	}

}

/*
static void status(int code){
	fprintf(stdout,"\n");
	switch(code){
		case LIBUSB_TRANSFER_COMPLETED:
			fprintf(stdout,"Success: LIBUSB_TRANSFER_COMPLETED\nTransfer completed.\n");
			break;
		case LIBUSB_TRANSFER_ERROR:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_ERROR\nTransfer failed.\n");
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_TIMED_OUT\nTransfer timed out.\n");
			break;
		case LIBUSB_TRANSFER_CANCELLED:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_CANCELLED\nTransfer was canceled.\n");
			break;
		case LIBUSB_TRANSFER_STALL:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_STALL\nFor bulk/interrupt endpoints: halt condition detected (endpoint stalled).\nFor control endpoints: control request not supported.\n");
			break;
		case LIBUSB_TRANSFER_NO_DEVICE:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_NO_DEVICE\nDevice was disconnected.\n");
			break;
		case LIBUSB_TRANSFER_OVERFLOW:
			fprintf(stdout,"Error: LIBUSB_TRANSFER_OVERFLOW\nDevice sent more data than requested.\n");
			break;
		default:
			fprintf(stdout,"Error: unknown error\nTry again(?)\n");
			break;
	}

}
*/
