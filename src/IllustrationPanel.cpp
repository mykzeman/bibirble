#include "IllustrationPanel.h"

#include <algorithm>

#include <wx/dcbuffer.h>
#include <wx/filename.h>

wxBEGIN_EVENT_TABLE(IllustrationPanel, wxPanel)
    EVT_PAINT(IllustrationPanel::OnPaint)
wxEND_EVENT_TABLE()

IllustrationPanel::IllustrationPanel(wxWindow* parent, const wxString& assetRelativePath,
                                      const wxString& caption)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 220)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 220));
    SetContent(assetRelativePath, caption);
}

void IllustrationPanel::SetContent(const wxString& assetRelativePath, const wxString& caption) {
    m_assetRelativePath = assetRelativePath;
    m_caption = caption;
    LoadImage();
    Refresh();
}

wxString IllustrationPanel::ResolveAssetPath(const wxString& relativePath) {
    wxString suffix = "assets/story/" + relativePath;

    wxArrayString prefixes;
    prefixes.Add("");
    prefixes.Add("./");
    prefixes.Add("../");
    prefixes.Add("Release/");
    prefixes.Add("./Release/");
    prefixes.Add("../Release/");
    prefixes.Add("Debug/");
    prefixes.Add("./Debug/");
    prefixes.Add("../Debug/");
    prefixes.Add("build/");
    prefixes.Add("./build/");
    prefixes.Add("../build/");

    for (const auto& prefix : prefixes) {
        wxString candidate = prefix + suffix;
        if (wxFileName::FileExists(candidate)) {
            return candidate;
        }
    }
    return "";
}

void IllustrationPanel::LoadImage() {
    m_hasImage = false;
    if (m_assetRelativePath.IsEmpty()) return;

    wxString path = ResolveAssetPath(m_assetRelativePath);
    if (path.IsEmpty()) return;

    if (m_image.LoadFile(path)) {
        m_hasImage = true;
    }
}

void IllustrationPanel::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxRect bounds = GetClientRect();
    if (bounds.width <= 0 || bounds.height <= 0) return;

    if (m_hasImage && m_image.IsOk()) {
        double scale = std::min((double)bounds.width / m_image.GetWidth(),
                                 (double)bounds.height / m_image.GetHeight());
        int drawW = std::max(1, (int)(m_image.GetWidth() * scale));
        int drawH = std::max(1, (int)(m_image.GetHeight() * scale));

        dc.SetBrush(wxBrush(wxColour(20, 20, 20)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(bounds);

        wxBitmap bitmap(m_image.Scale(drawW, drawH, wxIMAGE_QUALITY_HIGH));
        dc.DrawBitmap(bitmap, bounds.x + (bounds.width - drawW) / 2,
                      bounds.y + (bounds.height - drawH) / 2, true);
        return;
    }

    // Placeholder art: a warm gradient card with the caption centered, so the
    // layout and flow look right before real illustrations are dropped in.
    dc.GradientFillLinear(bounds, wxColour(200, 100, 50), wxColour(120, 60, 40), wxSOUTH);

    dc.SetPen(wxPen(wxColour(235, 230, 157), 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(bounds.Deflate(4, 4));

    if (!m_caption.IsEmpty()) {
        wxFont font(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD);
        dc.SetFont(font);
        dc.SetTextForeground(wxColour(235, 230, 157));

        wxString wrapped = m_caption;
        wxCoord textW, textH;
        dc.GetMultiLineTextExtent(wrapped, &textW, &textH);
        dc.DrawLabel(wrapped, bounds, wxALIGN_CENTER);
    }
}
