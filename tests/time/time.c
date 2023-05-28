#include <stdio.h>
#include <sys/time.h>

int main() {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    printf("tv.sec: %d\n", (int)tv.tv_sec);
    printf("tz.tz_minuteswest: %d\n", tz.tz_minuteswest);
    printf("tz.tz_dsttime: %d\n", tz.tz_dsttime);
    return 0;
}