#include <sys/socket.h>
#include <sys/kern_event.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main (int argc, char **argv)
{
  struct kev_request req;
  char buf[1024];
  int rc;
  struct kern_event_msg *kev;

  int ss = socket (PF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);
  req.vendor_code = KEV_VENDOR_APPLE;
  req.kev_class = KEV_ANY_CLASS;
  req.kev_subclass = KEV_ANY_SUBCLASS;

  if (ioctl (ss, SIOCSKEVFILT, &req)) {
    perror ("Unable to set filter\n"); exit (1);
  }

  while (1) {
    rc = read (ss, buf, 1024);

    kev = (struct kern_event_msg *) buf;

    printf ("Event %d: (%d bytes). Vendor: %d Class: %d/%d\n",
        kev->id, kev->total_size, kev->vendor_code, kev->kev_class, kev->kev_subclass);
    printf ("Code: %d\n", kev->event_code);
  }


  return 0;
}
