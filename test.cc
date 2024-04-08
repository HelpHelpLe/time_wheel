#include <iostream>
#include <thread>
#include <chrono>

#include "time_wheel.h"


void func(int fd) {
    std::cout << "关闭" << fd << std::endl;
}


int main() {
    gallnut::TimeWheel<3, 5> time_wheel(1);

    gallnut::Timer::ptr t = nullptr;

    for (int i = 0; i < 51; ++i) {
        auto timer = std::make_shared<gallnut::Timer>(i, i, func);
        time_wheel.append_timer(timer);
        if (i == 11) {
            t = timer;
        }
    }

    for (int i = 0; i < 51; ++i) {
        std::cout << "==============" << i << std::endl;
        if (i == 11) {
            t->delay = 11;
            time_wheel.adjust_timer(t);
        }
        time_wheel.tick();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (i == 12) {
            time_wheel.append_timer(std::make_shared<gallnut::Timer>(100, 25, func));
        }
    }
    return 0;
}
