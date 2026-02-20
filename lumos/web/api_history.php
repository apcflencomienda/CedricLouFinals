<?php
// ============================================
// Lumos - History Endpoint (Dashboard fetches this)
// ============================================

require_once 'db.php';

$type = $_GET['type'] ?? 'sensor';

if ($type === 'sensor') {
    // --- Get sensor history with AI responses ---
    $result = $conn->query("
        SELECT 
            s.id,
            s.temperature,
            s.light_level,
            s.created_at,
            a.color_hex,
            a.message AS ai_message,
            a.buzzer
        FROM sensor_logs s
        LEFT JOIN ai_responses a ON a.sensor_log_id = s.id
        ORDER BY s.id DESC
        LIMIT 50
    ");

    $logs = [];
    while ($row = $result->fetch_assoc()) {
        $row['buzzer'] = (bool) $row['buzzer'];
        $logs[] = $row;
    }

    echo json_encode(['success' => true, 'data' => $logs]);

} elseif ($type === 'chat') {
    // --- Get chat history ---
    $result = $conn->query("
        SELECT id, role, message, created_at
        FROM chat_history
        ORDER BY id ASC
        LIMIT 50
    ");

    $messages = [];
    while ($row = $result->fetch_assoc()) {
        $messages[] = $row;
    }

    echo json_encode(['success' => true, 'data' => $messages]);

} elseif ($type === 'latest') {
    // --- Get latest sensor reading + AI response ---
    $result = $conn->query("
        SELECT 
            s.temperature,
            s.light_level,
            s.created_at,
            a.color_hex,
            a.message AS ai_message,
            a.buzzer
        FROM sensor_logs s
        LEFT JOIN ai_responses a ON a.sensor_log_id = s.id
        ORDER BY s.id DESC
        LIMIT 1
    ");

    if ($row = $result->fetch_assoc()) {
        $row['buzzer'] = (bool) $row['buzzer'];
        echo json_encode(['success' => true, 'data' => $row]);
    } else {
        echo json_encode(['success' => true, 'data' => null]);
    }

} elseif ($type === 'location') {
    // --- Get current location ---
    $result = $conn->query("SELECT value FROM settings WHERE key_name = 'current_location'");
    $location = 'default';
    if ($row = $result->fetch_assoc()) {
        $location = $row['value'];
    }
    echo json_encode(['success' => true, 'location' => $location]);

} else {
    http_response_code(400);
    echo json_encode(['error' => 'Invalid type. Use: sensor, chat, latest, or location']);
}
?>