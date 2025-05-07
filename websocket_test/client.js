"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var ws_1 = require("ws");
var ws = new ws_1.default('ws://172.17.112.1:7777');
ws.on('open', function () {
    console.log('Connected to server');
    ws.send('World');
});
ws.on('message', function (message) {
    console.log("Received from server: ".concat(message));
});
ws.on('close', function () {
    console.log('Disconnected from server');
});
ws.on('error', function (error) {
    console.error('WebSocket error:', error);
});
