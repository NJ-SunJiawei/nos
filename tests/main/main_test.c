#include "os_init.h"

void test_1(void)
{
#define LOG_TEST_NUM 10000
    struct timeval tv;
    int64_t time_start;
    int64_t time_stop;

    int64_t log_out, printf_out;

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;

	for(int i = 0; i < LOG_TEST_NUM; ++i){
		os_log(TRACE, "ttttttttttttttttttttttttttttttttttttt%d", i);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	log_out = time_stop - time_start;

	usleep(2000);

    gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000000LL + tv.tv_usec;
	for(int i = 0; i < LOG_TEST_NUM; ++i){
		fprintf(stderr, "[0][os]main_test.c:test_2:39 ERROR:ttttttttttttttttttttttttttttttttttttt%d\n", i);
	}
    gettimeofday(&tv, NULL);
	time_stop = tv.tv_sec*1000000LL + tv.tv_usec;
	printf_out = time_stop - time_start;

	printf("mlog_out = %ldus, printf_out = %ldus\n", (log_out)/LOG_TEST_NUM , (printf_out)/LOG_TEST_NUM);

}

void test_2(void)
{
	char *buf = NULL;
	os_assert(buf);
}

void test_3(void)
{
	char *buf = NULL;
	memcpy(buf, 0x00, sizeof(void*));
}

void term(void)
{
    os_buf_default_destroy();
	os_core_terminate();
}

int main(void){
    os_buf_config_t config;

	os_cmlog_set_filenum(2);
	os_cmlog_set_log_path("/home/test");
	os_core_initialize();
	os_core_callback(term);

    os_buf_default_init(&config);
    os_buf_default_create(&config);

	test_1();
	//test_2();
	test_3();

	printf("daemon running...\n");
	while(1);
	return -1;
}
