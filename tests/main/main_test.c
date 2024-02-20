#include "os_init.h"

void test_2(void)
{
    struct timeval tv;
    int64_t time_start;
    int64_t time_stop;
    int64_t log_out;

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;

	for(int i = 0; i < 10000; ++i){
		os_log(ERROR, "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee_%d", i);
		//usleep(100);
	}

    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	log_out = time_stop - time_start;
	printf("mlog_out = %ldus\n", log_out);

}


void test_1(void)
{
#define LOG_TEST_NUM 500
    struct timeval tv;
    int64_t time_start;
    int64_t time_stop;

    int64_t log_out, printf_out;

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;

	for(int i = 0; i < LOG_TEST_NUM; ++i){
		os_log(TRACE, "ttttttttttttttttttttttttttttttttttttt%d", i);
		if(0 == i % 10) usleep(10);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	log_out = time_stop - time_start;

	usleep(2000);

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;
	for(int i = 0; i < LOG_TEST_NUM; ++i){
		fprintf(stderr, "[0][os]main_test.c:test_2:39 ERROR:ttttttttttttttttttttttttttttttttttttt%d\n", i);
		if(0 == i % 10) usleep(10);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	printf_out = time_stop - time_start;

	printf("mlog_out = %ldus, printf_out = %ldus\n", log_out/LOG_TEST_NUM , printf_out/LOG_TEST_NUM);

}

int main(void){
	os_cmlog_set_filenum(2);
	os_cmlog_set_log_refresh_ustime(500);//0.5ms
	os_cmlog_set_log_path("/home/test");
	os_core_initialize();

	test_1();
	//test_2();

	atexit(os_core_terminate);
	printf("daemon terminating...\n");
	sleep(10);
	return -1;
}
