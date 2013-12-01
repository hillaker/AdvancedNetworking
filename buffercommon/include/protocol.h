#include <uuid/uuid.h>
#include <sys/time.h>

#define NORMAL_KEY_PACKET 0xAFAF
#define SPECIAL_KEY_PACKET 0xFFFF

#pragma pack(1)
typedef struct 
{
	int packetType;
	int imageHeight;
	int imageWidth;
	int imageComponents;
	uuid_t packetID;
	struct timeval packetTime;
} packetHeader;
typedef struct 
{
	int packetType;
	int key;
	int modifier;
} keyPacket;
#pragma pack()
