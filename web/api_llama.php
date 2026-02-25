<?php
// ============================================
// Lumos - Llama API Helper (Production)
// ============================================

require_once 'db.php';

/**
 * Call Llama API via Groq
 * @param string $prompt The prompt to send
 * @return string The AI response text
 */
function callLlama($prompt)
{
    $url = 'https://api.groq.com/openai/v1/chat/completions';

    $data = [
        'model' => 'llama-3.3-70b-versatile',
        'messages' => [
            ['role' => 'user', 'content' => $prompt]
        ],
        'temperature' => 0.7,
        'max_tokens' => 256
    ];

    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Content-Type: application/json',
        'Authorization: Bearer ' . LLAMA_API_KEY
    ]);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_TIMEOUT, 30);

    $response = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $curlError = curl_error($ch);
    curl_close($ch);

    if ($curlError) {
        error_log("Llama CURL error: $curlError");
        return "Error: Could not reach API";
    }

    if ($httpCode !== 200) {
        error_log("Llama API error (HTTP $httpCode): $response");
        return "Error: API returned HTTP $httpCode";
    }

    // LOG EVERYTHING FOR DEBUGGING
    error_log("RAW RESPONSE FROM LLAMA: " . $response);

    $result = json_decode($response, true);

    if (isset($result['choices'][0]['message']['content'])) {
        return $result['choices'][0]['message']['content'];
    }
    error_log("Llama response missing expected fields: " . json_encode($result));
    return json_encode([
        "color_hex" => "#FFFFFF",
        "message" => "Error: Parse Failed",
        "buzzer" => false
    ]);
}
?>