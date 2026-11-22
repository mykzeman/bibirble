#include "StoryWorldMap.h"
#include "Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
// The map picture's shape when there's no picture (682 x 366).
constexpr double kDefaultAspect = 366.0 / 682.0;

wxColour PinColour(StoryWorldMap::PinState state) {
    switch (state) {
        case StoryWorldMap::PinState::Done: return wxColour(222, 170, 20);   // gold: every level passed
        case StoryWorldMap::PinState::Open: return Theme::Accent();          // levels to play
        case StoryWorldMap::PinState::Locked:
        default: return wxColour(150, 150, 150);                             // nothing unlocked yet
    }
}
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

wxRect StoryWorldMap::PictureRect() const {
    const wxSize size = GetClientSize();
    const double aspect = m_picture.IsOk() ? double(m_picture.GetHeight()) / m_picture.GetWidth() : kDefaultAspect;
    int w = size.x;
    int h = static_cast<int>(std::lround(w * aspect));
    if (h > size.y && size.y > 0) {  // keep the whole picture visible
        h = size.y;
        w = static_cast<int>(std::lround(h / aspect));
    }
    return wxRect((size.x - w) / 2, (size.y - h) / 2, w, h);
}

wxPoint StoryWorldMap::PinCentre(const Pin& pin) const {
    const wxRect r = PictureRect();
    return wxPoint(r.x + static_cast<int>(std::lround(r.width * pin.x / 100.0)),
                   r.y + static_cast<int>(std::lround(r.height * pin.y / 100.0)));
}

int StoryWorldMap::PinRadius() const {
    // Big enough to tap, small enough that the crowded pins around Israel
    // stay apart.
    return std::clamp(PictureRect().width / 110, 5, 11);
}

int StoryWorldMap::HitTest(const wxPoint& at) const {
    const int reach = PinRadius() + 3;
    int best = -1;
    long bestDist = 0;
    for (size_t i = 0; i < m_pins.size(); ++i) {
        const wxPoint c = PinCentre(m_pins[i]);
        const long dx = at.x - c.x, dy = at.y - c.y;
        const long dist = dx * dx + dy * dy;
        if (dist <= long(reach) * reach && (best == -1 || dist < bestDist)) {
            best = static_cast<int>(i);
            bestDist = dist;
        }
    }
    return best;
}

void StoryWorldMap::OnSize(wxSizeEvent& event) {
    event.Skip();
    // As tall as the picture is at this width, so the whole map shows.
    const int width = GetClientSize().x;
    if (width > 0) {
        const double aspect = m_picture.IsOk() ? double(m_picture.GetHeight()) / m_picture.GetWidth() : kDefaultAspect;
        int height = static_cast<int>(std::lround(width * aspect));
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
        if (!m_scaled.IsOk() || m_scaled.GetWidth() != pic.width || m_scaled.GetHeight() != pic.height) {
            m_scaled = wxBitmap(m_picture.Scale(pic.width, pic.height, wxIMAGE_QUALITY_HIGH));
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

    // Draw unselected pins first, then the hovered and selected ones on top
    // with their names.
    std::vector<size_t> order(m_pins.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) {
        auto rank = [this](size_t i) {
            return m_pins[i].id == m_selected ? 2 : (static_cast<int>(i) == m_hover ? 1 : 0);
        };
        return rank(a) < rank(b);
    });

    for (size_t i : order) {
        const Pin& pin = m_pins[i];
        const wxPoint c = PinCentre(pin);
        const bool selected = pin.id == m_selected;
        const bool hover = static_cast<int>(i) == m_hover;
        const double r = selected ? radius * 1.45 : (hover ? radius * 1.2 : radius);

        // Shadow, then the pin with a white rim (a dark rim when selected).
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(wxColour(0, 0, 0, 70)));
        gc->DrawEllipse(c.x - r + 1.5, c.y - r + 2, 2 * r, 2 * r);
        gc->SetBrush(wxBrush(PinColour(pin.state)));
        gc->SetPen(wxPen(selected ? wxColour(40, 30, 20) : *wxWHITE, selected ? 3 : 2));
        gc->DrawEllipse(c.x - r, c.y - r, 2 * r, 2 * r);

        // How many levels, when the pin is big enough to read it.
        if (r >= 8 && pin.levels > 0) {
            gc->SetFont(wxFont(std::max(7, int(r * 0.95)), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                               wxFONTWEIGHT_BOLD), *wxWHITE);
            const wxString n = wxString::Format("%d", pin.levels);
            double tw, th;
            gc->GetTextExtent(n, &tw, &th);
            gc->DrawText(n, c.x - tw / 2, c.y - th / 2);
        }

        if (selected || hover) {
            gc->SetFont(wxFont(labelSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD),
                        wxColour(40, 30, 20));
            double tw, th;
            gc->GetTextExtent(pin.name, &tw, &th);
            double lx = c.x - tw / 2;
            const double ly = c.y - r - th - 8;
            lx = std::clamp(lx, double(pic.x + 4), double(pic.GetRight() - tw - 4));
            gc->SetPen(wxPen(wxColour(40, 30, 20), 1));
            gc->SetBrush(wxBrush(wxColour(255, 250, 235, 235)));
            gc->DrawRoundedRectangle(lx - 6, ly - 3, tw + 12, th + 6, 5);
            gc->DrawText(pin.name, lx, ly);
        }
    }
}

void StoryWorldMap::OnLeftDown(wxMouseEvent& event) {
    const int hit = HitTest(event.GetPosition());
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
        SetCursor(hit >= 0 ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
        if (hit >= 0) {
            const Pin& pin = m_pins[hit];
            SetToolTip(wxString::Format("%s: %d %s", pin.name, pin.levels, pin.levels == 1 ? "level" : "levels"));
        } else {
            UnsetToolTip();
        }
        Refresh();
    }
    event.Skip();
}
