#include "CharacterPersistence.h"
#include <chrono>
#include <exception>
#include <utility>

namespace characterpersistence {
namespace {
uint32_t milliseconds() {
    return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
}
Session::Session(Clock clock) : now(clock ? std::move(clock) : Clock(milliseconds)), baseline(now()) {}
void Session::bind(Access next) {
    std::lock_guard lock(execution);
    access = std::move(next);
    bound.store(true, std::memory_order_release);
}
Session::Cycle::Cycle(std::shared_ptr<Session> owner, uint32_t sample) : owner(std::move(owner)), sample(sample) {}
Session::Cycle::~Cycle() {
    if (!done.exchange(true, std::memory_order_acq_rel)) owner->releaseCycle();
}
void Session::Cycle::complete() {
    if (!done.exchange(true, std::memory_order_acq_rel)) {
        owner->periodic(sample);
        owner->releaseCycle();
    }
}
std::shared_ptr<Session::Cycle> Session::beginCycle() {
    if (!bound.load(std::memory_order_acquire) || !running.load(std::memory_order_acquire)) return {};
    const uint32_t sample = now();
    if (uint32_t(sample - baseline.load(std::memory_order_acquire)) <= 60000 ||
        busy.exchange(true, std::memory_order_acq_rel)) return {};
    if (!running.load(std::memory_order_acquire)) { releaseCycle(); return {}; }
    try {
        return std::shared_ptr<Cycle>(new Cycle(shared_from_this(), sample));
    } catch (...) {
        releaseCycle();
        throw;
    }
}
bool Session::menusAllow() const {
    if (!access.visibility) return false;
    for (size_t index = 0; index < 3; ++index)
        if (access.visibility(index) != 0) return false;
    return true;
}
bool Session::save() {
    if (!access.capture) return false;
    auto snapshot = access.capture();
    if (!snapshot || snapshot->slot < 0) return false;
    if (!access.write || !access.write(std::move(*snapshot))) {
        report("Character save failed");
        return false;
    }
    return true;
}
void Session::periodic(uint32_t sample) {
    std::lock_guard lock(execution);
    try {
        if (access.named && access.named() && menusAllow() && running.load(std::memory_order_acquire)) {
            save();
            baseline.store(sample, std::memory_order_release);
        }
    } catch (const std::exception &error) {
        report(std::string("Character save: ") + error.what());
    } catch (...) {
        report("Character save: unexpected capture/write failure");
    }
}
void Session::releaseCycle() {
    {
        std::lock_guard lock(waiting);
        busy.store(false, std::memory_order_release);
    }
    wake.notify_all();
}
bool Session::force() {
    std::lock_guard lock(execution);
    try { return save(); }
    catch (const std::exception &error) { report(std::string("Character save: ") + error.what()); }
    catch (...) { report("Character save: unexpected capture/write failure"); }
    return false;
}
void Session::finish() {
    requestStop();
    {
        std::unique_lock lock(waiting);
        wake.wait(lock, [&] { return !busy.load(std::memory_order_acquire); });
    }
    std::lock_guard lock(execution);
    if (finished) return;
    finished = true;
    try { if (bound.load(std::memory_order_acquire) && menusAllow()) save(); }
    catch (const std::exception &error) { report(std::string("Character save: ") + error.what()); }
    catch (...) { report("Character save: unexpected final capture/write failure"); }
}
void Session::report(std::string message) {
    std::lock_guard lock(errorsMutex);
    errors.push_back(std::move(message));
}
std::vector<std::string> Session::takeErrors() {
    std::lock_guard lock(errorsMutex);
    return std::exchange(errors, {});
}
} // namespace characterpersistence
