#include <stdio.h>
#include <mach/mach.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
  host_t myhost = mach_host_self();
  mach_port_t psDefault;
  mach_port_t psDefault_control;
  task_array_t tasks;
  mach_msg_type_number_t numTasks;
  int t;
  kern_return_t kr;

  kr = processor_set_default (myhost, &psDefault);

  // control the port
  kr = host_processor_set_priv (myhost, psDefault, &psDefault_control);
  if (kr != KERN_SUCCESS) { fprintf (stderr, "[!] host_processor_set_priv - %d (need root?)", kr); exit (1); }

  kr = processor_set_tasks (psDefault_control, &tasks, &numTasks);
  if (kr != KERN_SUCCESS) { fprintf (stderr, "[!] processor_set_tasks - %d", kr); exit (2); }

  for (t = 0; t < numTasks; t++)
  {
    int pid;
    pid_for_task (tasks[t], &pid);
    printf ("Task: %d pid: %d\n", tasks[t], pid);
  }

  return 0;
}
