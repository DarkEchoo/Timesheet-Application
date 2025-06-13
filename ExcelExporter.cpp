#include <xlsxwriter.h>
#include "TimeSheetLogic.h"
#include <string>
#include <ctime>
#include <algorithm>
#define _CRT_SECURE_NO_WARNINGS

void exportTimesheetToXLSX(const Employee& emp, const std::string& filepath) {
    std::string filename = filepath;
    lxw_workbook* workbook = workbook_new(filename.c_str());
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, NULL);

    // Add 4-decimal formatting for money values
    lxw_format* money_format = workbook_add_format(workbook);
    format_set_num_format(money_format, "\"$\"#,##0.00");


    // Write headers
    worksheet_write_string(worksheet, 0, 0, "Date", NULL);
    worksheet_write_string(worksheet, 0, 1, "Job Number", NULL);
    worksheet_write_string(worksheet, 0, 2, "Locations", NULL);
    worksheet_write_string(worksheet, 0, 3, "Descriptions", NULL);
    worksheet_write_string(worksheet, 0, 4, "Time In", NULL);
    worksheet_write_string(worksheet, 0, 5, "Time Out", NULL);
    worksheet_write_string(worksheet, 0, 6, "Times", NULL);
    worksheet_write_string(worksheet, 0, 7, "Lunch", NULL);
    worksheet_write_string(worksheet, 0, 8, "Total Hours", NULL);
    worksheet_write_string(worksheet, 0, 9, "Total Hours This Week", NULL);
    worksheet_write_string(worksheet, 0, 10, "Overtime Hours", NULL);
    worksheet_write_string(worksheet, 0, 11, "Hourly Wage", NULL);
    worksheet_write_string(worksheet, 0, 12, "Daily Pay", NULL);
    worksheet_write_string(worksheet, 0, 13, "Weekly Pay", NULL);
    worksheet_write_string(worksheet, 0, 14, "Overtime Pay", NULL);




    // Write each entry

    std::vector<WorkEntry> sortedEntries = emp.timesheets;
    std::sort(sortedEntries.begin(), sortedEntries.end(), [](const WorkEntry& a, const WorkEntry& b) {
        std::tm ta = {}, tb = {};
        int ma, da, ya, mb, db, yb;

        if (sscanf_s(a.date.c_str(), "%d%*c%d%*c%d", &ma, &da, &ya) == 3 &&
            sscanf_s(b.date.c_str(), "%d%*c%d%*c%d", &mb, &db, &yb) == 3) {
            ta.tm_year = ya - 1900; ta.tm_mon = ma - 1; ta.tm_mday = da;
            tb.tm_year = yb - 1900; tb.tm_mon = mb - 1; tb.tm_mday = db;
            return std::mktime(&ta) < std::mktime(&tb);
        }
        return false;
        });

    double totalWeekHours = 0;
    time_t lastStartOfWeek = 0;

    int row = 1;
    for (const auto& entry : sortedEntries) {
        size_t count = entry.locations.size();

        // === WEEKLY TOTALS ===
        std::tm tm = {};
        int m, d, y;
        double overtimeHours = 0;

        if (sscanf_s(entry.date.c_str(), "%d%*c%d%*c%d", &m, &d, &y) == 3) {
            tm.tm_year = y - 1900;
            tm.tm_mon = m - 1;
            tm.tm_mday = d;

            time_t entryTime = mktime(&tm);
            std::tm baseTm = {};
            localtime_s(&baseTm, &entryTime);
            baseTm.tm_hour = baseTm.tm_min = baseTm.tm_sec = 0;

            time_t startOfWeek = mktime(&baseTm) - (baseTm.tm_wday * 86400);

            // Reset weekly hours if new week
            if (lastStartOfWeek != startOfWeek) {
                totalWeekHours = 0;
                lastStartOfWeek = startOfWeek;
            }

            totalWeekHours += entry.totalHours;
            overtimeHours = std::max(0.0, totalWeekHours - 40.0);
            if (overtimeHours > entry.totalHours) overtimeHours = entry.totalHours;
        }


        for (size_t i = 0; i < count; ++i) {
            worksheet_write_string(worksheet, row, 0, entry.date.c_str(), NULL);
            worksheet_write_string(worksheet, row, 1, entry.jobNumber.c_str(), NULL);
            worksheet_write_string(worksheet, row, 2, entry.locations[i].c_str(), NULL);
            worksheet_write_string(worksheet, row, 3, entry.descriptions[i].c_str(), NULL);
            worksheet_write_string(worksheet, row, 4, entry.timeIns[i].c_str(), NULL);
            worksheet_write_string(worksheet, row, 5, entry.timeOuts[i].c_str(), NULL);
            worksheet_write_number(worksheet, row, 6, entry.times[i], NULL);

            if (i == 0) {
                worksheet_write_number(worksheet, row, 7, entry.lunch, NULL);
                worksheet_write_number(worksheet, row, 8, entry.totalHours, NULL);
                worksheet_write_number(worksheet, row, 9, totalWeekHours, NULL);
                worksheet_write_number(worksheet, row, 10, overtimeHours, NULL);
                worksheet_write_number(worksheet, row, 11, emp.wage, money_format);
                worksheet_write_number(worksheet, row, 12, entry.totalHours * emp.wage, money_format);
                worksheet_write_number(worksheet, row, 13, totalWeekHours * emp.wage, money_format);
                worksheet_write_number(worksheet, row, 14, overtimeHours * emp.wage, money_format);
            }


            ++row;
        }
    }

    workbook_close(workbook); // ? properly finalize the Excel file
}

void saveTimesheet(const Employee& emp) {
    std::string fileName = emp.name + "_Entries.xlsx";
    exportTimesheetToXLSX(emp, fileName);
}
