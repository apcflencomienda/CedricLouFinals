<?php
// ============================================
// Lumos - Get Latest Command (Arduino polls this)
// ============================================

require_once 'db.php';

// --- Get the latest AI response ---
$result = $conn->query("SELECT color_hex, message, buzzer, created_at FROM ai_responses ORDER BY id DESC LIMIT 1");

if ($row = $result->fetch_assoc()) {
    echo json_encode([
        'color_hex' => $row['color_hex'],
        'message' => $row['message'],
        'buzzer' => (bool) $row['buzzer'],
        'timestamp' => $row['created_at']
    ]);
} else {
    echo json_encode([
        'color_hex' => '#4488FF',
        'message' => 'Lumos Ready!',
        'buzzer' => false,
        'timestamp' => date('Y-m-d H:i:s')
    ]);
}
?>