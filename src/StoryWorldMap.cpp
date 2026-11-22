#include "StoryWorldMap.h"
#include "Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
// The map picture's shape when there's no picture (1400 x 752).
constexpr double kDefaultAspect = 752.0 / 1400.0;
const char* kMagnifier = "\xF0\x9F\x94\x8D";  // 🔍

wxColour PinColour(StoryWorldMap::PinState state) {
    switch (state) {
        case StoryWorldMap::PinState::Done: return wxColour(222, 170, 20);   // gold: every level passed
        case StoryWorldMap::PinState::Open: return Theme::Accent();          // levels to play
        case StoryWorldMap::PinState::Locked:
        default: return wxColour(150, 150, 150);                             // nothing unlocked yet
    }
}

const wxColour kInk(40, 30, 20);
}  // namespace

StoryWorldMap::StoryWorldMap(wxWindow* parent, const wxImage& picture)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
      m_picture(picture) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(200, 120));
    Bind(wxEVT_PAINT, &StoryWorldMap::OnPaint, this);
    Bind(wxEVT_SIZE, &StoryWorldMap::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &StoryWorldMap::OnLeftDown, this);
    Bind(wxEVT_MOTION, &StoryWorldMap::OnMotion, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        if (m_hover != -1) {
            m_hover = -1;
            Refresh();
        }
    });
}

void StoryWorldMap::SetPins(std::vector<Pin> pins, const std::string& selected) {
    m_pins = std::move(pins);
    m_selected = selected;
    m_hover = -1;
    Refresh();
}

void StoryWorldMap::SetCloseUp(const CloseUp& closeUp, bool zoomed) {
    m_closeUp = closeUp;
    m_zoomed = zoomed;
    m_scaled = wxBitmap();
    Refresh();
}

double StoryWorldMap::Aspect() const {
    // The close-up is a square in percentages, so it has the picture's shape.
    return m_picture.IsOk() ? double(m_picture.GetHeight()) / m_picture.GetWidth() : kDefaultAspect;
}

wxRect StoryWorldMap::PictureRect() const {
    const wxSize size = GetClientSize();
    const double aspect = Aspect();
    int w = size.x;
    int h = static_cast<int>(std::lround(w * aspect));
    if (h > size.y && size.y > 0) {  // keep the whole view visible
        h = size.y;
        w = static_cast<int>(std::lround(h / aspect));
    }
    return wxRect((size.x - w) / 2, (size.y - h) / 2, w, h);
}

wxPoint StoryWorldMap::ToScreen(double x, double y) const {
    const wxRect r = PictureRect();
    double vx = 0, vy = 0, vs = 100;  // the part of the picture showing, in percent
    if (Zoomed()) {
        vx = m_closeUp.x;
        vy = m_closeUp.y;
        vs = m_closeUp.size;
    }
    return wxPoint(r.x + static_cast<int>(std::lround(r.width * (x - vx) / vs)),
                   r.y + static_cast<int>(std::lround(r.height * (y - vy) / vs)));
}

int StoryWorldMap::PinRadius() const {
    // Big enough to tap; on the whole map, small enough to stay apart.
    const int w = PictureRect().width;
    return Zoomed() ? std::clamp(w / 60, 9, 15) : std::clamp(w / 110, 6, 11);
}

wxRect StoryWorldMap::ZoomOutButton() const {
    const wxRect pic = PictureRect();
    return wxRect(pic.x + 14, pic.y + 14, 150, 30);
}

int StoryWorldMap::HitTest(const wxPoint& at) const {
    if (Zoomed() && ZoomOutButton().Contains(at)) return kZoomOut;
    const int reach = PinRadius() + 4;
    auto isNear = [&](const wxPoint& c, long& dist) {
        const long dx = at.x - c.x, dy = at.y - c.y;
        dist = dx * dx + dy * dy;
        return dist <= long(reach) * reach;
    };
    int best = -1;
    long bestDist = 0, dist = 0;
    for (size_t i = 0; i < m_pins.size(); ++i) {
        if (!PinVisible(m_pins[i])) continue;
        if (isNear(ToScreen(m_pins[i].x, m_pins[i].y), dist) && (best == -1 || dist < bestDist)) {
            best = static_cast<int>(i);
            bestDist = dist;
        }
    }
    if (!Zoomed() && m_closeUp.IsSet() && isNear(ToScreen(m_closeUp.pinX, m_closeUp.pinY), dist) &&
        (best == -1 || dist < bestDist)) {
        best = kCloseUpPin;
    }
    return best;
}

