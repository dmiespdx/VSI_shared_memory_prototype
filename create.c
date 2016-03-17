
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <locale.h>

#include "sharedMemory.h"

//
// This set of files is intended to demonstrate the feasibility of using a
// shared memory segment to communication messages to clients on the same
// platform.  For this test, the clients will be individual processes to
// demonstrate that the IPC mechanism works with no data corruption issues.
//
// This set of programs is NOT designed to recommend or demonstrate a possible
// implementation of a message broker but rather to demonstrate that messages
// can be communicated between processes in an efficient manner without any
// data integrity issues.  The actual implementation of the message broker
// will be considerably more complicated than what is demonstrated here
// resulting in lower throughput.  This test should approximate the absolutely
// best case scenario for throughput and the actual broker will certainly be
// slower.
//
// There are 3 basic programs defined in this set.  The first is the "create"
// program.  This program must be run before any of the others to create and
// initialize the shared memory area.  Running this program with no parameters
// will result in a shared memory area being created with the default size.
// See the create.c file for more information on this and what the default
// size is.  The path to where the shared memory storage file is kept is
// defined in the sharedMemory.h file as a constant string and can be changed
// to whatever the user wishes.  It's currently set to create the file in the
// "/var/run/shm" directory.
//
// The second program is the "write" program.  This program will write new
// messages into the message pool.  It can be run as a one-off program in
// which case it will write the requested number of records into the message
// pool and then quit.  It can also be run in a "continuous" mode in which it
// will run indefinitely (until the user stops it with <ctrl>c) reporting the
// timing results periodically at the intervals requested by the user.  See
// the write.c program for more details on how to run this program.  Note that
// when this program "writes" a record into the message pool, it does not
// actually alter any of the data already in the record in the message pool.
// The writes occur but the data being written is identical t what is already
// in the record in the message pool so nothing changes.  The record ID and
// the "data" portion of the new record are copied into the message pool to
// simulate what a CAN message might do when received but the other fields in
// the destination record are not altered.  This was done to "dirty" the
// memory area as far as the system caching is concerned to better simulate
// the real activity of a CAN message storage function.
//
// The third program is the "fetch" program.  This program will read existing
// messages from the message pool.  It can be run as a one-off program in which
// case it will read the requested number of records from the message pool
// and then quit.  It can also be run in a "continuous" mode in which it will
// run indefinitely (until the user stops it with <ctrl>c) reporting the
// timing results periodically at the intervals requested by the user.  See
// the fetch.c program for more details on how to run this program.  Each time
// a record is fetched, the ID and data fields are copied out of the message
// pool into the record supplied by the caller to simulate what the actual
// message retrieval functionality would be.
//
// All 3 programs can be given a parameter defining the number of messages to
// be used.  In the case of the create program, this is the size of the shared
// memory message pool and in the case of the other 2 programs, it is the
// number of operations to be performed before reporting the results, either
// in the one-off mode or the continuous mode.
//
// More information about each of the 3 programs can be gotten from their
// respective source files.
//
// The write and fetch programs also have an option to allow the user to
// change the algorithm used to access the records in the memory pool.  By
// default, the records are accessed sequentially starting at the beginning
// and proceeding upwards until the end of the memory pool is reached and then
// starting at the beginning again.  If the user specifies the "-r" option
// when running one of these, the algorithm will be changed to use a random
// number generator to access the records in the memory pool in a random
// fashion.  This "random" option was included because it was discovered that
// the call to the "rand" function in Linux was actually quite costly.  The
// throughput in random mode is approximately 1/3 that of the sequential
// accesses.
//
// The test programs were created as individual programs so that users can
// easily fire up the tests with any combination of readers and writers that
// they like.  From the testing that has been done so far, the throughput of
// the readers and writers is virtually identical but others can test it for
// themselves.
//

//
// Note: All references (and pointers) to data in the CAN message buffer are
// performed by using the index into the array of messages.  All of these
// references need to be relocatable references so this will work in multiple
// processes that have their shared memory segments mapped to different base
// addresses.
//


//
// Define the value of the number of messages that the shared memory segment
// will contain.  We also define the default value for this value here.  Note
// also that this default value can be overridden using the "-m" command line
// option.
//
static unsigned int totalSharedMemoryMessages = 1000 * 1000;

//
// Define the usage message function.
//
static void usage ( const char* executable )
{
    printf ( " \n\
Usage: %s options\n\
\n\
  Option     Meaning      Type     Default \n\
  ======  =============  ======  =========== \n\
    -m    Message Count   int     1,000,000 \n\
    -h    Help Message    N/A        N/A \n\
    -?    Help Message    N/A        N/A \n\
\n\n\
", executable );
}


