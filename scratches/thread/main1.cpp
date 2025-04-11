#include <iostream>
#include <unistd.h>

void doShit(int someID, bool isAThread)
{
    sleep(2);

    if(isAThread)
    {
        printf("ololo, inside a thread, batch ID: %d\n", someID);
    }
    else
    {
        std::cout << "ololo, inside the task #" << someID << std::endl;
    }

    double result = 0.0;
    for (int i = 0; i < 11111; i++)
    {
        result = result + sin(i) * tan(i);
    }
}