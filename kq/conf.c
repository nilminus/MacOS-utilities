#include <SystemConfiguration/SCPreferences.h>
#define kNumKids 2
#define kNumBytesInPic 10

void dumpDict(CFDictionaryRef dict){
  CFErrorRef myError;
  CFDataRef xml = CFPropertyListCreateData(kCFAllocatorDefault, dict, kCFPropertyListXMLFormat_v1_0, 0, &myError);

  if (xml) {
    write(1, CFDataGetBytePtr(xml), CFDataGetLength(xml)); 
    CFRelease(xml);
  } 
}

int main (int argc, char **argv) {
  CFStringRef myName = CFSTR("com.technologeeks.SystemConfigurationTest");
  CFArrayRef keyList;
  SCPreferencesRef prefs = NULL;
  char *val;
  CFIndex i; 
  CFDictionaryRef global;
  // Open a preferences session
  prefs = SCPreferencesCreate (NULL, myName, NULL);
  if (!prefs) { fprintf (stderr,"SCPreferencesCreate"); exit(1); }
  // retrieve preference namespaces
  keyList = SCPreferencesCopyKeyList (prefs);
  if (!keyList) { fprintf (stderr,"CopyKeyList failed\n"); exit(2);}
  // dump 'em
  for (i = 0; i < CFArrayGetCount(keyList); i++) { 
    dumpDict(SCPreferencesGetValue(prefs, CFArrayGetValueAtIndex(keyList, i)));
  }
}

