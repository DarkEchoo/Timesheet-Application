#include <wx/wx.h>
#include "TimeSheetLogic.h"// Include your logic (or better, split into header/cpp later)
#include <wx/stdpaths.h>
#include <sstream>
#include <fstream>       // ? REQUIRED for std::ifstream
#include <string>        // ? REQUIRED for std::string
#include <functional> // ← Make sure this is included at the top
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/datetime.h>
#include <wx/spinbutt.h>  // Required for wxSpinButton
#include <wx/spinctrl.h>
#include <regex>
#include <wx/listctrl.h>




// Below your includes in TimeSheetApp.cpp
std::string adminPassword;
bool isAdminLoggedIn = false;
const std::string adminConfigFile = "admin.config";
void showTimesheetDialog(Employee& emp);
bool editTimesheetDialog(wxWindow* parent, WorkEntry& entry);


class TimeSheetApp : public wxApp {
public:
    virtual bool OnInit();
};

class TimeSheetFrame : public wxFrame {
public:
    TimeSheetFrame(const wxString& title);
    void RefreshEmployeeList();
    void RefreshAdminUI();


private:
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnLoadEmployees(wxCommandEvent& event);
    void OnSelectEmployee(wxCommandEvent& event);
    void OnCreateEmployee(wxCommandEvent& event);
    wxMenuItem* clearEntriesMenuItem = nullptr;
    wxButton* createBtn;
    void showEditEmployeeDialog(Employee& emp);


    wxListBox* employeeList;
    wxButton* selectButton;
};

enum {
    ID_LoadEmployees = 1,
    ID_SelectEmployee
};

bool isThisWeek(const std::string& dateStr) {
    std::tm tm = {};
    int month, day, year;

    if (sscanf(dateStr.c_str(), "%d%*c%d%*c%d", &month, &day, &year) != 3)
        return false;

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    std::time_t entryTime = std::mktime(&tm);

    std::time_t now = std::time(nullptr);
    std::tm* nowTm = std::localtime(&now);
    nowTm->tm_hour = nowTm->tm_min = nowTm->tm_sec = 0;

    std::time_t startOfWeek = std::mktime(nowTm) - (nowTm->tm_wday * 86400);
    std::time_t endOfWeek = startOfWeek + (7 * 86400);

    return entryTime >= startOfWeek && entryTime < endOfWeek;
}


wxIMPLEMENT_APP(TimeSheetApp);

bool TimeSheetApp::OnInit() {
    loadEmployees();  // Load employee data
    for (auto& emp : employees) {
        loadTimesheet(emp);  // Load each employee's timesheets
    }

    // 🔐 Load saved admin password if one exists
    std::ifstream infile(adminConfigFile);
    if (infile) {
        std::getline(infile, adminPassword);
        infile.close();
    }

    TimeSheetFrame* frame = new TimeSheetFrame("Timesheet Generator");
    frame->Show(true);
    frame->RefreshEmployeeList();

    wxIcon icon;
    icon.LoadFile("appicon.ico", wxBITMAP_TYPE_ICO);
    frame->SetIcon(icon);

    return true;
}


TimeSheetFrame::TimeSheetFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400)) {

    // MENU BAR SETUP — Top of constructor
    wxMenuBar* menuBar = new wxMenuBar();
    wxMenu* helpMenu = new wxMenu();

    wxMenu* adminMenu = new wxMenu();
    adminMenu->Append(wxID_HIGHEST + 200, "Login / Logout", "Toggle admin mode");
    menuBar->Append(adminMenu, "&Admin");


    // Create Tools menu
    wxMenu* toolsMenu = new wxMenu();
    toolsMenu->Append(wxID_HIGHEST + 100, "Show Weekly Hours", "Display total and overtime hours this week");
    toolsMenu->Append(wxID_HIGHEST + 101, "Refresh Employees", "Reload employee data from disk");
    clearEntriesMenuItem = toolsMenu->Append(wxID_HIGHEST + 102, "Clear All Entries", "Delete all timesheet entries for selected employee");
    clearEntriesMenuItem->Enable(false);  // Hide access by default


    // Add Tools to menu bar
    menuBar->Append(toolsMenu, "&Tools");

    helpMenu->Append(wxID_ABOUT, "&About", "App Info");
    helpMenu->Append(wxID_HELP, "&Help Guide", "How to use the timesheet generator");
    helpMenu->AppendSeparator(); // Optional visual break
    helpMenu->Append(wxID_EXIT, "E&xit", "Close the application");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);


    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    selectButton = new wxButton(panel, ID_SelectEmployee, "Select Employee");
    createBtn = new wxButton(panel, wxID_HIGHEST + 1, "Create Employee");
    wxButton* exportBtn = new wxButton(panel, wxID_HIGHEST + 2, "Export Timesheet");
    employeeList = new wxListBox(panel, wxID_ANY);

    createBtn->Show(false);  // Hide on startup unless admin logs in

    sizer->Add(employeeList, 1, wxALL | wxEXPAND, 5);
    sizer->Add(selectButton, 0, wxALL | wxEXPAND, 5);
    sizer->Add(createBtn, 0, wxALL | wxEXPAND, 5);
    sizer->Add(exportBtn, 0, wxALL | wxEXPAND, 5);

    panel->SetSizer(sizer);
    // Force layout so widgets are visible
    sizer->Layout();

    // ⬇️ Set a proper minimum size to prevent being too small
    this->SetMinSize(wxSize(600, 400));

    // Center after resizing
    this->Centre();
    Centre();                   // Center after adjusting size


    Bind(wxEVT_MENU, [=](wxCommandEvent&) {
        if (adminPassword.empty()) {
            // First-time setup
            wxTextEntryDialog setupDlg(this, "Set a new Admin Password:", "Initial Admin Setup");
            setupDlg.CentreOnParent();
            if (setupDlg.ShowModal() == wxID_OK) {
                adminPassword = setupDlg.GetValue().ToStdString();
                std::ofstream out(adminConfigFile);
                out << adminPassword;
                out.close();

                // ⬇️ Make the file hidden on Windows
#ifdef _WIN32
#include <windows.h>
                SetFileAttributesA(adminConfigFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

                wxMessageBox("Admin password set successfully!", "Success", wxOK | wxICON_INFORMATION);
            }
            else {
                wxMessageBox("Admin password setup cancelled.", "Notice", wxOK | wxICON_WARNING);
                return;
            }
        }

        if (!isAdminLoggedIn) {
            wxTextEntryDialog dlg(this, "Enter Admin Password:", "Admin Login", "", wxOK | wxCANCEL | wxTE_PASSWORD);
            dlg.CentreOnParent();

            dlg.Bind(wxEVT_CHAR_HOOK, [&](wxKeyEvent& event) {
                if (event.ControlDown() && event.ShiftDown() && event.GetKeyCode() == 'R') {
                    if (wxMessageBox("Reset admin password? This cannot be undone.", "Reset?", wxYES_NO | wxICON_WARNING) == wxYES) {
                        std::remove(adminConfigFile.c_str());   // ⬅️ delete the file
                        adminPassword.clear();                  // ⬅️ clear in memory
                        wxMessageBox("Admin password has been reset.\nRestart the app to set a new password.", "Reset Successful", wxOK | wxICON_INFORMATION);
                        dlg.EndModal(wxID_CANCEL);              // ⬅️ exit dialog without login
                    }
                }
                else {
                    event.Skip();  // allow normal keypresses
                }
                });


            if (dlg.ShowModal() == wxID_OK) {
                std::string input = dlg.GetValue().ToStdString();
                if (input == adminPassword) {
                    isAdminLoggedIn = true;
                    createBtn->Show(true);

                    // ✅ Update layout correctly
                    createBtn->GetParent()->Layout();
                    createBtn->GetParent()->FitInside();

                    this->SetTitle("Timesheet Generator [Admin Mode]");
                    RefreshAdminUI();
                    wxMessageBox("Admin logged in successfully.", "Success", wxOK | wxICON_INFORMATION);
                }

                else {
                    wxMessageBox("Incorrect password.", "Access Denied", wxOK | wxICON_ERROR);
                }
            }
        }
        else {
            if (wxMessageBox("Log out of admin mode?", "Confirm", wxYES_NO | wxICON_QUESTION) == wxYES) {
                isAdminLoggedIn = false;
                createBtn->Show(false);
                Layout();
                this->SetTitle("Timesheet Generator");
                RefreshAdminUI();
                wxMessageBox("Logged out successfully.", "Info", wxOK | wxICON_INFORMATION);
            }
        }
        }, wxID_HIGHEST + 200);


    Bind(wxEVT_MENU, &TimeSheetFrame::OnLoadEmployees, this, wxID_HIGHEST + 101);


    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        int selection = employeeList->GetSelection();
        if (selection == wxNOT_FOUND) {
            wxMessageBox("Please select an employee first.", "No Employee Selected", wxOK | wxICON_WARNING);
            return;
        }

        Employee& selected = employees[selection];
        loadTimesheet(selected);

        double weekHours = 0;
        for (const auto& entry : selected.timesheets) {
            if (isThisWeek(entry.date)) {
                weekHours += entry.totalHours;
            }
        }

        double overtime = std::max(0.0, weekHours - 40.0);
        wxString summary = wxString::Format("Total hours this week: %.2f\nOvertime hours: %.2f", weekHours, overtime);
        wxMessageBox(summary, "Weekly Hours Summary", wxOK | wxICON_INFORMATION);

        }, wxID_HIGHEST + 100);


    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        int selection = employeeList->GetSelection();
        if (selection == wxNOT_FOUND) return;

        Employee& selected = employees[selection];
        loadTimesheet(selected);  // ensure data is fresh


        // Calculate hours
        double weekHours = 0;
        for (const auto& entry : selected.timesheets) {
            if (isThisWeek(entry.date)) {
                weekHours += entry.totalHours;
            }
        }

        double overtime = std::max(0.0, weekHours - 40.0);
        wxString summary = wxString::Format("Total hours this week: %.2f\nOvertime hours: %.2f", weekHours, overtime);
        wxMessageBox(summary, "Weekly Hours Summary", wxOK | wxICON_INFORMATION);

        }, wxID_HIGHEST + 6);

    Bind(wxEVT_MENU, [=](wxCommandEvent&) {
        Close(true);
        }, wxID_EXIT);

    Bind(wxEVT_MENU, [=](wxCommandEvent&) {
        wxDialog aboutDlg(this, wxID_ANY, "About Timesheet Generator", wxDefaultPosition, wxSize(400, 250));
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* title = new wxStaticText(&aboutDlg, wxID_ANY, "Timesheet Generator");
        title->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

        wxStaticText* info = new wxStaticText(&aboutDlg, wxID_ANY,
            "Version: 1.0\n"
            "Author: Jesse Earl\n"
            "Year: 2025\n\n"
            "This tool helps you log daily work hours across multiple job locations.\n"
            "You can export entries directly to Excel and validate lunch input.");

        wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
        btnSizer->AddButton(new wxButton(&aboutDlg, wxID_OK));
        btnSizer->Realize();

        sizer->Add(title, 0, wxALIGN_CENTER | wxTOP, 10);
        sizer->Add(info, 0, wxALL | wxEXPAND, 15);
        sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

        aboutDlg.SetSizerAndFit(sizer);
        aboutDlg.ShowModal();
        }, wxID_ABOUT);

    Bind(wxEVT_MENU, [=](wxCommandEvent&) {
        wxMessageBox(
            "HOW TO USE THE TIMESHEET GENERATOR\n\n"
            "1. Load or Create an Employee:\n"
            "   - Click 'Load Employees' to load saved profiles.\n"
            "   - Click 'Create Employee' (admin only) to make a new profile.\n\n"
            "2. Enter Timesheet Data:\n"
            "   - Select an employee, then click 'Enter Timesheet'.\n"
            "   - Enter the date and number of job locations (up to 6).\n"
            "   - For each location, enter address, job number, description, and time in/out.\n"
            "   - A lunch break is required if you work 5+ hours.\n"
            "   - If you work less than 5 hours, lunch must be 0 or up to 0.5 hrs.\n\n"
            "3. Review and Save:\n"
            "   - Confirm the summary shown before the entry is saved.\n\n"
            "4. View or Edit Timesheets:\n"
            "   - Click 'Select Employee' and choose 'View Timesheet'.\n"
            "   - Admins can edit or delete entries.\n\n"
            "5. Export Timesheet:\n"
            "   - Use the 'Export Timesheet' button to save data to Excel.\n\n"
            "6. Admin Features:\n"
            "   - Admin login unlocks employee creation, entry editing, and entry deletion.\n"
            "   - 'Clear All Entries' under Tools is only enabled in Admin mode.\n\n"
            "For questions, click About or contact the developer.",
            "Help Guide",
            wxOK | wxICON_INFORMATION
        );
        }, wxID_HELP);


    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
    int selection = employeeList->GetSelection();
    if (selection == wxNOT_FOUND) {
        wxMessageBox("Please select an employee first.", "No Employee Selected", wxOK | wxICON_WARNING);
        return;
    }

    Employee& selected = employees[selection];

    if (wxMessageBox("Are you sure you want to delete ALL timesheet entries for this employee?\nThis cannot be undone.",
                     "Confirm Deletion", wxYES_NO | wxICON_WARNING) == wxYES) {
        // Clear timesheet entries from memory
        selected.timesheets.clear();

        // Delete CSV file from disk
        std::string file = selected.name + "_Entries.csv";
        std::remove(file.c_str());

        // Save updated state
        saveTimesheet(selected);

        wxMessageBox("All timesheet entries deleted.", "Success", wxOK | wxICON_INFORMATION);
    }
}, wxID_HIGHEST + 102);



    Bind(wxEVT_BUTTON, &TimeSheetFrame::OnLoadEmployees, this, ID_LoadEmployees);
    Bind(wxEVT_BUTTON, &TimeSheetFrame::OnSelectEmployee, this, ID_SelectEmployee);
    Bind(wxEVT_BUTTON, &TimeSheetFrame::OnCreateEmployee, this, wxID_HIGHEST + 1);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        int selection = employeeList->GetSelection();
        if (selection == wxNOT_FOUND) return;

        Employee& emp = employees[selection];

        wxFileDialog saveDialog(
            this,
            "Save Timesheet As",
            wxEmptyString,
            emp.name + "_Timesheet.xlsx",
            "Excel files (*.xlsx)|*.xlsx",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT
        );

        if (saveDialog.ShowModal() == wxID_OK) {
            std::string path = saveDialog.GetPath().ToStdString();
            exportTimesheetToXLSX(emp, path);
            saveTimesheetToCSV(emp);
            wxMessageBox("Exported to:\n" + saveDialog.GetPath(), "Success", wxOK | wxICON_INFORMATION);
        }
        }, wxID_HIGHEST + 2);

}

