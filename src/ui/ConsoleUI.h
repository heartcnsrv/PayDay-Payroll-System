#ifndef PAYDAY_CONSOLE_UI_H
#define PAYDAY_CONSOLE_UI_H

#include "../core/PayrollTypes.h"

/*
 * Console presentation layer.
 * These screens orchestrate the app flow by loading structs, collecting input,
 * calling the payroll engine, and saving results through CSVManager.
 */
void ui_clear  (void);
void ui_logo   (void);
void ui_hr     (void);
void ui_dhr    (void);
void ui_pause  (int ms);
void ui_success(const char* msg);
void ui_error  (const char* msg);
void ui_warn   (const char* msg);
void ui_info   (const char* msg);

void   get_line  (const char* prompt, char* out, int len);
double get_double(const char* prompt);
int    get_int   (const char* prompt);
void   press_enter(void);

void screen_login          (Employee* out, Employee* employees, int count,
                             const char* emp_path);
void screen_forgot_password(Employee* employees, int count, const char* emp_path);

void screen_admin_menu    (Employee* admin, Employee* employees, int* emp_count,
                            const char* emp_path, const char* pay_path);

void screen_employee_menu (Employee* emp, const char* pay_path,
                            Employee* employees, int emp_count);

void screen_employee_list  (Employee* employees, int count);
void screen_add_employee   (Employee* employees, int* count, const char* path);
void screen_edit_employee  (Employee* employees, int  count, const char* path);
void screen_deactivate_emp (Employee* employees, int  count, const char* path);
void screen_calc_payroll   (Employee* emp, const char* pay_path);
void screen_payroll_list   (int filter_emp_id, Employee* employees, int emp_count,
                             const char* pay_path);
void screen_payslip        (int payroll_id, Employee* employees, int emp_count,
                             const char* pay_path);
void screen_release_payroll(Employee* employees, int emp_count, const char* pay_path);
void screen_report         (Employee* employees, int emp_count, const char* pay_path);

#endif
