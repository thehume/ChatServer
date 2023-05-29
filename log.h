#pragma once

#define dfLOG_LEVEL_DEBUG 0 // ����׿� Ȯ�λ���
#define dfLOG_LEVEL_SYSTEM 1 // ���� ����ó��
#define dfLOG_LEVEL_ERROR 2 //������, ���̺귯���� �ɰ��� ����
#define dfMAX_STRING 256

extern INT64 g_logCount;
extern int g_logLevel;
extern CRITICAL_SECTION g_log_CS;

void logInit();
void systemLog(LPCWSTR String, int LogLevel, LPCWSTR StringFormat, ...);