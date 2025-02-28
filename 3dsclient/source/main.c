#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include "alloca.h"

#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a, b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
  _a < _b ? _a : _b; })

#include <3ds.h>
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
#define KEYS_PRESSED(held, keys) (((held) & (keys)) == (keys))

#define COPY_INTO(buffer, value, offsetPtr)               \
	{                                                     \
		__typeof__(value) rd = value;                     \
		memcpy(buffer + *(offsetPtr), &(rd), sizeof(rd)); \
		*offsetPtr += sizeof(rd);                         \
	}

#define swap(a, b)                \
	{                             \
		__typeof__(a) temp = (a); \
		(a) = (b);                \
		(b) = (temp);             \
	}

char *ipAddr = "10.149.99.146";
//char *ipAddr = "192.168.178.138";
int port = 12346;
#define MICROSECONDS_IN_SECOND 1000000

void failExit(const char *fmt, ...);
int64_t difftimespec_us(const struct timespec *after, const struct timespec *before);
void dispose(void);

static u32 *SOC_buffer = NULL;

int mysock = 0;

volatile double fps2 = 0;
volatile bool terminate = 0;

struct
{
	Handle sync;
	double fps;
} config = {.fps = 125};

void myThreadFunc(void *arg)
{
	// Schedule
	struct timespec *frameTime = alloca(sizeof(struct timespec));
	struct timespec *oldFrameTime = alloca(sizeof(struct timespec));
	bool isOldFrameTimeSet = false;

	// Measure
	struct timespec *sendTime = alloca(sizeof(struct timespec));
	struct timespec *oldSendTime = alloca(sizeof(struct timespec));
	bool isOldSendTimeSet = false;

	int remaining_us = 0;

	bool isOldBufferSet = false;
	// Buttons + (2 Sticks + 1 Touchscreen) + seqNr
	int bufferLen = sizeof(u32) + sizeof(s16) * 6 + sizeof(u32);
	char *buffer = alloca(bufferLen);
	char *oldBuffer = alloca(bufferLen);
	u32 seqNr = 0;

	bool isnew3ds = 0;
	APT_CheckNew3DS((bool *)&isnew3ds);

	printf("Hello From Thread\n");
	while (!terminate)
	{
KEY_A
		svcWaitSynchronization(config.sync, INT64_MAX);
		s64 fps = config.fps;
		svcReleaseMutex(config.sync);
		s64 period_us = (s64)(1000000.0 / fps);
		clock_gettime(CLOCK_MONOTONIC, frameTime);

		if (isOldFrameTimeSet)
		{
			s64 elapsed_us = difftimespec_us(frameTime, oldFrameTime);
			remaining_us -= elapsed_us;
			double fps = 1000000.0 / elapsed_us;
		}

		swap(frameTime, oldFrameTime);
		isOldFrameTimeSet = true;

		if (remaining_us <= 0)
		{

			clock_gettime(CLOCK_MONOTONIC, sendTime);
			if (isOldSendTimeSet)
			{
				s64 elapsed_us = difftimespec_us(sendTime, oldSendTime);
				double fps = 1000000.0 / elapsed_us;
				//printf("FPS: %.5f\n", fps);
			}
			swap(sendTime, oldSendTime);
			isOldSendTimeSet = true;

			remaining_us += period_us;
			if (remaining_us <= 0)
			{
				remaining_us = period_us;
			}

			hidScanInput();

			// Buttons
			u32 kDown = hidKeysDown(); // For Sending
			u32 kHeld = hidKeysHeld(); // For handling user input

			touchPosition touch;
 
			//Read the touch screen coordinates
			hidTouchRead(&touch);


			// Circle pad and C-stick
			circlePosition pos = {0};
			hidCircleRead(&pos);

			circlePosition cstick = {0};
			if (isnew3ds)
				hidCstickRead(&cstick);

			int offset = 0;

			// Copy into buffer
			COPY_INTO(buffer, htonl(kHeld), &offset);

			s16 stickMasks[] = {pos.dx, pos.dy, cstick.dx, cstick.dy, touch.px, touch.py};
			for (int i = 0; i < 6; i++)
				COPY_INTO(buffer, htons(stickMasks[i]), &offset);

			// whether the buffer has been changed. Ignores the sequence number (-sizeof(u32))
			bool changed = !isOldBufferSet || memcmp(buffer, oldBuffer, bufferLen - sizeof(u32)) != 0;
			// if buffer is different from old buffer or old buffer is not set, send it
			if (changed)
			{
				seqNr++;
				COPY_INTO(buffer, htonl(seqNr), &offset);

				//printf("\nkDown: %lu\nkHeld: %lu\nX: %d, Y: %d\n", kDown, kHeld, pos.dx, pos.dy);

				// send
				struct sockaddr_in myAddr = {
					.sin_family = AF_INET,
					.sin_addr.s_addr = inet_addr(ipAddr),
					.sin_port = htons(port),
				};
				

				int sentBytes = sendto(mysock, buffer, bufferLen, 0, (const struct sockaddr *)&myAddr, sizeof(myAddr));

				/*
				if (sentBytes >= 0)
					printf("Sent %d bytes\n", sentBytes);
				else
					printf("Failed to send. Error: %i\n", errno);
				*/


				isOldBufferSet = true;
				swap(buffer, oldBuffer);
			}
		}
		else
			svcSleepThread(remaining_us * 1000LL);

		/// svcSleepThread(1000000);
	}
}

Thread t = 0;

#define STACKSIZE (4 * 1024)

int main(int argc, char *argv[])
{

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	printf("Hello, world!\n");

	svcCreateMutex(&config.sync, false);

	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	printf("Creating thread\n");

	t = threadCreate(myThreadFunc, 0, STACKSIZE, prio - 1, -2, false);
	if (t != NULL)
	{
		printf("Thread create success\n");
	}
	// allocate buffer for SOC service
	SOC_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_buffer == NULL)
	{
		failExit("memalign: failed to allocate\n");
	}

	int ret = 0;
	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0)
	{
		failExit("socInit: 0x%08X\n", (unsigned int)ret);
	}

	mysock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mysock < 0)
	{
		printf("Failed to create socket\n");
	}

	// initialize sa_data to length of 4

	// Main loop

	while (aptMainLoop())
	{

		// printf("FPS: %.2f\n", fps2);
		hidScanInput();

		if (KEYS_PRESSED(hidKeysHeld(), KEY_START | KEY_L | KEY_R | KEY_DDOWN))
		{
			break; // break in order to return to hbmenu
		}

		gspWaitForVBlank();
		gfxSwapBuffers();
	}

	dispose();

	gfxExit();
	return 0;
}

void dispose(void)
{
	if (mysock > 0)
		closesocket(mysock);
	terminate = true;
	if (t != 0)
	{
		threadJoin(t, U64_MAX);
		threadFree(t);
	}
}

//---------------------------------------------------------------------------------
void failExit(const char *fmt, ...)
{
	//---------------------------------------------------------------------------------

	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	dispose();
	printf(CONSOLE_RESET);
	printf("\nPress B to exit\n");

	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_B)
			exit(0);
	}
}

int64_t difftimespec_us(const struct timespec *after, const struct timespec *before)
{
	return ((int64_t)after->tv_sec - (int64_t)before->tv_sec) * (int64_t)1000000 + ((int64_t)after->tv_nsec - (int64_t)before->tv_nsec) / 1000;
}
