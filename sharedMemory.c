
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sharedMemory.h"

//
// Open the shared memory segment being used for this test.
//
sharedMemory_t* sharedMemoryOpen ( void )
{
	//
	// Open the shared memory file.
	//
	int fd = 0;
	fd = open ( sharedMemoryName, O_RDWR|O_CREAT, 0666);
	if (fd < 0)
	{
		printf ( "Unable to open the shared memory segment[%s] errno: %u[%s].\n",
				 sharedMemoryName, errno, strerror(errno) );
		return 0;
	}
    //
    // Go get the size of the shared memory segment.
    //
	int status;
    struct stat stats;
    status = fstat ( fd, &stats );
    if ( status == -1 )
    {
        printf ( "Unable to get the size of the shared memory segment[%s] errno: "
                 "%u[%s].\n", sharedMemoryName, errno, strerror(errno) );
        (void) close ( fd );
        return 0;
    }
    //
    // If the shared memory segment has not been initialized, error out.
    //
    if ( stats.st_size <= 0 )
    {
        printf ( "Shared memory segment[%s] is empty - Aborting\n",
                 sharedMemoryName );
    }
	//
	// Map the shared memory file into virtual memory.
	//
	sharedMemory = mmap ( NULL, stats.st_size, PROT_READ|PROT_WRITE,
					      MAP_SHARED, fd, 0 );
	(void) close ( fd );
	if ( sharedMemory == MAP_FAILED )
	{
		printf ( "Unable to map the shared memory segment. errno: %u[%s].\n",
				 errno, strerror(errno) );
		return 0;
	}
	return sharedMemory;
}


//
// Close the shared memory segment being used for this test.
//
// Note that this call will cause the data that has been modified in memory to
// be synchronized to the disk file that stores the data persistently.
//
void sharedMemoryClose ( sharedMemory_t* sharedMemory, 
						 unsigned int sharedMemorySegmentSize )
{
	(void) munmap ( sharedMemory, sharedMemorySegmentSize );
}


//
// Return the size of the shared memory segment from the shared memory segment.
//
unsigned int sharedMemoryGetSegmentSize ( sharedMemory_t* sharedMemory )
{
	return sharedMemory->totalSharedMemorySize;
}


//
// Return the number of message buffers in the shared memory segment from the
// shared memory segment.
//
unsigned int sharedMemoryGetPoolSize ( sharedMemory_t* sharedMemory )
{
	return sharedMemory->totalMessageCount;
}


//
//  I n s e r t M e s s a g e 
//
// Insert a new message into the message buffer.
//
// This function does not actually do anything to change the structure of the
// data message pools.  It will use the ID field in the incoming message to
// compute the index into the message pool for this message and then it will
// copy the contents of the ID and data fields from the incoming message into
// the message pool to simulate changing data in the pool (in case it effects
// caching and such).  To assure us that this is all working, the flags field
// in the CAN message will be incremented each time a message is copied into
// the pool so we can get an idea of the distribution and assure ourselves
// that the writing is actually happening.
//
int insertMessage ( struct canMessage_t* newMessage )
{
    canMessageIndex_t newIndex = 0;
	canMessage_t*     message = 0;

	//
	// Get the message ID to be used as the index into the array of messages
	// in the message pool.
	//
	newIndex = newMessage->canMessage.can_id;

	//
	// Get the address of this message in the share memory pool.
	//
	message = &( sharedMemory->messagePoolBase[newIndex] );

    //
    // Acquire the lock on the shared memory segment.
	//
	// Note that this call will hang if someone else is currently using the
	// shared memory segment.  It will return once the lock is acquired and it
	// is safe to manipulate the share memory data.
    //
	sharedMemoryLock();

	//
	// Copy the message ID into the Id and data fields from the incoming
	// message into the message pool entry.
	//
    message->canMessage.can_id = newMessage->canMessage.can_id;

    //
    // Give up the shared memory block lock.
    //
    sharedMemoryUnlock();

    //
    // Return the index of the incoming CAN message block to the caller.
    //
    return newIndex;
}


//
//	f e t c h M e s s a g e 
//
// Retrieve a new message into the message buffer.
//
// This function does not do anything to change the structure of the data
// message pools, it is a read only function.  It will use the ID field in the
// incoming message to compute the index into the message pool for this
// message and then it will copy the contents of the message into the message
// structure supplied by the caller.  To assure us that this is all working,
// the flags field in the CAN message will be incremented each time a message
// is copied out of the pool so we can get an idea of the distribution and
// assure ourselves that the reading is actually happening.
//
int fetchMessage ( struct canMessage_t* newMessage )
{
    canMessageIndex_t newIndex = 0;
	canMessage_t*     message = 0;

	//
	// Get the message ID to be used as the index into the array of messages
	// in the message pool.
	//
	newIndex = newMessage->canMessage.can_id;

	//
	// Get the address of this message in the share memory pool.
	//
	message = &( sharedMemory->messagePoolBase[newIndex] );

    //
    // Acquire the lock on the shared memory segment.
	//
	// Note that this call will hang if someone else is currently using the
	// shared memory segment.  It will return once the lock is acquired and it
	// is safe to manipulate the share memory data.
    //
	sharedMemoryLock();

	//
	// Copy the message ID and data fields from the message in the shared
	// memory message pool into the user supplied message.
	//
    newMessage->canMessage.can_id = message->canMessage.can_id;

    //
    // Give up the shared memory block lock.
    //
    sharedMemoryUnlock();

    //
    // Return the index of the incoming CAN message block to the caller.
    //
    return newIndex;
}


//
// Acquire the shared memory lock.  This call will hang if the lock is
// currently not available and return when the lock has been successfully
// acquired.
//
void sharedMemoryLock ( void )
{
	pthread_mutex_lock ( &sharedMemory->lock );
}


//
// Release the shared memory lock.  If another process or thread is waiting on
// this lock, the scheduler will decide which will run so the order of
// processes/threads may not be the same as the order in which they called the
// lock function.
//
void sharedMemoryUnlock ( void )
{
	pthread_mutex_unlock ( &sharedMemory->lock );
}
