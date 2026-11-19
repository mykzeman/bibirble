#pragma once

#include <wx/wx.h>

// A wxStaticText that re-wraps its text to whatever width the layout gives it,
// so text blocks follow the window as it's resized instead of wrapping at a
// fixed pixel width. Add it to its sizer with wxEXPAND (or with a proportion
// in a horizontal sizer) so the sizer, not the text, decides its width.
class WrappingText : public wxStaticText {
public:
    WrappingText(wxWindow* parent, const wxString& label, long style = 0)
        // wxST_NO_AUTORESIZE: otherwise every SetLabel() snaps the control
        // back to its unwrapped width, which would trigger a re-wrap at the
        // wrong width.
        : wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize,
                       style | wxST_NO_AUTORESIZE),
          m_text(label) {
        // A small minimum width so the unwrapped text never forces the
        // window (or a scrolled area) to be wider than it is.
        SetMinSize(wxSize(kMinWidth, -1));
        Bind(wxEVT_SIZE, &WrappingText::OnSize, this);
    }

    // Keeps the unwrapped text so it can be wrapped again at a new width.
    void SetLabel(const wxString& label) override {
        if (m_inWrap) {
            // Called back from Wrap() with the wrapped text.
            wxStaticText::SetLabel(label);
            return;
        }
        m_text = label;
        Rewrap(GetClientSize().x);
        RelayoutAncestors();
    }

    // Keep min-size changes from the caller to a small width (see above).
    void SetMinSize(const wxSize& size) override {
        wxStaticText::SetMinSize(wxSize(size.x < 0 ? kMinWidth : size.x, size.y));
    }

private:
    static constexpr int kMinWidth = 50;

    void OnSize(wxSizeEvent& event) {
        event.Skip();
        const int width = event.GetSize().x;
        if (width > 0 && width != m_wrapWidth) {
            Rewrap(width);
            // Height may have changed: let the containers make room for it.
            CallAfter([this]() { RelayoutAncestors(); });
        }
    }

    void Rewrap(int width) {
        m_inWrap = true;
        wxStaticText::SetLabel(m_text);
        if (width > kMinWidth / 2) {
            Wrap(width);
            m_wrapWidth = width;
        }
        m_inWrap = false;
        InvalidateBestSize();
    }

    void RelayoutAncestors() {
        for (wxWindow* w = GetParent(); w; w = w->GetParent()) {
            w->Layout();
            if (auto* scrolled = dynamic_cast<wxScrolledWindow*>(w)) scrolled->FitInside();
            if (w->IsTopLevel()) break;
        }
    }

    wxString m_text;
    int m_wrapWidth = -1;
    bool m_inWrap = false;
};
