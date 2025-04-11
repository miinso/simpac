#include <thread>

#include "module/some-shit.h"

int main(int argc, char* argv[]) {
    printf("with std::thread\n");

    for (int i = 0; i < numberOfTasks; i++) {
        int batchID = i + 1; // for each thread
        std::thread t([batchID] { doShit(batchID, true); });
        t.detach(); // fire-and-forget, now main is free
        printf("spawned thread #%d with batchID = %d\n", i, batchID);
    }

    doShit(0, false);

    return 0;
}