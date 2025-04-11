#include <cmath>
#include <cstdio>

#include "some-shit.h"

void doShit(int someID, bool isAThread) {
    cross_sleep(2);

    if (isAThread) {
        printf("ololo, inside a thread, batch ID: %d\n", someID);
    } else {
        printf("ololo, inside the task #%d\n", someID);
    }

    double result = 0.0f;
    for (int i = 0; i < 11111; ++i) {
        result = result + sin(i) * tan(i);
    }
}