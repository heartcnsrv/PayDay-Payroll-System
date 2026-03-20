#ifndef PAYDAY_CSV_MANAGER_H
#define PAYDAY_CSV_MANAGER_H

#include "../core/PayrollTypes.h"

/*
 * CSV-backed persistence layer.
 * These functions act as the "database" of the system,
 * handling reading from and writing to CSV files.
 */

/*
 * Loads employee data from a CSV file.
 * - path: file location of the CSV
 * - out: array where employee records will be stored
 * - max: maximum number of records to load
 * Returns: number of employees successfully loaded
 */
int csv_load_employees(const char* path, Employee* out, int max);

/*
 * Saves employee data into a CSV file.
 * - path: file location of the CSV
 * - arr: array of employee records to save
 * - count: number of employee records
 * Returns: number of records successfully saved
 */
int csv_save_employees(const char* path, const Employee* arr, int count);

/*
 * Generates the next available employee ID.
 * - arr: existing employee records
 * - count: number of current employees
 * Returns: next unique employee ID
 */
int csv_next_employee_id(const Employee* arr, int count);

/*
 * Loads payroll records from a CSV file.
 * - path: file location of the CSV
 * - out: array where payroll records will be stored
 * - max: maximum number of records to load
 * Returns: number of payroll records successfully loaded
 */
int csv_load_payroll(const char* path, PayrollRecord* out, int max);

/*
 * Saves payroll records into a CSV file.
 * - path: file location of the CSV
 * - arr: array of payroll records to save
 * - count: number of payroll records
 * Returns: number of records successfully saved
 */
int csv_save_payroll(const char* path, const PayrollRecord* arr, int count);

/*
 * Generates the next available payroll record ID.
 * - arr: existing payroll records
 * - count: number of current payroll records
 * Returns: next unique payroll ID
 */
int csv_next_payroll_id(const PayrollRecord* arr, int count);

/*
 * Gets today's date as a string.
 * - buf: buffer where the date string will be stored
 * - buflen: size of the buffer
 * Format depends on implementation (e.g., YYYY-MM-DD)
 */
void csv_today(char* buf, int buflen);

#endif