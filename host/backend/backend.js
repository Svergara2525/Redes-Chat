const { execFile, spawn } = require("node:child_process");
const path = require("node:path");
const express = require("express");
const cors = require("cors");

const app = express();

app.use(
  cors({
    origin: "*",
    methods: ["GET", "POST", "OPTIONS"],
    allowedHeaders: ["Content-Type"],
  }),
);

app.use(express.json());

const scriptsDir = path.resolve(__dirname, "../scripts_clientes");
const sendScript = path.join(scriptsDir, "cliente_send");
const recvScript = path.join(scriptsDir, "cliente_recv");
const frontendDistDir = path.resolve(__dirname, "../frontend/dist");

function parseReceivedMessage(line) {
  const clientMatch = line.match(/client_id=([^;\n\r]+)/);
  const messageMatch = line.match(/message=([^\n\r]*)/);

  if (!messageMatch) {
    return null;
  }

  return {
    client_id: clientMatch?.[1] ?? "",
    message: messageMatch[1].trim(),
  };
}

app.post("/api/sendMessage", (req, res) => {
  const { message } = req.body;

  if (!message?.client_id || !message?.group_id || !message?.message) {
    return res.status(400).json({ status: "Invalid message payload" });
  }

  execFile(
    sendScript,
    [message.client_id, message.group_id, message.message],
    (error, stdout, stderr) => {
      if (error) {
        console.error("Error:", error);
        return res.status(500).json({ status: "Error sending message" });
      }

      console.log("Salida:", stdout);
      res.status(200).json({ status: "Message sent" });
    },
  );
});

app.get("/api/messages/stream", (req, res) => {
  const { client_id, group_id } = req.query;

  if (!client_id || !group_id) {
    return res.status(400).json({ status: "Missing client_id or group_id" });
  }

  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache, no-transform");
  res.setHeader("Connection", "keep-alive");
  res.flushHeaders?.();

  console.log("SSE open:", client_id, group_id, req.headers.origin);
  res.write("data: Connected to Back\n\n");

  const proc = spawn("stdbuf", ["-oL", "-eL", recvScript, client_id, group_id], {
    stdio: "pipe",
  });

  let stdoutBuffer = "";

  const heartbeat = setInterval(() => {
    res.write(": keepalive\n\n");
  }, 25000);

  proc.on("error", (error) => {
    console.error("Error starting recv:", error);
    clearInterval(heartbeat);
    res.end();
  });

  proc.stdout.on("data", (d) => {
    stdoutBuffer += d.toString();

    const lines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = lines.pop() ?? "";

    lines.forEach((line) => {
      const parsedMessage = parseReceivedMessage(line);

      console.log(line);

      if (parsedMessage?.message) {
        if (parsedMessage.client_id === client_id) {
          console.log("SSE ignored own message:", client_id);
          return;
        }

        res.write(`data: ${JSON.stringify(parsedMessage)}\n\n`);
      }
    });
  });

  proc.stderr.on("data", (d) => {
    console.error(d.toString());
  });

  proc.on("close", (code) => {
    console.log("SSE recv closed:", client_id, group_id, code);
    clearInterval(heartbeat);
    res.end();
  });

  req.on("close", () => {
    console.log("SSE client closed:", client_id, group_id);
    clearInterval(heartbeat);
    if (!proc.killed) {
      proc.kill("SIGINT");
    }
  });
});

app.use(express.static(frontendDistDir));

app.get(/^(?!\/api).*/, (req, res) => {
  res.sendFile(path.join(frontendDistDir, "index.html"));
});

app.listen(3001, "::", () => {
  console.log("Server running on port 3001");
});
