//https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API
//also 
//https://github.com/ongzzzzzz/p5.web-serial

var log = "";
let port;
let reader;
let inputDone;
let outputDone;
let inputStream;
let outputStream;

class LineBreakTransformer {
  constructor() {
    this.container = '';
  }

  transform(chunk, controller) {
    this.container += chunk;
    const lines = this.container.split('\r\n');
    this.container = lines.pop();
    lines.forEach(line => controller.enqueue(line));
  }

  flush(controller) {
    controller.enqueue(this.container);
  }
}

var BAUDRATE = 115200;
async function connect() {
  // - Request a port and open a connection.
  port = await navigator.serial.requestPort();
  // - Wait for the port to open.
  await port.open({ baudRate: BAUDRATE });

	reader = port.readable
    .pipeThrough(new TextDecoderStream())
    .pipeThrough(new TransformStream(new LineBreakTransformer()))
    .getReader();
  // readLoop();

  const encoder = new TextEncoderStream();
  outputDone = encoder.readable.pipeTo(port.writable);
  outputStream = encoder.writable;
}

async function readLoop() {
  // CODELAB: Add read loop here.
  while (true) {
    const { value, done } = await reader.read();
    if (value) {
      console.log(value);
      log += value;
    }
    if (done) {
      console.log('[readLoop] DONE', done);
      reader.releaseLock();
      break;
    }
  }
}

async function readLoopWrapper(fn) {
  // CODELAB: Add read loop here.
  while (true) {
    const { value, done } = await reader.read();
    if (value) {
  		fn(value); 
    }
    if (done) {
      console.log('[readLoop] DONE', done);
      reader.releaseLock();
      break;
    }
  }
}

async function readWithTimeout(timeout) {
  // const reader = port.readable.getReader();
  const timer = setTimeout(() => {
    reader.releaseLock();
  }, timeout);
  const result = await reader.read();
  clearTimeout(timer);
  // reader.releaseLock();
  return result;
}

function writeToStream(...lines) {
  // CODELAB: Write to output stream
  // CODELAB: Write to output stream
  const writer = outputStream.getWriter();
  lines.forEach((line) => {
    console.log('[SEND]', line);
    writer.write(line + '\r');
  });
  writer.releaseLock();
}
