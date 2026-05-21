#pragma once

#include <string>
#include <vector>


extern std::string adminPassword;
extern bool isAdminLoggedIn;
extern const std::string adminConfigFile;

struct WorkEntry {
    std::string date;
    std::string jobNumber;
    std::vector<std::string> locations;
    std::vector<std::string> descriptions;
    std::vector<std::string> timeIns;   // ? Correct: now these are tied to each entry
    std::vector<std::string> timeOuts;
    std::vector<double> times;
    double lunch = 0;
    double totalHours = 0;
    WorkEntry() = default;
};

struct Employee {
    std::string name;
    std::string id;
    double wage = 0.0;
    std::vector<WorkEntry> timesheets;
};

extern std::vector<Employee> employees;
void saveTimesheetToCSV(const Employee& emp);
bool isValidEmployeeName(const std::string& name);
bool isValidDate(const std::string& date);
bool isValidJobNumber(const std::string& jobNumber);
std::string getValidString(const std::string& prompt);
std::string getNonEmptyString(const std::string& prompt);
int getPositiveInt(const std::string& prompt);
double getPositiveDouble(const std::string& prompt);
double getValidLunchBreak(double totalTime);
double computeHours(const std::string& timeIn, const std::string& timeOut);
std::string join(const std::vector<std::string>& vec, const std::string& delimiter);
bool employeeExists(const std::string& name);
extern bool isAdminLoggedIn;



void displayMainMenu();
void addNewEmployee();
void selectEmployee();
void viewEmployees();
void deleteEntries();
void selectEntries();
void enterTimesheet(Employee& emp);
void displayTimesheet(const Employee& emp);

void saveEmployees();
void loadEmployees();
void saveTimesheet(const Employee& emp);
void loadTimesheet(Employee& emp);
void exportTimesheetToXLSX(const Employee& emp, const std::string& filepath);
void saveTimesheetToCSV(const Employee& emp);
void showEditEmployeeDialog(Employee& emp);


