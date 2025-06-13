#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <fstream>
#include <conio.h>
#include <cstdlib>  // For system()

// Define CLEAR_SCREEN before including wxWidgets headers
#ifdef _WIN32
#define NOMINMAX    // Prevents macro collision with std::max
#include <windows.h>
#define CLEAR_SCREEN() system("cls")
#else
#define CLEAR_SCREEN() system("clear")
#endif

// wxWidgets inclusion (outside of platform-specific block)
#ifdef WX_PRECOMP
#include "wx/wxprec.h"
#else
#include "wx/wx.h"
#endif

using namespace std;



struct WorkEntry {
    string date;
    string jobNumber;
    string location;
    string description;
    vector<string> locations;
    vector<string> descriptions;
    vector<double> times;
    double lunch;
    double totalHours;
};


struct Employee {
    string name;
    vector<WorkEntry> timesheets;
};

vector<Employee> employees;

bool isValidEmployeeName(const string& name);

void displayMainMenu();
void addNewEmployee();
void selectEmployee();
void viewEmployees();
void deleteEntries();
void enterTimesheet(Employee& emp);
void displayTimesheet(const Employee& emp);
void selectEntries();
void saveEmployees();
void loadEmployees();
void saveTimesheet(const Employee& emp);
void loadTimesheet(Employee& emp);


int main() {
    loadEmployees();
    for (auto& emp : employees) {
        loadTimesheet(emp);
    }
    displayMainMenu();
    return 0;
}

bool isValidDate(const std::string& date) {
    // Identify the separator
    char separator = '\0';
    if (date.find('/') != std::string::npos) {
        separator = '/';
    }
    else if (date.find('-') != std::string::npos) {
        separator = '-';
    }
    else {
        return false; // Invalid format
    }

    // Split the date into components
    size_t firstSep = date.find(separator);
    size_t secondSep = date.find(separator, firstSep + 1);

    if (firstSep == std::string::npos || secondSep == std::string::npos) {
        return false; // Invalid format
    }

    std::string monthStr = date.substr(0, firstSep);
    std::string dayStr = date.substr(firstSep + 1, secondSep - firstSep - 1);
    std::string yearStr = date.substr(secondSep + 1);

    // Check that all parts are numeric
    if (monthStr.empty() || dayStr.empty() || yearStr.empty()) return false;
    if (!std::all_of(monthStr.begin(), monthStr.end(), ::isdigit)) return false;
    if (!std::all_of(dayStr.begin(), dayStr.end(), ::isdigit)) return false;
    if (!std::all_of(yearStr.begin(), yearStr.end(), ::isdigit)) return false;

    int month = std::stoi(monthStr);
    int day = std::stoi(dayStr);
    int year = std::stoi(yearStr);

    // Basic range checks
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (year < 1900 || year > 2100) return false;

    // Additional checks for months with fewer than 31 days
    if (month == 2) {
        bool isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        if (day > (isLeap ? 29 : 28)) return false;
    }
    else if (month == 4 || month == 6 || month == 9 || month == 11) {
        if (day > 30) return false;
    }

    return true;
}

