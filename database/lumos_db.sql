-- Lumos: The AI Environment Companion
-- Database Schema

CREATE DATABASE IF NOT EXISTS lumos_db;
USE lumos_db;

-- Sensor readings from Arduino
CREATE TABLE IF NOT EXISTS sensor_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT NOT NULL,
    light_level FLOAT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- AI responses from Gemini
CREATE TABLE IF NOT EXISTS ai_responses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sensor_log_id INT NULL,
    color_hex VARCHAR(7) DEFAULT '#FFFFFF',
    message VARCHAR(255) DEFAULT '',
    buzzer TINYINT(1) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sensor_log_id) REFERENCES sensor_logs(id) ON DELETE SET NULL
);

-- Chat messages between user and AI
CREATE TABLE IF NOT EXISTS chat_history (
    id INT AUTO_INCREMENT PRIMARY KEY,
    role ENUM('user', 'ai') NOT NULL,
    message TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- System settings (current location, preferences)
CREATE TABLE IF NOT EXISTS settings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    key_name VARCHAR(50) UNIQUE NOT NULL,
    value VARCHAR(255) NOT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Pre-seed default location
INSERT INTO settings (key_name, value) VALUES ('current_location', 'default')
ON DUPLICATE KEY UPDATE value = value;
