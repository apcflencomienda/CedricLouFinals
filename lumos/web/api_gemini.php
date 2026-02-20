<?php
// ============================================
// Lumos - Gemini API Helper
// ============================================

require_once 'db.php';

/**
 * Call Google Gemini API with a prompt
 * @param string $prompt The prompt to send
 * @return string The AI response text
 */
function callGemini($prompt)
{
    $url = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=' . constant('Default Gemini API Key');

    $data = [
        'contents' => [
            [
                'parts' => [
                    ['text' => $prompt]
                ]
            ]
        ],
        'generationConfig' => [
            'temperature' => 0.7,
            'maxOutputTokens' => 256
        ]
    ];

    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_TIMEOUT, 30);

    $response = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($httpCode !== 200) {
        error_log("Gemini API error (HTTP $httpCode): $response");
        return json_encode(['color_hex' => '#FFFFFF', 'message' => 'AI unavailable', 'buzzer' => false]);
    }

    $result = json_decode($response, true);

    if (isset($result['candidates'][0]['content']['parts'][0]['text'])) {
        return $result['candidates'][0]['content']['parts'][0]['text'];
    }

    return json_encode(['color_hex' => '#FFFFFF', 'message' => 'No response', 'buzzer' => false]);
}
?>