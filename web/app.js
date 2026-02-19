// ============================================
// Lumos - Dashboard JavaScript
// ============================================

const API_BASE = './';  // Same directory as index.html

// State
let previousTemp = null;
let previousLight = null;

// ============================================
// INITIALIZATION
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    loadLatestData();
    loadSensorHistory();
    loadChatHistory();
    loadLocation();

    // Auto-refresh every 5 seconds
    setInterval(loadLatestData, 5000);
    setInterval(loadLocation, 10000);

    // Chat form handler
    document.getElementById('chatForm').addEventListener('submit', handleChatSubmit);
});

// ============================================
// LOAD LATEST SENSOR DATA
// ============================================
async function loadLatestData() {
    try {
        const res = await fetch(`${API_BASE}api_history.php?type=latest`);
        const json = await res.json();

        if (json.success && json.data) {
            updateSensorDisplay(json.data);
        }
    } catch (err) {
        console.error('Failed to load latest data:', err);
    }
}

function updateSensorDisplay(data) {
    const temp = parseFloat(data.temperature);
    const light = parseFloat(data.light_level);
    const colorHex = data.color_hex || '#6366f1';
    const message = data.ai_message || 'Monitoring...';
    const buzzer = data.buzzer;

    // Update temperature
    const tempEl = document.getElementById('tempValue');
    tempEl.textContent = temp.toFixed(1);

    // Temperature bar (10-40°C range)
    const tempPercent = Math.min(100, Math.max(0, ((temp - 10) / 30) * 100));
    document.getElementById('tempBar').style.width = tempPercent + '%';

    // Temperature trend
    if (previousTemp !== null) {
        const diff = temp - previousTemp;
        const trendEl = document.getElementById('tempTrend');
        if (diff > 0.5) trendEl.textContent = '↑ Rising';
        else if (diff < -0.5) trendEl.textContent = '↓ Falling';
        else trendEl.textContent = '→ Stable';
    }
    previousTemp = temp;

    // Update light
    const lightEl = document.getElementById('lightValue');
    lightEl.textContent = light.toFixed(1);

    // Light bar
    document.getElementById('lightBar').style.width = light + '%';

    // Light trend
    if (previousLight !== null) {
        const diff = light - previousLight;
        const trendEl = document.getElementById('lightTrend');
        if (diff > 2) trendEl.textContent = '↑ Brighter';
        else if (diff < -2) trendEl.textContent = '↓ Dimmer';
        else trendEl.textContent = '→ Stable';
    }
    previousLight = light;

    // Update mood card
    updateMoodCard(colorHex, message, buzzer);

    // Update timestamp
    if (data.created_at) {
        const date = new Date(data.created_at);
        document.getElementById('lastUpdate').textContent = 'Updated ' + timeAgo(date);
    }
}

function updateMoodCard(colorHex, message, buzzer) {
    // AI message
    document.getElementById('aiMessage').textContent = message;

    // Mood emoji based on color
    const emoji = getMoodEmoji(colorHex);
    document.getElementById('aiSubtext').textContent = getMoodText(colorHex);

    // Update orb color
    const orb = document.getElementById('moodOrb');
    orb.style.background = `linear-gradient(135deg, ${colorHex}, ${adjustBrightness(colorHex, -30)})`;
    orb.style.boxShadow = `0 0 30px ${colorHex}40`;
    orb.textContent = emoji;

    // Update card border
    const card = document.getElementById('moodCard');
    card.style.borderColor = `${colorHex}40`;
    card.style.boxShadow = `0 0 40px ${colorHex}10`;

    // Update ambient glow
    document.getElementById('ambientGlow').style.background = colorHex;

    // Buzzer indicator
    const buzzerEl = document.getElementById('buzzerIndicator');
    if (buzzer) {
        buzzerEl.classList.remove('hidden');
    } else {
        buzzerEl.classList.add('hidden');
    }
}

// ============================================
// SENSOR HISTORY
// ============================================
async function loadSensorHistory() {
    try {
        const res = await fetch(`${API_BASE}api_history.php?type=sensor`);
        const json = await res.json();

        if (json.success && json.data.length > 0) {
            renderSensorHistory(json.data);
        }
    } catch (err) {
        console.error('Failed to load sensor history:', err);
    }
}

function renderSensorHistory(logs) {
    const container = document.getElementById('sensorHistory');
    container.innerHTML = '';

    logs.forEach((log, index) => {
        const date = new Date(log.created_at);
        const item = document.createElement('div');
        item.className = 'sensor-history-item flex items-center gap-3 p-3 rounded-xl';
        item.style.animationDelay = `${index * 50}ms`;

        const colorHex = log.color_hex || '#4488FF';

        item.innerHTML = `
            <div class="color-dot w-3 h-3 rounded-full flex-shrink-0" style="background: ${colorHex}; color: ${colorHex};"></div>
            <div class="flex-1 min-w-0">
                <div class="flex items-center gap-3 text-sm">
                    <span class="text-gray-300 font-mono">${parseFloat(log.temperature).toFixed(1)}°C</span>
                    <span class="text-gray-600">·</span>
                    <span class="text-gray-300 font-mono">${parseFloat(log.light_level).toFixed(1)}%</span>
                    ${log.ai_message ? `<span class="text-gray-600">·</span><span class="text-gray-500 truncate">${log.ai_message}</span>` : ''}
                </div>
            </div>
            <span class="text-xs text-gray-600 flex-shrink-0">${timeAgo(date)}</span>
        `;

        container.appendChild(item);
    });
}

