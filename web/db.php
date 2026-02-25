<?php
// ============================================
// Lumos - Database & Config
// ============================================

// --- Load .env file for secrets ---
$envPath = __DIR__ . '/../env/.env';
if (file_exists($envPath)) {
    $lines = file($envPath, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    foreach ($lines as $line) {
        if (strpos(trim($line), '#') === 0) continue;
        list($name, $value) = array_map('trim', explode('=', $line, 2));
        if (!getenv($name)) {
            putenv("{$name}={$value}");
            $_ENV[$name] = $value;
        }
    }
}

// --- LLAMA API KEY (Production) ---
// NOTE: The Groq API key is loaded securely from env/.env and never hardcoded in this file.
define('LLAMA_API_KEY', getenv('LLAMA_API_KEY'));

// --- Database Config ---
define('DB_HOST', 'localhost');
define('DB_USER', 'root');
define('DB_PASS', '');
define('DB_NAME', 'lumos_db');

// --- Connect to MySQL ---
$conn = new mysqli("localhost", "root", "", "lumos_db", 3307);
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