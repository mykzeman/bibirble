#pragma once

#include <wx/wx.h>

// Displays a "cinematic illustration" for Story Mode: a real image loaded
// from an assets folder if one is present, or a placeholder art card (a
// gradient panel with the caption) if not. This lets the UI/gameplay flow be
// built and wired up before real artwork exists -- once image files are
// dropped into assets/story/..., they display automatically with no code
// changes.
class IllustrationPanel : public wxPanel {
public:
    IllustrationPanel(wxWindow* parent, const wxString& assetRelativePath, const wxString& caption);

    void SetContent(const wxString& assetRelativePath, const wxString& caption);

    // Resolves "assets/story/<relativePath>" against a handful of candidate
    // working directories, mirroring BibleData::ResolveDataFilePath. Returns
    // an empty string if no matching file is found.
    static wxString ResolveAssetPath(const wxString& relativePath);

private:
    void OnPaint(wxPaintEvent& event);
    void LoadImage();

    wxString m_assetRelativePath;
    wxString m_caption;
    wxImage m_image;
    bool m_hasImage = false;

    wxDECLARE_EVENT_TABLE();
};