void TimeSheetFrame::OnQuit(wxCommandEvent& event) {
    Close(true);
}

void TimeSheetFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox("Timesheet Application GUI", "About", wxOK | wxICON_INFORMATION);
}


void TimeSheetFrame::OnLoadEmployees(wxCommandEvent& event) {
    RefreshEmployeeList();  // Clean and reload employee list
}


void showTimesheetEntries(Employee& emp) {
    wxDialog* dlg = new wxDialog(nullptr, wxID_ANY, "Timesheet Entries", wxDefaultPosition, wxSize(600, 600));
    dlg->CentreOnParent();
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // --- Search box ---
    wxTextCtrl* searchBox = new wxTextCtrl(dlg, wxID_ANY, "", wxDefaultPosition, wxSize(-1, -1));
    searchBox->SetHint("Filter by date (e.g., 05/01/2025)");

    // --- Sort choice ---
    wxArrayString sortOptions;
    sortOptions.Add("Sort by Date");
    sortOptions.Add("Sort by Job Number");
    sortOptions.Add("Sort by Job Name");

    wxChoice* sortChoice = new wxChoice(dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, sortOptions);
    sortChoice->SetSelection(0);  // Default sort

    // --- Combine search and sort horizontally ---
    wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);
    topRowSizer->Add(searchBox, 1, wxRIGHT | wxEXPAND, 5);
    topRowSizer->Add(sortChoice, 0, wxEXPAND);

    wxListCtrl* entryList = new wxListCtrl(dlg, wxID_ANY, wxDefaultPosition, wxSize(-1, 450), wxLC_REPORT | wxLC_SINGLE_SEL);


    entryList->InsertColumn(0, "Date");
    entryList->InsertColumn(1, "Job #");
    entryList->InsertColumn(2, "Name");
    entryList->InsertColumn(3, "Desc");
    entryList->InsertColumn(4, "Time In");
    entryList->InsertColumn(5, "Time Out");
    entryList->InsertColumn(6, "Hours");
    entryList->InsertColumn(7, "Lunch");
    entryList->InsertColumn(8, "Total");


    wxButton* editBtn = new wxButton(dlg, wxID_HIGHEST + 10, "Edit Selected Entry");

    std::function<void()> refreshEntries;

    refreshEntries = [=, &emp]() {
        entryList->DeleteAllItems();
        std::string filter = searchBox->GetValue().ToStdString();

        std::vector<WorkEntry> filtered = emp.timesheets;

        // --- Apply sorting based on selection ---
        int sortIndex = sortChoice->GetSelection();
        std::sort(filtered.begin(), filtered.end(), [=](const WorkEntry& a, const WorkEntry& b) {
            switch (sortIndex) {
            case 0: return a.date < b.date;
            case 1: return a.jobNumber < b.jobNumber;
            case 2:
                if (!a.locations.empty() && !b.locations.empty())
                    return a.locations[0] < b.locations[0];
                return false;
            default: return false;
            }
            });

        for (const auto& entry : filtered) {
            if (!filter.empty() && entry.date.find(filter) == std::string::npos)
                continue;

            size_t count = entry.locations.size();
            for (size_t i = 0; i < count; ++i) {
                std::string label = "Date: " + entry.date;
                label += " | Job Number: " + entry.jobNumber;

                if (i < entry.locations.size())
                    label += " | Name: " + entry.locations[i];

                if (i < entry.descriptions.size())
                    label += " | Desc: " + entry.descriptions[i];

                if (i < entry.timeIns.size() && i < entry.timeOuts.size()) {
                    label += " | Time In: " + entry.timeIns[i];
                    label += " | Time Out: " + entry.timeOuts[i];
                }

                if (i < entry.times.size())
                    label += " | " + std::to_string(entry.times[i]) + " hrs";

                if (i == 0) {
                    label += " | Lunch: " + std::to_string(entry.lunch);
                    label += " | Total: " + std::to_string(entry.totalHours);
                }

                long row = entryList->InsertItem(entryList->GetItemCount(), entry.date);
                entryList->SetItem(row, 1, entry.jobNumber);
                entryList->SetItem(row, 2, i < entry.locations.size() ? entry.locations[i] : "");
                entryList->SetItem(row, 3, i < entry.descriptions.size() ? entry.descriptions[i] : "");
                entryList->SetItem(row, 4, i < entry.timeIns.size() ? entry.timeIns[i] : "");
                entryList->SetItem(row, 5, i < entry.timeOuts.size() ? entry.timeOuts[i] : "");

                if (i < entry.times.size())
                    entryList->SetItem(row, 6, std::to_string(entry.times[i]));

                if (i == 0) {
                    entryList->SetItem(row, 7, std::to_string(entry.lunch));
                    entryList->SetItem(row, 8, std::to_string(entry.totalHours));
                }

                for (int col = 0; col < 9; ++col) {
                    entryList->SetColumnWidth(col, wxLIST_AUTOSIZE);
                }


            }
        }

        };

    searchBox->Bind(wxEVT_TEXT, [=](wxCommandEvent&) {
        refreshEntries();
        });

    sortChoice->Bind(wxEVT_CHOICE, [=](wxCommandEvent&) {
        refreshEntries();
        });

    editBtn->Bind(wxEVT_BUTTON, [=, &emp](wxCommandEvent&) {
        if (!isAdminLoggedIn) {
            wxMessageBox("You must be in Admin Mode to edit entries.", "Access Denied", wxOK | wxICON_WARNING);
            return;
        }

        long sel = entryList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND) {
            wxMessageBox("Please select an entry to edit.", "No Selection", wxOK | wxICON_WARNING);
            return;
        }

        WorkEntry& entry = emp.timesheets[sel];

        if (editTimesheetDialog(dlg, entry)) {
            saveTimesheet(emp);
            wxMessageBox("Entry updated successfully.", "Updated", wxOK | wxICON_INFORMATION);
            refreshEntries();
        }
        });


    sizer->Add(topRowSizer, 0, wxALL | wxEXPAND, 10);  // <- Updated line
    sizer->Add(entryList, 1, wxALL | wxEXPAND, 5);
    sizer->Add(editBtn, 0, wxALL | wxEXPAND, 5);
    editBtn->Show(isAdminLoggedIn);


    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(dlg, wxID_OK));
    btnSizer->Realize();
    sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

    dlg->SetSizer(sizer);
    refreshEntries();
    dlg->ShowModal();
    dlg->Destroy();
}

