#include <stdio.h>
#include <mach/mach.h>
#include <stdlib.h>
#include <mach-o/arch.h>

int main (void)
{
  kern_return_t kr;
  host_name_port_t host = mach_host_self();
  host_priv_t host_priv;
  processor_port_array_t processors;
  natural_t count, infoCount;
  processor_basic_info_data_t basicInfo;
  processor_cpu_load_info_data_t cpuInfo;
  int p;

  kr = host_get_host_priv_port (host, &host_priv);
  if (kr != KERN_SUCCESS) {
    fprintf (stderr, "host_get_host_priv_port %d (you should be root!)\n", kr);
    exit (1);
  }

  kr = host_processors (host_priv, &processors, &count);
  if (kr != KERN_SUCCESS) {
    fprintf (stderr, "host_processors %d", kr);
    exit (1);
  }

  // Got it
  for (p = 0; p < count; p++)
  {
    infoCount = PROCESSOR_BASIC_INFO_COUNT;
    
    kr = processor_info (processors[p],
                        PROCESSOR_BASIC_INFO,
                        &host,
                        (processor_info_t)&basicInfo,
                        &infoCount);
  

  if (kr != KERN_SUCCESS) { fprintf (stderr, "?!\n"); exit (3); }

  // Dump to our screen. Use NX APIs to resolve cpu type and subtype
  printf ("%s processor %s in slot %d: %s\n",
      (basicInfo.is_master ? "Master" : "Slave"),
       NXGetArchInfoFromCpuType(basicInfo.cpu_type, 
                                basicInfo.cpu_subtype)->description,
      basicInfo.slot_num,
      (basicInfo.running ? "Running" : "Not running")
    );

    kr = processor_info (processors[p],
                        PROCESSOR_CPU_LOAD_INFO,
                        &host,
                        (processor_info_t)&cpuInfo,
                        &infoCount);

    if (kr != KERN_SUCCESS) { fprintf (stderr, "?!\n"); exit (3); }
    printf ("\tTicks SYSTEM: %d\n \tTicks USER: %d\n\tTicks IDLE: %d\n\tTicks NICE: %d\n",
            cpuInfo.cpu_ticks[CPU_STATE_SYSTEM],
            cpuInfo.cpu_ticks[CPU_STATE_USER],
            cpuInfo.cpu_ticks[CPU_STATE_IDLE],
            cpuInfo.cpu_ticks[CPU_STATE_NICE]);

  }
}
