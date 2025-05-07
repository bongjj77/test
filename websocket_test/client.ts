import * as WebSocket from 'ws';

const ws = new WebSocket('ws://localhost:7777');

ws.on('open', () => {
  console.log('Connected to server');
  ws.send('World');
});

ws.on('message', (message: string) => {
  console.log(`Received from server: ${message}`);
});

ws.on('close', () => {
  console.log('Disconnected from server');
});

ws.on('error', (error) => {
  console.error('WebSocket error:', error);
});
