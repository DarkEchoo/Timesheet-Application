#include <fstream>
#include <string>
#include "TimeSheetLogic.h"

void saveTimesheetToCSV(const Employee& emp) {
    std::string fileName = emp.name + "_Entries.csv";
    std::ofstream file(fileName);

    if (!file) return;

    // Write headers
    file << "Date,Job Number,Locations,Descriptions,Time In,Time Out,Times,Lunch,Total Hours\n";

    for (const auto& entry : emp.timesheets) {
        size_t count = entry.locations.size();
        for (size_t i = 0; i < count; ++i) {
            file << '"' << entry.date << "\","
                << '"' << entry.jobNumber << "\","
                << '"' << entry.locations[i] << "\","
                << '"' << entry.descriptions[i] << "\","
                << '"' << entry.timeIns[i] << "\","
                << '"' << entry.timeOuts[i] << "\","
                << '"' << entry.times[i] << "\","
                << ((i == 0) ? std::to_string(entry.lunch) : "") << ","   // only write lunch on first line
                << ((i == 0) ? std::to_string(entry.totalHours) : "") << "\n"; // only write total on first line
        }
    }



    file.close();
}
