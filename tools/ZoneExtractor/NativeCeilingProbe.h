#pragma once
// Scope and frame layout from steam-dungeon-dome-observation-sites.md.
#include "DungeonCeilingCapture.h"
#include <windows.h>
#include <thread>

namespace captureabi {
class NativeCeilingProbe {
    using Read = bool (*)(uintptr_t, void *, size_t);
    using TargetSet = std::set<dungeoncapture::Point>;
    struct Row {
        dungeoncapture::CeilingKey key;
        dungeoncapture::CeilingValue value;
    };
    static constexpr DWORD64 slot = 1ull << 3;
    static constexpr DWORD64 enable = 1ull << 6;
    static constexpr DWORD64 control = 0xf00000c0ull;
    static constexpr uintptr_t finalCeiling = 0x30b009;
    inline static thread_local NativeCeilingProbe *active = nullptr;
    inline static thread_local uintptr_t ownedAddress = 0;
    inline static thread_local DWORD64 previousAddress = 0, previousControl = 0;
    uintptr_t address_;
    unsigned char *const &zone_;
    const TargetSet &targets_;
    Read read_;
    std::vector<Row> rows_;
    size_t count_ = 0;
    PVOID handler_ = nullptr;
    bool installed_ = false, invalid_ = false;

    void observe(const CONTEXT &c) {
        uintptr_t owner = 0;
        int32_t x = 0, y = 0, floor = 0, ceiling = 0;
        uint8_t dome = 0;
        if (c.Rbp < 0x80 || !read_(c.Rbp - 0x80, &owner, sizeof owner)) {
            invalid_ = true;
            return;
        }
        if (owner != reinterpret_cast<uintptr_t>(zone_) || !owner)
            return;
        if (!read_(c.Rbp - 0x78, &x, sizeof x) || !read_(c.Rbp - 0x68, &y, sizeof y) ||
            !read_(c.Rbp - 0x5c, &floor, sizeof floor)) {
            invalid_ = true;
            return;
        }
        if (floor >= 0 || !targets_.count({x, y}))
            return;
        if (!read_(c.Rbp + 0x28, &dome, sizeof dome) || !read_(c.Rbp + 0xd4, &ceiling, sizeof ceiling) ||
            dome > 1 || uint32_t(c.R15) >= 16 || uint32_t(c.R14) >= 16 || count_ == rows_.size()) {
            invalid_ = true;
            return;
        }
        rows_[count_++] = {{x, y, floor, int32_t(uint32_t(c.R15)), int32_t(uint32_t(c.R14))}, {dome != 0, ceiling}};
    }

    static LONG CALLBACK exception(EXCEPTION_POINTERS *event) {
        CONTEXT &c = *event->ContextRecord;
        if (event->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
            !ownedAddress || c.Rip != ownedAddress || !(c.Dr6 & slot))
            return EXCEPTION_CONTINUE_SEARCH;
        if (active)
            active->observe(c);
        else {
            // If explicit cleanup failed, retire our slot without a stale callback.
            c.Dr3 = previousAddress;
            c.Dr7 = (c.Dr7 & ~control) | previousControl;
            ownedAddress = 0;
        }
        c.Dr6 &= ~slot;
        c.EFlags |= 1u << 16; // Resume the original instruction without retriggering its execution breakpoint.
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    bool changeSlot(bool install) {
        HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, GetCurrentThreadId());
        if (!thread)
            return false;
        bool changed = false;
        DWORD64 savedAddress = previousAddress, savedControl = previousControl;
        try {
            std::thread([&] {
                const DWORD suspended = SuspendThread(thread);
                if (suspended == DWORD(-1))
                    return;
                CONTEXT c{};
                c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (suspended == 0 && GetThreadContext(thread, &c)) {
                    if (install && !(c.Dr7 & 0xff)) {
                        savedAddress = c.Dr3;
                        savedControl = c.Dr7 & control;
                        c.Dr3 = address_;
                        c.Dr7 = (c.Dr7 & ~control) | enable;
                        changed = SetThreadContext(thread, &c) != FALSE;
                    } else if (!install && c.Dr3 == address_ && (c.Dr7 & control) == enable) {
                        c.Dr3 = savedAddress;
                        c.Dr7 = (c.Dr7 & ~control) | savedControl;
                        changed = SetThreadContext(thread, &c) != FALSE;
                    }
                }
                ResumeThread(thread);
            }).join();
        } catch (...) {
            CloseHandle(thread);
            return false;
        }
        CloseHandle(thread);
        if (changed && install) {
            previousAddress = savedAddress;
            previousControl = savedControl;
        }
        return changed;
    }

public:
    NativeCeilingProbe(uintptr_t module, unsigned char *const &zone, const TargetSet &targets, Read read)
        : address_(module + finalCeiling), zone_(zone), targets_(targets), read_(read), rows_(targets.size() * 256) {}
    NativeCeilingProbe(const NativeCeilingProbe &) = delete;
    NativeCeilingProbe &operator=(const NativeCeilingProbe &) = delete;
    ~NativeCeilingProbe() { if (installed_) stop(); }

    bool arm() {
        if (targets_.empty())
            return true;
        constexpr std::array<uint8_t, 9> fingerprint{0x41, 0x8b, 0xdc, 0x8b, 0x85, 0xd4, 0, 0, 0};
        std::array<uint8_t, fingerprint.size()> current{};
        if (!read_(address_, current.data(), current.size()) || current != fingerprint)
            return false;
        if (ownedAddress)
            return false;
        handler_ = AddVectoredExceptionHandler(1, &exception);
        if (!handler_)
            return false;
        if (!changeSlot(true)) {
            RemoveVectoredExceptionHandler(handler_);
            return false;
        }
        ownedAddress = address_;
        active = this;
        installed_ = true;
        return true;
    }

    bool stop() {
        if (!installed_)
            return true;
        active = nullptr;
        installed_ = false;
        if (!changeSlot(false))
            return false;
        ownedAddress = 0;
        RemoveVectoredExceptionHandler(handler_);
        return true;
    }

    bool collect(dungeoncapture::CeilingRows &output) const {
        if (invalid_)
            return false;
        for (size_t i = 0; i < count_; ++i)
            if (!output.emplace(rows_[i].key, rows_[i].value).second)
                return false;
        return true;
    }
};
} // namespace captureabi
