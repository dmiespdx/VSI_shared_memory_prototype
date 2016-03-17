//
//	w r i t e . c 
//
//  Write records into the shared memory message pool.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>

#include "sharedMemory.h"

//
// NOTE: All references (and pointers) to data in the can message buffer is
// performed by using the index into the array of messages.  All of these
// references need to be relocatable references so this will work in multiple
// processes that have their shared memory segments mapped to different base
// addresses.
//


//
// Define the value of the number of messages that we will write in the shared
// memory segment for each iteration through the write loop.  Note that this
// default value can be overridden using the "-m" command line option.
//
static unsigned int messagesToStore = 1000 * 1000;

//
// Define the flag that causes this function to write data continuously instead
// of just once.
//
static bool continuousRun = false;

//
// Define the flag that will cause us to use the "rand" function to write
// records into the shared memory pool at random positions.  If this is not
// set, records will be written sequentially.  
//
static bool useRandom = false;

//
// Define the usage message function.
//
static void usage ( const char* executable )
{
    printf ( " \n\
Usage: %s options\n\
\n\
  Option     Meaning       Type     Default \n\
  ======  ==============  ======  =========== \n\
    -c    Continuous       bool      false \n\
    -m    Message Count    int     1,000,000 \n\
    -h    Help Message     N/A        N/A \n\
    -r    Random Write     bool    1,000,000 \n\
    -?    Help Message     N/A       false \n\
\n\n\
",
             executable );
}


//
// M A I N
//
int main ( int argc, char* const argv[] )
{
	//
	// The following locale settings will allow the use of the comma
	// "thousands" separator format specifier to be used.  e.g. "10000"
	// will print as "10,000" (using the %'u spec.).
	//
	setlocale ( LC_ALL, "");

    //
    // Parse any command line options the user may have supplied.
    //
	int status;
	char ch;

    while ( ( ch = getopt ( argc, argv, "chm:r?" ) ) != -1 )
    {
        switch ( ch )
        {
		  //
		  // Get the continuous run option flag if present.
		  //
		  case 'c':
			printf ( "Record writing will run continuously. <ctrl-c> to "
			         "quit...\n" );
		    continuousRun = true;
			break;

		  //
		  // Get the requested buffer size argument and validate it.
		  //
		  case 'm':
		    messagesToStore = atol ( optarg );
			if ( messagesToStore <= 0 )
			{
				printf ( "Invalid buffer count[%u] specified.\n",
						 messagesToStore );
				usage ( argv[0] );
				exit (255);
			}
			break;

		  //
		  // Get the random insert option flag if present.
		  //
		  case 'r':
			printf ( "Record writing will be random.\n" );
		    useRandom = true;
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
        printf ( "Invalid parameters[s] encountered: %s\n", argv[argc] );
        usage ( argv[0] );
        exit (255);
    }
	//
	// Open the shared memory file.
	//
	sharedMemory = sharedMemoryOpen();
	if ( sharedMemory == 0 )
	{
		printf ( "Unable to open the shared memory segment - Aborting\n" );
		exit (255);
	}
	//
	// Get the sizes of the buffer pool and the entire shared memory
	// segment.
	//
	unsigned int bufferPoolSize = sharedMemoryGetPoolSize ( sharedMemory );
	sharedMemorySize            = sharedMemoryGetSegmentSize ( sharedMemory );

	//
	// Define the CAN message that we will use to insert records into the
	// shared memory segment.
	//
	canMessage_t      canMessage;
	canMessageIndex_t messageIndex;

	(void) memset ( &canMessage, 0, sizeof(canMessage) );

	//
	// Define the performance spec variables.
	//
	struct timespec startTime;
	struct timespec stopTime;
    unsigned long   startTimeNs;
    unsigned long   stopTimeNs;
    unsigned long   diffTimeNs;
    unsigned long   totalNs = 0;
    unsigned long   totalRecords = 0;
	unsigned long   rps;

    //
    // Initialize the random number generator.
    //
    srand ( 1 );

	//
	// Repeat the following at least once...
	//
	// Note that if "continuous" mode has been selected, this loop will run
	// forever.  The user will need to kill it manually from the command line.
	//
	// Also note that the inclusion of the "rand" function call in the loop
	// below has significantly slowed down the performance of the loop.  It
	// went from about 25M inserts/sec to 11M with the "rand" in place.
	//
	do
	{
		//
		// For the number of iterations specified by the caller...
		//
		clock_gettime(CLOCK_REALTIME, &startTime);
		for ( unsigned int i = 0; i < messagesToStore; i++ )
		{
			//
			// Generate a message index and use that to populate our
			// canMessage structure.  Note that this index may be random or
			// sequential depending on whether the user specified the random
			// behavior.  The default is to use the sequential mode.
			//
			if ( useRandom )
			{
				messageIndex = rand() % bufferPoolSize;
			}
			else
			{
				messageIndex = i % bufferPoolSize;
			}
			canMessage.canMessage.can_id = messageIndex;

			//
			// Go insert this message into the message pool.
			//
			// Note: We will also increment the "flags" field to see how many
			// times each message has been hit in the message pool.  This is
			// not a normal function for the insert function but for this
			// test, it's a useful thing to do.
			//
			messageIndex = insertMessage ( &canMessage );
		}
		clock_gettime(CLOCK_REALTIME, &stopTime);

		//
		// Compute all of the timing metrics.
		//
		startTimeNs = startTime.tv_sec * 1000000000 + startTime.tv_nsec;
		stopTimeNs  = stopTime.tv_sec  * 1000000000 + stopTime.tv_nsec;
		diffTimeNs = stopTimeNs - startTimeNs;
		rps = messagesToStore / ( diffTimeNs / 1000000000.0 );
		totalNs += diffTimeNs;
		totalRecords += messagesToStore;

		//
		// Display the amount of time it took to process this iteration of the
		// insert loop.
		//
		printf ( "%'d records in %'lu nsec. %'lu msec. - %'lu records/sec - Avg: %'lu\n",
				 messagesToStore,
				 stopTimeNs - startTimeNs,
				 (stopTimeNs - startTimeNs) / 1000000,
				 rps,
				 (unsigned long)( totalRecords / ( totalNs / 1000000000.0 ) )
			   );

	}   while ( continuousRun );
	//
	// Close our shared memory segment and exit.
	//
	sharedMemoryClose ( sharedMemory, sharedMemorySize );

	//
	// Return a good completion code to the caller.
	//
    return 0;
}
