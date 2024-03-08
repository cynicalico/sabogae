#ifndef SABOGAE_HERMES_V0_H
#define SABOGAE_HERMES_V0_H

#include "prio_list_v0.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <ranges>
#include <vector>

#include <iostream>
#include <concepts>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "fmt/format.h"
#include "sabogae/ds/prio_list.h"
//#include "hermes_v0.h"
#include "sabogae/util/time.h"

namespace gae {

inline std::size_t type_id_seq = 0;

template<typename T>
inline const std::size_t type_id_impl = type_id_seq++;

template<typename T>
constexpr std::size_t type_id() {
    return type_id_impl<std::remove_cvref_t<T>>;
}

struct Payload {
};

template<typename T>
concept IsPayload = std::derived_from<T, Payload>;

template<typename T>
class ReceiverImpl;

class Receiver {
public:
    template<typename T>
    void call(const T &pay) {
        static_cast<ReceiverImpl<T> *>(this)->impl(pay);
    }
};

template<typename T>
class ReceiverImpl : public Receiver {
    using Func_t = std::function<void(const T &)>;

public:
    Func_t func;

    explicit ReceiverImpl(Func_t &&func) : func(std::forward<Func_t>(func)) {}

    void impl(const T &pay) { func(pay); }
};

class Hermes {
public:
    template<typename T, typename Func>
    requires IsPayload<T>
    void sub(const std::string &name, std::vector<std::string> &&deps, Func &&func) {
        const std::lock_guard lock(receiver_mutex_);

        auto e = type_id<T>();

        while (receivers_.size() <= e)
            receivers_.emplace_back();

        receivers_[e].add(
                name,
                std::forward<std::vector<std::string>>(deps),
                std::make_unique<ReceiverImpl<T>>(std::forward<Func>(func)));

        check_create_buffer_<T>(name);
    }

    template<typename T, typename Func>
    requires IsPayload<T>
    void sub(const std::string &name, Func &&func) {
        sub<T>(name, {}, std::forward<Func>(func));
    }

    template<typename T, typename... Args>
    requires IsPayload<T>
    void send_nowait(Args &&...args) {
        const std::lock_guard lock(receiver_mutex_);

        auto e = type_id<T>();

        if (receivers_.size() <= e)
            return;

        T payload = T{{}, std::forward<Args>(args)...};
        for (const auto &r: receivers_[e])
            r->template call<T>(payload);
    }

    template<typename T, typename... Args>
    requires IsPayload<T>
    void send(Args &&...args) {
        const std::lock_guard lock(buffer_mutex_);

        auto e = type_id<T>();

        if (buffers_.size() <= e)
            return;

        for (auto &p: buffers_[e])
            p.second.emplace_back(new T{{}, args...});
    }

    template<typename T>
    requires IsPayload<T>
    void poll(const std::string &name) {
        const std::lock_guard lock(receiver_mutex_);
        const std::lock_guard lock2(buffer_mutex_);

        auto e = type_id<T>();

        auto it = buffers_[e].find(name);
        if (it == buffers_[e].end())
            return;

        for (auto &p: it->second) {
            T &pay = *static_cast<T *>(p);
            receivers_[e][name]->template call<T>(pay);
            delete p;
        }
        it->second.clear();
    }

private:
    std::vector<gae::PrioList<std::unique_ptr<Receiver>>> receivers_{};
    std::vector<std::unordered_map<std::string, std::vector<Payload *>>> buffers_{};

    inline static std::recursive_mutex receiver_mutex_;
    inline static std::recursive_mutex buffer_mutex_;

    template<typename T>
    void check_create_buffer_(const std::string &name) {
        auto e = type_id<T>();

        while (buffers_.size() <= e)
            buffers_.emplace_back();

        auto it = buffers_[e].find(name);
        if (it == buffers_[e].end())
            buffers_[e].emplace_hint(it, name, std::vector<Payload *>{});
    }
};

} // namespace gae

struct Foo : public gae::Payload {
    int a;
    int b;
};

struct Bar : public gae::Payload {
    float a;
    float b;
    std::string c;
};

struct Foo2 {
    int a;
    int b;
};

struct Bar2 {
    float a;
    float b;
    std::string c;
};

namespace imp {
template<typename T>
struct EPI;
} // namespace imp

// Specialize EPI with the name of the module for use in the event system
// This is intended to be used outside of a namespace so the namespace becomes part
// of the name (just in case it is what qualifies it from another module)
#define IMP_PRAISE_HERMES(module)         \
  template<> struct imp::EPI<module> {    \
    static constexpr auto name = #module; \
  }

