#include <uuid/uuid.h>
#include <sys/time.h>

#pragma pack(1)
typedef struct 
{
	unsigned long packetLength;
	uuid_t packetID;
	struct timeval packetTime;
} packetHeader;
#pragma pack()