bool isValidJobNumber(const string& jobNumber) {
    // Allow "NA" as a valid input
    if (jobNumber == "NA") return true;

    // Check each character: digits, dots, or special characters
    for (char c : jobNumber) {
        if (!isdigit(c) && c != '.' && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

string getValidString(const string& prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        if (!input.empty() && input.length() <= 240) {
            return input;
        }
        cout << "Invalid input. Please enter a non-empty value (max 240 characters)." << endl;
    }
}


int getPositiveInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value && value > 0) {
            cin.ignore();
            return value;
        }
        else {
            cout << "Invalid input. Please enter a positive number." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

void displayMainMenu() {
    int choice;
    while (true) {
        CLEAR_SCREEN();
        cout << "Main Menu:" << endl;
        cout << "1. Select Current Employee" << endl;
        cout << "2. View Employees" << endl;
        cout << "3. Delete Entries" << endl;
        cout << "4. Select Entries" << endl;
        cout << "5. Exit" << endl;
        cout << "Enter your choice: ";

        if (!(cin >> choice)) {
            // Clear the error state and discard invalid input
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            system("pause");
            continue;
        }

        switch (choice) {
        case 1:
            selectEmployee();
            break;
        case 2:
            viewEmployees();
            break;
        case 3:
            deleteEntries();
            break;
        case 4:
            selectEntries();
            break;
        case 5:
            return;
        default:
            cout << "Invalid choice, try again." << endl;
            system("pause");
        }
    }
}


double getPositiveDouble(const string& prompt) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= 0) {
            cin.ignore();
            return value;
        }
        else {
            cout << "Invalid input. Please enter a positive number or zero." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

string getNonEmptyString(const string& prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        if (!input.empty()) return input;
        cout << "Input cannot be empty. Please try again." << endl;
    }
}

double getValidLunchBreak(double totalTime) {
    double lunch;
    while (true) {
        lunch = getPositiveDouble("Lunch break (hours): ");
        if (lunch <= totalTime) return lunch;
        cout << "Lunch break cannot be greater than the total work time (" << totalTime << " hours)." << endl;
    }
}

bool employeeExists(const string& name) {
    for (const auto& emp : employees) {
        if (emp.name == name) return true;
    }
    return false;
}

void addNewEmployee() {
    CLEAR_SCREEN();
    Employee emp;
    bool validName = false;

    while (!validName) {
        cout << "Enter new employee name (First Last): ";
        cin >> ws;  // Clear leading whitespace
        getline(cin, emp.name);

        // Check if the input is non-empty and contains at least one space
        if (!emp.name.empty() && emp.name.find(' ') != string::npos) {
            if (!employeeExists(emp.name)) {
                validName = true;
            }
            else {
                cout << "Employee with this name already exists. Try a different name." << endl;
            }
        }
        else {
            cout << "Input cannot be empty or missing a last name. Please try again." << endl;
        }
    }

    employees.push_back(emp);
    saveEmployees();
    cout << "Employee added and saved." << endl;
    system("pause");
}

bool isValidEmployeeName(const string& name) {
    if (name.empty() || name.find(' ') == string::npos) return false;
    for (char c : name) {
        if (!isalpha(c) && c != ' ') return false;  // Only letters and spaces allowed
    }
    return true;
}


void selectEmployee() {
    CLEAR_SCREEN();
    if (employees.empty()) {
        cout << "No employees found. Add a new one." << endl;
        addNewEmployee();
        return;
    }
    cout << "Select an employee (0 to add new, ESC to go back):" << endl;
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << i + 1 << ". " << employees[i].name << endl;
    }
    int choice;
    cin >> choice;
    if (choice == -1) return;
    if (choice == 27) {
        addNewEmployee();
    }
    else if (choice > 0 && choice <= employees.size()) {
        enterTimesheet(employees[choice - 1]);
    }
    else {
        cout << "Invalid choice." << endl;
        system("pause");
    }
}

void viewEmployees() {
    CLEAR_SCREEN();
    if (employees.empty()) {
        cout << "No employees to view." << endl;
        system("pause");
        return;
    }

    while (true) {
        CLEAR_SCREEN();
        cout << "Employee List:" << endl;
        for (size_t i = 0; i < employees.size(); ++i) {
            cout << i + 1 << ". " << employees[i].name << endl;
        }

        int choice = getPositiveInt("\nSelect an employee (ESC to go back): ");
        if (choice == 27) return;

        if (choice > 0 && choice <= employees.size()) {
            Employee& selectedEmployee = employees[choice - 1];
            int action;
            while (true) {
                CLEAR_SCREEN();
                cout << "Selected Employee: " << selectedEmployee.name << endl;
                cout << "1. Modify Name" << endl;
                cout << "2. Delete Employee" << endl;
                cout << "3. Go Back" << endl;
                action = getPositiveInt("Choose an action: ");

                if (action == 1) {
                    cout << "Enter new name for " << selectedEmployee.name << " or press ESC to cancel: ";
                    string newName;

                    // Capture input and allow ESC to cancel
                    char ch = _getch();
                    if (ch == 27) {  // ESC key
                        cout << "\nModification canceled." << endl;
                        system("pause");
                        continue;  // Go back to the selected employee menu
                    }
                    else {
                        cin.putback(ch);  // Put the first character back into the stream
                        getline(cin, newName);

                        if (isValidEmployeeName(newName)) {
                            string oldFileName = selectedEmployee.name + "_Entries.csv";
                            string newFileName = newName + "_Entries.csv";
                            rename(oldFileName.c_str(), newFileName.c_str());
                            selectedEmployee.name = newName;
                            saveEmployees();
                            cout << "Employee name updated successfully." << endl;
                            system("pause");
                            break;  // Exit the action loop after successful modification
                        }
                        else {
                            cout << "Invalid name. Please enter both first and last name separated by a space." << endl;
                            system("pause");
                        }
                    }

                }
                else if (action == 2) {
                    cout << "Are you sure you want to delete " << selectedEmployee.name << "? (y/n): ";
                    char confirm;
                    cin >> confirm;
                    if (confirm == 'y' || confirm == 'Y') {
                        string fileName = selectedEmployee.name + "_Entries.csv";
                        if (remove(fileName.c_str()) == 0) {
                            cout << "Deleted timesheet file: " << fileName << endl;
                        }
                        else {
                            cout << "No timesheet file to delete for this employee." << endl;
                        }
                        employees.erase(employees.begin() + (choice - 1));
                        saveEmployees();
                        cout << "Employee deleted." << endl;
                        system("pause");
                        return;
                    }
                    else {
                        cout << "Deletion canceled." << endl;
                        system("pause");
                    }
                }
                else if (action == 3) {
                    break;
                }
                else {
                    cout << "Invalid choice. Please try again." << endl;
                    system("pause");
                }
            }
        }
        else {
            cout << "Invalid choice. Please try again." << endl;
            system("pause");
        }
    }
}

