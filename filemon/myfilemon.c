#include <sys/types.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include "fsevents.h"

#define COLOR_OP YELLOW
#define COLOR_PROC BLUE
#define COLOR_PATH CYAN
#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

#define COLOR_OPTION	"-c"
#define COLOR_LONG_OPTION	"--color"

#define FILTER_ALL_OPTION	"-a"
#define FILTER_PROC_OPTION	"-p"
#define FILTER_PROC_LONG_OPTION	"--proc"
#define FILTER_FILE_OPTION	"-f"
#define FILTER_FILE_LONG_OPTION	"--file"
#define FILTER_EVENT_OPTION	"-e"
#define FILTER_EVENT_LONG_OPTION "--event"
#define STOP_OPTION	"-s"
#define STOP_LONG_OPTION	"--stop"
#define LINK_OPTION	"-l"
#define LINK_LONG_OPTION	"--link"

#define MAX_FILTERS 10
#define BUFSIZE 1024 * 1024

#pragma pack(1)  // to be on the safe side. Not really necessary.. struct fields are aligned.
typedef struct kfs_event_a {
  uint16_t type;
  uint16_t refcount;
  pid_t    pid;
} kfs_event_a;

typedef struct kfs_event_arg {
  uint16_t type;
  uint16_t pathlen;
  char data[0];
} kfs_event_arg;

int g_dumpArgs = 0;

  void
usage()
{
  fprintf(stderr, "Usage: %s [options]\n", getprogname());
  fprintf(stderr,"\t" FILTER_ALL_OPTION  ":  Show all events\n");
  fprintf(stderr,"\t" FILTER_PROC_OPTION  "|" FILTER_PROC_LONG_OPTION  "  pid/procname:  filter only this process or PID (process Name for now...)\n");
  fprintf(stderr,"\t" FILTER_FILE_OPTION  "|" FILTER_FILE_LONG_OPTION  "  string[,string]:        filter only paths containing this string (/ will catch everything)\n");
  fprintf(stderr,"\t" FILTER_EVENT_OPTION "|" FILTER_EVENT_LONG_OPTION " event[,event]: filter only these events\n");
  fprintf(stderr,"\t" STOP_OPTION "|" STOP_LONG_OPTION         ":                auto-stop the process generating event\n");
  fprintf(stderr,"\t" LINK_OPTION "|" LINK_LONG_OPTION         ":                auto-create a hard link to file (prevents deletion by program :-)\n");
  fprintf(stderr,"\t" COLOR_OPTION "|" COLOR_LONG_OPTION " (or set JCOLOR=1 first)\n");

  fprintf(stderr,"\tThis is Nilminus updated filemon, compiled on " __DATE__ "\n");
}

int lastPID = 0;

  static char* 
getProcName(long pid)
{
  static char procName[1000];
  size_t len = 1000;
  int rc;
  int mib[4];

  if (pid != lastPID)
  {
    memset(procName, '\0', sizeof(procName));

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = pid;

    if ((rc = sysctl(mib, 4, procName, &len, NULL, 0)) < 0)
    {
      perror("trace facility failure, KERN_PROC_PID\n");
      exit(1);
    }
    lastPID = pid;
  }

  // printf ("\nGOT PID: %ld and rc: %d -  %s", pid, rc, ((struct kinfo_proc *)procName)->kp_proc.p_comm);
  return (((struct kinfo_proc*)procName)->kp_proc.p_comm);
}

// Pass process name
  int
interesting_process (int pid, char *Filters[], int NumFilters)
{
  // filter myself out
  if (pid == getpid()) return 0;
  // Otherwise, if no user defined filters, get all
  // TODO
  if (!NumFilters) return 1;

  while (NumFilters > 0)
  {
    char *pidName = getProcName(pid);
    // fprintf (stderr, "Checking %s vs %s\n", pidName, Filters[NumFilters-1]);
    if (strstr(pidName, Filters[NumFilters-1])) return 1;
    NumFilters--;
  }
  return 0;
}

  int
