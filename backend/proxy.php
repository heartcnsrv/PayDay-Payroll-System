<?php
// ============================================================
//  Forwards every /api/* request to the C backend on :8081.
// ============================================================

define('C_SERVER', 'http://127.0.0.1:8081');

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(200); exit; }

$uri      = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);
$segments = array_values(array_filter(explode('/', trim($uri, '/')), 'strlen'));
$endpoint = '';
foreach (array_reverse($segments) as $seg) {
    if ($seg !== 'api') { $endpoint = trim($seg); break; }
}

$allowed = ['auth', 'employees', 'payroll', 'report'];
if (!in_array($endpoint, $allowed, true)) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Unknown endpoint: ' . $endpoint]);
    exit;
}

$body = file_get_contents('php://input');

$ch = curl_init(C_SERVER . '/' . $endpoint);
curl_setopt_array($ch, [
    CURLOPT_POST           => true,
    CURLOPT_POSTFIELDS     => $body,
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_TIMEOUT        => 10,
    CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
]);

$response = curl_exec($ch);
$code     = curl_getinfo($ch, CURLINFO_HTTP_CODE);
$error    = curl_error($ch);
curl_close($ch);

if ($response === false || $error) {
    http_response_code(502);
    echo json_encode([
        'ok'    => false,
        'error' => 'C backend unreachable. Is PayDayServer running? (' . $error . ')'
    ]);
    exit;
}

http_response_code($code ?: 200);
echo $response;
