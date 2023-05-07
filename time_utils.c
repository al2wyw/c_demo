//
// Created by 李扬 on 2023/4/28.
//

#include <sys/time.h>
#include <stdio.h>
#include <time.h>

long long get_timestamp_ms()//获取时间戳函数ms
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void get_format_time_ms(char *str_time) {
    struct tm *tm_t;
    struct timeval time;
    gettimeofday(&time,NULL);
    tm_t = localtime(&time.tv_sec);
    if(NULL != tm_t) {
        sprintf(str_time,"%04d-%02d-%02d %02d:%02d:%02d.%03d",
                tm_t->tm_year+1900,
                tm_t->tm_mon+1,
                tm_t->tm_mday,
                tm_t->tm_hour,
                tm_t->tm_min,
                tm_t->tm_sec,
                time.tv_usec/1000);
    }
}