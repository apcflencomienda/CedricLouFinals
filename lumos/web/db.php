<?php
// ============================================
// Lumos - Database & Config
// ============================================

// --- GEMINI API KEY ---
// Get your free key at: https://aistudio.google.com/apikey
// Removed Gemini API Key for security reasons.

// --- Database Config ---
define('DB_HOST', 'localhost');
define('DB_USER', 'root');
define('DB_PASS', '');
define('DB_NAME', 'lumos_db');

// --- Connect to MySQL ---
$conn = new mysqli(DB_HOST, DB_USER, DB_PASS, DB_NAME);
if ($conn->connect_error) {
    http_response_code(500);
    die(json_encode(['error' => 'Database connection failed: ' . $conn->connect_error]));
}

// --- CORS Headers (allow Arduino + frontend) ---
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}
?>