# VSI_shared_memory_prototype

Code to demonstrate the feasibility of using shared memory for interprocess
communications for the Vehicle Signal Interface project.

### Reasons for this Prototype

When work was started on the problem of interfacing the existing automobile
signals with higher level software programs that will need access to various
combinations of signals.  The existing "automotive-message-broker" software
that attempts to perform this function utilizes the Linux D-Bus system as it's
message transport interface so it was logical to do some performance
measurements on the D-Bus system.  When this was done, it showed that the
D-Bus system could only handle on the order of 30,000 messages per second (on
hardware comparable to that used for this test - See below for hardware
specifications).

Looking at the CAN database on a production model vehicle resulted in the
identification of approximately 1500 individual signals defined in the
database which could easily result in upwards of 50,000 signals per second
being presented on the CAN bus.  This CAN message rate would obviously swamp
the capabilities of the existing D-Bus transport system being used by the AMB
so an alternative needed to be explored.  This prototype is one of those
alternative message transport mechanisms.

### Overview

This set of files is intended to demonstrate the feasibility of using a shared
memory segment to communication messages to clients on the same platform.  For
this test, the clients will be individual processes to demonstrate that the
IPC and synchronization mechanism works with no data corruption issues.

This set of programs is NOT designed to recommend or demonstrate a possible
implementation of a message broker but rather to demonstrate that messages can
be communicated between processes in an efficient manner without any data
integrity issues.  The actual implementation of the message broker will be
considerably more complicated than what is demonstrated here resulting in
lower throughput.  This test should approximate the absolutely best case
scenario for throughput and the actual broker will certainly be slower.

### create

There are 3 basic programs defined in this set.  The first is the "create"
program.  This program must be run before any of the others to create and
initialize the shared memory area.  Running this program with no parameters
will result in a shared memory area being created with the default size.  See
the create.c file for more information on this and what the default size is.
The path to where the shared memory storage file is kept is defined in the
sharedMemory.h file as a constant string and can be changed to whatever the
user wishes.  It's currently set to create the file in the "/var/run/shm"
directory.

### write

The second program is the "write" program.  This program will write new
messages into the message pool.  It can be run as a one-off program in which
case it will write the requested number of records into the message pool and
then quit.  It can also be run in a "continuous" mode in which it will run
indefinitely (until the user stops it with ctrl-c) reporting the timing
results periodically at the intervals requested by the user.  See the write.c
program for more details on how to run this program.  Note that when this
program "writes" a record into the message pool, it does not actually alter
any of the data already in the record in the message pool.  The writes occur
but the data being written is identical t what is already in the record in the
message pool so nothing changes.  The record ID and the "data" portion of the
new record are copied into the message pool to simulate what a CAN message
might do when received but the other fields in the destination record are not
altered.  This was done to "dirty" the memory area as far as the system
caching is concerned to better simulate the real activity of a CAN message
storage function.

### fetch

The third program is the "fetch" program.  This program will read existing
messages from the message pool.  It can be run as a one-off program in which
case it will read the requested number of records from the message pool and
then quit.  It can also be run in a "continuous" mode in which it will run
indefinitely (until the user stops it with ctrl-c) reporting the timing
results periodically at the intervals requested by the user.  See the fetch.c
program for more details on how to run this program.  Each time a record is
fetched, the ID and data fields are copied out of the message pool into the
record supplied by the caller to simulate what the actual message retrieval
functionality would be.

### Common characteristics

All 3 programs can be given a parameter defining the number of messages to be
used.  In the case of the create program, this is the size of the shared
memory message pool and in the case of the other 2 programs, it is the number
of operations to be performed before reporting the results, either in the
one-off mode or the continuous mode.

More information about each of the 3 programs can be gotten from their
respective source files and they all have a "help" command line option, either
"-h" or "-?".

The write and fetch programs also have an option to allow the user to change
the algorithm used to access the records in the memory pool.  By default, the
records are accessed sequentially starting at the beginning and proceeding
upwards until the end of the memory pool is reached and then starting at the
beginning again.  If the user specifies the "-r" option when running one of
these, the algorithm will be changed to use a random number generator to
access the records in the memory pool in a random fashion.  This "random"
option was included because it was discovered that the call to the "rand"
function in Linux was actually quite costly.  The throughput in random mode is
approximately 1/3 that of the sequential accesses.

The test programs were created as individual programs so that users can easily
fire up the tests with any combination of readers and writers that they like.
From the testing that has been done so far, the throughput of the readers and
writers is virtually identical but others can test it for themselves.

### Results

Running the above programs on my laptop produced the following results:

#### Hardware

	System Information
		Manufacturer: Dell Inc.
		Product Name: Latitude E7450

	Processor Information
		Family: Core i7
		Manufacturer: Intel(R) Corporation
		Signature: Type 0, Family 6, Model 61, Stepping 4
		Version: Intel(R) Core(TM) i7-5600U CPU @ 2.60GHz
		Max Speed: 2600 MHz
		Current Speed: 2600 MHz
		Core Count: 2
		Core Enabled: 2
		Thread Count: 4
		Characteristics:
			64-bit capable
			Multi-Core
			Hardware Thread
			Execute Protection
			Enhanced Virtualization
			Power/Performance Control

	Memory Array Mapped Address
		Starting Address: 0x00000000000
		Ending Address: 0x001FFFFFFFF
		Range Size: 8 GB

	Disk Information
		SAMSUNG SSD PM871 mSATA 512GB (EMT42D0Q)

#### Timing

Typical:

1,000,000 records in 31,053,184 nsec. 31 msec.
	32,202,816 records/sec - Avg: 32,177,772 records/sec

Approximately the same performance was observed for both the "write" and
"fetch" programs.

Obviously the timing results are highly dependent on the hardware platform
that the code is run on so everyone running these tests is likely to observe
different performance numbers.

However, it seems safe to say that we can expect significantly higher
performance from a shared memory implementation than the existing D-Bus
implementations being used by systems like automotive-message-broker.

The system was run with the default of one million messages per iteration with
30 processes running (half write and half fetch processes) for over an hour
and there was no corruption of any of the records in the shared memory
segment.