interesting_file (char *FileName, char *Filters[], int NumFilters)
{

  // if no filters - everything is interesting
  if (!NumFilters) return 1;

  while (NumFilters > 0)
  {
    // fprintf(stderr,"Checking %s vs %s\n", FileName, Filters[NumFilters-1]);
    if (strstr(FileName, Filters[NumFilters-1])) return 1;
    NumFilters--;
  }

  return 0;
}

  static char*
typeToString (uint32_t Type)
{
  switch (Type)
  {
    case FSE_CREATE_FILE: return      ("Created      ");
    case FSE_DELETE: return           ("Deleted       ");
    case FSE_STAT_CHANGED: return     ("Changed stat  ");
    case FSE_RENAME:       return     ("Renamed       ");
    case FSE_CONTENT_MODIFIED: return ("Modified      ");
    case FSE_CREATE_DIR:	return    ("Created dir   ");
    case FSE_CHOWN:	        return    ("Chowned       ");

    case FSE_EXCHANGE: return            ("Exchanged     "); /* 5 */
    case FSE_FINDER_INFO_CHANGED: return ("Finder Info   "); /* 6 */
    case FSE_XATTR_MODIFIED: return      ("Changed xattr "); /* 9 */
    case FSE_XATTR_REMOVED: return       ("Removed xattr "); /* 10 */

    case FSE_DOCID_CREATED: return ("DocID created "); // 11
    case FSE_DOCID_CHANGED: return ("DocID changed "); // 11

    default : return ("?! ");
  }
}

  int
doArg(char *arg, int Print)
{
  // Dump an arg value
  unsigned short *argType = (unsigned short *) arg;
  unsigned short *argLen   = (unsigned short *) (arg + 2);
  uint32_t	*argVal = (uint32_t *) (arg+4);
  uint64_t	*argVal64 = (uint64_t *) (arg+4);
  dev_t		*dev;
  char		*str;



  switch (*argType)
  {

    case FSE_ARG_INT64: // This is a timestamp field on the FSEvent
      if (g_dumpArgs) printf ("Arg64: %lld ", *argVal64);
      break;

    case FSE_ARG_STRING:
      str = (char *)argVal;
      if (Print) printf("%s ", str);
      break;

    case FSE_ARG_DEV:
      dev = (dev_t *) argVal;
      if (g_dumpArgs) printf ("DEV: %d,%d ", major(*dev), minor(*dev)); break;

    case FSE_ARG_MODE:
      if (g_dumpArgs) printf("MODE: %x ", *argVal); break;
    case FSE_ARG_PATH:
      printf ("PATH: %s", (char *)argVal ); break;
    case FSE_ARG_INO:
      if (g_dumpArgs) printf ("INODE: %d ", *argVal); break;
    case FSE_ARG_UID:
      if (g_dumpArgs) printf ("UID: %d ", *argVal); break;
    case FSE_ARG_GID:
      if (g_dumpArgs) printf ("GID: %d ", *argVal); break;
#define FSE_ARG_FINFO    0x000c   // next arg is a packed finfo (dev, ino, mode, uid, gid)
    case FSE_ARG_FINFO:
      printf ("FINFO\n"); break;
    case FSE_ARG_DONE:	
      if (Print)printf("\n");return 2;

    default:
      printf ("(ARG of type %hd, len %hd)\n", *argType, *argLen);
      exit(0);
  }

  return (4 + *argLen);

}

  int
