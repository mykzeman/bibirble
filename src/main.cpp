#include "BibirbleWindow.h"
#include "loading_dialog.h"
#include "PersistenceManager.h"
#include <wx/wx.h>

class BibirbleApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("Bibirble");

        // Load the player's Bible version behind a small progress dialog; the
        // window takes the loaded verses over instead of parsing them again.
        PersistenceManager prefs;
        prefs.Load();
        LoadingDialog dlg(nullptr, prefs.GetBibleVersion());
        dlg.ShowModal();
        if (!dlg.FoundFile()) {
            wxMessageBox(
                "Could not find or read bible_sections.json. The app will start, but no data will be loaded.",
                "Warning", wxOK | wxICON_WARNING);
        }

        BibirbleWindow* frame = new BibirbleWindow(nullptr, dlg.TakeData());
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(BibirbleApp);

int main(int argc, char* argv[]) {
    return wxEntry(argc, argv);
}
