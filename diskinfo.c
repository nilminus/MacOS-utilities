#include <sys/disk.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFSIZE 1024

int main (int argc, char **argv)
{
  uint64_t bs, bc, rc;
  char fp[BUFSIZE];
  char p[BUFSIZE];

  strncpy (p, argv[1], BUFSIZE);
  if (p[0] != '/') {
    snprintf (p, BUFSIZE-5, "/dev/%s", argv[1]);
  }

  int fd = open (p, O_RDONLY);
  if (fd == -1){
    fprintf (stderr, "%s: unable to open %s\n", argv[0], p);
    perror ("open");
    exit (1);
  }

  rc = ioctl (fd, DKIOCGETBLOCKSIZE, &bs);
  if (rc < 0) {
    fprintf (stderr, "%s: DKIOCGETBLOCKSIZE failed\n", argv[0]); exit (2);
  } else {
    fprintf (stderr, "Block size:\t%llu\n", bs);
  }

  rc = ioctl (fd, DKIOCGETBLOCKCOUNT, &bc);
  fprintf (stderr, "Block count:\t%llu\n", bc);

  rc = ioctl (fd, DKIOCGETFIRMWAREPATH, &fp);
  fprintf (stderr, "Fw Path:\t%s\nTotal size:\t%lluM\n", fp, (bs *bc) / (1024*1024));


  return 0;
}
