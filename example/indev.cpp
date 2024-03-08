#include "hermes_v0.h"

int main() {
    auto t = gae::Ticker(1.0);
    std::size_t i = 0;

    imp::Hermes::sub<Foo2>("a", [](const Foo2 &p) {});
    imp::Hermes::sub<Foo2>("b", [](const Foo2 &p) {});
    imp::Hermes::sub<Foo2>("c", [](const Foo2 &p) {});

    i = 0;
    t.reset();
    while (t.elapsed_sec() < 1.0) {
        t.tick();
        imp::Hermes::send_nowait<Foo2>(1, 2);
        i++;
    }
    fmt::println("{} iterations", i);

    auto h = gae::Hermes();
    h.sub<Foo>("a", [](const Foo &p) {});
    h.sub<Foo>("b", [](const Foo &p) {});
    h.sub<Foo>("c", [](const Foo &p) {});

    i = 0;
    t.reset();
    while (t.elapsed_sec() < 1.0) {
        t.tick();
        h.send_nowait<Foo>(1, 2);
        i++;
    }
    fmt::println("{} iterations", i);
}