void deleteEntries() {
    CLEAR_SCREEN();
    if (employees.empty()) {
        cout << "No employees to clear timesheets for." << endl;
        system("pause");
        return;
    }

    // Display the list of employees before asking for input
    cout << "Employee List:" << endl;
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << i + 1 << ". " << employees[i].name << endl;
    }

    int choice;
    cout << "\nEnter the employee number to clear timesheets (ESC to go back): ";
    cin >> choice;
    if (choice == 27) return;

    if (choice > 0 && choice <= employees.size()) {
        string fileName = employees[choice - 1].name + "_Entries.csv";
        if (remove(fileName.c_str()) == 0) {
            cout << "Deleted timesheet file: " << fileName << endl;
            // Clear timesheets in memory as well
            employees[choice - 1].timesheets.clear();
            saveEmployees();
            cout << "Timesheet data cleared for " << employees[choice - 1].name << "." << endl;
        }
        else {
            cout << "No timesheet file to delete for this employee." << endl;
        }
    }
    else {
        cout << "Invalid choice." << endl;
    }
    system("pause");
}


void enterTimesheet(Employee& emp) {
    CLEAR_SCREEN();
    WorkEntry entry;
    bool validDate = false;

    // Prompt for Date
    while (!validDate) {
        cout << "Enter date (MM/DD/YYYY or MM-DD-YYYY): ";
        cin >> ws;
        getline(cin, entry.date);

        if (isValidDate(entry.date)) {
            validDate = true;
        }
        else {
            cout << "Invalid date format. Please use MM/DD/YYYY or MM-DD-YYYY." << endl;
        }
    }

    // Prompt for Job Number
    while (true) {
        cout << "Enter job number (or NA if not available): ";
        getline(cin, entry.jobNumber);
        if (entry.jobNumber.empty()) {
            entry.jobNumber = "NA";
        }

        if (isValidJobNumber(entry.jobNumber)) {
            break;
        }
        else {
            cout << "Invalid job number. Please enter only numbers or 'NA'." << endl;
        }
    }

    // Prompt for Work Locations and Times
    int locationCount = getPositiveInt("How many locations did you work at today? ");
    entry.totalHours = 0;
    for (int i = 0; i < locationCount; ++i) {
        string location = getValidString("Location " + to_string(i + 1) + ": ");
        entry.locations.push_back(location);

        string description = getValidString("Description: ");
        entry.descriptions.push_back(description);

        double time = getPositiveDouble("Time worked at this location (hours): ");
        entry.times.push_back(time);
        entry.totalHours += time;  // Accumulate total work time
    }

    // Calculate total time before lunch deduction
    cout << "Total work time (before lunch): " << entry.totalHours << " hours" << endl;

    // Prompt for Lunch Break (only after total time is calculated)
    entry.lunch = getValidLunchBreak(entry.totalHours);
    entry.totalHours -= entry.lunch;

    // Save and display total hours worked
    emp.timesheets.push_back(entry);
    saveTimesheet(emp);
    cout << "Total hours worked today (after lunch): " << entry.totalHours << endl;
    cout << "Timesheet saved." << endl;
    system("pause");
}

void selectEntries() {
    CLEAR_SCREEN();
    if (employees.empty()) {
        cout << "No employees available." << endl;
        system("pause");
        return;
    }

    int empIndex;
    cout << "Select an employee to view entries (ESC to go back):" << endl;
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << i + 1 << ". " << employees[i].name << endl;
    }
    cin >> empIndex;
    if (empIndex == 27) return;
    if (empIndex > 0 && empIndex <= employees.size()) {
        const Employee& emp = employees[empIndex - 1];
        displayTimesheet(emp);
    }
    else {
        cout << "Invalid choice." << endl;
        system("pause");
    }
}

