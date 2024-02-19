#include "os_init.h"

int main(){
	os_cmlog_set_log_path("/home/test");
	os_core_initialize();

	for(int i = 0; i < 10; ++i){
		os_log(TRACE, "ttttttttttttttttttttttt_%d", i);
	}

	atexit(os_core_terminate);
	printf("daemon terminating...");
	return -1;
}
