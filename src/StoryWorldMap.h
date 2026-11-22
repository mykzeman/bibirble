#pragma once

#include <wx/wx.h>
#include <functional>
#include <string>
#include <vector>

// The picture map at the top of Story Mode: the Bible lands (data/story_map.png)
// scaled to the window's width, with a pin on every region that has levels.
// A pin's colour shows how its levels are going, and clicking one selects that
// region (StoryMapScreen then lists its levels underneath).
//
// The land around Israel is too crowded for one pin each, so it has a
// close-up: on the whole map its regions share one magnifier pin, and clicking
// that zooms into a square of the picture where each region has its own pin.
// A "Whole map" button zooms back out.
class StoryWorldMap : public wxPanel {
public:
    enum class PinState { Locked, Open, Done };

    struct Pin {
        std::string id;
        wxString name;
        double x = 0;  // percent of the picture's width
        double y = 0;  // percent of the picture's height
        int levels = 0;  // levels shown in this region
        PinState state = PinState::Locked;
        bool inCloseUp = false;
        bool approximate = false;  // only a traditional spot: dashed ring, "(approx.)" label
    };

    struct CloseUp {
        wxString name;
        double x = 0, y = 0, size = 0;  // the zoomed square, in percent of the picture
        double pinX = 0, pinY = 0;      // its pin on the whole map
        bool IsSet() const { return size > 0; }
    };

    // `picture` may be invalid (no map image found): the pins are drawn on a
    // plain background instead.
    StoryWorldMap(wxWindow* parent, const wxImage& picture);

    void SetPins(std::vector<Pin> pins, const std::string& selected);
    void SetCloseUp(const CloseUp& closeUp, bool zoomed);
    void SetOnSelect(std::function<void(const std::string& regionId)> cb) { m_onSelect = std::move(cb); }
    // Told when the player zooms in or out, so the screen remembers it.
    void SetOnZoom(std::function<void(bool zoomed)> cb) { m_onZoom = std::move(cb); }

private:
    static constexpr int kCloseUpPin = -2;  // HitTest(): the close-up's pin
    static constexpr int kZoomOut = -3;     // HitTest(): the "Whole map" button

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);

    bool Zoomed() const { return m_zoomed && m_closeUp.IsSet(); }
    bool PinVisible(const Pin& pin) const { return Zoomed() ? pin.inCloseUp : !pin.inCloseUp; }
    double Aspect() const;
    wxRect PictureRect() const;
    wxPoint ToScreen(double x, double y) const;  // picture percentages -> panel pixels
    int PinRadius() const;
    wxRect ZoomOutButton() const;
    int HitTest(const wxPoint& at) const;  // index into m_pins, kCloseUpPin, kZoomOut, or -1
    void ZoomTo(bool zoomed);

    wxImage m_picture;
    wxBitmap m_scaled;  // what's showing (whole picture or close-up) at the current size
    bool m_scaledZoomed = false;
    std::vector<Pin> m_pins;
    std::string m_selected;
    CloseUp m_closeUp;
    bool m_zoomed = false;
    int m_hover = -1;
    std::function<void(const std::string&)> m_onSelect;
    std::function<void(bool)> m_onZoom;
};
