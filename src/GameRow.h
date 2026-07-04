#pragma once

#include <wx/wx.h>
#include <vector>
#include <string>

#include "GuessColor.h"

class GameRow : public wxPanel {
public:
    explicit GameRow(wxWindow* parent, const wxArrayString& books);

    void setDisabled(bool disabled);
    bool isComplete() const;

    std::string getBook() const;
    std::vector<std::string> getDigits() const;
    std::vector<wxTextCtrl*> GetDigitCtrls();

    void setBookColor(GuessColor color);
    void setDigitColors(const std::vector<GuessColor>& colors);
    void lockSubmitted();

    // Clears the row back to its pre-game state (empty book/digits, default
    // colors, unlocked, disabled) so a finished game can be replayed.
    void Reset();

private:
    wxComboBox* m_bookSelect;
    wxTextCtrl* m_c1;
    wxTextCtrl* m_c2;
    wxTextCtrl* m_v1;
    wxTextCtrl* m_v2;
    bool m_isBookLocked = false;
    wxString m_lockedBookSelection;
    
    void SetupUi(const wxArrayString& books);
    wxTextCtrl* CreateDigitInput();
};
