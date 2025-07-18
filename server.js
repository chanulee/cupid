const express = require('express');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const WebSocket = require('ws');
const path = require('path');

// Create Express app
const app = express();
const port = 3000;

// WebSocket server setup with explicit error handling
const wss = new WebSocket.Server({ port: 8080 }, () => {
  console.log(`WebSocket Server running on ws://localhost:8080`);
});

// Track connected clients
let connectedClients = new Set();

// WebSocket server error handling
wss.on('error', (error) => {
  console.error('WebSocket Server Error:', error);
});

// Enhanced connection logging
wss.on('connection', (ws) => {
  console.log('New WebSocket client connected');
  console.log(`Total clients connected: ${connectedClients.size}`);
  
  // Send a test message to confirm connection
  try {
    ws.send('connected');
    console.log('Sent connection test message to client');
  } catch (e) {
    console.error('Error sending test message:', e);
  }

  connectedClients.add(ws);
  console.log(`Client added. Total clients now: ${connectedClients.size}`);

  ws.on('error', (error) => {
    console.error('WebSocket Client Error:', error);
  });

  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    connectedClients.delete(ws);
    console.log(`Client removed. Total clients now: ${connectedClients.size}`);
  });
});

// Broadcast function with error handling
function broadcast(message) {
  connectedClients.forEach(client => {
    try {
      if (client.readyState === WebSocket.OPEN) {
        client.send(message);
        console.log('Broadcast to client:', message);
      }
    } catch (e) {
      console.error('Error broadcasting to client:', e);
      connectedClients.delete(client);
    }
  });
}

// --- Serial Port Setup ---
let serialPort;

function connectSerialPort() {
  try {
    console.log(`Attempting to connect to port: /dev/cu.usbserial-1120`);
    serialPort = new SerialPort({
      path: '/dev/cu.usbserial-1120',
      baudRate: 115200,
    });

    setupSerialCommunication();

  } catch (err) {
    console.error('Failed during serial port connection:', err);
    console.log('Retrying in 5 seconds...');
    setTimeout(connectSerialPort, 5000);
  }
}

function setupSerialCommunication() {
  const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

  parser.on('data', (data) => {
    const trimmed = data.trim();
    console.log('Received from Arduino:', trimmed);
    
    if (connectedClients.size > 0) {
      broadcast(trimmed);
    } else {
      console.log('No WebSocket clients connected to broadcast to');
    }
  });

  serialPort.on('error', (err) => {
    console.error('Serial port error:', err);
    // Error will be caught by 'close' event, which will trigger reconnection.
  });

  serialPort.on('close', () => {
    console.log('Serial port closed. Attempting to reconnect...');
    serialPort = null; // Clear the port object
    setTimeout(connectSerialPort, 5000);
  });
}

// Express routes
app.use(express.json());
app.use(express.static(path.join(__dirname)));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

// Keep existing /send endpoint for Arduino commands
app.post('/send', async (req, res) => {
  const { number } = req.body;

  if (!serialPort || !serialPort.isOpen) {
    return res.status(503).json({ error: 'Serial port not connected' });
  }

  try {
    // Write the raw number string without a newline
    serialPort.write(number, (err) => {
      if (err) {
        console.error('Error writing to serial port:', err);
        return res.status(500).json({ error: 'Failed to send command' });
      }
      console.log(`Sent command to Arduino: ${number}`);
      res.json({ message: `Sent command: ${number}` });
    });
  } catch (err) {
    // This catch block might be redundant with the callback, but good for safety
    console.error('Error during write operation:', err);
    res.status(500).json({ error: 'Failed to send command' });
  }
});

// Start server
app.listen(port, () => {
  console.log(`HTTP server running at http://localhost:3000`);
  connectSerialPort();
});

// Cleanup
process.on('SIGTERM', cleanup);
process.on('SIGINT', cleanup);

function cleanup() {
  console.log('Cleaning up...');
  if (serialPort && serialPort.isOpen) {
    serialPort.close();
  }
  process.exit();
}