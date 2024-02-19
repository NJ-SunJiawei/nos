#include "os_init.h"

int main(){
    struct timeval tv;
    int64_t time_start;
    int64_t time_stop;

    int64_t log_out, printf_out;

	os_cmlog_set_log_path("/home/test");
	os_core_initialize();

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;

	for(int i = 0; i < 10000; ++i){
		os_log(TRACE, "ttttttttttttttttttttttt_%d", i);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	log_out = time_stop - time_start;

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;
	for(int i = 0; i < 10000; ++i){
		fprintf(stderr, "ttttttttttttttttttttttt_%d\n", i);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	printf_out = time_stop - time_start;

	printf("mlog_out = %ldus, printf_out = %ldus\n", log_out , printf_out);

	atexit(os_core_terminate);
	printf("daemon terminating...\n");
	sleep(5);
	return -1;
}
