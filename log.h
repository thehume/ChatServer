#pragma once

#define dfLOG_LEVEL_DEBUG 0 // 디버그용 확인사항
#define dfLOG_LEVEL_SYSTEM 1 // 각종 예외처리
#define dfLOG_LEVEL_ERROR 2 //컨텐츠, 라이브러리의 심각한 오류
#define dfMAX_STRING 256

extern INT64 g_logCount;
extern int g_logLevel;
extern CRITICAL_SECTION g_log_CS;

void logInit();
void systemLog(LPCWSTR String, int LogLevel, LPCWSTR StringFormat, ...);