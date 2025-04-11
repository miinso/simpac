#ifndef SOMESHIT_H
#define SOMESHIT_H

#if defined(_WIN32)
#    include <windows.h>
inline void cross_sleep(unsigned seconds) {
    Sleep(seconds * 1000);
}
#else
#    include <unistd.h>
inline void cross_sleep(unsigned seconds) {
    sleep(seconds);
}
#endif

const int numberOfTasks = 32;

void doShit(int someID, bool isAThread);

#endif