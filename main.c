/* ============================================================
   PayDay | main.c
   Console application entry point.

   Data lives in data/employees.csv and data/payroll.csv.
   Edit those files directly to add/change seed data.
   ============================================================ */
#include "src/core/PayrollTypes.h"
#include "src/ui/ConsoleUI.h"
#include "src/utils/CSVManager.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <direct.h>
  #define mkdir_compat(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define mkdir_compat(p) mkdir(p, 0755)
#endif

int main(void) {
    mkdir_compat("data");

    const char* emp_path = "data/employees.csv";
    const char* pay_path = "data/payroll.csv";

    /* Create empty payroll CSV if it doesn't exist yet */
    FILE* pf = fopen(pay_path, "r");
    if (!pf) {
        pf = fopen(pay_path, "w");
        if (pf) {
            fprintf(pf,
                "id,employee_id,period_start,period_end,base_salary,"
                "overtime_hours,overtime_rate,overtime_pay,"
                "tax_deduction,sss_deduction,philhealth_deduction,"
                "pagibig_deduction,other_deductions,"
                "gross_pay,total_deductions,net_pay,generated_date,status\n");
            fclose(pf);
        }
    } else {
        fclose(pf);
    }

    /* Load employees from CSV */
    Employee employees[MAX_EMPLOYEES];
    int emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);

    if (emp_count == 0) {
        ui_clear(); ui_logo();
        printf("  ERROR: data/employees.csv not found or empty.\n");
        printf("  Place the employees.csv file in the data/ folder next to this program.\n\n");
        return 1;
    }

    /* Main loop — returns to login after every logout */
    for (;;) {
        emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);

        Employee current;
        memset(&current, 0, sizeof(current));
        screen_login(&current, employees, emp_count, emp_path);

        emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);

        if (current.role == ROLE_ADMIN)
            screen_admin_menu(&current, employees, &emp_count, emp_path, pay_path);
        else
            screen_employee_menu(&current, pay_path, employees, emp_count);
    }

    return 0;
}
