#!/usr/sbin/dtrace -s
#pragma D option flowindent

mach_trap:::entry
{
  self->tracing = 1;
  printf("file at: 0x%p opened with mode %x", arg0, arg1);
}

fbt:::entry
/self->tracing/
{
  printf("Entry: %x %x %x", arg0, arg1, arg2);
}

fbt::open:entry /self->tracing/ {
  printf ("PID %d (%s) is opening \n" , ((proc_t)arg0)->p_pid , ((proc_t)arg0)->p_comm);
  printf("%x\n", arg1);
  // printf("%s\n", copyinstr(arg1));
}

fbt:::return /self->tracing/ {
  printf ("Returned %x\n", arg1); }

mach_trap:::return 
/self->tracing/
{
  self->tracing = 0; exit(0);
  /* Undo tracing */ /* finish script */
}
