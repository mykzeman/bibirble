#include "BibirbleWindow.h"
#include "loading_dialog.h"
#include <wx/wx.h>

class BibirbleApp : public wxApp {
public:
    bool OnInit() override {
        const std::string dataPath = BibleData::ResolveDataFilePath("bible_sections.json");

        // Show a small loading dialog that verifies the presence of bible_sections.json
        LoadingDialog dlg(nullptr, wxString::FromUTF8(dataPath.c_str()));
        dlg.ShowModal();
        if (!dlg.FoundFile()) {
            wxMessageBox(
                "Could not find or read bible_sections.json. The app will start, but no data will be loaded.",
                "Warning", wxOK | wxICON_WARNING);
        }

        BibirbleWindow* frame = new BibirbleWindow(nullptr, dataPath);
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(BibirbleApp);

int main(int argc, char* argv[]) {
    return wxEntry(argc, argv);
}