namespace imp {
class Hermes {
public:
    template<typename T>
    using Receiver = std::function<void(const T &)>;

    template<typename T>
    static void presub_cache(const std::string &name);

    template<typename T>
    static void sub(const std::string &name, std::vector<std::string> &&deps, Receiver<T> &&recv);

    template<typename T>
    static void sub(const std::string &name, Receiver<T> &&recv);

    template<typename T, typename... Args>
    static void send(Args &&... args);

    template<typename T, typename... Args>
    static void send_nowait(Args &&... args);

    // Call the receivers in reverse-dependency order
    // Useful if you need things to shut down correctly
    template<typename T, typename... Args>
    static void send_nowait_rev(Args &&... args);

    template<typename T>
    static void poll(const std::string &name);

    template<typename T>
    static std::vector<std::string> get_prio();

    template<typename T>
    static bool has_pending();

    template<typename T>
    static std::vector<PendingItemInfo> get_pending();

private:
    template<typename T>
    inline static PrioList<Receiver<T>> receivers_{};

    template<typename T>
    inline static std::unordered_map<std::string, std::vector<T>> buffers_{};

    inline static std::recursive_mutex receiver_mutex_;
    inline static std::recursive_mutex buffer_mutex_;

    template<typename T>
    static void check_create_buffer_(const std::string &name);
};
} // namespace imp

namespace imp {
template<typename T>
void Hermes::presub_cache(const std::string &name) {
    check_create_buffer_<T>(name);
}

template<typename T>
void Hermes::sub(const std::string &name, std::vector<std::string> &&deps, Receiver<T> &&recv) {
    const std::lock_guard lock2(receiver_mutex_);

    auto &receivers = receivers_<T>;
    receivers.add(name, std::forward<std::vector<std::string>>(deps), std::forward<Receiver<T>>(recv));
    check_create_buffer_<T>(name);
}

template<typename T>
void Hermes::sub(const std::string &name, Receiver<T> &&recv) {
    sub<T>(name, {}, std::forward<Receiver<T>>(recv));
}

template<typename T, typename... Args>
void Hermes::send(Args &&... args) {
    const std::lock_guard lock1(buffer_mutex_);

    auto pay = T{std::forward<Args>(args)...};
    for (auto &p: buffers_<T>) {
        p.second.emplace_back(pay);
    }
}

template<typename T, typename... Args>
void Hermes::send_nowait(Args &&... args) {
    const std::lock_guard lock2(receiver_mutex_);

    auto pay = T{std::forward<Args>(args)...};
    for (const auto &r: receivers_<T>) {
        r(pay);
    }
}

template<typename T, typename... Args>
void Hermes::send_nowait_rev(Args &&... args) {
    const std::lock_guard lock2(receiver_mutex_);

    auto pay = T{std::forward<Args>(args)...};
    for (const auto &r: receivers_<T> | std::views::reverse) {
        r(pay);
    }
}

template<typename T>
void Hermes::poll(const std::string &name) {
    const std::lock_guard lock1(buffer_mutex_);
    const std::lock_guard lock2(receiver_mutex_);

    auto &b = buffers_<T>;
    if (auto it = b.find(name); it != b.end()) {
        auto &r = receivers_<T>[name];
        for (const auto &p: it->second) {
            r(p);
        }
        it->second.clear();
    }
}

template<typename T>
std::vector<std::string> Hermes::get_prio() {
    const std::lock_guard lock(receiver_mutex_);

    auto ret = std::vector<std::string>{};
    for (const auto &p: receivers_<T>)
        ret.emplace_back(receivers_<T>.name_from_id(p.id));
    return ret;
}

template<typename T>
bool Hermes::has_pending() {
    const std::lock_guard lock(receiver_mutex_);
    return receivers_<T>.has_pending();
}

template<typename T>
std::vector<PendingItemInfo> Hermes::get_pending() {
    const std::lock_guard lock(receiver_mutex_);
    return receivers_<T>.get_pending();
}

template<typename T>
void Hermes::check_create_buffer_(const std::string &name) {
    const std::lock_guard lock1(buffer_mutex_);

    auto &buffers = buffers_<T>;
    if (!buffers.contains(name)) {
        buffers[name] = std::vector<T>{};
    }
}
} // namespace imp

#endif //SABOGAE_HERMES_V0_H
