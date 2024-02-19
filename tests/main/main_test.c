#include "os_init.h"

int main(){
	os_initialize();

	os_ctlog_set_log_path("/home");

	os_log(ERROR, "sssssssssssssss");

	atexit(os_terminate);
	printf("daemon terminating...");
	return -1;
}
