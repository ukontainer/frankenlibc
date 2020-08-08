#if defined(__x86_64__)

#define _COMM_PAGE64_BASE_ADDRESS	( 0x00007fffffe00000 )   /* base address of allocated memory */
#define _COMM_PAGE_START_ADDRESS		_COMM_PAGE64_BASE_ADDRESS
#define _COMM_PAGE64_START_ADDRESS	( _COMM_PAGE64_BASE_ADDRESS )	/* address traditional commpage code starts on */
#define _COMM_PAGE_TIME_DATA_START	(_COMM_PAGE_START_ADDRESS+0x050)	/* base of offsets below (_NT_SCALE etc) */
#define _COMM_PAGE_NEWTIMEOFDAY_DATA	(_COMM_PAGE64_START_ADDRESS + 0x0D0)

#elif defined(__arm64__)

/* from xnu/osfmk/arm/cpu_capabilities.h */
#define _COMM_PAGE64_BASE_ADDRESS		(0x0000000FFFFFC000ULL) /* In TTBR0 */
#define _COMM_PAGE_START_ADDRESS		_COMM_PAGE64_BASE_ADDRESS
#define _COMM_PAGE_TIMEBASE_OFFSET		(_COMM_PAGE_START_ADDRESS+0x088)	// uint64_t timebase offset for constructing mach_absolute_time()
#define _COMM_PAGE_USER_TIMEBASE		(_COMM_PAGE_START_ADDRESS+0x090)	// uint8_t is userspace mach_absolute_time supported (can read the timebase)
#define _COMM_PAGE_NEWTIMEOFDAY_DATA		(_COMM_PAGE_START_ADDRESS+0x120)	// used by gettimeofday(). Currently, sizeof(new_commpage_timeofday_data_t) = 40.

#endif
