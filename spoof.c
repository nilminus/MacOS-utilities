#include <sys/socket.h>
#include <net/if.h>
#include <net/ndrv.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char** argv)
{
  int s, rc;
  struct sockaddr_ndrv ndrv;
  u_int8_t packet[1500];

  if (geteuid() != 0) {
    fprintf (stderr, "You are wasting my fcking time, little man. Come back as root\n");
    exit (1);
  }

  s = socket (PF_NDRV, SOCK_RAW, 0);
  if (s < 0) { perror ("socket"); exit (2); }

  strlcpy ((char *)ndrv.snd_name, "en0", sizeof(ndrv.snd_name));
  ndrv.snd_family = AF_NDRV;
  ndrv.snd_len = sizeof(ndrv);
  

  rc = bind (s, (struct sockaddr*)&ndrv, sizeof(ndrv));
  if (rc < 0) { perror ("bind"); exit (3); }

  memset (&packet, 0, sizeof(packet));

  packet[0] = 0xFF;
  packet[6] = 0xFF;
  packet[12] = 0; 
  packet[13] = 0;

  strcpy ((char *)&packet[14], "You can put whatever you want here...\0");
  rc = sendto (s, &packet, 1500, 0, (struct sockaddr*) &ndrv, sizeof(ndrv));


  return 0;
}
