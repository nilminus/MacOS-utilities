#include <stdio.h>

void main(void){
	printf("%s\n", __TIME__);
	printf("%s\n", __DATE__);
	printf("%d\n", __LINE__);
	printf("%s\n", __FILE__);
	printf("%s\n", __PRETTY_FUNCTION__);
}