void StoryWorldMap::ZoomTo(bool zoomed) {
    if (zoomed == m_zoomed) return;
    m_zoomed = zoomed;
    m_hover = -1;
    m_scaled = wxBitmap();
    UnsetToolTip();
    Refresh();
    if (m_onZoom) m_onZoom(zoomed);
}

void StoryWorldMap::OnSize(wxSizeEvent& event) {
    event.Skip();
    // As tall as the picture is at this width, so the whole map shows.
    const int width = GetClientSize().x;
    if (width > 0) {
        int height = static_cast<int>(std::lround(width * Aspect()));
        // On a short window keep the map to about 60% of the screen, so the
        // region's levels show underneath without scrolling.
        if (wxWindow* parent = GetParent()) {
            const int cap = std::max(200, parent->GetClientSize().y * 6 / 10);
            height = std::min(height, cap);
        }
        if (GetMinSize().y != height) {
            SetMinSize(wxSize(200, height));
            CallAfter([this]() {
                if (wxWindow* parent = GetParent()) {
                    parent->Layout();
                    if (auto* scrolled = dynamic_cast<wxScrolledWindow*>(parent)) scrolled->FitInside();
                }
            });
        }
    }
    m_scaled = wxBitmap();  // rescaled on the next paint
    Refresh();
}

void StoryWorldMap::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(Theme::Background()));
    dc.Clear();

    const wxRect pic = PictureRect();
    if (pic.width <= 0 || pic.height <= 0) return;
    if (m_picture.IsOk()) {
        if (!m_scaled.IsOk() || m_scaled.GetWidth() != pic.width || m_scaled.GetHeight() != pic.height ||
            m_scaledZoomed != Zoomed()) {
            wxImage view = m_picture;
            if (Zoomed()) {
                const int pw = m_picture.GetWidth(), ph = m_picture.GetHeight();
                wxRect crop(static_cast<int>(pw * m_closeUp.x / 100), static_cast<int>(ph * m_closeUp.y / 100),
                            static_cast<int>(pw * m_closeUp.size / 100), static_cast<int>(ph * m_closeUp.size / 100));
                crop.Intersect(wxRect(0, 0, pw, ph));
                if (!crop.IsEmpty()) view = m_picture.GetSubImage(crop);
            }
            m_scaled = wxBitmap(view.Scale(pic.width, pic.height, wxIMAGE_QUALITY_BICUBIC));
            m_scaledZoomed = Zoomed();
        }
        dc.DrawBitmap(m_scaled, pic.x, pic.y, true);
    } else {
        dc.SetBrush(wxBrush(wxColour(222, 205, 160)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRoundedRectangle(pic, 8);
    }

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    const int radius = PinRadius();
    const int labelSize = std::clamp(pic.width / 70, 9, 13);

    auto drawLabel = [&](const wxString& text, double cx, double top, bool bold) {
        gc->SetFont(wxFont(labelSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL), kInk);
        double tw, th;
        gc->GetTextExtent(text, &tw, &th);
        const double lx = std::clamp(cx - tw / 2, double(pic.x + 4), double(pic.GetRight() - tw - 4));
        gc->SetPen(wxPen(kInk, 1));
        gc->SetBrush(wxBrush(wxColour(255, 250, 235, bold ? 240 : 200)));
        gc->DrawRoundedRectangle(lx - 5, top - 2, tw + 10, th + 4, 5);
        gc->DrawText(text, lx, top);
        return th;
    };

    auto drawPin = [&](const wxPoint& c, double r, PinState state, int levels, bool selected, bool closeUp,
                       bool approximate = false) {
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(wxColour(0, 0, 0, 70)));
        gc->DrawEllipse(c.x - r + 1.5, c.y - r + 2, 2 * r, 2 * r);
        gc->SetBrush(wxBrush(PinColour(state)));
        gc->SetPen(wxPen(selected ? kInk : *wxWHITE, selected ? 3 : 2));
        gc->DrawEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
        if (approximate) {
            // A dashed ring: the spot is only approximate.
            wxPen dashed(selected ? kInk : wxColour(60, 45, 30), 2, wxPENSTYLE_SHORT_DASH);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(dashed);
            gc->DrawEllipse(c.x - r - 5, c.y - r - 5, 2 * r + 10, 2 * r + 10);
        }
        if (closeUp) {
            // A second ring: this pin opens a close-up.
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(selected ? kInk : *wxWHITE, 2));
            gc->DrawEllipse(c.x - r - 4, c.y - r - 4, 2 * r + 8, 2 * r + 8);
        }
        if (r >= 8 && levels > 0) {
            gc->SetFont(wxFont(std::max(7, int(r * 0.9)), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                               wxFONTWEIGHT_BOLD), *wxWHITE);
            const wxString n = wxString::Format("%d", levels);
            double tw, th;
            gc->GetTextExtent(n, &tw, &th);
            gc->DrawText(n, c.x - tw / 2, c.y - th / 2);
        }
    };

    // Regions in the close-up share one pin on the whole map.
    if (!Zoomed() && m_closeUp.IsSet()) {
        int levels = 0;
        bool anyOpen = false, allDone = true, any = false, selectedInside = false;
        for (const auto& pin : m_pins) {
            if (!pin.inCloseUp) continue;
            any = true;
            levels += pin.levels;
            if (pin.state == PinState::Open) anyOpen = true;
            if (pin.state != PinState::Done) allDone = false;
            if (pin.id == m_selected) selectedInside = true;
        }
        if (any) {
            const PinState state = allDone ? PinState::Done : anyOpen ? PinState::Open : PinState::Locked;
            const wxPoint c = ToScreen(m_closeUp.pinX, m_closeUp.pinY);
            const bool hover = m_hover == kCloseUpPin;
            const double r = radius * (hover ? 1.6 : 1.4);
            drawPin(c, r, state, levels, selectedInside, true);
            drawLabel(wxString::FromUTF8(kMagnifier) + " " + m_closeUp.name, c.x, c.y + r + 8,
                      selectedInside || hover);
        }
    }

    // Unselected pins first, then the hovered and selected ones on top.
    std::vector<size_t> order;
    for (size_t i = 0; i < m_pins.size(); ++i) if (PinVisible(m_pins[i])) order.push_back(i);
    std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) {
        auto rank = [this](size_t i) {
            return m_pins[i].id == m_selected ? 2 : (static_cast<int>(i) == m_hover ? 1 : 0);
        };
        return rank(a) < rank(b);
    });
    for (size_t i : order) {
        const Pin& pin = m_pins[i];
        const wxPoint c = ToScreen(pin.x, pin.y);
        const bool selected = pin.id == m_selected;
        const bool hover = static_cast<int>(i) == m_hover;
        const double r = selected ? radius * 1.35 : (hover ? radius * 1.2 : radius);
        drawPin(c, r, pin.state, pin.levels, selected, false, pin.approximate);
        // In the close-up there's room to name every place; on the whole map
        // only the selected or hovered one.
        const wxString label = pin.approximate ? pin.name + " (approx.)" : pin.name;
        if (Zoomed() || selected || hover) {
            drawLabel(label, c.x, c.y + r + (pin.approximate ? 9 : 5), selected || hover);
        }
    }

    if (Zoomed()) {
        const wxRect b = ZoomOutButton();
        const bool hover = m_hover == kZoomOut;
        gc->SetPen(wxPen(kInk, 1));
        gc->SetBrush(wxBrush(hover ? Theme::Accent().ChangeLightness(85) : Theme::Accent()));
        gc->DrawRoundedRectangle(b.x, b.y, b.width, b.height, 8);
        gc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), Theme::AccentText());
        const wxString text = wxString::FromUTF8("\xE2\x97\x80 Whole map");  // ◀
        double tw, th;
        gc->GetTextExtent(text, &tw, &th);
        gc->DrawText(text, b.x + (b.width - tw) / 2, b.y + (b.height - th) / 2);
    }
}

