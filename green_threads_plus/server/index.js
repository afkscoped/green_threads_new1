const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Connect to C App
const C_APP_HOST = process.env.C_APP_HOST || 'app'; // 'app' is the docker service name
const C_APP_PORT = 8081;
let cClient = null;

function connectToC() {
    cClient = new net.Socket();
    cClient.connect(C_APP_PORT, C_APP_HOST, () => {
        console.log(`Connected to C App at ${C_APP_HOST}:${C_APP_PORT}`);
    });

    let buffer = '';

    cClient.on('data', (data) => {
        buffer += data.toString();

        let boundary = buffer.indexOf('\n');
        while (boundary !== -1) {
            const line = buffer.substring(0, boundary);
            buffer = buffer.substring(boundary + 1);
            boundary = buffer.indexOf('\n');

            if (line.trim()) {
                try {
                    // Validate JSON before broadcasting
                    JSON.parse(line);
                    wss.clients.forEach(client => {
                        if (client.readyState === WebSocket.OPEN) {
                            client.send(line);
                        }
                    });
                } catch (e) {
                    console.error("JSON Parse Error on line:", line, e.message);
                }
            }
        }
    });

    cClient.on('close', () => {
        console.log('Connection to C App closed, retrying...');
        setTimeout(connectToC, 1000);
    });

    cClient.on('error', (err) => {
        console.log('C App Connect Error: ' + err.message);
        setTimeout(connectToC, 1000); // Retry
    });
}

connectToC();

// API Endpoints
app.post('/api/control', (req, res) => {
    // Forward to C App
    if (cClient) {
        const cmd = req.body.command; // simple string protocol
        cClient.write(cmd + '\n');
        res.json({ status: 'sent' });
    } else {
        res.status(503).json({ error: 'C App not connected' });
    }
});

app.get('/api/status', (req, res) => {
    res.json({ connected: !!cClient });
});

server.listen(3001, () => {
    console.log('Relay server listening on port 3001');
});
