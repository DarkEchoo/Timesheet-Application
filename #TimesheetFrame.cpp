#include "TimesheetFrame.h"
#include "TimeSheetApplication.cpp" // Your console logic reused here
#include "TimeSheetLogic.h"

enum {
    ID_Add = 1,
    ID_View = 2
};

wxBEGIN_EVENT_TABLE(TimesheetFrame, wxFrame)
EVT_BUTTON(ID_Add, TimesheetFrame::OnAddEmployee)
EVT_BUTTON(ID_View, TimesheetFrame::OnViewTimesheet)
wxEND_EVENT_TABLE()

TimesheetFrame::TimesheetFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400)) {

    wxPanel* panel = new wxPanel(this);
    employeeList = new wxListBox(panel, wxID_ANY, wxPoint(10, 10), wxSize(350, 300));
    addBtn = new wxButton(panel, ID_Add, "Add Employee", wxPoint(380, 10), wxSize(180, 30));
    viewBtn = new wxButton(panel, ID_View, "View Timesheet", wxPoint(380, 50), wxSize(180, 30));

    loadEmployees();
    RefreshEmployeeList();
}

void TimesheetFrame::RefreshEmployeeList() {
    employeeList->Clear();
    for (const auto& emp : employees) {
        employeeList->Append(emp.name);
    }
}

void TimesheetFrame::OnAddEmployee(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter employee name (First Last):", "Add Employee");
    if (dlg.ShowModal() == wxID_OK) {
        std::string name = dlg.GetValue().ToStdString();
        if (isValidEmployeeName(name)) {
            if (!employeeExists(name)) {
                Employee emp;
                emp.name = name;
                employees.push_back(emp);
                saveEmployees();
                RefreshEmployeeList();
                wxMessageBox("Employee added successfully.");
            }
            else {
                wxMessageBox("Employee already exists.");
            }
        }
        else {
            wxMessageBox("Invalid name. Use First Last format.");
        }
    }
}

void TimesheetFrame::OnViewTimesheet(wxCommandEvent& event) {
    int index = employeeList->GetSelection();
    if (index != wxNOT_FOUND) {
        Employee& emp = employees[index];
        loadTimesheet(emp);

        wxString output;
        for (const auto& entry : emp.timesheets) {
            output += "Date: " + entry.date + "\n";
            output += "Job #: " + entry.jobNumber + "\n";
            output += "Total Hours: " + wxString::Format("%.2f", entry.totalHours) + "\n\n";
        }

        wxMessageBox(output.IsEmpty() ? "No timesheet entries." : output, emp.name + "'s Timesheet");
    }
}
