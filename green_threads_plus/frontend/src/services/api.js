const API_URL = 'http://localhost:3001/api';
const WS_URL = 'ws://localhost:3001';

export const api = {
    connectWebSocket: (onMessage) => {
        const ws = new WebSocket(WS_URL);
        ws.onopen = () => console.log('WS Connected');
        ws.onmessage = (event) => {
            try {
                // If the C app sends raw JSON, parse it
                const data = JSON.parse(event.data);
                onMessage(data);
            } catch (e) {
                // Or handle raw text
                console.log('Received:', event.data);
            }
        };
        return ws;
    },

    sendCommand: async (command) => {
        return fetch(`${API_URL}/control`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command })
        });
    },

    spawn: async (params) => {
        // Construct command string: spawn:matrix:size:workers
        const cmd = `spawn:${params.type}:${params.size}:${params.workers}`;
        return api.sendCommand(cmd);
    }
};