//
// M A I N
//
int main ( int argc, char* const argv[] )
{
	//
	// Set the locale so our fancy printf formats work correctly.
	//
	setlocale ( LC_ALL, "");

    //
    // Parse any command line options the user may have supplied.
    //
	int status;
	char ch;

    while ( ( ch = getopt ( argc, argv, "hm:?" ) ) != -1 )
    {
		//
		// Depending on the current command line option...
		//
        switch ( ch )
        {
		  //
		  // Get the requested buffer size argument and validate it.
		  //
		  case 'm':
		    totalSharedMemoryMessages = atol ( optarg );
			if ( totalSharedMemoryMessages <= 0 )
			{
				printf ( "Invalid message count[%u] specified.\n",
						 totalSharedMemoryMessages );
				usage ( argv[0] );
				exit (255);
			}
			break;

          case 'h':
          case '?':
          default:
            usage ( argv[0] );
            exit ( 0 );
        }
    }
    //
	// If the user supplied any arguments other than the buffer size value,
	// they are not valid arguments so complain and quit.
    //
	argc -= optind;

    if ( argc != 0 )
    {
        printf ( "Invalid parameter encountered: %s\n", argv[argc] );
        usage ( argv[0] );
        exit (255);
    }
	//
	// Compute the sizes of the buffer pool and the entire shared memory
	// segment.
	//
	unsigned int bufferPoolSize = totalSharedMemoryMessages * sizeof(canMessage_t);
	sharedMemorySize = sizeof(sharedMemory_t) + bufferPoolSize;

	//
	// Open the shared memory file.
	//
	int fd = 0;
	fd = open ( sharedMemoryName, O_RDWR|O_CREAT, 0666);
	if (fd <= 0)
	{
		printf ( "Unable to open shared memory segment[%s] errno: %u[%s].\n",
				 sharedMemoryName, errno, strerror(errno) );
		exit (255);
	}
    //
    // Go get the size of the shared memory segment.
    //
    struct stat stats;
    status = fstat ( fd, &stats );
    if ( status == -1 )
    {
        printf ( "Unable to get the size of the shared memory segment[%s] "
		         "errno: %u[%s].\n", sharedMemoryName, errno, strerror(errno) );
        (void) close ( fd );
        exit (255);
    }
    //
    // Initialize the shared memory segment.
    //
	printf ( "Initializing the shared memory segment...\n" );
	
	//
	// If the shared memory size is not zero, the shared memory segment
	// already exists.  In this case, we will destroy the old segment and
	// create a new on of the specified size.
	//
	if ( stats.st_size != 0 )
	{
        printf ( "Shared memory segment[%s]\n"
		         "    already exists with size %'lu\n"
				 "    It will be destroyed and recreated with size %'u\n",
		         sharedMemoryName, stats.st_size, sharedMemorySize );
	}
	//
	// Make the pseudo-file for the shared memory segment the size of the
	// shared memory segment we are going to create.  Note that this will
	// destroy any existing data if the segment already exists.
	//
	status = ftruncate ( fd, sharedMemorySize );
	if (status != 0)
	{
		printf ( "Unable to resize the shared memory segment to [%u] bytes - "
				 "errno: %u[%s].\n", sharedMemorySize, errno, strerror(errno) );
		exit (255);
	}
	//
	// Map the shared memory file into virtual memory.
	//
	sharedMemory = mmap ( NULL, sharedMemorySize, PROT_READ|PROT_WRITE,
					      MAP_SHARED, fd, 0 );
	(void) close ( fd );
	if ( sharedMemory == MAP_FAILED )
	{
		printf ( "Unable to map shared memory segment. errno: %u[%s].\n",
				 errno, strerror(errno) );
		exit (255);
	}
	//
	// Initialize all of the infrastructure and allocate and initialize all of
	// the buffer records.
	//
	// Initialize the data fields in the shared memory segment.
	//
	sharedMemory->totalMessageCount     = totalSharedMemoryMessages;
	sharedMemory->totalSharedMemorySize = sharedMemorySize;

	sharedMemory->freeListCount = totalSharedMemoryMessages;
	sharedMemory->freeListHead  = 0;
	sharedMemory->freeListTail  = totalSharedMemoryMessages - 1;

	//
	// Initialize the message buffers.
	//
	// Note that this should not be necessary because the mmap call above
	// should zero all of the allocated memory but...
	//
	(void) memset ( sharedMemory->messagePoolBase, 0, sharedMemory->totalSharedMemorySize );

	//
	// Initialize the list of available CAN message buffers to include the
	// entire list of buffers.
	//
	for ( int i = 0; i < sharedMemory->totalMessageCount; i++ )
	{
		sharedMemory->messagePoolBase[i].nextMessageIndex = i + 1;
	}
	//
	// Last buffer indicator.
	//
	sharedMemory->messagePoolBase[sharedMemory->totalMessageCount - 1].nextMessageIndex
		= CAN_END_OF_LIST;

	//
	// Initialize the mutex object.
	//
	// Initialize the mutex attributes structure.
	//
	status =  pthread_mutexattr_init ( &sharedMemory->mutexAttributes );
	if ( status != 0 )
	{
		printf ( "Unable to initialize mutex attributes - errno: %u[%s].\n",
				 status, strerror(status) );
		exit (255);
	}
	//
	// Set this mutex to be a process shared mutex.
	//
	status = pthread_mutexattr_setpshared ( &sharedMemory->mutexAttributes,
											PTHREAD_PROCESS_SHARED );
	if ( status != 0 )
	{
		printf ( "Unable to set shared mutex attributes - errno: %u[%s].\n",
				 status, strerror(status) );
		exit (255);
	}
	//
	// Initialize the mutex itself.
	//
	status = pthread_mutex_init ( &sharedMemory->lock,
								  &sharedMemory->mutexAttributes );
	if ( status != 0 )
	{
		printf ( "Unable to initialize shared mutex - errno: %u[%s].\n",
				 status, strerror(status) );
		exit (255);
	}
	//
	// Initialize all of the data records in the shared memory message pool.
	//
	// For each message buffer in the message pool...
	//
	for ( unsigned int i = 0; i < totalSharedMemoryMessages; i++ )
	{
		sharedMemory->messagePoolBase[i].canMessage.can_id = i;
	}
	//
	// Unmap our shared memory segment and exit.
	//
	status = munmap ( sharedMemory, sharedMemorySize );

	if ( status != 0 )
	{
		printf ( "Unable to unmap the shared memory segment - errno: %u[%s].\n",
				 status, strerror(status) );
		exit (255);
	}
	//
	// Return a good completion code to the caller.
	//
    return 0;
}
