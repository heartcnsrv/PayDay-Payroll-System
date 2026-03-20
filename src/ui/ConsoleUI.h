#ifndef PAYDAY_CONSOLE_UI_H
#define PAYDAY_CONSOLE_UI_H

#include "../core/PayrollTypes.h"

/*
 * Console presentation layer.
 * Handles user interaction, screen display, and navigation.
 * Connects user input with backend logic (CSV + payroll processing).
 */

/* Clears the console screen */
void ui_clear(void);

/* Displays the system logo or title */
void ui_logo(void);

/* Displays a horizontal line separator */
void ui_hr(void);

/* Displays a double horizontal line separator */
void ui_dhr(void);

/* Pauses execution for a given number of milliseconds */
void ui_pause(int ms);

/* Displays a success message */
void ui_success(const char* msg);

/* Displays an error message */
void ui_error(const char* msg);

/* Displays a warning message */
void ui_warn(const char* msg);

/* Displays an informational message */
void ui_info(const char* msg);

/*
 * Reads a line of input from the user.
 * - prompt: message shown before input
 * - out: buffer to store input
 * - len: max length of input
 */
void get_line(const char* prompt, char* out, int len);

/*
 * Gets a double (decimal) input from the user.
 */
double get_double(const char* prompt);

/*
 * Gets an integer input from the user.
 */
int get_int(const char* prompt);

/* Waits for user to press Enter */
void press_enter(void);

/*
 * Login screen.
 * Authenticates user based on employee data.
 */
void screen_login(Employee* out, Employee* employees, int count,
                  const char* emp_path);

/*
 * Forgot password screen.
 * Allows user to recover or reset password.
 */
void screen_forgot_password(Employee* employees, int count, const char* emp_path);

/*
 * Admin dashboard menu.
 * Provides access to employee management and payroll functions.
 */
void screen_admin_menu(Employee* admin, Employee* employees, int* emp_count,
                       const char* emp_path, const char* pay_path);

/*
 * Employee dashboard menu.
 * Allows employees to view payroll and personal data.
 */
void screen_employee_menu(Employee* emp, const char* pay_path,
                          Employee* employees, int emp_count);

/*
 * Displays list of all employees.
 */
void screen_employee_list(Employee* employees, int count);

/*
 * Adds a new employee.
 * Updates the employee list and saves to CSV.
 */
void screen_add_employee(Employee* employees, int* count, const char* path);

/*
 * Edits existing employee details.
 */
void screen_edit_employee(Employee* employees, int count, const char* path);

/*
 * Deactivates an employee (soft delete).
 */
void screen_deactivate_emp(Employee* employees, int count, const char* path);

/*
 * Calculates payroll for a specific employee.
 */
void screen_calc_payroll(Employee* emp, const char* pay_path);

/*
 * Displays payroll records.
 * Can filter by employee ID.
 */
void screen_payroll_list(int filter_emp_id, Employee* employees, int emp_count,
                         const char* pay_path);

/*
 * Displays detailed payslip for a given payroll record.
 */
void screen_payslip(int payroll_id, Employee* employees, int emp_count,
                    const char* pay_path);

/*
 * Marks payroll as released to employees.
 */
void screen_release_payroll(Employee* employees, int emp_count, const char* pay_path);

/*
 * Generates payroll reports.
 */
void screen_report(Employee* employees, int emp_count, const char* pay_path);

#endif