#pragma once

#include "stdio.h"
#define LVK_PRINTF_IMPL(...)	printf(__VA_ARGS__);printf("\n")
#define LVK_LOG_INFO(...)		printf("INFO: ");	LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_ERR(...)		printf("ERROR: ");	LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_WARN(...)		printf("WARN: ");	LVK_PRINTF_IMPL(__VA_ARGS__)