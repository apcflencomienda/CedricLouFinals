<?php
// ...existing code...

echo "Testing Groq Llama 3 API Connection...\n";
$prompt = "You are Lumos. Reply with a valid JSON object ONLY: {\"color_hex\": \"#00FF00\", \"message\": \"Groq works!\", \"buzzer\": false}";

$response = callLlama($prompt);
echo "Raw Response:\n" . $response . "\n";
?>