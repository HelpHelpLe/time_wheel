#include <iostream>
#include <unordered_map>
#include <list>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <array>

#include "timer.h"

namespace gallnut {
    
    template <int max_level, int slots_per_wheel>
    class TimeWheel {
    public:
        TimeWheel(int ticks_per_slot);

        bool append_timer(Timer::ptr timer);

        bool remove_timer(Timer::ptr timer);

        bool adjust_timer(Timer::ptr timer);
        
        void tick();
    private:
        void demote_timer(Timer::ptr timer);

        void handle_timer(Timer::ptr timer);
    private:
        int m_ticks_per_slot;
        int m_max_ticks;
        int m_current_ticks;
        std::array<int, max_level> m_current_slots;
        std::array<int, max_level> m_level_base;
        std::array<std::array<Slot, slots_per_wheel>, max_level> m_wheel; // 时间轮
    };

    template <int max_level, int slots_per_wheel>
    TimeWheel<max_level, slots_per_wheel>::TimeWheel(int ticks_per_slot)
        :m_ticks_per_slot(ticks_per_slot), m_max_ticks(1), m_current_ticks(0), m_current_slots({0}) {

        m_level_base[0] = 1;
        for (int i = 1; i < max_level; ++i) {
            m_level_base[i] = m_level_base[i - 1] * slots_per_wheel;
        }

        for (int i = 0; i < max_level; ++i) {
            m_max_ticks += (m_level_base[i] * slots_per_wheel - 1);
        }
    }

    template <int max_level, int slots_per_wheel>
    bool TimeWheel<max_level, slots_per_wheel>::append_timer(Timer::ptr timer) {
        int delay = timer->delay;

        if (delay >= m_max_ticks) {
            return false;
        }
        timer->start_point = m_current_ticks;
        int slots = delay / m_ticks_per_slot;
        int level_num = 0;
        while (slots >= slots_per_wheel) {
            ++level_num;
            slots /= slots_per_wheel;
        }
        if (level_num >= max_level) {
            return false;
        }

        int slot_num = (m_current_slots[level_num] + slots) % slots_per_wheel;
        timer->level_num = level_num;
        timer->slot_num = slot_num;
        m_wheel[level_num][slot_num].append_timer(timer);
        return true;
    }

    template <int max_level, int slots_per_wheel>
    bool TimeWheel<max_level, slots_per_wheel>::remove_timer(Timer::ptr timer) {
        int level_num = timer->level_num;
        int slot_num = timer->slot_num;
        if (level_num < 0 || level_num >= max_level || slot_num < 0 || slot_num >= slots_per_wheel) {
            return false;
        }
        m_wheel[level_num][slot_num].remove_timer(timer);
        return true;
    }

    template <int max_level, int slots_per_wheel>
    bool TimeWheel<max_level, slots_per_wheel>::adjust_timer(Timer::ptr timer) {
        bool del_flag = remove_timer(timer);
        return del_flag && append_timer(timer);
    }

    template <int max_level, int slots_per_wheel>
    void TimeWheel<max_level, slots_per_wheel>::demote_timer(Timer::ptr timer) {
        if (timer == nullptr)
            return;
        int level_num = timer->level_num;
        if (level_num > 0) {
            int low_level = level_num - 1;
            Timer::ptr t;
            while (timer != nullptr) {
                t = timer;
                
                timer = timer->next;
                
                if (timer != nullptr)
                    timer->prev.reset();
                t->next.reset();
                
                int delay = t->delay;
                int duration = m_current_ticks - t->start_point;
                if (duration < 0) {
                    duration += m_max_ticks;
                }                
                delay -= duration;
                t->delay = delay;

                int slots = delay / m_level_base[low_level];
                int slot_num = t->slot_num;
                slot_num = (m_current_slots[low_level] + slots) % slots_per_wheel;
                t->start_point = m_current_ticks;
                t->level_num = low_level;
                t->slot_num = slot_num;
                
                m_wheel[low_level][slot_num].append_timer(t);
            }
        }
    }

    template <int max_level, int slots_per_wheel>
    void TimeWheel<max_level, slots_per_wheel>::handle_timer(Timer::ptr timer) {
        Timer::ptr t;
        while (timer != nullptr) {
            t = timer;
            timer = timer->next;
            t->close_conn(t->socket_fd);
        }
    }

    template <int max_level, int slots_per_wheel>
    void TimeWheel<max_level, slots_per_wheel>::tick() {
        int level_num = 0;
        int timeout_slot = m_current_slots[level_num];
        // 处理过期定时器
        handle_timer(m_wheel[0][timeout_slot].take_all_timer());

        // 总的ticks + 1
        ++m_current_ticks;
        if (m_current_ticks == m_max_ticks) {
            m_current_ticks = 0;
        }

        // 0级指针往下一个时间槽摆动
        int slot_num = (timeout_slot + 1) % slots_per_wheel;
        m_current_slots[0] = slot_num;
        ++level_num;

        while (slot_num == 0 && level_num < max_level) {
            // 当级指针向下一个时间槽摆动
            slot_num = (m_current_slots[level_num] + 1) % slots_per_wheel;
            m_current_slots[level_num] = slot_num;
            ++level_num;
        }

        for (int i = level_num - 1; i > 0; --i) {
            slot_num = m_current_slots[i];
            // 降级当前时间槽的定时器
            demote_timer(m_wheel[i][slot_num].take_all_timer());
        }
    }
}