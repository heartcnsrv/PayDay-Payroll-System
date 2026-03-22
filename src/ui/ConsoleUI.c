#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

/* ============================================================

   ADMIN MENU:
     [1] Employee List
     [2] Add Employee
     [3] Edit Employee
     [4] Deactivate Employee
     [5] Calculate Salary / Overtime / Deductions
     [6] Payroll Records  (view payslip + release)
     [7] Payroll Report
     [0] Logout

   EMPLOYEE MENU:
     [1] My Payroll Records  (view payslip)
     [0] Logout

   LOGIN:  enter [F] for forgot password
   ============================================================ */

#include "ConsoleUI.h"
#include "../core/PayrollEngine.h"
#include "../utils/CSVManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Console workflow layer.
 * It does not own persistence itself; instead it calls CSVManager whenever a
 * user action should read or save Employee/PayrollRecord structs.
 */
#ifdef _WIN32
  #include <windows.h>
  #define CLEAR "cls"
#else
  #define _POSIX_C_SOURCE 200112L
  #include <unistd.h>
  #define CLEAR "clear"
#endif

#define R   "\033[0m"
#define BLD "\033[1m"
#define GRN "\033[92m"
#define YLW "\033[93m"
#define PNK "\033[95m"
#define CYN "\033[96m"
#define GRY "\033[90m"
#define RED "\033[91m"
#define WHT "\033[97m"

void ui_clear  (void) { system(CLEAR); }
void ui_hr     (void) { printf(PNK "  --------------------------------------------------\n" R); }
void ui_dhr    (void) { printf(PNK "  --------------------------------------------------\n" R); }
void ui_success(const char* m) { printf(GRN "\n  [OK] %s\n" R, m); }
void ui_error  (const char* m) { printf(RED "\n  [ERR] %s\n" R, m); }
void ui_warn   (const char* m) { printf(YLW "\n  [WARN] %s\n" R, m); }
void ui_info   (const char* m) { printf(CYN "\n  [INFO] %s\n" R, m); }

