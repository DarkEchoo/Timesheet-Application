Timesheet Generator
A GUI and CLI-based timesheet management tool that allows employees to log daily work hours across multiple job locations and export timesheet data to CSV or Excel. Admins can manage employee data and enforce time entry validation policies.

Features
General
Add, edit, and view employees

Track work entries with multiple job locations per day

Validate input formats and lunch break policies

Automatically calculates total and overtime hours

GUI (wxWidgets-based)
Admin login system for secure access to editing features

Employee management (create, edit, delete)

Entry filtering and sorting (by date, job number, or job name)

View, edit, and export timesheets to .xlsx and .csv

Visual weekly summary including total and overtime hours

CLI (Console)
Create/select employees

Enter timesheets interactively

View, delete, and manage entries

Validate time and date formats

Stores data as CSV locally

Technologies Used
C++17

wxWidgets (GUI)

libxlsxwriter (Excel export)

Standard C++ STL (file I/O, vector, string, etc.)

File Structure
File	Description
TimeSheet.cpp	CLI-based application logic and entry management
TimeSheetApp.cpp	Main GUI application logic
TimesheetFrame.h	GUI window frame declaration
CSVExporter.cpp	Exports timesheet entries to CSV
ExcelExporter.cpp	Exports timesheet entries to Excel (with pay info)
TimeSheetLogic.h	Shared structures like Employee, WorkEntry, etc.

Build Instructions
Windows (Visual Studio)
Install wxWidgets and libxlsxwriter

Set up include and linker paths:

Include directories: wxWidgets/include, libxlsxwriter/include

Linker: libxlsxwriter.lib, wxWidgets core libraries

Build the project using Visual Studio C++ (x64 recommended)

Admin Mode
First-time setup prompts for an admin password

Enables:

Creating new employees

Editing existing timesheets

Deleting entries and employees

Clearing all entries for a user

Password is saved in a hidden config file: admin.config

Export Details
CSV (<EmployeeName>_Entries.csv)
Includes:

Date

Job Number

Locations (semicolon-separated)

Descriptions

Time In/Out

Lunch break

Total Hours

Excel (<EmployeeName>_Timesheet.xlsx)


Adds:

Weekly hours tracking

Overtime calculations

Hourly wage

Daily and weekly pay summaries

Usage Notes
Time format: HH:MM (24-hour)

Date format: MM/DD/YYYY or MM-DD-YYYY

Max locations per day: 6

Lunch required: For shifts ≥ 5 hours

Lunch limit: For shifts < 5 hours, lunch must be 0 or ≤ 0.5 hours

Author
Jesse Earl – 2025
Feel free to reach out for contributions or suggestions.

## Acknowledgements

This application uses [wxWidgets](https://www.wxwidgets.org/), a C++ GUI library licensed under the wxWidgets Library License.

https://www.wxwidgets.org/about/licence/

This application uses [libxlsxwriter](https://libxlsxwriter.github.io/), a C library for creating Excel `.xlsx` files, licensed under the BSD 2-Clause License.

See `THIRD_PARTY_LICENSES/libxlsxwriter_LICENSE.txt` for full license.
See `THIRD_PARTY_LICENSES/wxWidgets_LICENSE.txt` for full license.

