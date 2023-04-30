#include <switch.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 8192
#define CPU_CLOCK 204E6 // 204 MHz should be a BIG improvement in battery life if the console is used in handheld mode
#define DEFAULT_CLOCK 1020E6

char ipAddress[16];
u8 data[5];
u32 cur_clock;

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int broadcast(int sck, char *host, char *port)
{
  uint8_t msg[] = "xbox_switch";
  char buffer[128] = {0};
  struct hostent *he;
  struct sockaddr_in remote;
  struct sockaddr_storage from;
  socklen_t from_len = sizeof(from);

  if ((he = gethostbyname(host)) == NULL)
  {
    return -1;
  }

  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(port));
  remote.sin_addr = *(struct in_addr *)he->h_addr;
  memset(remote.sin_zero, 0, sizeof(remote.sin_zero));

  int on = 1;
  setsockopt(sck, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000000;
  setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  while (1)
  {
    while (1)
    {
      sendto(sck, msg, sizeof(msg), 0, (struct sockaddr *)&remote, sizeof(remote));
      recvfrom(sck, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &from_len);

      if (strlen(buffer) > 0)
        break;
    }

    char addr[40] = {0};

    const char *ptr = inet_ntop(from.ss_family, get_in_addr((struct sockaddr *)&from), addr, sizeof(addr));

    if (strcmp("xbox", buffer) == 0)
    {
      strcpy(ipAddress, ptr);
      break;
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  u8 currentIpBlock = 0;
  u8 ipBlocks[4] = {192, 168, 1, 255};

  socketInitializeDefault();
  consoleInit(NULL);

  printf("|------------------------|\n");
  printf("| Switch XBOX Controller |\n");
  printf("|------------------------|\n\n");

  printf("Please set up the IP where the UDP broadcast should be sent to!\n");
  printf("Usually this address is 192.168.X.255 where X is the 3. block of your local IP.\n");
  printf("If UDP broadcasting does not work for you, use the IP of your computer instead.\n");
  printf("Use the DPAD to enter the IP address below and press the A button to connect.\n");
  printf("ZL/ZR can be used to decrement/increment the IP by 10.\n\n");

  consoleUpdate(NULL);

  appletSetScreenShotPermission(0); // Disable the screenshot function because it is not needed for the program

  // pad definition
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);

  // touchscreen initialization
  hidInitializeTouchScreen();
  HidTouchScreenState state = {0};

  // Clock control initialization
  ClkrstSession clkrstSession;
  clkrstInitialize();

  // IP selection
  while (1)
  {
    padUpdate(&pad);
    u64 kDown = padGetButtonsDown(&pad);

    if (currentIpBlock == 0)
      printf("\x1b[11;1HCurrent IP Adress: [%d].%d.%d.%d\t\t\n", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
    if (currentIpBlock == 1)
      printf("\x1b[11;1HCurrent IP Adress: %d.[%d].%d.%d\t\t\n", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
    if (currentIpBlock == 2)
      printf("\x1b[11;1HCurrent IP Adress: %d.%d.[%d].%d\t\t\n", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
    if (currentIpBlock == 3)
      printf("\x1b[11;1HCurrent IP Adress: %d.%d.%d.[%d]\t\t\n", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);

    // Handle inputs
    if (kDown & HidNpadButton_Up)
      ipBlocks[currentIpBlock]++;
    if (kDown & HidNpadButton_Down)
      ipBlocks[currentIpBlock]--;

    if (kDown & HidNpadButton_ZR)
      ipBlocks[currentIpBlock] += 10;
    if (kDown & HidNpadButton_ZL)
      ipBlocks[currentIpBlock] -= 10;

    if (kDown & HidNpadButton_Right && currentIpBlock < 3)
      currentIpBlock++;
    if (kDown & HidNpadButton_Left && currentIpBlock > 0)
      currentIpBlock--;

    if (kDown & HidNpadButton_A)
      break;

    consoleUpdate(NULL);

    svcSleepThread(10E6);
  }

  printf("\x1b[11;1HConnecting to: %d.%d.%d.%d\t\t\n", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);

  char computersIp[16];
  sprintf(computersIp, "%d.%d.%d.%d", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);

  struct sockaddr_in si_other;
  int s, slen = sizeof(si_other);

  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    printf("Socket creation failed: %d\n", s);
  }

  broadcast(s, computersIp, "8192");

  printf("Connected to: %s\n\n", ipAddress);

  memset((char *)&si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);

  if (inet_aton(ipAddress, &si_other.sin_addr) == 0)
  {
    fprintf(stderr, "inet_aton() failed\n");
  }

  while (appletMainLoop())
  {
    padUpdate(&pad);
    u64 kHeld = padGetButtons(&pad);
    u64 kDown = padGetButtonsDown(&pad); // used to modify clock

    HidAnalogStickState analog_stick_l = padGetStickPos(&pad, 0);
    HidAnalogStickState analog_stick_r = padGetStickPos(&pad, 1);

    // DPAD Up
    if (kHeld & HidNpadButton_Up)
      sendto(s, "\x1\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x1\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // DPAD Down
    if (kHeld & HidNpadButton_Down)
      sendto(s, "\x2\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x2\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // DPAD Left
    if (kHeld & HidNpadButton_Left)
      sendto(s, "\x3\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x3\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // DPAD Right
    if (kHeld & HidNpadButton_Right)
      sendto(s, "\x4\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x4\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Plus button
    if (kHeld & HidNpadButton_Plus)
      sendto(s, "\x5\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x5\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Minus button
    if (kHeld & HidNpadButton_Minus)
      sendto(s, "\x6\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x6\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Left stick click
    if (kHeld & HidNpadButton_StickL)
      sendto(s, "\x7\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x7\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Right stick click
    if (kHeld & HidNpadButton_StickR)
      sendto(s, "\x8\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x8\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // L Shoulder
    if (kHeld & HidNpadButton_L)
      sendto(s, "\x9\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x9\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // R Shoulder
    if (kHeld & HidNpadButton_R)
      sendto(s, "\xA\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xA\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Touchscreen touch (XBOX Button)
    hidGetTouchScreenStates(&state, 1);
    if (state.count != 0)
      sendto(s, "\xB\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xB\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Right SR or left SL (XBOX Button alt)
    if (kHeld & (HidNpadButton_RightSR | HidNpadButton_LeftSL))
      sendto(s, "\xB\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xB\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // B button
    if (kHeld & HidNpadButton_B)
      sendto(s, "\xC\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xC\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // A button
    if (kHeld & HidNpadButton_A)
      sendto(s, "\xD\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xD\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Y button
    if (kHeld & HidNpadButton_Y)
      sendto(s, "\xE\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xE\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // X Button
    if (kHeld & HidNpadButton_X)
      sendto(s, "\xF\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\xF\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // ZL Trigger
    if (kHeld & HidNpadButton_ZL)
      sendto(s, "\x10\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x10\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // ZR Trigger
    if (kHeld & HidNpadButton_ZR)
      sendto(s, "\x11\x1", 2, 0, (struct sockaddr *)&si_other, slen);
    else
      sendto(s, "\x11\x0", 2, 0, (struct sockaddr *)&si_other, slen);

    // Program quit (Right SL)
    if (kDown & HidNpadButton_RightSL)
      break;

    // Cpu Clock modifier (Left SR)
    if (kDown & HidNpadButton_LeftSR)
    {
      clkrstOpenSession(&clkrstSession, PcvModuleId_CpuBus, 3);
      clkrstGetClockRate(&clkrstSession, &cur_clock);

      if (cur_clock == (int)CPU_CLOCK)
      {
        // Restore CPU clock
        printf("Restoring default CPU clock.\n\n");
        clkrstSetClockRate(&clkrstSession, DEFAULT_CLOCK);
      }

      else
      {
        // Underclock the CPU
        printf("Underclocking the CPU to %.1fMHz\n\n", CPU_CLOCK / 1E6);
        clkrstSetClockRate(&clkrstSession, CPU_CLOCK);
        clkrstGetClockRate(&clkrstSession, &cur_clock);
      }

      clkrstCloseSession(&clkrstSession);
    }

    data[0] = 0x12;
    data[1] = analog_stick_l.x >> 8;
    data[2] = analog_stick_l.x & 0xFF;
    data[3] = analog_stick_l.y >> 8;
    data[4] = analog_stick_l.y & 0xFF;
    sendto(s, data, 5, 0, (struct sockaddr *)&si_other, slen);

    data[0] = 0x13;
    data[1] = analog_stick_r.x >> 8;
    data[2] = analog_stick_r.x & 0xFF;
    data[3] = analog_stick_r.y >> 8;
    data[4] = analog_stick_r.y & 0xFF;
    sendto(s, data, 5, 0, (struct sockaddr *)&si_other, slen);

    consoleUpdate(NULL);
  }

  // Restore clock
  clkrstExit();

  printf("Quitting the program. Goodbye!");
  consoleUpdate(NULL);
  svcSleepThread(500E6); //500ms time to read output

  consoleExit(NULL);
  return 0;
}
