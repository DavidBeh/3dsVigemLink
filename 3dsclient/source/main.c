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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <3ds.h>
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
#define KEYS_PRESSED(held, keys) (((held) & (keys)) == (keys))

// char *ipAddr = "10.149.99.146";
char *ipAddr = "192.168.178.138";
int port = 12346;

void failExit(const char *fmt, ...);
void dispose(void);

static u32 *SOC_buffer = NULL;

int mysock = 0;

int main(int argc, char *argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("Hello, world!\n");

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
	if (mysock >= 0)
	{

		printf("Socket created\n");
		char *payload = "Hello, world!";

		// send to 10.149.99.146
		struct sockaddr_in myAddr = {0};
		myAddr.sin_family = AF_INET;
		myAddr.sin_port = htons(12346);
		myAddr.sin_addr.s_addr = inet_addr(ipAddr);

		// set to non-blocking
		// fcntl(mysock, F_SETFL, fcntl(mysock, F_GETFL, 0) | O_NONBLOCK);

		int sentBytes = sendto(mysock, payload, strlen(payload), 0, (const struct sockaddr *)&myAddr, sizeof(myAddr));
		if (sentBytes >= 0)
		{
			printf("Sent %d bytes\n", sentBytes);
		}
		else
		{
			printf("Failed to send. Error: %i\n", errno);
		}
	}
	else
	{
		printf("Failed to create socket\n");
	}

	// initialize sa_data to length of 4

	// Main loop

	bool isOldBufferSet = false;
	int bufferLen = sizeof(u32) + sizeof(s16) * 4 + sizeof(u32);
	char *buffer = alloca(bufferLen);
	char *oldBuffer = alloca(bufferLen);
	u32 seqNr = 0;

#define swap(a, b)                \
	{                             \
		__typeof__(a) temp = (a); \
		(a) = (b);                \
		(b) = (temp);             \
	}

	bool isnew3ds = 0;
	// check if new 3ds
	APT_CheckNew3DS((bool *)&isnew3ds);

	while (aptMainLoop())
	{

		hidScanInput();

#define COPY_INTO(buffer, value, offsetPtr)               \
	{                                                     \
		__typeof__(value) rd = value;                     \
		memcpy(buffer + *(offsetPtr), &(rd), sizeof(rd)); \
		*offsetPtr += sizeof(rd);                         \
	}

		// Buttons
		u32 kDown = hidKeysDown(); // For Sending
		u32 kHeld = hidKeysHeld(); // For handling user input

#pragma region Sending Keys
		// Circle pad and C-stick
		circlePosition pos = {0};
		hidCircleRead(&pos);

		circlePosition cstick = {0};
		if (isnew3ds)
			hidCstickRead(&cstick);

		int offset = 0;

		// Copy into buffer
		COPY_INTO(buffer, htonl(kHeld), &offset);

		s16 stickMasks[] = {pos.dx, pos.dy, cstick.dx, cstick.dy};
		for (int i = 0; i < 4; i++)
			COPY_INTO(buffer, htons(stickMasks[i]), &offset);

		bool changed = !isOldBufferSet || memcmp(buffer, oldBuffer, bufferLen - sizeof(u32)) != 0;
		// if buffer is different from old buffer or old buffer is not set, send it
		if (changed)
		{
			seqNr++;
			COPY_INTO(buffer, htonl(seqNr), &offset);

			printf("\nkDown: %lu\nkHeld: %lu\nX: %d, Y: %d\n", kDown, kHeld, pos.dx, pos.dy);

			// send
			struct sockaddr_in myAddr = {
				.sin_family = AF_INET,
				.sin_addr.s_addr = inet_addr(ipAddr),
				.sin_port = htons(port),
			};

			int sentBytes = sendto(mysock, buffer, bufferLen, 0, (const struct sockaddr *)&myAddr, sizeof(myAddr));

			if (sentBytes >= 0)
				printf("Sent %d bytes\n", sentBytes);

			else
				printf("Failed to send. Error: %i\n", errno);

			isOldBufferSet = true;
			swap(buffer, oldBuffer);
		}
#pragma endregion Sending Keys

		if (KEYS_PRESSED(kHeld, KEY_START | KEY_L | KEY_R | KEY_DDOWN))
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
}

//---------------------------------------------------------------------------------
void failExit(const char *fmt, ...)
{
	//---------------------------------------------------------------------------------

	dispose();

	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
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
