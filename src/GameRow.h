#pragma once

#include <wx/wx.h>
#include <vector>
#include <string>

class GameRow : public wxPanel {
public:
    explicit GameRow(wxWindow* parent, const wxArrayString& books);
    
    void setDisabled(bool disabled);
    bool isComplete() const;
    
    std::string getBook() const;
    std::vector<std::string> getDigits() const;
    std::vector<wxTextCtrl*> GetDigitCtrls();
    
    void setBookColor(const std::string& color);
    void setDigitColors(const std::vector<std::string>& colors);
    void lockSubmitted();

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
