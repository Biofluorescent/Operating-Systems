#include <stdio.h>
#include <time.h>

int main(){

    time_t timer;
    char timeString[40];
    struct tm* time_info;

    time(&timer);
    time_info = localtime(&timer);
    strftime(timeString, 40, "%I:%M%P, %A, %B %e, %Y", time_info);

    printf("%s\n", timeString);

    return 0;
}