main (int argc, char **argv)
{
  int fsed, cloned_fsed;
  int i;
  int rc;
  fsevent_clone_args clone_args;
  char *buf = malloc(BUFSIZE);
  unsigned short *arg_type;
  int autostop = 0;
  int autolink = 0;
  char *fileFilters[MAX_FILTERS] = {0};
  char *procFilters[MAX_FILTERS] = {0};
  int8_t events[FSE_MAX_EVENTS];

  int numFileFilters = 0;
  int numProcFilters = 0;
  int color = 1;

  int arg = 1;

  if (argc < 2) { usage(); exit(1); }

  for (arg = 1; arg < argc; arg++)
  {
    if (strcmp(argv[arg], "-h") == 0) { usage(); exit(1); }
    if (strcmp(argv[arg], "-a") == 0) { continue; }

    if ((strcmp(argv[arg], FILTER_PROC_OPTION) == 0) ||
        (strcmp(argv[arg], FILTER_PROC_LONG_OPTION) == 0))
    {
      if (arg == argc -1)
      {
        fprintf(stderr, "%s: Option requires an argument\n",
            argv[arg]);
        exit(2);
      }
      // Got it - add filters, separate by ","

      char *begin = argv[arg+1];
      char *sep = strchr (begin, ',');
      while (sep)
      {
        *sep = '\0';
        fprintf(stderr,"Adding Process filter %d: %s\n", numProcFilters, begin);
        procFilters[numProcFilters++] = strdup(begin);
        begin = sep + 1;
        sep = strchr (begin, ',');

      }
      fprintf(stderr,"Adding Process filter %d: %s\n", numProcFilters, begin);
      procFilters[numProcFilters++] = strdup(begin);
      arg++; continue;
    }


    if ((strcmp(argv[arg], FILTER_EVENT_OPTION) == 0) ||
        (strcmp(argv[arg], FILTER_EVENT_LONG_OPTION) == 0))
    {
      if (arg == argc -1)
      {
        fprintf(stderr, "%s: Option requires an argument\n",
            argv[arg]);
        exit(2);
      }
      
      // TOADD event filters
      arg++; continue;
    }

    if ((strcmp(argv[arg], FILTER_FILE_OPTION) == 0) ||
        (strcmp(argv[arg], FILTER_FILE_LONG_OPTION) == 0))
    {
      if (arg == argc -1)
      {
        fprintf(stderr, "%s: Option requires an argument\n",
            argv[arg]);
        exit(2);
      }

      // Got it - add filters, separate by ","

      char *begin = argv[arg+1];
      char *sep = strchr (begin, ',');
      while (sep)
      {
        *sep = '\0';
        fprintf(stderr,"Adding File filter %d: %s\n", numFileFilters, begin);
        fileFilters[numFileFilters++] = strdup(begin);
        begin = sep + 1;
        sep = strchr (begin, ',');

      }
      fprintf(stderr,"Adding File filter %d: %s\n", numFileFilters, begin);
      fileFilters[numFileFilters++] = strdup(begin);
      arg++; continue;
    }


    if ((strcmp(argv[arg], COLOR_OPTION) == 0) || (strcmp(argv[arg], COLOR_LONG_OPTION) == 0))
    {

      color++;
      continue;
    }
    if ((strcmp(argv[arg], STOP_OPTION) == 0) || (strcmp(argv[arg], STOP_LONG_OPTION) == 0))
    {
      autostop++;
      continue;
    }

    if ((strcmp(argv[arg], LINK_OPTION) == 0) || (strcmp(argv[arg], LINK_LONG_OPTION) == 0))
    {
      autolink++;
      continue;
    }

    fprintf(stderr, "%s: Unknown option\n", argv[arg]); exit(3);
  }

  if (autostop && (!numFileFilters && !numProcFilters))
  {
    fprintf(stderr, "Error: Cannot allow auto-stopping of processes without either a file or process filter.\nIf you are sure you want to do this, set a null filter\n"); 
    exit(4);
  }

  if (geteuid()){
    fprintf(stderr, "Opening /dev/fsevents requires root permissions\n");
    exit(3);
  } else {
    fsed = open ("/dev/fsevents", O_RDONLY);
  }

  if (fsed < 0)
  {
    perror ("open");
    exit(1);
  }

  for (i = 0; i < FSE_MAX_EVENTS; i++)
  {
    events[i] = FSE_REPORT;
  }

  memset(&clone_args, '\0', sizeof(clone_args));
  clone_args.fd = &cloned_fsed;
  clone_args.event_queue_depth = 100;
  clone_args.event_list = events;
  clone_args.num_events = FSE_MAX_EVENTS;

  rc = ioctl (fsed, FSEVENTS_CLONE, &clone_args);
  if (rc < 0) { perror ("ioctl"); exit(2);}

  while ((rc = read (cloned_fsed, buf, BUFSIZE)) > 0)
  {
    int offInBuf = 0;

    while (offInBuf < rc)
    {
      struct kfs_event_a *fse = (struct kfs_event_a *)(buf + offInBuf);
      struct kfs_event_arg *fse_arg;

      if (fse->type == FSE_EVENTS_DROPPED)
      {
        printf("Some events dropped\n");
        break;
      }

      if (!fse->pid) { printf("%x %x\n", fse->type, fse->refcount); }

      int print = 0;
      char *procName = getProcName(fse->pid);
      offInBuf += sizeof(struct kfs_event_a);
      fse_arg = (struct kfs_event_arg *) &buf[offInBuf];

      if (interesting_process(fse->pid, procFilters, numProcFilters)
          && interesting_file(fse_arg->data, fileFilters, numFileFilters))
      {
        printf ("%5d %s%s\t%s%s%s ", fse->pid,
            color ? COLOR_PROC: "" , procName,  
            color ? COLOR_OP : "", typeToString(fse->type), color ? NORMAL : "" );

        // The path name is null terminated, so that's cool
        printf ("%s%s%s\t",
            color ? COLOR_PATH : "" , fse_arg->data, color ? NORMAL :"");

        // Fix: Don't autolink own files
        if (fse->type == FSE_CREATE_FILE && autolink && (fse->pid != getpid()) &&
            !strstr(fse_arg->data, "filemon"))
        {
          int fileLen = strlen(fse_arg->data);

          char *linkName = malloc (fileLen + 20);
#ifndef ARM
          strcpy(linkName, fse_arg->data);
#else
          if (strncmp(fse_arg->data, "/private/var",12) == 0)
          {
            // Might be in some subdir which will go away
            strcpy(linkName, filemonDir);
            strcat(linkName,"/");
            strcat (linkName, basename(fse_arg->data));

          }
#endif
          snprintf(linkName + strlen(linkName), fileLen + 20, ".filemon.%d", autolink);
          int rc = link (fse_arg->data, linkName);
          if (rc) { fprintf(stderr,"%sWarning: Unable to autolink %s%s - file must have been deleted already\n",
              color ? RED : "",
              fse_arg->data,
              color ? NORMAL : "");}

          else    { fprintf(stderr,"%sAuto-linked %s to %s%s\n",
              color ? GREEN : "",
              fse_arg->data, linkName,
              color ? NORMAL :"");
          autolink++;
          }

          free (linkName);
        }

        // Autostop only if this is a file creation, and interesting
        if (autostop && fse->type == FSE_CREATE_FILE ) {
          fprintf(stderr, "%sAuto-stopping process %s (%d) on file operation%s\n",
              color ? RED : "",
              procName,
              fse->pid,
              color ? NORMAL : "");
          kill (fse->pid, SIGSTOP);
        }

        print = 1;


      }

      offInBuf += sizeof(kfs_event_arg) + fse_arg->pathlen ;

      int arg_len = doArg(buf + offInBuf,print);
      offInBuf += arg_len;
      while (arg_len >2 && offInBuf < rc)
      {
        arg_len = doArg(buf + offInBuf, print);
        offInBuf += arg_len;
      }

    }
    memset(buf, '\0', BUFSIZE);
    if (rc > offInBuf) { printf ("*** Warning: Some events might have been lost\n"); }
  }
}



