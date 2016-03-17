#pragma once
#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <pthread.h>
#include <linux/can.h>

//
// Note: All references (and pointers) to data in the can message buffer are
// performed by using the index into the array of messages.  All of these
// references need to be relocatable references so this will work in multiple
// processes that have their shared memory segments mapped to different base
// addresses.
//

//
// Define the data types that we will be using in the CAN messages.
//
typedef unsigned int  canMessageId_t;      // 4 bytes
typedef unsigned long canMessageData_t;    // 8 bytes
typedef unsigned int  canMessageIndex_t;   // 4 bytes

static const canMessageIndex_t CAN_END_OF_LIST = 0xffffffff;

//
// This is the structure of a canMessage.  Note that for purposes of this demo,
// we don't really care what's in the message data.  We only care about the ID
// field and the 8 bytes of data at the end of the message.
//
typedef struct canMessage_t
{
    //
	// This is the index into the buffer array where the next message is
	// located.  If this is the last message in the list, this value will be
	// CAN_END_OF_LIST.
    //
    canMessageIndex_t nextMessageIndex;

    //
    // This is the CAN message data itself.
    //
	// /**
	//  * struct can_frame - basic CAN frame structure
	//  *
	//  * @can_id:  CAN ID of the frame and CAN_*_FLAG flags, see canid_t 
	//              definition
	//  * @can_dlc: frame payload length in byte (0 .. 8) aka data length code
	//  *           N.B. the DLC field from ISO 11898-1 Chapter 8.4.2.3 has a
	//  *           1:1 mapping of the 'data length code' to the real payload
	//              length
	//  * @__pad:   padding
	//  * @__res0:  reserved / padding
	//  * @__res1:  reserved / padding
	//  * @data:    CAN frame payload (up to 8 byte)
	//  *
	//  */
	//
	// struct can_frame {
	// 	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	// 	__u8    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	// 	__u8    __pad;   /* padding */
	// 	__u8    __res0;  /* reserved / padding */
	// 	__u8    __res1;  /* reserved / padding */
	// 	__u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
	// };
    //
	struct can_frame canMessage;

}   canMessage_t;


#endif		// End of CAN_MESSAGE_H