void displayTimesheet(const Employee& emp) {
    CLEAR_SCREEN();
    if (emp.timesheets.empty()) {
        cout << "No timesheet entries for " << emp.name << "." << endl;
        system("pause");
        return;
    }

    cout << "Timesheet entries for " << emp.name << ":" << endl;
    for (size_t i = 0; i < emp.timesheets.size(); ++i) {
        const WorkEntry& entry = emp.timesheets[i];
        cout << "Entry " << i + 1 << " - Date: " << entry.date << endl;
        cout << "  Job Number: " << entry.jobNumber << endl;
        for (size_t j = 0; j < entry.locations.size(); ++j) {
            cout << "  Location " << j + 1 << ": " << entry.locations[j] << endl;
            cout << "    Description: " << entry.descriptions[j] << endl;
            cout << "    Time: " << entry.times[j] << " hours" << endl;
        }
        cout << "  Lunch break: " << entry.lunch << " hours" << endl;
        cout << "  Total hours worked: " << entry.totalHours << " hours" << endl;
    }
    system("pause");
}


void saveEmployees() {
    ofstream file("employees.txt");
    if (!file) {
        cout << "Error saving employees." << endl;
        return;
    }

    for (const auto& emp : employees) {
        file << emp.name << endl;
    }
    file.close();
}

void loadEmployees() {
    ifstream file("employees.txt");
    if (!file) {
        cout << "No existing employee file found. Starting fresh." << endl;
        return;
    }

    string name;
    while (getline(file, name)) {
        Employee emp;
        emp.name = name;
        employees.push_back(emp);
    }
    file.close();
}

void saveTimesheet(const Employee& emp) {
    string fileName = emp.name + "_Entries.csv";
    ofstream file(fileName);

    if (!file) {
        cout << "Error saving timesheet for " << emp.name << endl;
        return;
    }

    file << "Date,JobNumber,Locations,Descriptions,Times,Lunch,TotalHours\n";
    for (const auto& entry : emp.timesheets) {
        file << entry.date << "," << entry.jobNumber << ",";

        for (size_t i = 0; i < entry.locations.size(); ++i) {
            file << entry.locations[i];
            if (i < entry.locations.size() - 1) file << ";";
        }
        file << ",";

        for (size_t i = 0; i < entry.descriptions.size(); ++i) {
            file << entry.descriptions[i];
            if (i < entry.descriptions.size() - 1) file << ";";
        }
        file << ",";

        for (size_t i = 0; i < entry.times.size(); ++i) {
            file << entry.times[i];
            if (i < entry.times.size() - 1) file << ";";
        }

        file << "," << entry.lunch << "," << entry.totalHours << "\n";
    }
    file.close();
    cout << "Timesheet for " << emp.name << " saved successfully." << endl;
}

void loadTimesheet(Employee& emp) {
    string fileName = emp.name + "_Entries.csv";
    ifstream file(fileName);

    if (!file) {
        cout << "No existing timesheet for " << emp.name << "." << endl;
        return;
    }

    string line;
    getline(file, line);  // Skip the header line

    while (getline(file, line)) {
        WorkEntry entry;
        size_t pos = 0;
        vector<string> fields;

        while ((pos = line.find(',')) != string::npos) {
            fields.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        fields.push_back(line);

        if (fields.size() == 7) {
            entry.date = fields[0];
            entry.jobNumber = fields[1];

            string locString = fields[2];
            size_t locPos;
            while ((locPos = locString.find(';')) != string::npos) {
                entry.locations.push_back(locString.substr(0, locPos));
                locString.erase(0, locPos + 1);
            }
            entry.locations.push_back(locString);

            string descString = fields[3];
            size_t descPos;
            while ((descPos = descString.find(';')) != string::npos) {
                entry.descriptions.push_back(descString.substr(0, descPos));
                descString.erase(0, descPos + 1);
            }
            entry.descriptions.push_back(descString);

            string timeString = fields[4];
            size_t timePos;
            while ((timePos = timeString.find(';')) != string::npos) {
                entry.times.push_back(stod(timeString.substr(0, timePos)));
                timeString.erase(0, timePos + 1);
            }
            entry.times.push_back(stod(timeString));

            entry.lunch = stod(fields[5]);
            entry.totalHours = stod(fields[6]);
            emp.timesheets.push_back(entry);
        }
    }
    file.close();
    cout << "Timesheet for " << emp.name << " loaded successfully." << endl;
}