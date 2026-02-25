<?php
// ============================================
// Lumos - Sensor Data Endpoint
// Arduino POSTs sensor data here
// ============================================

// ...existing code...

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'POST method required']);
    exit();
}

// --- Get sensor data ---
$input = json_decode(file_get_contents('php://input'), true);

// Support both JSON body and form data
$temperature = $input['temperature'] ?? ($_POST['temperature'] ?? null);
$light_level = $input['light_level'] ?? ($_POST['light_level'] ?? null);

if ($temperature === null || $light_level === null) {
    http_response_code(400);
    echo json_encode(['error' => 'Missing temperature or light_level']);
    exit();
}

$temperature = floatval($temperature);
$light_level = floatval($light_level);

// --- Store sensor reading ---
$stmt = $conn->prepare("INSERT INTO sensor_logs (temperature, light_level) VALUES (?, ?)");
$stmt->bind_param("dd", $temperature, $light_level);
$stmt->execute();
$sensorLogId = $conn->insert_id;
$stmt->close();

// --- Get current location context ---
$locResult = $conn->query("SELECT value FROM settings WHERE key_name = 'current_location'");
$location = 'default';
if ($row = $locResult->fetch_assoc()) {
    $location = $row['value'];
}

// --- Build AI prompt ---
$prompt = "You are Lumos, an AI environment companion. The user is currently in: \"$location\".

Current sensor readings:
- Temperature: {$temperature}°C
- Light Level: {$light_level}% (0% = dark, 100% = bright)

Based on the location context and sensor data, provide environmental advice.
Respond with ONLY a valid JSON object (no markdown, no code fences) with these exact keys:
{
  \"color_hex\": \"#RRGGBB hex color representing the mood/status\",
  \"message\": \"A short message (max 30 chars) for the LED matrix display\",
  \"buzzer\": true/false (only true if conditions are extreme/dangerous)
}

Guidelines:
- For comfortable conditions: use calming colors (blues, greens)
- For warm/hot: use warm colors (orange, red)
- For cold: use cool colors (cyan, purple)
- For dark rooms: suggest brighter lighting
- Adjust thresholds based on location (bedroom = relaxed, office = productive, kitchen = alert)
- buzzer should only be true for extreme temps (>35°C or <10°C) or very dark conditions in dangerous contexts";

// ...existing code...

// --- Parse AI response ---
// Clean up response - remove any markdown code fences
$aiResponse = preg_replace('/```json\s*/', '', $aiResponse);
$aiResponse = preg_replace('/```\s*/', '', $aiResponse);
$aiResponse = trim($aiResponse);

$parsed = json_decode($aiResponse, true);

// Fallback defaults
$colorHex = $parsed['color_hex'] ?? '#FFFFFF';
$message = $parsed['message'] ?? 'Reading sensors...';
$buzzer = $parsed['buzzer'] ?? false;

// Ensure message fits LED matrix
$message = substr($message, 0, 50);

// --- Store AI response ---
$stmt = $conn->prepare("INSERT INTO ai_responses (sensor_log_id, color_hex, message, buzzer) VALUES (?, ?, ?, ?)");
$buzzerInt = $buzzer ? 1 : 0;
$stmt->bind_param("issi", $sensorLogId, $colorHex, $message, $buzzerInt);
$stmt->execute();
$stmt->close();

// --- Return response ---
echo json_encode([
    'success' => true,
    'sensor_log_id' => $sensorLogId,
    'color_hex' => $colorHex,
    'message' => $message,
    'buzzer' => $buzzer
]);
?>