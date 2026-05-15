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

app.post("/api/sendMessage", (req, res) => {
  const { message } = req.body;

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

  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache");
  res.setHeader("Connection", "keep-alive");

  res.write("data: Connected to Back\n\n");

  const proc = spawn(recvScript, [client_id, group_id], {
    stdio: "pipe",
  });

  proc.stdout.on("data", (d) => {
    const text = d.toString().trim();
    console.log(text);
    res.write(`data: ${JSON.stringify({ message: text })}\n\n`);
  });

  proc.stderr.on("data", (d) => {
    console.error(d.toString());
  });

  proc.on("close", (code) => {
    console.log("Exit code:", code);
    res.end();
  });

  req.on("close", () => {
    proc.kill("SIGINT");
  });
});

app.listen(3001, "::", () => {
  console.log("Server running on port 3001");
});
