#include <uuid/uuid.h>
#include <sys/time.h>

#pragma pack(1)
typedef struct 
{
	int imageHeight;
	int imageWidth;
	int imageComponents;
	uuid_t packetID;
	struct timeval packetTime;
} packetHeader;
#pragma pack()