// ============================================
// CHAT
// ============================================
async function handleChatSubmit(e) {
    e.preventDefault();

    const input = document.getElementById('chatInput');
    const message = input.value.trim();
    if (!message) return;

    // Add user message to UI
    addChatMessage('user', message);
    input.value = '';

    // Show typing indicator
    showTypingIndicator();

    // Disable send button
    const sendBtn = document.getElementById('sendBtn');
    sendBtn.disabled = true;
    sendBtn.classList.add('opacity-50');

    try {
        const res = await fetch(`${API_BASE}api_chat.php`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ message })
        });

        const json = await res.json();
        removeTypingIndicator();

        if (json.success) {
            addChatMessage('ai', json.reply);

            // Update location if changed
            if (json.location) {
                document.getElementById('currentLocation').textContent = capitalizeFirst(json.location);
            }

            // Refresh sensor data
            setTimeout(loadLatestData, 1000);
        } else {
            addChatMessage('ai', 'Sorry, I encountered an error. Please try again.');
        }
    } catch (err) {
        removeTypingIndicator();
        addChatMessage('ai', 'Connection error. Make sure the server is running.');
        console.error('Chat error:', err);
    }

    sendBtn.disabled = false;
    sendBtn.classList.remove('opacity-50');
}

function addChatMessage(role, message) {
    const container = document.getElementById('chatMessages');

    const wrapper = document.createElement('div');
    wrapper.className = `flex gap-2 animate-fade-in-up ${role === 'user' ? 'justify-end' : ''}`;

    if (role === 'ai') {
        wrapper.innerHTML = `
            <div class="w-7 h-7 rounded-lg bg-accent/20 flex items-center justify-center text-sm flex-shrink-0 mt-0.5">🤖</div>
            <div class="chat-bubble-ai rounded-2xl rounded-tl-sm px-4 py-2.5 max-w-[85%]">
                <p class="text-sm text-gray-200">${escapeHtml(message)}</p>
            </div>
        `;
    } else {
        wrapper.innerHTML = `
            <div class="chat-bubble-user rounded-2xl rounded-tr-sm px-4 py-2.5 max-w-[85%]">
                <p class="text-sm text-gray-200">${escapeHtml(message)}</p>
            </div>
        `;
    }

    container.appendChild(wrapper);
    container.scrollTop = container.scrollHeight;
}

function showTypingIndicator() {
    const container = document.getElementById('chatMessages');
    const typing = document.createElement('div');
    typing.id = 'typingIndicator';
    typing.className = 'flex gap-2 animate-fade-in-up';
    typing.innerHTML = `
        <div class="w-7 h-7 rounded-lg bg-accent/20 flex items-center justify-center text-sm flex-shrink-0 mt-0.5">🤖</div>
        <div class="chat-bubble-ai rounded-2xl rounded-tl-sm px-4 py-3 max-w-[85%]">
            <div class="typing-dots"><span></span><span></span><span></span></div>
        </div>
    `;
    container.appendChild(typing);
    container.scrollTop = container.scrollHeight;
}

function removeTypingIndicator() {
    const el = document.getElementById('typingIndicator');
    if (el) el.remove();
}

async function loadChatHistory() {
    try {
        const res = await fetch(`${API_BASE}api_history.php?type=chat`);
        const json = await res.json();

        if (json.success && json.data.length > 0) {
            json.data.forEach(msg => {
                addChatMessage(msg.role, msg.message);
            });
        }
    } catch (err) {
        console.error('Failed to load chat history:', err);
    }
}

// ============================================
// LOCATION
// ============================================
async function loadLocation() {
    try {
        const res = await fetch(`${API_BASE}api_history.php?type=location`);
        const json = await res.json();

        if (json.success && json.location) {
            document.getElementById('currentLocation').textContent = capitalizeFirst(json.location);
        }
    } catch (err) {
        console.error('Failed to load location:', err);
    }
}

// ============================================
// HELPERS
// ============================================
function timeAgo(date) {
    const seconds = Math.floor((new Date() - date) / 1000);
    if (seconds < 10) return 'just now';
    if (seconds < 60) return seconds + 's ago';
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return minutes + 'm ago';
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return hours + 'h ago';
    return date.toLocaleDateString();
}

function capitalizeFirst(str) {
    return str.charAt(0).toUpperCase() + str.slice(1);
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function getMoodEmoji(hex) {
    const r = parseInt(hex.slice(1, 3), 16);
    const g = parseInt(hex.slice(3, 5), 16);
    const b = parseInt(hex.slice(5, 7), 16);

    if (r > 200 && g < 100) return '🔥';  // Red = hot
    if (r > 200 && g > 150) return '☀️';  // Orange/yellow = warm
    if (g > 180 && r < 100) return '🌿';  // Green = comfortable
    if (b > 200 && r < 100) return '❄️';  // Blue = cool
    if (r > 150 && b > 150) return '🌙';  // Purple = relaxed
    return '🌟';  // Default
}

function getMoodText(hex) {
    const r = parseInt(hex.slice(1, 3), 16);
    const g = parseInt(hex.slice(3, 5), 16);
    const b = parseInt(hex.slice(5, 7), 16);

    if (r > 200 && g < 100) return 'Temperature is high — consider cooling down';
    if (r > 200 && g > 150) return 'Warm conditions detected';
    if (g > 180 && r < 100) return 'Environment is comfortable';
    if (b > 200 && r < 100) return 'Cool conditions — stay cozy';
    if (r > 150 && b > 150) return 'Relaxed atmosphere';
    return 'Monitoring your environment';
}

function adjustBrightness(hex, amount) {
    const r = Math.max(0, Math.min(255, parseInt(hex.slice(1, 3), 16) + amount));
    const g = Math.max(0, Math.min(255, parseInt(hex.slice(3, 5), 16) + amount));
    const b = Math.max(0, Math.min(255, parseInt(hex.slice(5, 7), 16) + amount));
    return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
}
