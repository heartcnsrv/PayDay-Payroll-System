#include "PayrollEngine.h"
#include "../utils/CSVManager.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* Overtime pay is based on rate, hours, and overtime multiplier. */
double payroll_overtime_pay(double hourly_rate, double hours, double multiplier) {
    if (hours <= 0.0) return 0.0;
    return hourly_rate * hours * multiplier;
}

/* Percentage-based deductions applied to gross pay. */
double payroll_tax(double gross, double rate) {
    return gross * rate;
}

double payroll_sss(double gross, double rate) {
    return gross * rate;
}

double payroll_philhealth(double gross, double rate) {
    return gross * rate;
}

double payroll_net(double gross, double total_deductions) {
    double net = gross - total_deductions;
    return net < 0.0 ? 0.0 : net;
}

/*
 * Converts one Employee struct into one PayrollRecord struct.
 * The caller usually saves the finished record through CSVManager.
 */
void payroll_calculate(
    PayrollRecord*        out,
    const Employee*       emp,
    const char*           period_start,
    const char*           period_end,
    double                overtime_hours,
    double                other_deductions,
    const DeductionRates* rates)
{
    DeductionRates r;
    if (rates) {
        r = *rates;
    } else {
        r.tax_rate          = DEFAULT_TAX_RATE;
        r.sss_rate          = DEFAULT_SSS_RATE;
        r.philhealth_rate   = DEFAULT_PHILHEALTH_RATE;
        r.pagibig_fixed     = DEFAULT_PAGIBIG_FIXED;
    }

    memset(out, 0, sizeof(*out));

    /* Copy the employee identity/salary snapshot into the payroll record. */
    out->employee_id    = emp->id;
    out->base_salary    = emp->base_salary;

    strncpy(out->period_start, period_start, sizeof(out->period_start)-1);
    strncpy(out->period_end,   period_end,   sizeof(out->period_end)-1);

    out->overtime_hours = overtime_hours;
    out->overtime_rate  = DEFAULT_OVERTIME_RATE;
    out->overtime_pay   = payroll_overtime_pay(
                              emp->hourly_rate, overtime_hours, DEFAULT_OVERTIME_RATE);

    out->gross_pay      = emp->base_salary + out->overtime_pay;

    /* Once gross pay is known, the deduction fields can be computed. */
    out->tax_deduction         = payroll_tax(out->gross_pay, r.tax_rate);
    out->sss_deduction         = payroll_sss(out->gross_pay, r.sss_rate);
    out->philhealth_deduction  = payroll_philhealth(out->gross_pay, r.philhealth_rate);
    out->pagibig_deduction     = r.pagibig_fixed;
    out->other_deductions      = other_deductions;

    out->total_deductions = out->tax_deduction
                          + out->sss_deduction
                          + out->philhealth_deduction
                          + out->pagibig_deduction
                          + out->other_deductions;

    out->net_pay = payroll_net(out->gross_pay, out->total_deductions);

    /* Metadata used later by reports, payslips, and release actions. */
    csv_today(out->generated_date, sizeof(out->generated_date));
    strncpy(out->status, "generated", sizeof(out->status)-1);
}

int payroll_validate_employee(const Employee* e, char* err_out, int err_len) {
    /* Prevent invalid Employee structs from being written to employees.csv. */
    if (!e->username[0]) {
        snprintf(err_out, (size_t)err_len, "Username is required.");
        return 0;
    }
    if (strlen(e->username) < 2) {
        snprintf(err_out, (size_t)err_len, "Username must be at least 2 characters.");
        return 0;
    }
    if (!e->password[0] || strlen(e->password) < 3) {
        snprintf(err_out, (size_t)err_len, "Password must be at least 3 characters.");
        return 0;
    }
    if (!e->full_name[0]) {
        snprintf(err_out, (size_t)err_len, "Full name is required.");
        return 0;
    }
    if (e->base_salary <= 0.0) {
        snprintf(err_out, (size_t)err_len, "Base salary must be greater than 0.");
        return 0;
    }
    if (e->hourly_rate <= 0.0) {
        snprintf(err_out, (size_t)err_len, "Hourly rate must be greater than 0.");
        return 0;
    }
    return 1;
}

void payroll_generate_token(char* out, int len) {
    /* Demo-only reset token generation for forgot-password flows. */
    srand((unsigned int)time(NULL));
    int code = (rand() % 900000) + 100000;
    snprintf(out, (size_t)len, "%d", code);
}
