#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <fstream>
#include <conio.h>
#include <cstdlib>  // For system()
#include <xlsxwriter.h>
#include "TimeSheetLogic.h"
#include <sstream>





// Define CLEAR_SCREEN before including wxWidgets headers
#ifdef _WIN32
#define NOMINMAX    // Prevents macro collision with std::max
#include <windows.h>
#define CLEAR_SCREEN() system("cls")
#else
#define CLEAR_SCREEN() system("clear")
#endif

using namespace std;

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
    int choice = -1;
    cout << "Enter your choice (ESC to go back): ";
    char ch = _getch();

    if (ch == 27) {
        return;  // ESC key pressed
    }
    else if (isdigit(ch)) {
        choice = ch - '0';  // Convert char digit to int
    }
    else {
        cout << "\nInvalid input.\n";
        system("pause");
        return;
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

        if (choice > 0 && choice <= static_cast<int>(employees.size())) {
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

    if (choice > 0 && choice <= static_cast<int>(employees.size())) {
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
    loadTimesheet(emp);

    // Everything inside this loop is before pushing the entry
    while (true) {
        entry = WorkEntry();  // ? Reset the object at the top of loop

        // Step 1: Ask for date
        cout << "Enter date (MM/DD/YYYY or MM-DD-YYYY): ";
        getline(cin, entry.date);

        if (!isValidDate(entry.date)) {
            cout << "Invalid date format. Please try again." << endl;
            continue;
        }

        bool duplicate = std::any_of(emp.timesheets.begin(), emp.timesheets.end(),
            [&](const WorkEntry& e) { return e.date == entry.date; });

        if (duplicate) {
            cout << "A timesheet for this date already exists. Please choose another date." << endl;
            continue;
        }

        // Step 2: Get number of locations
        int locationCount = getPositiveInt("How many locations did you work at today? ");

        entry.totalHours = 0;

        for (int i = 0; i < locationCount; ++i) {
            cout << "\n--- Location " << (i + 1) << " ---" << endl;

            std::string location = getValidString("Location: ");
            std::string jobNumber = getValidString("Job Number (or NA): ");
            std::string description = getValidString("Job Description: ");
            std::string timeIn = getValidString("Time In (HH:MM): ");
            std::string timeOut = getValidString("Time Out (HH:MM): ");
            double hours = computeHours(timeIn, timeOut);
            if (hours <= 0) {
                cout << "Invalid time range. Start over.\n";
                continue;
            }
            entry.timeIns.push_back(timeIn);
            entry.timeOuts.push_back(timeOut);

            if (hours > 16) {
                cout << "Hours too high. Start over.\n";
                entry.locations.clear();
                entry.descriptions.clear();
                entry.times.clear();
                entry.totalHours = 0;
                break;  // This breaks the location loop; control continues to outer while
            }

            entry.locations.push_back(location);
            entry.descriptions.push_back(description);
            entry.times.push_back(hours);
            entry.totalHours += hours;
            entry.jobNumber = jobNumber;  // Assuming it's same for all locations that day
        }

        cout << "Total hours before lunch: " << entry.totalHours << endl;
        entry.lunch = getValidLunchBreak(entry.totalHours);

        if (entry.totalHours < 5 && entry.lunch > 0.5) {
            cout << "Lunch must be 0 or up to 0.5 hours for days under 5 hours.\n";
            continue;
        }

        entry.totalHours -= entry.lunch;

        // ? VALIDATED — now commit it
        emp.timesheets.push_back(entry);
        saveTimesheet(emp);

        cout << "Total hours after lunch: " << entry.totalHours << endl;
        cout << "Timesheet saved.\n";
        system("pause");
        break;  // exit loop after success
    }
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
    if (empIndex > 0 && empIndex <= static_cast<int>(employees.size())) {
        const Employee& emp = employees[empIndex - 1];
        displayTimesheet(emp);
    }
    else {
        cout << "Invalid choice." << endl;
        system("pause");
    }
}

double computeHours(const std::string& timeIn, const std::string& timeOut) {
    int h1, m1, h2, m2;
    char ampm1[4] = { 0 };  // Extra byte for null-termination
    char ampm2[4] = { 0 };


    if (sscanf_s(timeIn.c_str(), "%d:%d %2s", &h1, &m1, ampm1) != 3 ||
        sscanf_s(timeOut.c_str(), "%d:%d %2s", &h2, &m2, ampm2) != 3) {
        std::cerr << "Invalid time format (expected HH:MM AM/PM).\n";
        return -1;
    }

    // Convert to 24-hour format
    if (_stricmp(ampm1, "PM") == 0 && h1 != 12) h1 += 12;
    if (_stricmp(ampm1, "AM") == 0 && h1 == 12) h1 = 0;
    if (_stricmp(ampm2, "PM") == 0 && h2 != 12) h2 += 12;
    if (_stricmp(ampm2, "AM") == 0 && h2 == 12) h2 = 0;


    double start = h1 + m1 / 60.0;
    double end = h2 + m2 / 60.0;
    double diff = end - start;

    if (diff <= 0) return -1;  // Invalid time range
    return diff;
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
        file << emp.name << "," << emp.id << "," << emp.wage << endl;
    }
    file.close();
}

void loadEmployees() {
    ifstream file("employees.txt");
    if (!file) {
        cout << "No existing employee file found. Starting fresh." << endl;
        return;
    }

    employees.clear();  // Clear list before reloading

std::string line;
while (getline(file, line)) {
    std::stringstream ss(line);
    std::string name, id, wageStr;

    getline(ss, name, ',');
    getline(ss, id, ',');
    getline(ss, wageStr);

    Employee emp;
    emp.name = name;
    emp.id = id;
    emp.wage = wageStr.empty() ? 0.0 : std::stod(wageStr);

    employees.push_back(emp);
}

}



void loadTimesheet(Employee& emp) {
    emp.timesheets.clear();

    std::string filename = emp.name + "_Entries.csv";
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);  // Skip header line

    while (std::getline(file, line)) {
        WorkEntry entry;

        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        bool insideQuotes = false;
        std::string temp;

        // Handle comma splitting while respecting quotes
        for (char c : line) {
            if (c == '"') {
                insideQuotes = !insideQuotes;
            }
            else if (c == ',' && !insideQuotes) {
                fields.push_back(temp);
                temp.clear();
            }
            else {
                temp += c;
            }
        }
        fields.push_back(temp);  // Add final field

        if (fields.size() < 9) continue;  // Skip invalid lines

        entry.date = fields[0];
        entry.jobNumber = fields[1];

        auto split = [](const std::string& s) {
            std::vector<std::string> result;
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ';')) {
                std::string trimmed;
                for (char c : item) {
                    if (!std::isspace(c)) trimmed += c;
                }
                if (!trimmed.empty()) result.push_back(trimmed);
            }
            return result;
            };

        entry.locations = split(fields[2]);
        entry.descriptions = split(fields[3]);
        entry.timeIns = split(fields[4]);
        entry.timeOuts = split(fields[5]);

        std::vector<std::string> timeStrs = split(fields[6]);
        for (const std::string& t : timeStrs) {
            try {
                entry.times.push_back(std::stod(t));
            }
            catch (...) {
                entry.times.push_back(0);
            }
        }

        try {
            entry.lunch = std::stod(fields[7]);
            entry.totalHours = std::stod(fields[8]);
        }
        catch (...) {
            entry.lunch = 0;
            entry.totalHours = 0;
        }

        emp.timesheets.push_back(entry);
    }

    file.close();
}

