/************************************************************************
 *File name: os_clog_f.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#include "os_init.h"
#include "private/os_clog_priv.h"

Data  *g_l2rlogBuf = NULL;
Data  *g_l2LogBufStartPtr = NULL;
Data  *g_l2LogBufBasePtr = NULL;
Data  *g_logBufRcvdFromL2 = NULL;
Data  *g_l2LogBaseBuff = NULL;
unsigned int    g_logBufLenRcvdFromL2 = 0;
unsigned int    g_l2LogBufLen = 0;
unsigned int    startL2Logging = 0;
unsigned int    g_l2logBuffPos = 0;

