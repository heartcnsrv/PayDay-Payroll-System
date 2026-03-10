"use strict";
// ============================================================
//  PayDay | src/gui/js/app.js
//  Frontend only — all logic is in the C server.
//  Calls /api/* → backend/proxy.php → C HttpServer on :8081
// ============================================================

const API = '/api';
let currentUser = null;
let resetUsername = '';

// ── API helper ────────────────────────────────────────────────
async function api(endpoint, body) {
  try {
    const res  = await fetch(`${API}/${endpoint}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    const text = await res.text();
    if (text.trimStart().startsWith('<')) {
      console.error('[PayDay] Got HTML — is PayDayServer.exe running?');
      return { ok: false, error: 'C backend unreachable. Is PayDayServer.exe running?' };
    }
    return JSON.parse(text);
  } catch(e) {
    return { ok: false, error: 'Network error: ' + e.message };
  }
}

// ── Format currency ───────────────────────────────────────────
function peso(n) {
  return '₱' + Number(n).toLocaleString('en-PH', { minimumFractionDigits: 2 });
}

// ── Screen switcher ───────────────────────────────────────────
function showScreen(id) {
  document.querySelectorAll('.screen').forEach(s => {
    s.classList.toggle('active', s.id === id);
    s.classList.toggle('hidden', s.id !== id);
  });
}

function showPage(id) {
  document.querySelectorAll('.page').forEach(p => {
    p.classList.toggle('active', p.id === id);
    p.classList.toggle('hidden', p.id !== id);
  });
  document.querySelectorAll('.sidebar-nav a').forEach(a => {
    a.classList.toggle('active', a.dataset.page === id);
  });
}

// ── Login forms ───────────────────────────────────────────────
function showLogin()  {
  document.getElementById('login-form').classList.remove('hidden');
  document.getElementById('forgot-form').classList.add('hidden');
  document.getElementById('reset-form').classList.add('hidden');
}
function showForgot() {
  document.getElementById('login-form').classList.add('hidden');
  document.getElementById('forgot-form').classList.remove('hidden');
}

function setErr(id, msg) {
  const el = document.getElementById(id);
  el.textContent = msg;
  el.classList.toggle('hidden', !msg);
}

async function doLogin() {
  const u = document.getElementById('login-username').value.trim();
  const p = document.getElementById('login-password').value;
  if (!u || !p) return setErr('login-error', 'Please fill in all fields.');
  setErr('login-error', '');
  const r = await api('auth', { action: 'login', username: u, password: p });
  if (!r.ok) return setErr('login-error', r.error);
  currentUser = r;
  initApp();
}

async function doForgot() {
  const u = document.getElementById('forgot-username').value.trim();
  const e = document.getElementById('forgot-email').value.trim();
  if (!u || !e) return setErr('forgot-error', 'Please fill in all fields.');
  setErr('forgot-error', '');
  const r = await api('auth', { action: 'forgot_password', username: u, email: e });
  if (!r.ok) return setErr('forgot-error', r.error);
  resetUsername = u;
  // Show token (in real system: this goes to email)
  document.getElementById('forgot-form').classList.add('hidden');
  document.getElementById('reset-form').classList.remove('hidden');
  document.getElementById('reset-error').textContent = '';
  alert(`Reset code: ${r.token}\n(In production this would be emailed.)`);
}

async function doReset() {
  const token = document.getElementById('reset-token').value.trim();
  const pass  = document.getElementById('reset-pass').value;
  if (!token || !pass) return setErr('reset-error', 'Please fill in all fields.');
  const r = await api('auth', {
    action: 'reset_password',
    username: resetUsername,
    token, new_pass: pass
  });
  if (!r.ok) return setErr('reset-error', r.error);
  alert('Password reset! Please log in.');
  showLogin();
}

function doLogout() {
  currentUser = null;
  document.getElementById('login-username').value = '';
  document.getElementById('login-password').value = '';
  showScreen('screen-login');
  showLogin();
}

// ── Initialise app after login ────────────────────────────────
function initApp() {
  showScreen('screen-app');

  // User chip
  const initials = currentUser.full_name
    ? currentUser.full_name.split(' ').map(w => w[0]).join('').slice(0,2).toUpperCase()
    : currentUser.username.slice(0,2).toUpperCase();
  document.getElementById('user-chip').innerHTML = `
    <div class="avatar">${initials}</div>
    <div class="user-info">
      <div class="name">${currentUser.full_name || currentUser.username}</div>
      <div class="role">${currentUser.role === 0 ? 'Administrator' : 'Employee'}</div>
    </div>`;

  // Build nav
  const nav = document.getElementById('sidebar-nav');
  if (currentUser.role === 0) {
    nav.innerHTML = `
      <a data-page="page-dashboard"  onclick="navTo('page-dashboard')">
        <span class="nav-icon">🏠</span> Dashboard</a>
      <a data-page="page-employees"  onclick="navTo('page-employees')">
        <span class="nav-icon">👥</span> Employees</a>
      <a data-page="page-payroll"    onclick="navTo('page-payroll')">
        <span class="nav-icon">💰</span> Payroll</a>
      <a data-page="page-calc"       onclick="navTo('page-calc')">
        <span class="nav-icon">🧮</span> Calculate</a>
      <a data-page="page-report"     onclick="navTo('page-report')">
        <span class="nav-icon">📊</span> Reports</a>`;
    navTo('page-dashboard');
  } else {
    nav.innerHTML = `
      <a data-page="page-dashboard"   onclick="navTo('page-dashboard')">
        <span class="nav-icon">🏠</span> Dashboard</a>
      <a data-page="page-my-payroll"  onclick="navTo('page-my-payroll')">
        <span class="nav-icon">💰</span> My Payroll</a>`;
    navTo('page-dashboard');
  }
}

function navTo(page) {
  showPage(page);
  const builders = {
    'page-dashboard':  buildDashboard,
    'page-employees':  buildEmployees,
    'page-payroll':    buildPayrollList,
    'page-calc':       buildCalc,
    'page-report':     buildReport,
    'page-my-payroll': buildMyPayroll,
  };
  if (builders[page]) builders[page]();
}

// ── DASHBOARD ─────────────────────────────────────────────────
async function buildDashboard() {
  const pg = document.getElementById('page-dashboard');
  pg.innerHTML = '<p class="text-muted">Loading...</p>';

  let empCount = 0, grossTotal = 0, netTotal = 0, payCount = 0;
  if (currentUser.role === 0) {
    const [er, pr] = await Promise.all([
      api('employees', { action: 'list' }),
      api('payroll',   { action: 'list', employee_id: 0 }),
    ]);
    if (er.ok) empCount = (er.employees || []).length;
    if (pr.ok) {
      payCount   = (pr.records || []).length;
      grossTotal = (pr.records || []).reduce((s, r) => s + r.gross_pay, 0);
      netTotal   = (pr.records || []).reduce((s, r) => s + r.net_pay,   0);
    }
    pg.innerHTML = `
      <div class="page-header">
        <div><h2>Dashboard</h2><p>Welcome back, ${currentUser.full_name || currentUser.username}</p></div>
      </div>
      <div class="stats-grid">
        <div class="stat-card blush">
          <div class="stat-label">Active Employees</div>
          <div class="stat-value">${empCount}</div>
          <div class="stat-sub">registered in system</div>
        </div>
        <div class="stat-card sage">
          <div class="stat-label">Payroll Records</div>
          <div class="stat-value">${payCount}</div>
          <div class="stat-sub">total processed</div>
        </div>
        <div class="stat-card sand">
          <div class="stat-label">Total Gross Released</div>
          <div class="stat-value" style="font-size:1.4rem">${peso(grossTotal)}</div>
          <div class="stat-sub">all time</div>
        </div>
        <div class="stat-card blush">
          <div class="stat-label">Total Net Released</div>
          <div class="stat-value" style="font-size:1.4rem">${peso(netTotal)}</div>
          <div class="stat-sub">after deductions</div>
        </div>
      </div>
      <div class="card">
        <div class="card-title">Quick Actions</div>
        <div style="display:flex;gap:12px;flex-wrap:wrap">
          <button class="btn btn-primary" style="width:auto" onclick="navTo('page-employees')">👥 View Employees</button>
          <button class="btn btn-sage"    onclick="navTo('page-calc')">🧮 Calculate Payroll</button>
          <button class="btn btn-sand"    onclick="navTo('page-report')">📊 Generate Report</button>
        </div>
      </div>`;
  } else {
    pg.innerHTML = `
      <div class="page-header">
        <div><h2>My Dashboard</h2>
          <p>${currentUser.department || ''} · ${currentUser.position || ''}</p></div>
      </div>
      <div class="stats-grid">
        <div class="stat-card blush">
          <div class="stat-label">Base Salary</div>
          <div class="stat-value" style="font-size:1.4rem">${peso(currentUser.base_salary)}</div>
          <div class="stat-sub">monthly</div>
        </div>
        <div class="stat-card sage">
          <div class="stat-label">Hourly Rate</div>
          <div class="stat-value" style="font-size:1.4rem">${peso(currentUser.hourly_rate)}</div>
          <div class="stat-sub">for overtime</div>
        </div>
      </div>
      <div class="card">
        <div class="card-title">Quick Actions</div>
        <button class="btn btn-primary" style="width:auto" onclick="navTo('page-my-payroll')">💰 View My Payroll</button>
      </div>`;
  }
}

// ── EMPLOYEES ─────────────────────────────────────────────────
async function buildEmployees() {
  const pg = document.getElementById('page-employees');
  pg.innerHTML = '<p class="text-muted">Loading...</p>';
  const r = await api('employees', { action: 'list' });
  if (!r.ok) { pg.innerHTML = `<div class="alert alert-error">${r.error}</div>`; return; }

  const rows = (r.employees || []).map(e => `
    <tr>
      <td>${e.id}</td>
      <td><strong>${e.full_name}</strong><br><small class="text-muted">${e.username}</small></td>
      <td>${e.department}</td>
      <td>${e.position}</td>
      <td>${peso(e.base_salary)}</td>
      <td><span class="badge ${e.role===0?'badge-admin':'badge-employee'}">${e.role===0?'Admin':'Employee'}</span></td>
      <td>
        <button class="btn btn-outline sm" onclick="showEditEmployee(${e.id})">Edit</button>
        <button class="btn btn-danger  sm" onclick="deactivateEmployee(${e.id},'${e.full_name.replace(/'/g,"\\'")}')">Remove</button>
      </td>
    </tr>`).join('');

  pg.innerHTML = `
    <div class="page-header">
      <div><h2>Employees</h2><p>${(r.employees||[]).length} active employees</p></div>
      <button class="btn btn-primary" style="width:auto" onclick="navTo('page-add-emp')">+ Add Employee</button>
    </div>
    <div class="card">
      <div class="table-wrap">
        <table>
          <thead><tr>
            <th>ID</th><th>Name</th><th>Department</th><th>Position</th>
            <th>Base Salary</th><th>Role</th><th>Actions</th>
          </tr></thead>
          <tbody>${rows || '<tr><td colspan="7" class="text-muted" style="text-align:center;padding:24px">No employees found.</td></tr>'}</tbody>
        </table>
      </div>
    </div>`;
}

// ── ADD EMPLOYEE ──────────────────────────────────────────────
function buildAddEmployee() {
  navTo('page-add-emp');
}
async function navToAddEmp() {
  const pg = document.getElementById('page-add-emp');
  showPage('page-add-emp');
  document.querySelectorAll('.sidebar-nav a').forEach(a => a.classList.remove('active'));
  pg.innerHTML = `
    <div class="page-header">
      <div><h2>Add Employee</h2><p>Create a new employee account</p></div>
      <button class="btn btn-ghost sm" onclick="navTo('page-employees')">← Back</button>
    </div>
    <div class="card">
      <div id="add-emp-alert"></div>
      <div class="form-grid">
        <div class="form-group"><label>Username *</label><input id="ae-username" type="text" placeholder="e.g. jsmith"/></div>
        <div class="form-group"><label>Password *</label><input id="ae-password" type="text" placeholder="min 3 chars"/></div>
        <div class="form-group full"><label>Full Name *</label><input id="ae-fullname" type="text" placeholder="Juan Smith"/></div>
        <div class="form-group"><label>Department</label><input id="ae-dept" type="text" placeholder="e.g. Engineering"/></div>
        <div class="form-group"><label>Position</label><input id="ae-position" type="text" placeholder="e.g. Developer"/></div>
        <div class="form-group"><label>Email</label><input id="ae-email" type="email" placeholder="user@company.com"/></div>
        <div class="form-group"><label>Phone</label><input id="ae-phone" type="text" placeholder="09XXXXXXXXX"/></div>
        <div class="form-group"><label>Date Hired</label><input id="ae-hired" type="date"/></div>
        <div class="form-group"><label>Base Salary (₱) *</label><input id="ae-base" type="number" placeholder="30000"/></div>
        <div class="form-group"><label>Hourly Rate (₱) *</label><input id="ae-hourly" type="number" placeholder="150"/></div>
        <div class="form-group"><label>Role</label>
          <select id="ae-role"><option value="1">Employee</option><option value="0">Admin</option></select></div>
      </div>
      <div class="form-actions">
        <button class="btn btn-ghost" onclick="navTo('page-employees')">Cancel</button>
        <button class="btn btn-primary" style="width:auto" onclick="submitAddEmployee()">Add Employee</button>
      </div>
    </div>`;
  // Set today as default hire date
  document.getElementById('ae-hired').value = new Date().toISOString().split('T')[0];
}

async function submitAddEmployee() {
  const body = {
    action:      'add',
    username:    document.getElementById('ae-username').value.trim(),
    password:    document.getElementById('ae-password').value,
    full_name:   document.getElementById('ae-fullname').value.trim(),
    department:  document.getElementById('ae-dept').value.trim(),
    position:    document.getElementById('ae-position').value.trim(),
    email:       document.getElementById('ae-email').value.trim(),
    phone:       document.getElementById('ae-phone').value.trim(),
    date_hired:  document.getElementById('ae-hired').value,
    base_salary: parseFloat(document.getElementById('ae-base').value) || 0,
    hourly_rate: parseFloat(document.getElementById('ae-hourly').value) || 0,
    role:        parseInt(document.getElementById('ae-role').value),
  };
  const r = await api('employees', body);
  const alertEl = document.getElementById('add-emp-alert');
  if (!r.ok) {
    alertEl.innerHTML = `<div class="alert alert-error">✗ ${r.error}</div>`;
    return;
  }
  alertEl.innerHTML = `<div class="alert alert-success">✓ Employee added successfully (ID: ${r.id})</div>`;
  setTimeout(() => navTo('page-employees'), 1200);
}

async function showEditEmployee(id) {
  const r = await api('employees', { action: 'get', id });
  if (!r.ok) { alert(r.error); return; }
  const e = r.employee;
  document.getElementById('modal-body').innerHTML = `
    <h3 style="font-family:var(--serif);font-size:1.4rem;margin-bottom:20px">Edit Employee</h3>
    <div id="edit-alert"></div>
    <div class="form-grid">
      <div class="form-group full"><label>Full Name</label><input id="edit-name" value="${e.full_name}"/></div>
      <div class="form-group"><label>Department</label><input id="edit-dept" value="${e.department}"/></div>
      <div class="form-group"><label>Position</label><input id="edit-pos" value="${e.position}"/></div>
      <div class="form-group"><label>Email</label><input id="edit-email" value="${e.email}"/></div>
      <div class="form-group"><label>Phone</label><input id="edit-phone" value="${e.phone}"/></div>
      <div class="form-group"><label>Base Salary</label><input id="edit-base" type="number" value="${e.base_salary}"/></div>
      <div class="form-group"><label>Hourly Rate</label><input id="edit-hourly" type="number" value="${e.hourly_rate}"/></div>
      <div class="form-group full"><label>New Password (leave blank to keep)</label><input id="edit-pass" type="text" placeholder=""/></div>
    </div>
    <div class="form-actions">
      <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
      <button class="btn btn-primary" style="width:auto" onclick="submitEditEmployee(${id})">Save Changes</button>
    </div>`;
  openModal();
}

async function submitEditEmployee(id) {
  const body = {
    action:      'update', id,
    full_name:   document.getElementById('edit-name').value.trim(),
    department:  document.getElementById('edit-dept').value.trim(),
    position:    document.getElementById('edit-pos').value.trim(),
    email:       document.getElementById('edit-email').value.trim(),
    phone:       document.getElementById('edit-phone').value.trim(),
    base_salary: parseFloat(document.getElementById('edit-base').value) || 0,
    hourly_rate: parseFloat(document.getElementById('edit-hourly').value) || 0,
    password:    document.getElementById('edit-pass').value,
  };
  const r = await api('employees', body);
  if (!r.ok) {
    document.getElementById('edit-alert').innerHTML = `<div class="alert alert-error">✗ ${r.error}</div>`;
    return;
  }
  closeModal();
  buildEmployees();
}

async function deactivateEmployee(id, name) {
  if (!confirm(`Remove ${name} from the system?`)) return;
  const r = await api('employees', { action: 'deactivate', id });
  if (!r.ok) { alert(r.error); return; }
  buildEmployees();
}

// ── PAYROLL LIST ──────────────────────────────────────────────
async function buildPayrollList() {
  const pg = document.getElementById('page-payroll');
  pg.innerHTML = '<p class="text-muted">Loading...</p>';
  const r = await api('payroll', { action: 'list', employee_id: 0 });
  if (!r.ok) { pg.innerHTML = `<div class="alert alert-error">${r.error}</div>`; return; }

  const rows = (r.records || []).map(p => `
    <tr>
      <td>${p.id}</td>
      <td>${p.employee_name}</td>
      <td>${p.period_start} – ${p.period_end}</td>
      <td>${peso(p.gross_pay)}</td>
      <td>${peso(p.total_deductions)}</td>
      <td><strong>${peso(p.net_pay)}</strong></td>
      <td><span class="badge badge-${p.status}">${p.status}</span></td>
      <td>
        <button class="btn btn-outline sm" onclick="showPayslip(${p.id})">View</button>
        ${p.status==='generated'
          ? `<button class="btn btn-sage sm" onclick="releasePayroll(${p.id})">Release</button>`
          : ''}
      </td>
    </tr>`).join('');

  pg.innerHTML = `
    <div class="page-header">
      <div><h2>Payroll Records</h2><p>${(r.records||[]).length} records total</p></div>
      <button class="btn btn-primary" style="width:auto" onclick="navTo('page-calc')">+ New Payroll</button>
    </div>
    <div class="card">
      <div class="table-wrap">
        <table>
          <thead><tr>
            <th>#</th><th>Employee</th><th>Period</th>
            <th>Gross</th><th>Deductions</th><th>Net Pay</th><th>Status</th><th>Actions</th>
          </tr></thead>
          <tbody>${rows || '<tr><td colspan="8" style="text-align:center;padding:24px" class="text-muted">No payroll records yet.</td></tr>'}</tbody>
        </table>
      </div>
    </div>`;
}

async function releasePayroll(id) {
  if (!confirm('Mark this payroll as released?')) return;
  const r = await api('payroll', { action: 'release', id });
  if (!r.ok) { alert(r.error); return; }
  buildPayrollList();
}

async function showPayslip(id) {
  const r = await api('payroll', { action: 'get', id });
  if (!r.ok) { alert(r.error); return; }
  const p = r.payroll;
  document.getElementById('modal-body').innerHTML = `
    <div class="payslip">
      <div class="payslip-header">
        <h3>PayDay</h3>
        <p>Pay Slip — ${p.period_start} to ${p.period_end}</p>
        <p style="margin-top:4px;font-weight:600">${p.employee_name}</p>
      </div>
      <div class="payslip-row"><span class="label">Base Salary</span><span class="value">${peso(p.base_salary)}</span></div>
      <div class="payslip-row"><span class="label">Overtime (${p.overtime_hours}h × ₱${p.overtime_rate})</span><span class="value">${peso(p.overtime_pay)}</span></div>
      <div class="payslip-row"><span class="label" style="font-weight:600">Gross Pay</span><span class="value" style="color:var(--sage-2)">${peso(p.gross_pay)}</span></div>
      <div class="divider"></div>
      <div class="payslip-row deduction"><span class="label">Income Tax (15%)</span><span class="value">- ${peso(p.tax_deduction)}</span></div>
      <div class="payslip-row deduction"><span class="label">SSS (4.5%)</span><span class="value">- ${peso(p.sss_deduction)}</span></div>
      <div class="payslip-row deduction"><span class="label">PhilHealth (2.5%)</span><span class="value">- ${peso(p.philhealth_deduction)}</span></div>
      <div class="payslip-row deduction"><span class="label">Pag-IBIG</span><span class="value">- ${peso(p.pagibig_deduction)}</span></div>
      <div class="payslip-row deduction"><span class="label">Other Deductions</span><span class="value">- ${peso(p.other_deductions)}</span></div>
      <div class="payslip-row deduction"><span class="label" style="font-weight:600">Total Deductions</span><span class="value" style="font-weight:700">- ${peso(p.total_deductions)}</span></div>
      <div class="payslip-row total"><span class="label">NET PAY</span><span class="value">${peso(p.net_pay)}</span></div>
      <div style="margin-top:16px;text-align:center">
        <span class="badge badge-${p.status}" style="font-size:.8rem">${p.status.toUpperCase()}</span>
        <p class="text-muted" style="font-size:.78rem;margin-top:8px">Generated ${p.generated_date}</p>
      </div>
    </div>
    <div class="form-actions">
      <button class="btn btn-ghost" onclick="closeModal()">Close</button>
    </div>`;
  openModal();
}

// ── CALCULATE PAYROLL ─────────────────────────────────────────
async function buildCalc() {
  const pg = document.getElementById('page-calc');
  pg.innerHTML = '<p class="text-muted">Loading employees...</p>';
  const r = await api('employees', { action: 'list' });
  if (!r.ok) { pg.innerHTML = `<div class="alert alert-error">${r.error}</div>`; return; }

  const empOpts = (r.employees || [])
    .filter(e => e.role !== 0)
    .map(e => `<option value="${e.id}">${e.full_name} — ${e.department}</option>`)
    .join('');

  // Default period: this month
  const now = new Date();
  const y = now.getFullYear(), m = String(now.getMonth()+1).padStart(2,'0');
  const lastDay = new Date(y, now.getMonth()+1, 0).getDate();
  const ps = `${y}-${m}-01`, pe = `${y}-${m}-${lastDay}`;

  pg.innerHTML = `
    <div class="page-header">
      <div><h2>Calculate Payroll</h2><p>Compute salary with overtime and deductions</p></div>
    </div>
    <div class="card" style="max-width:640px">
      <div id="calc-alert"></div>
      <div class="form-grid">
        <div class="form-group full">
          <label>Employee *</label>
          <select id="calc-emp">${empOpts}</select>
        </div>
        <div class="form-group"><label>Period Start *</label><input id="calc-ps" type="date" value="${ps}"/></div>
        <div class="form-group"><label>Period End *</label>  <input id="calc-pe" type="date" value="${pe}"/></div>
        <div class="form-group"><label>Overtime Hours</label><input id="calc-ot" type="number" value="0" min="0" step="0.5"/></div>
        <div class="form-group"><label>Other Deductions (₱)</label><input id="calc-ded" type="number" value="0" min="0"/></div>
      </div>
      <div id="calc-preview" class="mt-24"></div>
      <div class="form-actions">
        <button class="btn btn-outline" onclick="previewCalc()">Preview</button>
        <button class="btn btn-primary" style="width:auto" onclick="submitCalc()">Generate Payroll</button>
      </div>
    </div>`;
}

async function getCalcBody() {
  return {
    action:           'calculate',
    employee_id:      parseInt(document.getElementById('calc-emp').value),
    period_start:     document.getElementById('calc-ps').value,
    period_end:       document.getElementById('calc-pe').value,
    overtime_hours:   parseFloat(document.getElementById('calc-ot').value)  || 0,
    other_deductions: parseFloat(document.getElementById('calc-ded').value) || 0,
  };
}

async function previewCalc() {
  // Get employee details to show preview without saving
  const empId = parseInt(document.getElementById('calc-emp').value);
  const er = await api('employees', { action: 'get', id: empId });
  if (!er.ok) return;
  const e = er.employee;
  const ot    = parseFloat(document.getElementById('calc-ot').value)  || 0;
  const other = parseFloat(document.getElementById('calc-ded').value) || 0;
  const overtimePay = e.hourly_rate * ot * 1.25;
  const gross = e.base_salary + overtimePay;
  const tax   = gross * 0.15;
  const sss   = gross * 0.045;
  const ph    = gross * 0.025;
  const pi    = 100;
  const totalDed = tax + sss + ph + pi + other;
  const net = Math.max(0, gross - totalDed);

  document.getElementById('calc-preview').innerHTML = `
    <div style="background:var(--cream);border-radius:10px;padding:20px;border:1px solid var(--cream-3)">
      <div style="font-family:var(--serif);font-size:1rem;margin-bottom:12px">Preview — ${e.full_name}</div>
      <div class="payslip-row"><span class="label">Base Salary</span><span class="value">${peso(e.base_salary)}</span></div>
      <div class="payslip-row"><span class="label">Overtime Pay</span><span class="value">${peso(overtimePay)}</span></div>
      <div class="payslip-row"><span class="label">Gross Pay</span><span class="value" style="color:var(--sage-2);font-weight:700">${peso(gross)}</span></div>
      <div class="payslip-row deduction"><span class="label">Total Deductions</span><span class="value">- ${peso(totalDed)}</span></div>
      <div class="payslip-row total"><span class="label">Estimated Net</span><span class="value">${peso(net)}</span></div>
    </div>`;
}

async function submitCalc() {
  const body = await getCalcBody();
  if (!body.employee_id || !body.period_start || !body.period_end)
    return document.getElementById('calc-alert').innerHTML =
      '<div class="alert alert-error">Please fill in all required fields.</div>';

  const r = await api('payroll', body);
  const alertEl = document.getElementById('calc-alert');
  if (!r.ok) {
    alertEl.innerHTML = `<div class="alert alert-error">✗ ${r.error}</div>`; return;
  }
  alertEl.innerHTML = `<div class="alert alert-success">✓ Payroll generated! Net pay: ${peso(r.payroll.net_pay)}</div>`;
  // Show payslip in modal
  setTimeout(() => showPayslip(r.payroll.id), 600);
}

// ── REPORT ────────────────────────────────────────────────────
async function buildReport() {
  const pg = document.getElementById('page-report');
  const now = new Date();
  const y = now.getFullYear(), m = String(now.getMonth()+1).padStart(2,'0');
  const lastDay = new Date(y, now.getMonth()+1, 0).getDate();

  pg.innerHTML = `
    <div class="page-header">
      <div><h2>Payroll Report</h2><p>Summary of payroll activity</p></div>
    </div>
    <div class="card" style="max-width:640px;margin-bottom:24px">
      <div class="form-grid">
        <div class="form-group"><label>Period Start</label><input id="rep-ps" type="date" value="${y}-${m}-01"/></div>
        <div class="form-group"><label>Period End</label>  <input id="rep-pe" type="date" value="${y}-${m}-${lastDay}"/></div>
      </div>
      <div class="form-actions">
        <button class="btn btn-primary" style="width:auto" onclick="runReport()">Generate Report</button>
      </div>
    </div>
    <div id="report-result"></div>`;
}

async function runReport() {
  const ps = document.getElementById('rep-ps').value;
  const pe = document.getElementById('rep-pe').value;
  const r  = await api('report', { period_start: ps, period_end: pe });
  const el = document.getElementById('report-result');
  if (!r.ok) { el.innerHTML = `<div class="alert alert-error">${r.error}</div>`; return; }

  const rows = (r.rows || []).map(row => `
    <tr>
      <td>${row.employee_name}</td>
      <td>${row.department}</td>
      <td>${row.period_start} – ${row.period_end}</td>
      <td>${peso(row.gross_pay)}</td>
      <td>${peso(row.total_deductions)}</td>
      <td><strong>${peso(row.net_pay)}</strong></td>
      <td><span class="badge badge-${row.status}">${row.status}</span></td>
    </tr>`).join('');

  el.innerHTML = `
    <div class="stats-grid" style="margin-bottom:20px">
      <div class="stat-card sage"><div class="stat-label">Records</div><div class="stat-value">${r.record_count}</div></div>
      <div class="stat-card blush"><div class="stat-label">Total Gross</div><div class="stat-value" style="font-size:1.3rem">${peso(r.total_gross)}</div></div>
      <div class="stat-card sand"><div class="stat-label">Total Net</div><div class="stat-value" style="font-size:1.3rem">${peso(r.total_net)}</div></div>
    </div>
    <div class="card">
      <div class="table-wrap">
        <table>
          <thead><tr>
            <th>Employee</th><th>Department</th><th>Period</th>
            <th>Gross</th><th>Deductions</th><th>Net Pay</th><th>Status</th>
          </tr></thead>
          <tbody>
            ${rows || '<tr><td colspan="7" style="text-align:center;padding:24px" class="text-muted">No records found for this period.</td></tr>'}
            ${r.record_count > 0 ? `
            <tr class="report-total-row">
              <td colspan="3"><strong>TOTAL</strong></td>
              <td><strong>${peso(r.total_gross)}</strong></td>
              <td><strong>${peso(r.total_deductions)}</strong></td>
              <td><strong>${peso(r.total_net)}</strong></td>
              <td></td>
            </tr>` : ''}
          </tbody>
        </table>
      </div>
    </div>`;
}

// ── MY PAYROLL (employee view) ────────────────────────────────
async function buildMyPayroll() {
  const pg = document.getElementById('page-my-payroll');
  pg.innerHTML = '<p class="text-muted">Loading...</p>';
  const r = await api('payroll', { action: 'list', employee_id: currentUser.id });
  if (!r.ok) { pg.innerHTML = `<div class="alert alert-error">${r.error}</div>`; return; }

  const rows = (r.records || []).map(p => `
    <tr>
      <td>${p.period_start} – ${p.period_end}</td>
      <td>${peso(p.gross_pay)}</td>
      <td>${peso(p.total_deductions)}</td>
      <td><strong>${peso(p.net_pay)}</strong></td>
      <td><span class="badge badge-${p.status}">${p.status}</span></td>
      <td><button class="btn btn-outline sm" onclick="showPayslip(${p.id})">View Slip</button></td>
    </tr>`).join('');

  pg.innerHTML = `
    <div class="page-header">
      <div><h2>My Payroll</h2><p>${currentUser.full_name || currentUser.username}</p></div>
    </div>
    <div class="card">
      <div class="table-wrap">
        <table>
          <thead><tr>
            <th>Period</th><th>Gross Pay</th><th>Deductions</th><th>Net Pay</th><th>Status</th><th></th>
          </tr></thead>
          <tbody>${rows || '<tr><td colspan="6" style="text-align:center;padding:24px" class="text-muted">No payroll records yet.</td></tr>'}</tbody>
        </table>
      </div>
    </div>`;
}

// ── Modal ─────────────────────────────────────────────────────
function openModal()  { document.getElementById('modal-overlay').classList.remove('hidden'); }
function closeModal() { document.getElementById('modal-overlay').classList.add('hidden'); }

// ── Override navTo for add-emp page ──────────────────────────
const _navTo = navTo;
window.navTo = function(page) {
  if (page === 'page-add-emp') { navToAddEmp(); return; }
  _navTo(page);
};