void ui_pause(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

void ui_logo(void) {
    printf("\n");
    printf(GRY "   Employee Payroll System\n\n" R);
}

void get_line(const char* prompt, char* out, int len) {
    printf(CYN "  %s" WHT, prompt);
    fflush(stdout);
    out[0] = '\0';
    if (fgets(out, len, stdin)) {
        int l = (int)strlen(out);
        if (l > 0 && out[l-1] == '\n') out[l-1] = '\0';
    }
    printf(R);
}

double get_double(const char* prompt) {
    char b[32]; get_line(prompt, b, sizeof(b)); return atof(b);
}
int get_int(const char* prompt) {
    char b[16]; get_line(prompt, b, sizeof(b)); return atoi(b);
}
void press_enter(void) {
    char b[4]; get_line("Press Enter to continue...", b, sizeof(b));
}

static void phdr(const char* title, const char* sub) {
    ui_clear(); ui_logo(); ui_dhr();
    printf(WHT BLD "  %s\n" R, title);
    if (sub && sub[0]) printf(GRY "  %s\n" R, sub);
    ui_hr();
}

static int confirm_yn(const char* msg) {
    char b[8];
    printf(YLW "\n  %s [y/N]: " WHT, msg);
    fflush(stdout);
    if (fgets(b, sizeof(b), stdin)) return b[0]=='y'||b[0]=='Y';
    return 0;
}

void screen_forgot_password(Employee* employees, int count, const char* emp_path) {
    /* Finds the matching Employee struct, updates its password, then saves CSV. */
    phdr("Forgot Password", "Reset your password using your email");

    char username[64], email[128];
    get_line("Username : ", username, sizeof(username));
    get_line("Email    : ", email,    sizeof(email));

    int found = -1;
    for (int i = 0; i < count; i++) {
        if (employees[i].active &&
            strcmp(employees[i].username, username) == 0 &&
            strcmp(employees[i].email,    email)    == 0) {
            found = i; break;
        }
    }
    if (found < 0) {
        ui_error("No account found with that username and email.");
        ui_pause(1500); return;
    }

    char token[16];
    payroll_generate_token(token, sizeof(token));
    ui_hr();
    printf(WHT "  Reset code: " GRN BLD "%s\n" R, token);
    printf(GRY "  (Production: this would be emailed to: %s)\n" R, email);
    ui_hr();

    char entered[16], newpass[64], confirm[64];
    get_line("Enter reset code   : ", entered,  sizeof(entered));
    if (strcmp(entered, token) != 0) {
        ui_error("Wrong reset code."); ui_pause(1500); return;
    }
    get_line("New password       : ", newpass,  sizeof(newpass));
    get_line("Confirm password   : ", confirm,  sizeof(confirm));
    if (strcmp(newpass, confirm) != 0) {
        ui_error("Passwords do not match."); ui_pause(1500); return;
    }
    if (strlen(newpass) < 3) {
        ui_error("Password must be at least 3 characters."); ui_pause(1500); return;
    }

    strncpy(employees[found].password, newpass, sizeof(employees[found].password)-1);
    csv_save_employees(emp_path, employees, count);
    ui_success("Password reset! You can now log in.");
    ui_pause(1500);
}

void screen_login(Employee* out, Employee* employees, int count,
                  const char* emp_path) {
    /* Authentication is a scan over the loaded Employee struct array. */
    char username[64], password[64];
    for (;;) {
        phdr("Login", "Sign in to PayDay");
        printf(GRY "  Tip: type  F  as username to reset your password\n" R);
        ui_hr();
        get_line("Username : ", username, sizeof(username));

        if (username[0] == 'F' || username[0] == 'f') {
            screen_forgot_password(employees, count, emp_path);
            continue;
        }

        get_line("Password : ", password, sizeof(password));

        for (int i = 0; i < count; i++) {
            if (!employees[i].active) continue;
            if (strcmp(employees[i].username, username) == 0 &&
                strcmp(employees[i].password, password) == 0) {
                *out = employees[i];
                ui_success("Login successful.");
                ui_pause(800); return;
            }
        }
        ui_error("Invalid username or password. Try again.");
        ui_pause(1200);
    }
}

void screen_employee_list(Employee* employees, int count) {
    phdr("Employee List", NULL);
    printf(GRY "  %-4s  %-22s  %-18s  %-16s  %13s  %-8s\n" R,
           "ID","Full Name","Department","Position","Base Salary","Role");
    ui_hr();
    int n = 0;
    for (int i = 0; i < count; i++) {
        if (!employees[i].active) continue;
        printf("  %-4d  %-22s  %-18s  %-16s  %13.2f  %-8s\n",
               employees[i].id, employees[i].full_name,
               employees[i].department, employees[i].position,
               employees[i].base_salary,
               employees[i].role==ROLE_ADMIN ? "Admin" : "Employee");
        n++;
    }
    if (!n) ui_info("No active employees.");
    ui_hr();
    printf(GRY "  %d employee(s)\n" R, n);
    ui_hr();
    press_enter();
}

void screen_add_employee(Employee* employees, int* count, const char* path) {
    phdr("Add Employee", "Create a new account");

    /* Build a new Employee struct from console input before saving it. */
    Employee e;
    memset(&e, 0, sizeof(e));
    e.id     = csv_next_employee_id(employees, *count);
    e.active = 1;
    e.role   = ROLE_EMPLOYEE;

    get_line("Username       : ", e.username,   sizeof(e.username));
    get_line("Password       : ", e.password,   sizeof(e.password));
    get_line("Full Name      : ", e.full_name,  sizeof(e.full_name));
    get_line("Department     : ", e.department, sizeof(e.department));
    get_line("Position       : ", e.position,   sizeof(e.position));
    get_line("Email          : ", e.email,      sizeof(e.email));
    get_line("Phone          : ", e.phone,      sizeof(e.phone));
    get_line("Date Hired (YYYY-MM-DD): ", e.date_hired, sizeof(e.date_hired));
    if (!e.date_hired[0]) csv_today(e.date_hired, sizeof(e.date_hired));
    e.base_salary = get_double("Base Salary (PHP) : ");
    e.hourly_rate = get_double("Hourly Rate (PHP) : ");

    printf(CYN "  Role [1=Employee / 2=Admin]: " WHT);
    fflush(stdout);
    char rb[4]; fgets(rb, sizeof(rb), stdin);
    e.role = (rb[0]=='2') ? ROLE_ADMIN : ROLE_EMPLOYEE;
    printf(R);

    char err[256];
    if (!payroll_validate_employee(&e, err, sizeof(err))) {
        ui_error(err); ui_pause(1500); return;
    }
    for (int i = 0; i < *count; i++) {
        if (employees[i].active && strcmp(employees[i].username, e.username)==0) {
            ui_error("Username already taken."); ui_pause(1500); return;
        }
    }

    ui_hr();
    printf(WHT "  Summary\n" R);
    printf("  %-16s %s\n", "Username:",    e.username);
    printf("  %-16s %s\n", "Full Name:",   e.full_name);
    printf("  %-16s %s\n", "Department:",  e.department);
    printf("  %-16s %s\n", "Position:",    e.position);
    printf("  %-16s PHP %.2f\n", "Base Salary:", e.base_salary);
    printf("  %-16s PHP %.2f/hr\n","Hourly Rate:", e.hourly_rate);
    printf("  %-16s %s\n", "Role:", e.role==ROLE_ADMIN?"Admin":"Employee");
    ui_hr();

    if (!confirm_yn("Add this employee?")) { ui_warn("Cancelled."); ui_pause(800); return; }

    employees[(*count)++] = e;
    csv_save_employees(path, employees, *count);
    ui_success("Employee added successfully.");
    ui_pause(1200);
}

void screen_edit_employee(Employee* employees, int count, const char* path) {
    phdr("Edit Employee", "Update employee information");
    printf(GRY "  %-4s  %-22s  %-18s\n" R, "ID","Full Name","Department");
    ui_hr();
    for (int i = 0; i < count; i++) {
        if (!employees[i].active) continue;
        printf("  %-4d  %-22s  %-18s\n",
               employees[i].id, employees[i].full_name, employees[i].department);
    }
    ui_hr();

    int id = get_int("Employee ID to edit (0=cancel): ");
    if (!id) return;

    Employee* e = NULL;
    for (int i = 0; i < count; i++)
        if (employees[i].id == id && employees[i].active) { e = &employees[i]; break; }
    if (!e) { ui_error("Employee not found."); ui_pause(1200); return; }

    ui_hr();
    printf(WHT "  Editing: %s\n" GRY "  Leave blank to keep current value.\n\n" R,
           e->full_name);

    char tmp[128];

#define EDIT_STR(label, field) \
    printf(CYN "  " label " [%s]: " WHT, e->field); fflush(stdout); \
    fgets(tmp, sizeof(tmp), stdin); tmp[strcspn(tmp,"\n")]='\0'; printf(R); \
    if (tmp[0]) strncpy(e->field, tmp, sizeof(e->field)-1);

    EDIT_STR("Full Name  ", full_name)
    EDIT_STR("Department ", department)
    EDIT_STR("Position   ", position)
    EDIT_STR("Email      ", email)
    EDIT_STR("Phone      ", phone)

    printf(CYN "  Base Salary [%.2f]: " WHT, e->base_salary); fflush(stdout);
    fgets(tmp, sizeof(tmp), stdin); tmp[strcspn(tmp,"\n")]='\0'; printf(R);
    if (tmp[0]) { double v=atof(tmp); if(v>0) e->base_salary=v; }

    printf(CYN "  Hourly Rate [%.2f]: " WHT, e->hourly_rate); fflush(stdout);
    fgets(tmp, sizeof(tmp), stdin); tmp[strcspn(tmp,"\n")]='\0'; printf(R);
    if (tmp[0]) { double v=atof(tmp); if(v>0) e->hourly_rate=v; }

    printf(CYN "  New Password (blank=keep): " WHT); fflush(stdout);
    fgets(tmp, sizeof(tmp), stdin); tmp[strcspn(tmp,"\n")]='\0'; printf(R);
    if (strlen(tmp) >= 3) strncpy(e->password, tmp, sizeof(e->password)-1);

    if (!confirm_yn("Save changes?")) { ui_warn("Cancelled."); ui_pause(800); return; }
    csv_save_employees(path, employees, count);
    ui_success("Employee updated.");
    ui_pause(1200);
}

void screen_deactivate_emp(Employee* employees, int count, const char* path) {
    phdr("Deactivate Employee", "Remove employee from the system");
    printf(GRY "  %-4s  %-22s  %-18s\n" R, "ID","Full Name","Department");
    ui_hr();
    for (int i = 0; i < count; i++) {
        if (!employees[i].active) continue;
        printf("  %-4d  %-22s  %-18s\n",
               employees[i].id, employees[i].full_name, employees[i].department);
    }
    ui_hr();

    int id = get_int("Employee ID to deactivate (0=cancel): ");
    if (!id) return;

    for (int i = 0; i < count; i++) {
        if (employees[i].id==id && employees[i].active) {
            printf(YLW "\n  Deactivate: %s?\n" R, employees[i].full_name);
            if (!confirm_yn("This removes their access permanently.")) {
                ui_warn("Cancelled."); ui_pause(800); return;
            }
            employees[i].active = 0;
            csv_save_employees(path, employees, count);
            ui_success("Employee deactivated.");
            ui_pause(1200); return;
        }
    }
    ui_error("Employee not found."); ui_pause(1200);
}

void screen_calc_payroll(Employee* emp, const char* pay_path) {
    /* Convert one Employee struct into a PayrollRecord, then persist it. */
    char sub[160];
    snprintf(sub, sizeof(sub), "%s  |  Base: PHP %.2f  |  Rate: PHP %.2f/hr",
             emp->full_name, emp->base_salary, emp->hourly_rate);
    phdr("Calculate Payroll", sub);

    char ps[16], pe[16];
    get_line("Period Start (YYYY-MM-DD) : ", ps, sizeof(ps));
    get_line("Period End   (YYYY-MM-DD) : ", pe, sizeof(pe));
    if (!ps[0]||!pe[0]) { ui_error("Both period dates are required."); ui_pause(1200); return; }

    ui_hr();
    printf(WHT "  Earnings\n" R);
    printf(GRY "  Base salary: PHP %.2f (fixed)\n" R, emp->base_salary);
    double ot = get_double("  Overtime hours (0 if none) : ");

    ui_hr();
    printf(WHT "  Deductions  " GRY "(standard rates — can add custom below)\n" R);
    printf(GRY "  Tax %.0f%%  |  SSS %.1f%%  |  PhilHealth %.1f%%  |  Pag-IBIG PHP %.0f\n" R,
           DEFAULT_TAX_RATE*100, DEFAULT_SSS_RATE*100,
           DEFAULT_PHILHEALTH_RATE*100, DEFAULT_PAGIBIG_FIXED);
    double other = get_double("  Additional deductions (PHP) : ");

    PayrollRecord rec;
    payroll_calculate(&rec, emp, ps, pe, ot, other, NULL);

    /* Load existing payroll rows so the new record can get an ID and be saved. */
    PayrollRecord payroll[MAX_PAYROLL];
    int pay_count = csv_load_payroll(pay_path, payroll, MAX_PAYROLL);
    rec.id = csv_next_payroll_id(payroll, pay_count);

    ui_dhr();
    printf(WHT BLD "  PAYROLL SUMMARY — %s\n" R, emp->full_name);
    printf(GRY "  Period: %s  to  %s\n\n" R, ps, pe);

    printf("  Base Salary                    PHP %12.2f\n",  rec.base_salary);
    printf("  Overtime  (%.1fh × PHP %.2f × %.2fx)  PHP %10.2f\n",
           ot, emp->hourly_rate, DEFAULT_OVERTIME_RATE, rec.overtime_pay);
    printf(GRN "  --------------------------------------------------\n"
               "  GROSS PAY                      PHP %12.2f\n" R, rec.gross_pay);

    printf(RED
           "\n  Income Tax (15%%)               PHP %12.2f\n"
           "  SSS       (4.5%%)               PHP %12.2f\n"
           "  PhilHealth(2.5%%)               PHP %12.2f\n"
           "  Pag-IBIG  (fixed)              PHP %12.2f\n"
           "  Other Deductions               PHP %12.2f\n"
           "  ---------------------------------------------\n"
           "  TOTAL DEDUCTIONS               PHP %12.2f\n" R,
           rec.tax_deduction, rec.sss_deduction,
           rec.philhealth_deduction, rec.pagibig_deduction,
           rec.other_deductions, rec.total_deductions);

    ui_dhr();
    printf(WHT BLD "  NET PAY                        " GRN "PHP %12.2f\n" R, rec.net_pay);
    ui_dhr();

    if (!confirm_yn("Save this payroll record?")) { ui_warn("Cancelled."); ui_pause(800); return; }

    payroll[pay_count++] = rec;
    csv_save_payroll(pay_path, payroll, pay_count);
    printf(GRN "  Saved as Payroll Record #%d\n" R, rec.id);
    ui_pause(1400);
}

void screen_payslip(int payroll_id, Employee* employees, int emp_count,
                    const char* pay_path) {
    /* Reads payroll rows, finds one record, then joins back to employee details. */
    PayrollRecord payroll[MAX_PAYROLL];
    int pay_count = csv_load_payroll(pay_path, payroll, MAX_PAYROLL);

    PayrollRecord* p = NULL;
    for (int i = 0; i < pay_count; i++)
        if (payroll[i].id == payroll_id) { p = &payroll[i]; break; }
    if (!p) { ui_error("Record not found."); ui_pause(1200); return; }

    const char* name = "Unknown", *dept = "", *pos = "";
    for (int i = 0; i < emp_count; i++) {
        if (employees[i].id == p->employee_id) {
            name = employees[i].full_name;
            dept = employees[i].department;
            pos  = employees[i].position;
            break;
        }
    }

    ui_clear();
    ui_dhr();
    printf(PNK BLD "                    P A Y  S L I P\n" R);
    ui_dhr();
    printf(WHT "  Employee   : " R "%s\n",   name);
    printf(WHT "  Dept / Pos : " R "%s / %s\n", dept, pos);
    printf(WHT "  Period     : " R "%s  to  %s\n", p->period_start, p->period_end);
    printf(WHT "  Generated  : " R "%s\n",   p->generated_date);
    printf(WHT "  Status     : " R);
    if (strcmp(p->status,"released")==0) printf(GRN "Released\n" R);
    else                                  printf(YLW "Generated (pending release)\n" R);
    ui_hr();
    printf(WHT "  EARNINGS\n" R);
    printf("  Base Salary                    PHP %12.2f\n", p->base_salary);
    printf("  Overtime  (%.1fh)              PHP %12.2f\n", p->overtime_hours, p->overtime_pay);
    printf(GRN
           "  --------------------------------------------------\n"
           "  GROSS PAY                      PHP %12.2f\n" R, p->gross_pay);
    ui_hr();
    printf(WHT "  DEDUCTIONS\n" R);
    printf(RED
           "  Income Tax (15%%)               PHP %12.2f\n"
           "  SSS        (4.5%%)              PHP %12.2f\n"
           "  PhilHealth (2.5%%)              PHP %12.2f\n"
           "  Pag-IBIG   (fixed)             PHP %12.2f\n"
           "  Other Deductions               PHP %12.2f\n"
           "  --------------------------------------------------\n"
           "  TOTAL DEDUCTIONS               PHP %12.2f\n" R,
           p->tax_deduction, p->sss_deduction,
           p->philhealth_deduction, p->pagibig_deduction,
           p->other_deductions, p->total_deductions);
    ui_dhr();
    printf(WHT BLD "  NET PAY                        " GRN "PHP %12.2f\n" R, p->net_pay);
    ui_dhr();
    press_enter();
}

void screen_release_payroll(Employee* employees, int emp_count, const char* pay_path) {
    /* Marks a saved payroll record as released, then rewrites payroll.csv. */
    PayrollRecord payroll[MAX_PAYROLL];
    int pay_count = csv_load_payroll(pay_path, payroll, MAX_PAYROLL);

    phdr("Release Payroll", "Mark a generated record as released");
    printf(GRY "  %-4s  %-20s  %-25s  %12s\n" R, "#","Employee","Period","Net Pay");
    ui_hr();

    int any = 0;
    for (int i = 0; i < pay_count; i++) {
        if (strcmp(payroll[i].status,"generated")!=0) continue;
        const char* name="Unknown";
        for (int j=0;j<emp_count;j++)
            if (employees[j].id==payroll[i].employee_id) { name=employees[j].full_name; break; }
        char period[30];
        snprintf(period,sizeof(period),"%s ~ %s",payroll[i].period_start,payroll[i].period_end);
        printf("  %-4d  %-20s  %-25s  %12.2f\n",
               payroll[i].id, name, period, payroll[i].net_pay);
        any++;
    }
    if (!any) { ui_info("No pending records to release."); ui_pause(1200); return; }
    ui_hr();

    int id = get_int("Record # to release (0=cancel): ");
    if (!id) return;

    for (int i = 0; i < pay_count; i++) {
        if (payroll[i].id == id) {
            if (strcmp(payroll[i].status,"released")==0) {
                ui_warn("Already released."); ui_pause(1000); return;
            }
            if (!confirm_yn("Release this payroll?")) { ui_warn("Cancelled."); ui_pause(800); return; }
            strncpy(payroll[i].status, "released", sizeof(payroll[i].status)-1);
            csv_save_payroll(pay_path, payroll, pay_count);
            ui_success("Payroll released.");
            ui_pause(1200); return;
        }
    }
    ui_error("Record not found."); ui_pause(1200);
}

void screen_payroll_list(int filter_emp_id, Employee* employees, int emp_count,
                         const char* pay_path, int allow_release) {
    for (;;) {
        /* Refresh from disk each time so the list reflects the latest CSV data. */
        PayrollRecord payroll[MAX_PAYROLL];
        int pay_count = csv_load_payroll(pay_path, payroll, MAX_PAYROLL);

        char sub[80] = "All employees";
        if (filter_emp_id > 0)
            for (int i=0;i<emp_count;i++)
                if (employees[i].id==filter_emp_id) {
                    snprintf(sub,sizeof(sub),"%s",employees[i].full_name); break;
                }

        phdr("Payroll Records", sub);
        printf(GRY "  %-4s  %-20s  %-12s  %-12s  %12s  %12s  %-10s\n" R,
               "#","Employee","Start","End","Gross","Net Pay","Status");
        ui_hr();

        int n = 0;
        for (int i = 0; i < pay_count; i++) {
            PayrollRecord* p = &payroll[i];
            if (filter_emp_id>0 && p->employee_id!=filter_emp_id) continue;
            const char* name="Unknown";
            for (int j=0;j<emp_count;j++)
                if (employees[j].id==p->employee_id) { name=employees[j].full_name; break; }
            const char* sc = (strcmp(p->status,"released")==0) ? GRN : YLW;
            printf("  %-4d  %-20s  %-12s  %-12s  %12.2f  %12.2f  %s%-10s%s\n",
                   p->id, name, p->period_start, p->period_end,
                   p->gross_pay, p->net_pay, sc, p->status, R);
            n++;
        }
        if (!n) ui_info("No payroll records found.");
        ui_hr();
        printf(GRY "  %d record(s)\n\n" R, n);
        printf(WHT "  [V]" R " View Payslip   ");
            if (allow_release) {
                printf(WHT "[R]" R " Release   ");
            }
            printf(WHT "[0]" R " Back\n");
        ui_hr();

        char ch[8]; get_line("Choice : ", ch, sizeof(ch));
        if (ch[0]=='0') break;
        if (ch[0]=='v'||ch[0]=='V') {
            int id = get_int("Record # to view: ");
            screen_payslip(id, employees, emp_count, pay_path);
        }
        else if ((ch[0]=='r'||ch[0]=='R') && allow_release) {
            screen_release_payroll(employees, emp_count, pay_path);
        }
    }
}

void screen_report(Employee* employees, int emp_count, const char* pay_path) {
    /* Aggregates saved payroll structs into a console summary report. */
    PayrollRecord payroll[MAX_PAYROLL];
    int pay_count = csv_load_payroll(pay_path, payroll, MAX_PAYROLL);

    phdr("Payroll Report", "Summary by period");
    char ps[16], pe[16];
    get_line("Period Start (YYYY-MM-DD, blank=all) : ", ps, sizeof(ps));
    get_line("Period End   (YYYY-MM-DD, blank=all) : ", pe, sizeof(pe));
    ui_hr();

    printf(GRY "  %-22s  %-16s  %-25s  %12s  %12s  %12s\n" R,
           "Employee","Department","Period","Gross","Deductions","Net Pay");
    ui_hr();

    double tg=0, td=0, tn=0;
    int rc=0;
    for (int i=0; i<pay_count; i++) {
        PayrollRecord* p = &payroll[i];
        if (ps[0] && strcmp(p->period_start, ps)<0) continue;
        if (pe[0] && strcmp(p->period_end,   pe)>0) continue;
        const char* name="Unknown", *dept="";
        for (int j=0;j<emp_count;j++)
            if (employees[j].id==p->employee_id) {
                name=employees[j].full_name; dept=employees[j].department; break;
            }
        char period[28];
        snprintf(period,sizeof(period),"%s ~ %s",p->period_start,p->period_end);
        printf("  %-22s  %-16s  %-25s  %12.2f  %12.2f  %12.2f\n",
               name, dept, period, p->gross_pay, p->total_deductions, p->net_pay);
        tg+=p->gross_pay; td+=p->total_deductions; tn+=p->net_pay; rc++;
    }
    ui_dhr();
    printf(WHT BLD "  %-22s  %-16s  %-25s  %12.2f  %12.2f  %12.2f\n" R,
           "TOTAL","","",tg,td,tn);
    printf(GRY "  %d record(s)\n" R, rc);
    ui_dhr();
    press_enter();
}

void screen_admin_menu(Employee* admin, Employee* employees, int* emp_count,
                       const char* emp_path, const char* pay_path) {
    /* Main admin process controller for employee and payroll workflows. */
    for (;;) {
        phdr("Admin Panel", admin->full_name);
        printf(WHT "  [1]" R "  Employee List\n");
        printf(WHT "  [2]" R "  Add Employee\n");
        printf(WHT "  [3]" R "  Edit Employee\n");
        printf(WHT "  [4]" R "  Deactivate Employee\n");
        ui_hr();
        printf(WHT "  [5]" R "  Calculate Salary  (overtime + deductions)\n");
        printf(WHT "  [6]" R "  Payroll Records   (view payslip / release)\n");
        printf(WHT "  [7]" R "  Payroll Report\n");
        ui_hr();
        printf(WHT "  [0]" R "  Logout\n");
        ui_hr();

        char ch[4]; get_line("Choice : ", ch, sizeof(ch));

        if      (ch[0]=='1') screen_employee_list(employees, *emp_count);
        else if (ch[0]=='2') screen_add_employee(employees, emp_count, emp_path);
        else if (ch[0]=='3') screen_edit_employee(employees, *emp_count, emp_path);
        else if (ch[0]=='4') screen_deactivate_emp(employees, *emp_count, emp_path);
        else if (ch[0]=='5') {
            phdr("Calculate Payroll", "Select an employee");
            printf(GRY "  %-4s  %-22s  %-18s  %13s\n" R,
                   "ID","Full Name","Department","Base Salary");
            ui_hr();
            for (int i=0;i<*emp_count;i++) {
                if (!employees[i].active) continue;
                printf("  %-4d  %-22s  %-18s  %13.2f\n",
                       employees[i].id, employees[i].full_name,
                       employees[i].department, employees[i].base_salary);
            }
            ui_hr();
            int id = get_int("Employee ID (0=cancel): ");
            if (!id) goto reload;
            Employee* t=NULL;
            for (int i=0;i<*emp_count;i++)
                if (employees[i].id==id && employees[i].active) { t=&employees[i]; break; }
            if (!t) { ui_error("Not found."); ui_pause(1000); goto reload; }
            screen_calc_payroll(t, pay_path);
        }
        else if (ch[0]=='6') screen_payroll_list(0, employees, *emp_count, pay_path, 1);
        else if (ch[0]=='7') screen_report(employees, *emp_count, pay_path);
        else if (ch[0]=='0') break;

        reload:
        /* After every action, reload employees.csv so later screens use fresh data. */
        *emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);
    }
}

void screen_employee_menu(Employee* emp, const char* pay_path,
                          Employee* employees, int emp_count) {
    /* Employee users only navigate their own saved payroll history. */
    for (;;) {
        phdr("Employee Portal", emp->full_name);
        printf(GRY "  %s  ·  %s  ·  Base: PHP %.2f\n\n" R,
               emp->department, emp->position, emp->base_salary);
        printf(WHT "  [1]" R "  My Payroll Records  (view payslip)\n");
        printf(WHT "  [0]" R "  Logout\n");
        ui_hr();
        char ch[4]; get_line("Choice : ", ch, sizeof(ch));
        if      (ch[0]=='1') screen_payroll_list(emp->id, employees, emp_count, pay_path, 0);
        else if (ch[0]=='0') break;
    }
}
