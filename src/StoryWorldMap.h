#pragma once

#include <wx/wx.h>
#include <functional>
#include <string>
#include <vector>

// The picture map at the top of Story Mode: the Bible lands (data/story_map.png)
// scaled to the window's width, with a pin on every region that has levels.
// A pin's colour shows how its levels are going, and clicking one selects that
// region (StoryMapScreen then lists its levels underneath).
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
    };

    // `picture` may be invalid (no map image found): the pins are drawn on a
    // plain background instead.
    StoryWorldMap(wxWindow* parent, const wxImage& picture);

    void SetPins(std::vector<Pin> pins, const std::string& selected);
    void SetOnSelect(std::function<void(const std::string& regionId)> cb) { m_onSelect = std::move(cb); }

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);

    wxRect PictureRect() const;
    wxPoint PinCentre(const Pin& pin) const;
    int PinRadius() const;
    int HitTest(const wxPoint& at) const;  // index into m_pins, or -1

    wxImage m_picture;
    wxBitmap m_scaled;  // m_picture at the current size
    std::vector<Pin> m_pins;
    std::string m_selected;
    int m_hover = -1;
    std::function<void(const std::string&)> m_onSelect;
};
