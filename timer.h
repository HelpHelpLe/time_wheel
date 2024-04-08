#include <functional>
#include <memory>



namespace gallnut {
    
    struct Timer {
        using ptr = std::shared_ptr<Timer>;
        int socket_fd;
        int delay;
        std::function<void(int)> close_conn;
        int level_num;
        int slot_num;
        int start_point;
        Timer::ptr next;
        Timer::ptr prev;

        Timer(int fd, int d, std::function<void(int)> func)
            :socket_fd(fd)
            ,delay(d)
            ,close_conn(func)
            ,level_num(0)
            ,slot_num(0)
            ,start_point(0)
            ,next(nullptr)
            ,prev(nullptr) {

        }
    };

    class Slot {
    public:
        Slot(): slot_head(nullptr) {}

        void append_timer(Timer::ptr timer);

        void remove_timer(Timer::ptr timer);

        Timer::ptr take_one_timer();

        Timer::ptr take_all_timer();

        bool empty();
    private:
        Timer::ptr slot_head;
    };

    void Slot::append_timer(Timer::ptr timer) {
        if (slot_head == nullptr) {
            slot_head = timer;
            slot_head->next = nullptr;
            slot_head->prev = nullptr;
        } else {
            timer->next = slot_head;
            slot_head->prev = timer;
            slot_head = timer;
        }
    }

    void Slot::remove_timer(Timer::ptr timer) {
        if (timer == nullptr)
            return;
        if (timer == slot_head) {
            slot_head = slot_head->next;
            if (slot_head != nullptr)
                slot_head->prev = nullptr;
            timer->next = nullptr;
            timer->prev = nullptr;
            return;
        }
        if (timer->next != nullptr)
            timer->next = timer->prev;
        if (timer->prev != nullptr)
            timer->prev = timer->next;
    }

    Timer::ptr Slot::take_one_timer() {
        if (slot_head == nullptr)
            return nullptr;
        Timer::ptr timer = slot_head;
        slot_head = slot_head->next;
        slot_head->prev = nullptr;
        return timer;
    }

    Timer::ptr Slot::take_all_timer() {
        Timer::ptr timer = slot_head;
        slot_head = nullptr;
        return timer;
    }

    bool Slot::empty() {
        return slot_head == nullptr;
    }
}