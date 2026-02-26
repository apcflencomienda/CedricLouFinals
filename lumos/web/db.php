<?php
// ============================================
// Lumos - Database & Config
// ============================================

// --- API KEY CONFIGURATION ---
// Load LLAMA_API_KEY from the .env file located in the env folder
$envFilePath = __DIR__ . '/../../env/.env';
if (file_exists($envFilePath)) {
    $envLines = file($envFilePath, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    foreach ($envLines as $line) {
        if (strpos(trim($line), '#') === 0)
            continue; // Skip comments
        list($name, $value) = explode('=', $line, 2);
        putenv(trim($name) . '=' . trim($value));
    }
}
define('LLAMA_API_KEY', getenv('LLAMA_API_KEY') ? getenv('LLAMA_API_KEY') : 'MISSING_KEY');
// --- Database Config ---
define('DB_HOST', 'localhost');
define('DB_USER', 'root');
define('DB_PASS', '');
define('DB_NAME', 'lumos_db');

// --- Connect to MySQL ---
$conn = new mysqli(DB_HOST, DB_USER, DB_PASS, DB_NAME, 3307);
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