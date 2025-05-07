import * as WebSocket from 'ws';

const wss = new WebSocket.Server({ port: 7777 });

wss.on('connection', (ws: WebSocket) => {
  console.log('Client connected');

  // 클라이언트의 pong 응답을 받을 때마다 이 함수가 호출됩니다.
  ws.on('pong', () => {
    console.log('Received pong from client');
  });

  const interval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.ping(); // ping 메시지를 클라이언트에 보냅니다.
    } else {
      clearInterval(interval); // 연결이 닫혀있다면 interval을 종료합니다.
    }
  }, 30000); // 30초마다 ping 메시지를 보냅니다.

  ws.on('message', (message: string) => {
    console.log(`Received message: ${message}`);

    const data = JSON.stringify({
      type: 'Hello',
      message: "hi"
    });
    ws.send(data);
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    clearInterval(interval); // 연결이 종료되면 interval을 종료합니다.
  });
});
