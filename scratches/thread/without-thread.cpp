#include <cstdio>

#include "some-shit.h"

int main() {
    printf("without std::thread\n");

    for (int i = 0; i < numberOfTasks; i++) {
        doShit(i, false);
    }

    return 0;
}