void TimeSheetFrame::showEditEmployeeDialog(Employee& emp){
    wxDialog dlg(this, wxID_ANY, "Edit Employee", wxDefaultPosition, wxSize(250, 250));
    dlg.CentreOnParent();
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxTextCtrl* nameField = new wxTextCtrl(&dlg, wxID_ANY);
    wxTextCtrl* idField = new wxTextCtrl(&dlg, wxID_ANY);
    wxTextCtrl* wageField = new wxTextCtrl(&dlg, wxID_ANY);

    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Name:"), 0, wxALL, 5);
    sizer->Add(nameField, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Employee ID:"), 0, wxALL, 5);
    sizer->Add(idField, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Wage:"), 0, wxALL, 5);
    sizer->Add(wageField, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(&dlg, wxID_OK));
    btnSizer->AddButton(new wxButton(&dlg, wxID_CANCEL));
    btnSizer->Realize();
    sizer->Add(btnSizer, 0, wxALL | wxALIGN_CENTER, 10);

    dlg.SetSizerAndFit(sizer);

    if (dlg.ShowModal() == wxID_OK) {
        std::string name = nameField->GetValue().ToStdString();
        std::string id = idField->GetValue().ToStdString();
        double wage = std::stod(wageField->GetValue().ToStdString());

        if (emp.id == id) {
            emp.name = name;
            emp.wage = wage;
            wxMessageBox("Employee updated successfully.", "Success", wxOK | wxICON_INFORMATION);
            return;
        }


        wxMessageBox("Employee ID not found.", "Error", wxOK | wxICON_ERROR);
    }
}



bool editTimesheetDialog(wxWindow* parent, WorkEntry& entry) {
    wxDialog dlg(parent, wxID_ANY, "Edit Timesheet Entry", wxDefaultPosition, wxSize(450, 500));
    dlg.CentreOnParent();
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Create a horizontal box for date field and spin
    wxBoxSizer* dateRow = new wxBoxSizer(wxHORIZONTAL);

    wxTextCtrl* dateField = new wxTextCtrl(&dlg, wxID_ANY, entry.date);
    wxSpinButton* spin = new wxSpinButton(&dlg, wxID_ANY, wxDefaultPosition, wxSize(20, -1), wxSP_VERTICAL);
    spin->Bind(wxEVT_SPIN_UP, [=](wxSpinEvent&) {
        wxDateTime dt;
        if (dt.ParseDate(dateField->GetValue())) {
            dt += wxDateSpan::Days(1);
            dateField->SetValue(dt.Format("%m/%d/%Y"));
        }
        });

    spin->Bind(wxEVT_SPIN_DOWN, [=](wxSpinEvent&) {
        wxDateTime dt;
        if (dt.ParseDate(dateField->GetValue())) {
            dt -= wxDateSpan::Days(1);
            dateField->SetValue(dt.Format("%m/%d/%Y"));
        }
        });

    spin->SetRange(-9999, 9999);  // Wide range just in case

    dateRow->Add(dateField, 1, wxRIGHT | wxEXPAND, 5);
    dateRow->Add(spin, 0);  // No margin, tight to field

    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Date (MM/DD/YYYY):"), 0, wxALL, 5);
    sizer->Add(dateRow, 0, wxALL | wxEXPAND, 5);
    dateField->Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
        wxDateTime dt;
        if (dt.ParseDate(dateField->GetValue())) {
            if (event.GetKeyCode() == WXK_UP) {
                dt += wxDateSpan::Days(1);  // Move forward 1 day
                dateField->SetValue(dt.Format("%m/%d/%Y"));
            }
            else if (event.GetKeyCode() == WXK_DOWN) {
                dt -= wxDateSpan::Days(1);  // Move backward 1 day
                dateField->SetValue(dt.Format("%m/%d/%Y"));
            }
            else {
                event.Skip();  // Let other keys through
            }
        }
        else {
            event.Skip();  // Don't block input if date is invalid
        }
        });



    // Auto-fill job number with "1"
    wxTextCtrl* jobField = new wxTextCtrl(&dlg, wxID_ANY, "1");

    // Bind Enter key to act like pressing OK
    jobField->Bind(wxEVT_KEY_DOWN, [&dlg](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
            dlg.EndModal(wxID_OK);  // Simulate clicking OK
        }
        else {
            event.Skip();
        }
        });
    wxTextCtrl* lunchField = new wxTextCtrl(&dlg, wxID_ANY, std::to_string(entry.lunch));

    wxString locStr, descStr, timeStr;
    for (size_t i = 0; i < entry.locations.size(); ++i) {
        locStr += entry.locations[i] + "; ";
        descStr += entry.descriptions[i] + "; ";
        timeStr += wxString::Format("%.2f; ", entry.times[i]);
    }

    wxString timeInStr, timeOutStr;
    for (size_t i = 0; i < entry.timeIns.size(); ++i) {
        timeInStr += wxString(entry.timeIns[i]) + "; ";
    }
    for (size_t i = 0; i < entry.timeOuts.size(); ++i) {
        timeOutStr += wxString(entry.timeOuts[i]) + "; ";
    }


    wxTextCtrl* locField = new wxTextCtrl(&dlg, wxID_ANY, locStr.Trim());
    wxTextCtrl* descField = new wxTextCtrl(&dlg, wxID_ANY, descStr.Trim());
    wxTextCtrl* timeField = new wxTextCtrl(&dlg, wxID_ANY, timeStr.Trim());
    wxTextCtrl* timeInField = new wxTextCtrl(&dlg, wxID_ANY, timeInStr.Trim());
    wxTextCtrl* timeOutField = new wxTextCtrl(&dlg, wxID_ANY, timeOutStr.Trim());



    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Job Number:"), 0, wxALL, 5);
    sizer->Add(jobField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Locations (semicolon-separated):"), 0, wxALL, 5);
    sizer->Add(locField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Descriptions (semicolon-separated):"), 0, wxALL, 5);
    sizer->Add(descField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Time In (semicolon-separated HH:MM):"), 0, wxALL, 5);
    sizer->Add(timeInField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Time Out (semicolon-separated HH:MM):"), 0, wxALL, 5);
    sizer->Add(timeOutField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Times (auto-computed):"), 0, wxALL, 5);
    sizer->Add(timeField, 0, wxALL | wxEXPAND, 5);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Lunch (hours):"), 0, wxALL, 5);
    sizer->Add(lunchField, 0, wxALL | wxEXPAND, 5);

    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(&dlg, wxID_OK));
    btnSizer->AddButton(new wxButton(&dlg, wxID_CANCEL));
    btnSizer->Realize();
    sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

    dlg.SetSizerAndFit(sizer);

    while (true) {
        if (dlg.ShowModal() != wxID_OK)
            return false;  // user canceled

        try {
            std::string job = jobField->GetValue().ToStdString();
            std::string upperJob;
            for (char c : job) upperJob += toupper(c);

            if (upperJob == "NA" || upperJob == "N/A" || upperJob == "N-A") {
                // valid
            }
            else {
                bool valid = true;
                for (char c : job) {
                    if (!isdigit(c) && !ispunct(c)) {
                        valid = false;
                        break;
                    }
                }

                if (!valid || job.empty()) {
                    wxMessageBox("Invalid job number.\nUse only digits and special characters (or NA/N-A/N/A).", "Invalid Input", wxOK | wxICON_ERROR);
                    continue;  // re-show the dialog with same values
                }
            }

            entry.date = dateField->GetValue().ToStdString();
            entry.jobNumber = job;
            entry.lunch = std::stod(lunchField->GetValue().ToStdString());

            auto safeSplit = [](const wxString& str) {
                std::vector<std::string> result;
                wxArrayString parts = wxSplit(str, ';');
                for (auto& s : parts) {
                    wxString trimmed = s.Trim();
                    if (!trimmed.IsEmpty())
                        result.push_back(std::string(trimmed.ToStdString()));
                }
                return result;
                };

            std::vector<std::string> locs = safeSplit(locField->GetValue());
            std::vector<std::string> descs = safeSplit(descField->GetValue());
            std::vector<std::string> timeStrs = safeSplit(timeField->GetValue());
            std::vector<std::string> timeIns = safeSplit(timeInField->GetValue());
            std::vector<std::string> timeOuts = safeSplit(timeOutField->GetValue());


            if (locs.size() != descs.size() || locs.size() != timeIns.size() ||
                locs.size() != timeOuts.size()) {
                wxMessageBox("Mismatch in number of locations, descriptions, time in/out.", "Invalid Entry", wxOK | wxICON_ERROR);
                entry = WorkEntry();
                continue;
            }


            entry = WorkEntry();
            entry.locations = locs;
            entry.descriptions = descs;
            entry.times.clear();
            entry.totalHours = 0;

            for (size_t i = 0; i < timeIns.size(); ++i) {
                double h = computeHours(timeIns[i], timeOuts[i]);
                if (h < 0) {
                    wxMessageBox("Invalid time range at index " + std::to_string(i + 1), "Time Error", wxOK | wxICON_ERROR);
                    entry = WorkEntry();
                    continue;
                }
                entry.timeIns.push_back(timeIns[i]);
                entry.timeOuts.push_back(timeOuts[i]);
                entry.times.push_back(h);
                entry.totalHours += h;
            }


            if (entry.lunch > entry.totalHours) {
                wxMessageBox("Lunch break cannot exceed total time.", "Error", wxOK | wxICON_ERROR);
                entry = WorkEntry();
                continue;
            }

            // Match CLI version: if total time is less than 5, lunch must be 0 or <= 0.5
            if (entry.totalHours < 5.0 && entry.lunch > 0.5) {
                wxMessageBox("For days under 5 hours, lunch must be 0 or up to 0.5 hours.", "Lunch Policy", wxOK | wxICON_WARNING);
                entry = WorkEntry();
                continue;
            }


            entry.totalHours -= entry.lunch;
            return true;  // success

        }
        catch (const std::exception& e) {
            wxMessageBox(wxString("Error parsing input: ") + e.what(), "Exception", wxOK | wxICON_ERROR);
            entry = WorkEntry();
            continue;  // stay in the loop
        }
    }


    return false;
}



void editTimesheetEntry(Employee& emp, size_t index) {
    WorkEntry& entry = emp.timesheets[index];
    double totalHours = 0;

    // Clone data in case the user cancels
    std::vector<std::string> newLocations = entry.locations;
    std::vector<std::string> newDescriptions = entry.descriptions;
    std::vector<double> newTimes = entry.times;
    std::string newJobNumber = entry.jobNumber;
    double newLunch = entry.lunch;

    for (size_t i = 0; i < newLocations.size(); ++i) {
        wxDialog dlg(nullptr, wxID_ANY, "Edit Location " + std::to_string(i + 1), wxDefaultPosition, wxSize(400, 400));
        dlg.CentreOnParent();

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxTextCtrl* locField = new wxTextCtrl(&dlg, wxID_ANY, newLocations[i]);
        wxTextCtrl* jobField = new wxTextCtrl(&dlg, wxID_ANY, newJobNumber);
        wxTextCtrl* descField = new wxTextCtrl(&dlg, wxID_ANY, newDescriptions[i]);
        wxTextCtrl* timeInField = new wxTextCtrl(&dlg, wxID_ANY, "08:00");
        wxTextCtrl* timeOutField = new wxTextCtrl(&dlg, wxID_ANY, "12:00");

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Location:"), 0, wxALL, 5);
        sizer->Add(locField, 0, wxALL | wxEXPAND, 5);
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Job Number:"), 0, wxALL, 5);
        sizer->Add(jobField, 0, wxALL | wxEXPAND, 5);
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Description:"), 0, wxALL, 5);
        sizer->Add(descField, 0, wxALL | wxEXPAND, 5);
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Time In (HH:MM):"), 0, wxALL, 5);
        sizer->Add(timeInField, 0, wxALL | wxEXPAND, 5);
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, "Time Out (HH:MM):"), 0, wxALL, 5);
        sizer->Add(timeOutField, 0, wxALL | wxEXPAND, 5);

        wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
        btnSizer->AddButton(new wxButton(&dlg, wxID_OK));
        btnSizer->AddButton(new wxButton(&dlg, wxID_CANCEL));
        btnSizer->Realize();
        sizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

        dlg.SetSizerAndFit(sizer);

        while (true) {
            if (dlg.ShowModal() == wxID_OK)
                break;

            int confirm = wxMessageBox(
                "Are you sure you want to cancel? Any input will be discarded.",
                "Cancel Entry",
                wxYES_NO | wxICON_WARNING,
                &dlg
            );

            if (confirm == wxYES) return;
            // else: loop continues and dialog is shown again
        }

        newLocations[i] = locField->GetValue().ToStdString();
        newDescriptions[i] = descField->GetValue().ToStdString();
        newJobNumber = jobField->GetValue().ToStdString();

        try {
            std::string timeIn = timeInField->GetValue().ToStdString();
            std::string timeOut = timeOutField->GetValue().ToStdString();
            double h = computeHours(timeIn, timeOut);

        }
        catch (...) {
            wxMessageBox("Invalid hours input.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        totalHours += newTimes[i];
    }

    // Ask for new lunch value
    wxTextEntryDialog lunchDlg(nullptr, "Edit Lunch Break (hours):", "Edit Lunch", std::to_string(newLunch));
    lunchDlg.CentreOnParent();

    lunchDlg.SetTextValidator(wxFILTER_NUMERIC);
    if (lunchDlg.ShowModal() != wxID_OK) return;

    try {
        newLunch = std::stod(lunchDlg.GetValue().ToStdString());
        if (newLunch < 0 || newLunch > totalHours) {
            wxMessageBox("Lunch must be between 0 and total worked hours.", "Error", wxOK | wxICON_ERROR);
            return;
        }
    }
    catch (...) {
        wxMessageBox("Invalid lunch input.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Update the actual entry
    entry.locations = newLocations;
    entry.descriptions = newDescriptions;
    entry.times = newTimes;
    entry.jobNumber = newJobNumber;
    entry.lunch = newLunch;
    entry.totalHours = totalHours - newLunch;

    saveTimesheet(emp);  // Save changes to disk
    wxMessageBox("Entry updated successfully.", "Success", wxOK | wxICON_INFORMATION);
}


void TimeSheetFrame::OnSelectEmployee(wxCommandEvent& event) {
    int selection = employeeList->GetSelection();
    if (selection == wxNOT_FOUND) return;

    Employee& selected = employees[selection];


    wxDialog* dlg = new wxDialog(this, wxID_ANY, "Manage Employee", wxDefaultPosition, wxSize(300, 250));
    dlg->CentreOnParent();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(dlg, wxID_ANY, "What would you like to do?");
    wxButton* btnEnter = new wxButton(dlg, wxID_HIGHEST + 2, "Enter Timesheet");
    wxButton* btnSelect = new wxButton(dlg, wxID_HIGHEST + 3, "View Timesheet");
    wxButton* btnDelete = new wxButton(dlg, wxID_HIGHEST + 4, "Delete Timesheet");
    wxButton* btnDeleteEmployee = new wxButton(dlg, wxID_HIGHEST + 5, "Delete Employee");
    wxButton* btnEdit = new wxButton(dlg, wxID_HIGHEST + 6, "Edit Employee");
    wxButton* btnBack = new wxButton(dlg, wxID_CANCEL, "Back");


    btnDelete->Show(isAdminLoggedIn);
    btnDeleteEmployee->Show(isAdminLoggedIn);
    btnEdit->Show(isAdminLoggedIn);

    sizer->Add(label, 0, wxALL | wxALIGN_CENTER, 10);
    sizer->Add(btnEnter, 0, wxALL | wxEXPAND, 5);
    sizer->Add(btnSelect, 0, wxALL | wxEXPAND, 5);
    sizer->Add(btnDelete, 0, wxALL | wxEXPAND, 5);
    sizer->Add(btnDeleteEmployee, 0, wxALL | wxEXPAND, 5);
    sizer->Add(btnEdit, 0, wxALL | wxEXPAND, 5);
    sizer->Add(btnBack, 0, wxALL | wxEXPAND, 5);


    dlg->SetSizer(sizer);

    // Lambdas now safely use dlg as a pointer
    btnEnter->Bind(wxEVT_BUTTON, [dlg, &selected](wxCommandEvent&) {
        showTimesheetDialog(selected);
        dlg->EndModal(wxID_OK);
        });

    btnSelect->Bind(wxEVT_BUTTON, [dlg, &selected](wxCommandEvent&) {
        loadTimesheet(selected);                // ✅ Load entries from file here
        showTimesheetEntries(selected);         // ✅ Now show the filtered viewer
        dlg->EndModal(wxID_OK);
        });

    btnEdit->Bind(wxEVT_BUTTON, [this, &selected, dlg](wxCommandEvent&) {
        this->showEditEmployeeDialog(selected);
        dlg->EndModal(wxID_OK);
        });



    btnDeleteEmployee->Bind(wxEVT_BUTTON, [dlg, this, &selected](wxCommandEvent&) {
        if (wxMessageBox("Are you sure you want to delete this employee and all timesheet data?",
            "Confirm Delete", wxYES_NO | wxICON_WARNING) == wxYES) {
            std::string xlsxFile = selected.name + "_Entries.xlsx";
            std::remove(xlsxFile.c_str());  // Delete XLSX file

            // Also remove from vector
            auto it = std::remove_if(employees.begin(), employees.end(),
                [&](const Employee& emp) { return emp.name == selected.name; });
            employees.erase(it, employees.end());

            saveEmployees();  // Update employees.txt
            RefreshEmployeeList();


            wxMessageBox("Employee deleted successfully.", "Deleted", wxOK | wxICON_INFORMATION);
            dlg->EndModal(wxID_OK);
        }
        });

    btnDelete->Bind(wxEVT_BUTTON, [dlg, this, &selected](wxCommandEvent&) {
        if (selected.timesheets.empty()) {
            wxMessageBox("No entries to delete.", "Info", wxOK | wxICON_INFORMATION);
            dlg->EndModal(wxID_OK);
            return;
        }

        wxArrayString entryList;
        for (size_t i = 0; i < selected.timesheets.size(); ++i) {
            const auto& entry = selected.timesheets[i];
            wxString label = wxString::Format("%zu. %s - %s", i + 1, entry.date, entry.jobNumber);
            entryList.Add(label);
        }

        wxSingleChoiceDialog choiceDlg(dlg, "Select an entry to delete:", "Delete Entry", entryList);

        if (choiceDlg.ShowModal() == wxID_OK) {
            int index = choiceDlg.GetSelection();
            if (index >= 0 && index < static_cast<int>(selected.timesheets.size())) {
                selected.timesheets.erase(selected.timesheets.begin() + index);

                // Save updated XLSX file
                saveTimesheet(selected);

                wxMessageBox("Entry deleted and file updated.", "Success", wxOK | wxICON_INFORMATION);
            }
        }

        dlg->EndModal(wxID_OK);
        });

    dlg->ShowModal();
    dlg->Destroy(); // Clean up after modal closes
}


void TimeSheetFrame::OnCreateEmployee(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter new employee name (First Last):", "Create Employee");
    dlg.CentreOnParent();


    while (true) {
        while (true) {
            if (dlg.ShowModal() == wxID_OK)
                break;

            int confirm = wxMessageBox(
                "Are you sure you want to cancel? Any input will be discarded.",
                "Cancel Entry",
                wxYES_NO | wxICON_WARNING,
                &dlg
            );

            if (confirm == wxYES) return;
            // else: loop continues and dialog is shown again
        }

        wxString name = dlg.GetValue().Trim().Trim(false);
        std::string stdName = name.ToStdString();

        if (stdName.empty() || stdName.find(' ') == std::string::npos) {
            wxMessageBox("Please enter both first and last name separated by a space.", "Invalid Input", wxOK | wxICON_ERROR);
            continue;
        }

        // Check for only letters and spaces
        bool valid = true;
        for (char c : stdName) {
            if (!isalpha(c) && c != ' ') {
                valid = false;
                break;
            }
        }

        if (!valid) {
            wxMessageBox("Name can only contain letters and spaces. No numbers or symbols.", "Invalid Input", wxOK | wxICON_ERROR);
            continue;
        }

        // Auto-capitalize each word
        std::stringstream ss(stdName);
        std::string part, capitalized;
        while (ss >> part) {
            part[0] = toupper(part[0]);
            for (size_t i = 1; i < part.length(); ++i) {
                part[i] = tolower(part[i]);
            }
            if (!capitalized.empty()) capitalized += " ";
            capitalized += part;
        }

        if (employeeExists(capitalized)) {
            wxMessageBox("Employee already exists.", "Error", wxOK | wxICON_ERROR);
            continue;
        }

        Employee emp;
        emp.name = capitalized;

        // 🔹 Prompt for Employee ID
        wxTextEntryDialog idDlg(this, "Enter a unique employee ID (e.g., 00123):", "Employee ID");
        idDlg.CentreOnParent();
        if (idDlg.ShowModal() != wxID_OK || idDlg.GetValue().IsEmpty()) {
            wxMessageBox("Employee ID is required.", "Invalid Input", wxOK | wxICON_ERROR);
            return;
        }
        emp.id = idDlg.GetValue().ToStdString();

        // 🔹 Prompt for Wage
        wxTextEntryDialog wageDlg(this, "Enter hourly wage (e.g., 25.00):", "Employee Wage");
        wageDlg.CentreOnParent();
        if (wageDlg.ShowModal() != wxID_OK) {
            wxMessageBox("Wage entry cancelled.", "Cancelled", wxOK | wxICON_WARNING);
            return;
        }

        try {
            emp.wage = std::stod(wageDlg.GetValue().ToStdString());
            if (emp.wage < 0) throw std::invalid_argument("Negative wage");
        }
        catch (...) {
            wxMessageBox("Invalid wage. Must be a positive number.", "Invalid Input", wxOK | wxICON_ERROR);
            return;
        }

        employees.push_back(emp);
        saveEmployees();
        RefreshEmployeeList();

        wxMessageBox("Employee added successfully.", "Success", wxOK | wxICON_INFORMATION);

    }
}

void showTimesheetDialog(Employee& emp) {
    loadTimesheet(emp);
    std::string savedDate = "";
    std::string savedLocCountStr = "";

    while (true) {

        wxDialog dlg(nullptr, wxID_ANY, "Enter Date", wxDefaultPosition, wxSize(450, 600));

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

// Step 1: Get today’s date if none saved
        std::time_t now = std::time(nullptr);
        std::tm* nowTm = std::localtime(&now);
        char todayStr[11];
        std::strftime(todayStr, sizeof(todayStr), "%m/%d/%Y", nowTm);

        // ⬇️ INSERT THIS BLOCK BELOW
        char defaultTimeStr[9]; // Format buffer for time like "08:00 AM"
        std::strftime(defaultTimeStr, sizeof(defaultTimeStr), "%I:%M %p", nowTm);  // 12-hour format with AM/PM

        std::strftime(todayStr, sizeof(todayStr), "%m/%d/%Y", nowTm);

        if (savedDate.empty())
            savedDate = todayStr;


        // Step 2: Date input box (with today's date prefilled)

        wxTextCtrl* dateInput = new wxTextCtrl(&dlg, wxID_ANY, savedDate);

        // Prevent highlight on dialog open
        wxTheApp->CallAfter([=]() {
            dateInput->SetInsertionPointEnd();   // Move cursor to end
            dateInput->SetSelection(-1, -1);     // Remove any selection
            dateInput->SetFocus();               // Ensure focus stays here
            });

        dateInput->Bind(wxEVT_CHAR_HOOK, [&dlg](wxKeyEvent& event) {
            if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
                dlg.EndModal(wxID_OK);  // Simulate OK button
            }
            else {
                event.Skip();
            }
            });


        dateInput->Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
            wxDateTime dt;
            if (dt.ParseDate(dateInput->GetValue())) {
                if (event.GetKeyCode() == WXK_UP) {
                    dt += wxDateSpan::Days(1);  // ↑ adds a day
                    dateInput->SetValue(dt.Format("%m/%d/%Y"));
                }
                else if (event.GetKeyCode() == WXK_DOWN) {
                    dt -= wxDateSpan::Days(1);  // ↓ subtracts a day
                    dateInput->SetValue(dt.Format("%m/%d/%Y"));
                }
                else {
                    event.Skip();  // Allow other keys through
                }
            }
            else {
                event.Skip();  // If invalid date, allow editing
            }
            });


        // Step 3 (Replaced): Add up/down spin control next to date field
        wxBoxSizer* dateRow = new wxBoxSizer(wxHORIZONTAL);

        wxSpinButton* spin = new wxSpinButton(&dlg, wxID_ANY, wxDefaultPosition, wxSize(20, -1), wxSP_VERTICAL);
        spin->SetRange(-9999, 9999);  // Arbitrary large range

        dateRow->Add(dateInput, 1, wxRIGHT | wxEXPAND, 5);
        dateRow->Add(spin, 0);  // Small up/down arrows

        // Spin button logic
        spin->Bind(wxEVT_SPIN_UP, [=](wxSpinEvent&) {
            wxDateTime dt;
            if (dt.ParseDate(dateInput->GetValue())) {
                dt += wxDateSpan::Days(1);
                dateInput->SetValue(dt.Format("%m/%d/%Y"));
            }
            });
        spin->Bind(wxEVT_SPIN_DOWN, [=](wxSpinEvent&) {
            wxDateTime dt;
            if (dt.ParseDate(dateInput->GetValue())) {
                dt -= wxDateSpan::Days(1);
                dateInput->SetValue(dt.Format("%m/%d/%Y"));
            }
            });


        // Create calendar picker button
        wxButton* calendarBtn = new wxButton(&dlg, wxID_ANY, "Select Date");
        calendarBtn->SetToolTip("Open calendar to choose date");

        calendarBtn->Bind(wxEVT_BUTTON, [dlgPtr = &dlg, dateInput](wxCommandEvent&) {
            wxDialog pickerDlg(dlgPtr, wxID_ANY, "Select Date", wxDefaultPosition, wxSize(250, 150));
            pickerDlg.CentreOnParent();

            wxBoxSizer* pickerSizer = new wxBoxSizer(wxVERTICAL);
            wxDatePickerCtrl* dateCtrl = new wxDatePickerCtrl(&pickerDlg, wxID_ANY, wxDefaultDateTime);
            pickerSizer->Add(dateCtrl, 0, wxALL | wxALIGN_CENTER, 10);

            wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
            btnSizer->AddButton(new wxButton(&pickerDlg, wxID_OK));
            btnSizer->AddButton(new wxButton(&pickerDlg, wxID_CANCEL));
            btnSizer->Realize();

            pickerSizer->Add(btnSizer, 0, wxALL | wxALIGN_CENTER, 10);
            pickerDlg.SetSizerAndFit(pickerSizer);

            if (pickerDlg.ShowModal() == wxID_OK) {
                wxDateTime selected = dateCtrl->GetValue();
                dateInput->SetValue(selected.Format("%m/%d/%Y"));
            }
            });

                sizer->Add(calendarBtn, 0, wxALIGN_CENTER | wxALL, 5);
                sizer->Add(dateRow, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

                wxWindow* top = wxTheApp->GetTopWindow();

        WorkEntry entry;  // Make sure this is moved here for every new loop attempt
        entry = WorkEntry();  // Ensure all fields are cleared on retry

        // === Step 1: Get valid, non-duplicate date ===
        while (true) {
            wxDialog dateDlg(nullptr, wxID_ANY, "Enter Date", wxDefaultPosition, wxSize(450, 150));
            wxBoxSizer* dateSizer = new wxBoxSizer(wxVERTICAL);

            wxDateTime initialDate;
            if (!initialDate.ParseDate(savedDate) || !initialDate.IsValid()) {
                initialDate = wxDateTime::Now();
            }

            wxDatePickerCtrl* datePicker = new wxDatePickerCtrl(&dateDlg, wxID_ANY, initialDate, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN | wxDP_SHOWCENTURY);
            dateSizer->Add(new wxStaticText(&dateDlg, wxID_ANY, "Select the date:"), 0, wxALL, 5);
            dateSizer->Add(datePicker, 0, wxALL | wxEXPAND, 5);


            wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
            btnSizer->AddButton(new wxButton(&dateDlg, wxID_OK));
            btnSizer->AddButton(new wxButton(&dateDlg, wxID_CANCEL));
            btnSizer->Realize();
            dateSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

            dateDlg.SetSizerAndFit(dateSizer);
            if (dateDlg.ShowModal() != wxID_OK) {
                int confirm = wxMessageBox("Are you sure you want to cancel entering the date?", "Cancel Entry", wxYES_NO | wxICON_WARNING);
                if (confirm == wxYES) return;
                else continue;
            }

            savedDate = datePicker->GetValue().Format("%m/%d/%Y").ToStdString();

            // Check for duplicate date
            bool duplicate = std::any_of(emp.timesheets.begin(), emp.timesheets.end(),
                [&](const WorkEntry& e) { return e.date == savedDate; });

            if (duplicate) {
                wxMessageBox("A timesheet for this date already exists.", "Duplicate Entry", wxOK | wxICON_WARNING);
                savedDate.clear();  // Force re-entry
                continue;
            }

            break;  // Valid and non-duplicate date entered
        }

        // === Step 2: Ask for location count ===
        std::string savedLocCountStr;
        while (true) {
            wxDialog locCountDlg(nullptr, wxID_ANY, "Location Count", wxDefaultPosition, wxDefaultSize);
            wxBoxSizer* locSizer = new wxBoxSizer(wxVERTICAL);
            wxSpinCtrl* locSpinCtrl = new wxSpinCtrl(&locCountDlg, wxID_ANY, "1", wxDefaultPosition, wxSize(80, -1));
            locSpinCtrl->SetRange(1, 6);
            locSizer->Add(locSpinCtrl, 0, wxALL | wxEXPAND, 10);


            wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
            btnSizer->AddButton(new wxButton(&locCountDlg, wxID_OK));
            btnSizer->AddButton(new wxButton(&locCountDlg, wxID_CANCEL));
            btnSizer->Realize();
            locSizer->Add(btnSizer, 0, wxALL | wxALIGN_CENTER, 10);

            locCountDlg.SetSizerAndFit(locSizer);

            if (locCountDlg.ShowModal() != wxID_OK) {
                int confirm = wxMessageBox("Are you sure you want to cancel?", "Cancel Entry", wxYES_NO | wxICON_WARNING);
                if (confirm == wxYES) return;
                else continue;
            }

            int locCount = locSpinCtrl->GetValue();
            savedLocCountStr = std::to_string(locCount);
            break;
        }



        if (savedDate.empty() || savedLocCountStr.empty()) {
            wxMessageBox("Date and number of locations are required.", "Error", wxOK | wxICON_ERROR);
            continue;  // Retry with saved input
        }

        int locCount = 0;
        try {
            locCount = std::stoi(savedLocCountStr);
            if (locCount <= 0 || locCount > 6)
                throw std::out_of_range("Must be between 1 and 6");
        }
        catch (...) {
            wxMessageBox("Invalid number of locations.\nYou can enter a maximum of 6 jobs per timesheet.", "Error", wxOK | wxICON_ERROR);
            continue;  // Retry
        }


        double total = 0;
        entry.date = savedDate;

        std::string prevLoc = "";
        std::string prevJob = "";
        std::string prevDesc = "";
        std::string prevTime = "";
        std::string prevLunch = "";

        for (int i = 0; i < locCount; ++i) {
            std::string prevLoc = "";
            std::string prevJob = "";
            std::string prevDesc = "";
            std::string prevTime = "";
            std::string prevLunch = "";

            bool locationValid = false;
            while (!locationValid) {
                wxDialog locDlg(nullptr, wxID_ANY, "Location " + std::to_string(i + 1), wxDefaultPosition, wxSize(400, 400));

                wxBoxSizer* innerSizer = new wxBoxSizer(wxVERTICAL);

                wxTextCtrl* locField = new wxTextCtrl(&locDlg, wxID_ANY, prevLoc);
                locField->SetHint("e.g. 123 Main St");

                wxTextCtrl* jobField = new wxTextCtrl(&locDlg, wxID_ANY, prevJob);
                jobField->SetHint("e.g. 9001, NA");

                wxTextCtrl* descField = new wxTextCtrl(&locDlg, wxID_ANY, prevDesc);
                descField->SetHint("e.g. Framing, Site Prep");


                // Time In layout
                wxBoxSizer* timeInSizer = new wxBoxSizer(wxHORIZONTAL);
                wxTextCtrl* timeInField = new wxTextCtrl(&locDlg, wxID_ANY, "08:00 AM", wxDefaultPosition, wxSize(100, -1));

                // Prevent format corruption from typing
                timeInField->Bind(wxEVT_CHAR, [=](wxKeyEvent& event) {
                    static int hourDigitCount = 0;
                    static int minuteDigitCount = 0;
                    static wxString hourBuffer = "";
                    static wxString minuteBuffer = "";

                    int key = event.GetKeyCode();
                    if (key >= '0' && key <= '9') {
                        char digit = static_cast<char>(key);
                        long start, end;
                        timeInField->GetSelection(&start, &end);
                        wxString current = timeInField->GetValue();

                        wxDateTime dt;
                        if (!dt.ParseTime(current)) return;

                        if (start >= 0 && start <= 2) {
                            hourBuffer += digit;
                            hourDigitCount++;

                            int previewHour = std::stoi(std::string(hourBuffer.mb_str()));
                            if (previewHour < 1) previewHour = 1;
                            if (previewHour > 12) previewHour = 12;

                            wxString newVal = wxString::Format("%02d:%s %s", previewHour, current.Mid(3, 2), current.Mid(6));
                            timeInField->SetValue(newVal);

                            if (hourDigitCount == 2) {
                                timeInField->SetSelection(3, 5);  // move to minutes
                                hourDigitCount = 0;
                                hourBuffer = "";
                            }
                            else {
                                timeInField->SetSelection(0, 2);  // stay in hour
                            }
                            return;
                        }
                        else if (start >= 3 && start <= 5) {
                            minuteBuffer += digit;
                            minuteDigitCount++;

                            int previewMinute = std::stoi(std::string(minuteBuffer.mb_str()));
                            if (previewMinute > 59) previewMinute = 59;

                            wxString newVal = wxString::Format("%s:%02d %s", current.Mid(0, 2), previewMinute, current.Mid(6));
                            timeInField->SetValue(newVal);

                            if (minuteDigitCount == 2) {
                                timeInField->SetSelection(6, 8);  // move to AM/PM
                                minuteDigitCount = 0;
                                minuteBuffer = "";
                            }
                            else {
                                timeInField->SetSelection(3, 5);  // stay in minute
                            }
                            return;
                        }
                    }

                    if (key == WXK_LEFT || key == WXK_RIGHT || key == WXK_TAB ||
                        key == WXK_RETURN || key == WXK_NUMPAD_ENTER || key == WXK_UP || key == WXK_DOWN) {
                        event.Skip();
                        return;
                    }

                    // Block all other characters
                    });




                int height = timeInField->GetSize().GetHeight();
                wxSpinButton* timeInSpin = new wxSpinButton(&locDlg, wxID_ANY, wxDefaultPosition, wxSize(20, height), wxSP_VERTICAL);

                timeInSpin->SetRange(-999, 999);
                timeInSizer->Add(timeInField, 1, wxRIGHT, 5);
                timeInSizer->Add(timeInSpin, 0, wxALIGN_CENTER_VERTICAL);


                // Handle spin
                timeInSpin->Bind(wxEVT_SPIN_UP, [=](wxSpinEvent&) {
                    wxString val = timeInField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) return;

                    long pos = timeInField->GetInsertionPoint();

                    if (pos >= 6) {
                        dt += wxTimeSpan(12);
                    }
                    else if (pos <= 2) {
                        dt += wxTimeSpan(1);
                    }
                    else {
                        dt += wxTimeSpan(0, 1);
                    }

                    timeInField->SetValue(dt.Format("%I:%M %p"));

                    timeInField->CallAfter([=]() {
                        if (pos <= 2)
                            timeInField->SetSelection(0, 2);
                        else if (pos <= 5)
                            timeInField->SetSelection(3, 5);
                        else
                            timeInField->SetSelection(6, 8);
                        });
                    });

                timeInSpin->Bind(wxEVT_SPIN_DOWN, [=](wxSpinEvent&) {
                    wxString val = timeInField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) return;

                    long pos = timeInField->GetInsertionPoint();

                    if (pos >= 6) {
                        dt += wxTimeSpan(12);
                    }
                    else if (pos <= 2) {
                        dt -= wxTimeSpan(1);
                    }
                    else {
                        dt -= wxTimeSpan(0, 1);
                    }

                    timeInField->SetValue(dt.Format("%I:%M %p"));

                    timeInField->CallAfter([=]() {
                        if (pos <= 2)
                            timeInField->SetSelection(0, 2);
                        else if (pos <= 5)
                            timeInField->SetSelection(3, 5);
                        else
                            timeInField->SetSelection(6, 8);
                        });
                    });


                timeInField->SetHint("e.g. 08:00");



                timeInField->Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
                    wxString val = timeInField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) {
                        event.Skip(); return;
                    }

                    long pos = timeInField->GetInsertionPoint();
                    bool up = event.GetKeyCode() == WXK_UP;
                    bool down = event.GetKeyCode() == WXK_DOWN;

                    if (up || down) {
                        if (pos >= 6) {
                            dt += wxTimeSpan(12);  // Toggle AM/PM
                        }
                        else if (pos <= 2) {
                            dt += wxTimeSpan(up ? 1 : -1);  // Hour
                        }
                        else {
                            dt += wxTimeSpan(0, up ? 1 : -1);  // Minute
                        }

                        timeInField->SetValue(dt.Format("%I:%M %p"));

                        timeInField->CallAfter([=]() {
                            if (pos <= 2)
                                timeInField->SetSelection(0, 2);     // hour
                            else if (pos <= 5)
                                timeInField->SetSelection(3, 5);     // minute
                            else
                                timeInField->SetSelection(6, 8);     // AM/PM
                            });
                    }
                    else {
                        event.Skip();
                    }
                    });





                    timeInField->Bind(wxEVT_CHAR, [](wxKeyEvent& event) {
                        int key = event.GetKeyCode();

                        // Allow only navigation keys
                        if (key == WXK_LEFT || key == WXK_RIGHT ||
                            key == WXK_TAB || key == WXK_RETURN || key == WXK_NUMPAD_ENTER) {
                            event.Skip();
                            return;
                        }

                        event.Skip(); // Let non-character keys (F1–F12, etc.) pass
                        });

                    timeInSpin->Bind(wxEVT_SPIN_UP, [=](wxSpinEvent&) {
                        wxString val = timeInField->GetValue();
                        wxDateTime dt;
                        if (!dt.ParseTime(val)) return;

                        long pos = timeInField->GetInsertionPoint();

                        if (pos >= 6) {
                            dt += wxTimeSpan(12);  // Toggle AM/PM
                        }
                        else if (pos <= 2) {
                            dt += wxTimeSpan(1);  // Increase hour
                        }
                        else {
                            dt += wxTimeSpan(0, 1);  // Increase minute
                        }

                        timeInField->SetValue(dt.Format("%I:%M %p"));

                        timeInField->CallAfter([=]() {
                            if (pos <= 2)
                                timeInField->SetSelection(0, 2);  // Hour
                            else if (pos <= 5)
                                timeInField->SetSelection(3, 5);  // Minute
                            else
                                timeInField->SetSelection(6, 8);  // AM/PM
                            });
                        });


                    timeInField->Bind(wxEVT_LEFT_UP, [=](wxMouseEvent& event) {
                        timeInField->CallAfter([=]() {
                            long pos = timeInField->GetInsertionPoint();

                            if (pos >= 6) {
                                // Toggle AM/PM
                                wxString val = timeInField->GetValue();
                                wxDateTime dt;
                                if (dt.ParseTime(val)) {
                                    // Add 12 hours to toggle AM/PM
                                    dt += wxTimeSpan(12);
                                    timeInField->SetValue(dt.Format("%I:%M %p"));
                                }
                            }
                            else if (pos <= 2) {
                                timeInField->SetSelection(0, 2);  // Hour
                            }
                            else {
                                timeInField->SetSelection(3, 5);  // Minute
                            }
                            });

                        event.Skip();
                        });



// Time Out layout
                wxBoxSizer* timeOutSizer = new wxBoxSizer(wxHORIZONTAL);

                // Create time out text field
                wxTextCtrl* timeOutField = new wxTextCtrl(&locDlg, wxID_ANY, "05:00 PM", wxDefaultPosition, wxSize(100, -1));

                timeOutField->Bind(wxEVT_CHAR, [=](wxKeyEvent& event) {
                    static int hourDigitCount = 0;
                    static int minuteDigitCount = 0;
                    static wxString hourBuffer = "";
                    static wxString minuteBuffer = "";

                    int key = event.GetKeyCode();
                    if (key >= '0' && key <= '9') {
                        char digit = static_cast<char>(key);
                        long start, end;
                        timeOutField->GetSelection(&start, &end);
                        wxString current = timeOutField->GetValue();

                        wxDateTime dt;
                        if (!dt.ParseTime(current)) return;

                        if (start >= 0 && start <= 2) {
                            hourBuffer += digit;
                            hourDigitCount++;

                            int previewHour = std::stoi(std::string(hourBuffer.mb_str()));
                            if (previewHour < 1) previewHour = 1;
                            if (previewHour > 12) previewHour = 12;

                            wxString newVal = wxString::Format("%02d:%s %s", previewHour, current.Mid(3, 2), current.Mid(6));
                            timeOutField->SetValue(newVal);

                            if (hourDigitCount == 2) {
                                timeOutField->SetSelection(3, 5);  // move to minutes
                                hourDigitCount = 0;
                                hourBuffer = "";
                            }
                            else {
                                timeOutField->SetSelection(0, 2);  // stay in hour
                            }
                            return;
                        }
                        else if (start >= 3 && start <= 5) {
                            minuteBuffer += digit;
                            minuteDigitCount++;

                            int previewMinute = std::stoi(std::string(minuteBuffer.mb_str()));
                            if (previewMinute > 59) previewMinute = 59;

                            wxString newVal = wxString::Format("%s:%02d %s", current.Mid(0, 2), previewMinute, current.Mid(6));
                            timeOutField->SetValue(newVal);

                            if (minuteDigitCount == 2) {
                                timeOutField->SetSelection(6, 8);  // move to AM/PM
                                minuteDigitCount = 0;
                                minuteBuffer = "";
                            }
                            else {
                                timeOutField->SetSelection(3, 5);  // stay in minute
                            }
                            return;
                        }
                    }

                    if (key == WXK_LEFT || key == WXK_RIGHT || key == WXK_TAB ||
                        key == WXK_RETURN || key == WXK_NUMPAD_ENTER || key == WXK_UP || key == WXK_DOWN) {
                        event.Skip();
                        return;
                    }

                    // Block all other characters
                    });





                // Controlled hour/minute change with up/down
                timeOutField->Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
                    wxString val = timeOutField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) {
                        event.Skip(); return;
                    }

                    long pos = timeOutField->GetInsertionPoint();
                    bool up = event.GetKeyCode() == WXK_UP;
                    bool down = event.GetKeyCode() == WXK_DOWN;

                    if (up || down) {
                        if (pos >= 6) {
                            dt += wxTimeSpan(12);  // Toggle AM/PM
                        }
                        else if (pos <= 2) {
                            dt += wxTimeSpan(up ? 1 : -1);  // Hour
                        }
                        else {
                            dt += wxTimeSpan(0, up ? 1 : -1);  // Minute
                        }

                        timeOutField->SetValue(dt.Format("%I:%M %p"));

                        timeOutField->CallAfter([=]() {
                            if (pos <= 2)
                                timeOutField->SetSelection(0, 2);     // hour
                            else if (pos <= 5)
                                timeOutField->SetSelection(3, 5);     // minute
                            else
                                timeOutField->SetSelection(6, 8);     // AM/PM
                            });
                    }
                    else {
                        event.Skip();
                    }
                    });






                // Get its height and use it for spin button
                int outHeight = timeOutField->GetSize().GetHeight();
                wxSpinButton* timeOutSpin = new wxSpinButton(&locDlg, wxID_ANY, wxDefaultPosition, wxSize(20, outHeight), wxSP_VERTICAL);

                timeOutSpin->SetRange(-999, 999);

                // Add both controls side by side
                timeOutSizer->Add(timeOutField, 1, wxRIGHT, 5);
                timeOutSizer->Add(timeOutSpin, 0, wxALIGN_CENTER_VERTICAL);



                timeOutSpin->Bind(wxEVT_SPIN_UP, [=](wxSpinEvent&) {
                    wxString val = timeOutField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) return;

                    long pos = timeOutField->GetInsertionPoint();

                    if (pos >= 6) {
                        dt += wxTimeSpan(12);  // AM/PM toggle
                    }
                    else if (pos <= 2) {
                        dt += wxTimeSpan(1);   // Hour
                    }
                    else {
                        dt += wxTimeSpan(0, 1);  // Minute
                    }

                    timeOutField->SetValue(dt.Format("%I:%M %p"));

                    timeOutField->CallAfter([=]() {
                        if (pos <= 2)
                            timeOutField->SetSelection(0, 2);
                        else if (pos <= 5)
                            timeOutField->SetSelection(3, 5);
                        else
                            timeOutField->SetSelection(6, 8);
                        });
                    });

                timeOutSpin->Bind(wxEVT_SPIN_DOWN, [=](wxSpinEvent&) {
                    wxString val = timeOutField->GetValue();
                    wxDateTime dt;
                    if (!dt.ParseTime(val)) return;

                    long pos = timeOutField->GetInsertionPoint();

                    if (pos >= 6) {
                        dt += wxTimeSpan(12);  // AM/PM toggle (same behavior)
                    }
                    else if (pos <= 2) {
                        dt -= wxTimeSpan(1);   // Hour
                    }
                    else {
                        dt -= wxTimeSpan(0, 1);  // Minute
                    }

                    timeOutField->SetValue(dt.Format("%I:%M %p"));

                    timeOutField->CallAfter([=]() {
                        if (pos <= 2)
                            timeOutField->SetSelection(0, 2);
                        else if (pos <= 5)
                            timeOutField->SetSelection(3, 5);
                        else
                            timeOutField->SetSelection(6, 8);
                        });
                    });


                timeOutField->Bind(wxEVT_LEFT_UP, [=](wxMouseEvent& event) {
                    timeOutField->CallAfter([=]() {
                        long pos = timeOutField->GetInsertionPoint();

                        if (pos >= 6) {
                            wxString val = timeOutField->GetValue();
                            wxDateTime dt;
                            if (dt.ParseTime(val)) {
                                dt += wxTimeSpan(12);  // Toggle AM/PM
                                timeOutField->SetValue(dt.Format("%I:%M %p"));
                            }
                        }
                        else if (pos <= 2) {
                            timeOutField->SetSelection(0, 2);  // Hour
                        }
                        else {
                            timeOutField->SetSelection(3, 5);  // Minute
                        }
                        });

                    event.Skip();
                    });




                wxTextCtrl* lunchField = nullptr;
                if (locCount == 1) {
                    lunchField = new wxTextCtrl(&locDlg, wxID_ANY, prevLunch);
                    lunchField->SetHint("e.g. 0.5");
                }

                // Add to dialog
                innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Location:"), 0, wxALL, 5);
                innerSizer->Add(locField, 0, wxALL | wxEXPAND, 5);
                innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Job Number:"), 0, wxALL, 5);
                innerSizer->Add(jobField, 0, wxALL | wxEXPAND, 5);
                innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Job Description:"), 0, wxALL, 5);
                innerSizer->Add(descField, 0, wxALL | wxEXPAND, 5);
                innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Time In (HH:MM):"), 0, wxALL, 5);
                innerSizer->Add(timeInSizer, 0, wxALL | wxEXPAND, 5);
                innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Time Out (HH:MM):"), 0, wxALL, 5);
                innerSizer->Add(timeOutSizer, 0, wxALL | wxEXPAND, 5);


                if (lunchField) {
                    innerSizer->Add(new wxStaticText(&locDlg, wxID_ANY, "Lunch Break (hours):"), 0, wxALL, 5);
                    innerSizer->Add(lunchField, 0, wxALL | wxEXPAND, 5);
                }

                wxStdDialogButtonSizer* innerBtns = new wxStdDialogButtonSizer();
                innerBtns->AddButton(new wxButton(&locDlg, wxID_OK));
                innerBtns->AddButton(new wxButton(&locDlg, wxID_CANCEL));
                innerBtns->Realize();
                innerSizer->Add(innerBtns, 0, wxALIGN_CENTER | wxALL, 10);

                locDlg.SetSizerAndFit(innerSizer);
                locDlg.CentreOnScreen();
                if (locDlg.ShowModal() != wxID_OK) {
                    int confirm = wxMessageBox(
                        "Are you sure you want to cancel this location entry? Any input will be discarded.",
                        "Cancel Location Entry",
                        wxYES_NO | wxICON_WARNING
                    );
                    if (confirm == wxYES) return;  // Exit the entire entry process
                    else continue;  // Re-show the same location dialog
                }

                // Save previous input (so it reloads on retry)
                prevLoc = locField->GetValue().ToStdString();
                prevJob = jobField->GetValue().ToStdString();
                std::string job = prevJob;
                std::string upperJob;
                for (char c : job) upperJob += toupper(c);

                // Allow only specific letter cases or pure special-character/digit combos
                if (upperJob == "NA" || upperJob == "N/A" || upperJob == "N-A") {
                    // valid
                }
                else {
                    bool valid = true;
                    for (char c : job) {
                        if (!isdigit(c) && !ispunct(c)) {
                            valid = false;
                            break;
                        }
                    }

                    if (!valid || job.empty()) {
                        wxMessageBox("Invalid job number.\nUse only digits and special characters (or NA/N-A/N/A).", "Invalid Input", wxOK | wxICON_ERROR);
                        continue; // force re-entry
                    }
                }

                prevDesc = descField->GetValue().ToStdString();

                // Force uppercase
                std::string timeIn = timeInField->GetValue().ToStdString();
                std::string timeOut = timeOutField->GetValue().ToStdString();

                std::transform(timeIn.begin(), timeIn.end(), timeIn.begin(), ::toupper);
                std::transform(timeOut.begin(), timeOut.end(), timeOut.begin(), ::toupper);

                std::regex timePattern(R"(^((0[1-9])|(1[0-2])):[0-5][0-9] (AM|PM)$)");

                if (!std::regex_match(timeIn, timePattern) || !std::regex_match(timeOut, timePattern)) {
                    wxMessageBox("Time must be in format HH:MM AM/PM (e.g. 08:00 AM)", "Invalid Time Format", wxOK | wxICON_ERROR);
                    entry = WorkEntry(); // Optional: discard partial entry
                    continue;
                }

                entry.timeIns.push_back(timeIn);
                entry.timeOuts.push_back(timeOut);

                double hours = computeHours(timeIn, timeOut);
                if (hours <= 0) {
                    wxMessageBox("Invalid time range. End must be after start.", "Time Error", wxOK | wxICON_ERROR);
                    entry = WorkEntry();
                    continue;
                }


                entry.locations.push_back(prevLoc);
                entry.jobNumber = prevJob;
                entry.descriptions.push_back(prevDesc);
                entry.times.push_back(hours);
                total += hours;


                if (lunchField) {
                    wxString lunchStr = lunchField->GetValue().Trim();
                    if (lunchStr.IsEmpty()) {
                        wxMessageBox("Lunch break is required.", "Error", wxOK | wxICON_ERROR);
                        entry = WorkEntry();
                        continue;
                    }

                    try {
                        entry.lunch = std::stod(lunchStr.ToStdString());

                        if (entry.totalHours < 5.0) {
                            if (entry.lunch > 0.5) {
                                wxMessageBox("Lunch must be 0 or up to 0.5 hours for days under 5 hours.", "Lunch Policy", wxOK | wxICON_WARNING);
                                entry = WorkEntry();
                                continue;
                            }
                        }
                        else {
                            if (entry.lunch <= 0.0) {
                                wxMessageBox("Lunch break is required for shifts 5 hours or longer.", "Lunch Required", wxOK | wxICON_ERROR);
                                entry = WorkEntry();
                                continue;
                            }
                        }
                    }
                    catch (...) {
                        wxMessageBox("Invalid lunch input.", "Error", wxOK | wxICON_ERROR);
                        entry = WorkEntry();
                        continue;
                    }
                }

                locationValid = true; // ✅ Success!

            }
        }

        if (locCount > 1) {
            wxTextEntryDialog lunchDlg(nullptr, "Enter Lunch Break (in hours):", "Lunch");
            lunchDlg.CentreOnScreen();  // Ensures lunch input is centered too
            lunchDlg.SetTextValidator(wxFILTER_NUMERIC);
            if (lunchDlg.ShowModal() != wxID_OK) continue;

            std::string lunchStr = lunchDlg.GetValue().ToStdString();
            std::string lunchStrClean = lunchStr;
            lunchStrClean.erase(std::remove_if(lunchStrClean.begin(), lunchStrClean.end(), ::isspace), lunchStrClean.end());

            if (lunchStrClean.empty()) {
                wxMessageBox("Lunch break is required.", "Error", wxOK | wxICON_ERROR);
                continue;
            }

            try {
                entry.lunch = std::stod(lunchStrClean);
                if (entry.lunch < 0.0 || entry.lunch > 4.0) {
                    throw std::out_of_range("Invalid lunch range.");
                }
            }
            catch (...) {
                wxMessageBox("Invalid lunch duration. Must be a number between 0 and 4.", "Error", wxOK | wxICON_ERROR);
                continue;
            }

        }

        entry.totalHours = total - entry.lunch;

        std::string summary = "Please verify your entry:\n\n";
        summary += "Date: " + entry.date + "\n";
        summary += "Job Number: " + entry.jobNumber + "\n\n";

        for (size_t i = 0; i < entry.locations.size(); ++i) {
            summary += "Location " + std::to_string(i + 1) + ": " + entry.locations[i] + "\n";
            summary += "  Description: " + entry.descriptions[i] + "\n";
            summary += "  Hours Worked: " + std::to_string(entry.times[i]) + "\n\n";
        }

        summary += "Lunch Break: " + std::to_string(entry.lunch) + " hours\n";
        summary += "Total Hours (after lunch): " + std::to_string(entry.totalHours) + "\n\n";

        // ? Show Yes/No prompt
        int confirm = wxMessageBox(summary, "Confirm Timesheet", wxYES_NO | wxICON_QUESTION);
        if (confirm != wxYES) {
            wxMessageBox("Entry discarded. Please re-enter your timesheet.", "Cancelled", wxOK | wxICON_INFORMATION);
            return; // Stop here — do not save
        }

        // ? Save only if confirmed
        emp.timesheets.push_back(entry);
        saveTimesheetToCSV(emp);
        wxMessageBox("Timesheet entry saved!\nTotal hours after lunch: " +
            std::to_string(entry.totalHours), "Success", wxOK | wxICON_INFORMATION);

        break;
    }
}

std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << vec[i];
    }
    return oss.str();
}


void TimeSheetFrame::RefreshEmployeeList() {
    employeeList->Clear();
    employees.clear();
    loadEmployees();
    for (const auto& emp : employees) {
        employeeList->Append(emp.name);
    }
}

void TimeSheetFrame::RefreshAdminUI() {
    createBtn->Show(isAdminLoggedIn);
    if (clearEntriesMenuItem) {
        clearEntriesMenuItem->Enable(isAdminLoggedIn);  // Just enable/disable instead of Show()
    }
    Layout();
}