void StoryWorldMap::OnLeftDown(wxMouseEvent& event) {
    const int hit = HitTest(event.GetPosition());
    if (hit == kZoomOut) {
        ZoomTo(false);
        return;
    }
    if (hit == kCloseUpPin) {
        ZoomTo(true);
        return;
    }
    if (hit < 0) {
        event.Skip();
        return;
    }
    const std::string id = m_pins[hit].id;
    m_selected = id;
    Refresh();
    // The screen rebuilds itself (and this panel) on select, so not from
    // inside this click handler.
    CallAfter([this, id]() {
        if (m_onSelect) m_onSelect(id);
    });
}

void StoryWorldMap::OnMotion(wxMouseEvent& event) {
    const int hit = HitTest(event.GetPosition());
    if (hit != m_hover) {
        m_hover = hit;
        SetCursor(hit != -1 ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
        if (hit >= 0) {
            const Pin& pin = m_pins[hit];
            SetToolTip(wxString::Format("%s%s: %d %s", pin.name, pin.approximate ? " (approximate spot)" : "",
                                        pin.levels, pin.levels == 1 ? "level" : "levels"));
        } else if (hit == kCloseUpPin) {
            SetToolTip("Zoom in to " + m_closeUp.name);
        } else {
            UnsetToolTip();
        }
        Refresh();
    }
    event.Skip();
}
