#pragma once

#include <wx/wx.h>
#include <wx/event.h>
#include <functional>

namespace wx_callafter_compat {
    // Define a simple custom event type for call-after lambdas (avoid inline variable to support older MSVC)
    inline wxEventType wxEVT_CALLAFTER() { static wxEventType t = wxNewEventType(); return t; }

    class CallAfterEvent : public wxEvent {
    public:
        CallAfterEvent(std::function<void()>&& f)
            : wxEvent(wxEVT_CALLAFTER()), func_(std::move(f)) {}

        CallAfterEvent(const CallAfterEvent& other)
            : wxEvent(other), func_(other.func_) {}

        wxEvent* Clone() const override { return new CallAfterEvent(*this); }
        void Run() const { func_(); }

    private:
        std::function<void()> func_;
    };

    inline void DispatchCallAfter(wxEvent& evt) {
        CallAfterEvent& cae = static_cast<CallAfterEvent&>(evt);
        cae.Run();
    }

    inline void EnsureRegistered() {
        static bool registered = false;
        if (!registered && wxTheApp) {
            // Bind to the app so posted events are dispatched on the main thread
            wxTheApp->Bind(wxEVT_CALLAFTER(), &DispatchCallAfter);
            registered = true;
        }
    }

    template<typename Callable>
    inline void Post(Callable&& c) {
        EnsureRegistered();
        if (wxTheApp) {
            auto evt = new CallAfterEvent(std::function<void()>(std::forward<Callable>(c)));
            // QueueEvent will take ownership of the event and will deliver it on the main thread
            wxTheApp->QueueEvent(evt);
        } else {
            // Fall back to direct call (best-effort, not thread-safe)
            std::forward<Callable>(c)();
        }
    }
}

// Public API: wxCallAfter(callable)
template<typename Callable>
inline void wxCallAfter(Callable&& c) {
    wx_callafter_compat::Post(std::forward<Callable>(c));
}
