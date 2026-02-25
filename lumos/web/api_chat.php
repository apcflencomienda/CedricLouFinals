<?php
// ============================================
// Lumos - Chat Endpoint
// Frontend sends chat messages here
// ============================================

// ...existing code...

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'POST method required']);
    exit();
}

// --- Get message ---
$input = json_decode(file_get_contents('php://input'), true);
$userMessage = $input['message'] ?? ($_POST['message'] ?? '');

if (empty(trim($userMessage))) {
    http_response_code(400);
    echo json_encode(['error' => 'Message is required']);
    exit();
}

// --- Store user message ---
$stmt = $conn->prepare("INSERT INTO chat_history (role, message) VALUES ('user', ?)");
$stmt->bind_param("s", $userMessage);
$stmt->execute();
$stmt->close();

// --- Check for location update ---
$locationPatterns = [
    '/(?:i am|i\'m|im|moved to|now in|going to|at the|in the|in my|at my)\s+(?:the\s+)?(\w[\w\s]*?)(?:\s+now)?$/i',
    '/^(\w[\w\s]*?)(?:\s+now)?$/i'
];

$detectedLocation = null;
foreach ($locationPatterns as $pattern) {
    if (preg_match($pattern, trim($userMessage), $matches)) {
        $detectedLocation = trim($matches[1]);
        break;
    }
}

// Update location if detected
if ($detectedLocation) {
    $stmt = $conn->prepare("UPDATE settings SET value = ? WHERE key_name = 'current_location'");
    $stmt->bind_param("s", $detectedLocation);
    $stmt->execute();
    $stmt->close();
}

// --- Get current location ---
$locResult = $conn->query("SELECT value FROM settings WHERE key_name = 'current_location'");
$location = 'default';
if ($row = $locResult->fetch_assoc()) {
    $location = $row['value'];
}

// --- Get recent chat context ---
$chatContext = "";
$chatResult = $conn->query("SELECT role, message FROM chat_history ORDER BY id DESC LIMIT 6");
$messages = [];
while ($row = $chatResult->fetch_assoc()) {
    $messages[] = $row;
}
$messages = array_reverse($messages);
foreach ($messages as $msg) {
    $prefix = $msg['role'] === 'user' ? 'User' : 'Lumos';
    $chatContext .= "$prefix: {$msg['message']}\n";
}

// --- Get latest sensor data ---
$sensorContext = "";
$sensorResult = $conn->query("SELECT temperature, light_level FROM sensor_logs ORDER BY id DESC LIMIT 1");
if ($row = $sensorResult->fetch_assoc()) {
    $sensorContext = "Latest sensor data - Temperature: {$row['temperature']}°C, Light: {$row['light_level']}%";
}

// --- Build AI prompt ---
$prompt = "You are Lumos, a friendly AI environment companion for a smart workspace. 
Current location context: \"$location\"
$sensorContext

Recent conversation:
$chatContext

The user just said: \"$userMessage\"

Respond naturally and helpfully. If the user mentions moving to a new location, acknowledge the change and explain how you'll adjust the environment (lighting, temperature comfort thresholds).

Keep your response concise (2-3 sentences max). Be warm and conversational.

After your conversational response, on a new line, provide a JSON object for the Arduino:
{\"color_hex\": \"#RRGGBB\", \"message\": \"short LED text (max 30 chars)\", \"buzzer\": false}

The color should reflect the mood of the conversation or the new environment setting.";

// ...existing code...

// --- Separate conversational response from JSON ---
$parts = preg_split('/(\{[^}]+\})\s*$/', $aiResponse, -1, PREG_SPLIT_DELIM_CAPTURE);
$conversationalResponse = trim($parts[0] ?? $aiResponse);
$jsonPart = $parts[1] ?? null;

// Clean markdown from conversational response
$conversationalResponse = preg_replace('/```json\s*/', '', $conversationalResponse);
$conversationalResponse = preg_replace('/```\s*/', '', $conversationalResponse);
$conversationalResponse = trim($conversationalResponse);

// --- Store AI response in chat ---
$stmt = $conn->prepare("INSERT INTO chat_history (role, message) VALUES ('ai', ?)");
$stmt->bind_param("s", $conversationalResponse);
$stmt->execute();
$stmt->close();

// --- Parse and store Arduino command if JSON found ---
if ($jsonPart) {
    $parsed = json_decode($jsonPart, true);
    if ($parsed) {
        $colorHex = $parsed['color_hex'] ?? '#4488FF';
        $message = substr($parsed['message'] ?? 'Mode updated', 0, 50);
        $buzzerInt = ($parsed['buzzer'] ?? false) ? 1 : 0;

        $stmt = $conn->prepare("INSERT INTO ai_responses (color_hex, message, buzzer) VALUES (?, ?, ?)");
        $stmt->bind_param("ssi", $colorHex, $message, $buzzerInt);
        $stmt->execute();
        $stmt->close();
    }
}

// --- Return response ---
echo json_encode([
    'success' => true,
    'reply' => $conversationalResponse,
    'location' => $location
]);
?>