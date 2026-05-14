const { execFile } = require("node:child_process");
const { spawn } = require("node:child_process");

const express = require("express");

const app = express();
app.use(express.json());

app.post("/api/sendMessage", (req, res) => {
  const { message } = req.body;
  console.log("Received message:", message);
  execFile(
    "host_test/scripts_clientes/cliente_send",
    [message.client_id, message.group_id, message.message],
    (error, stdout, stderr) => {
      if (error) {
        console.error("Error:", error);
        res.status(500).json({ status: "Error sending message" });
        return;
      }
      console.log("Salida:", stdout);
    },
  );
  res.status(200).json({ status: "Message sent" });
});

const proc = spawn("host_test/scripts_clientes/cliente_recv", ["arg1"], {
  stdio: "pipe",
});

proc.stdout.on("data", (d) => console.log(d.toString()));
proc.stderr.on("data", (d) => console.error(d.toString()));
proc.on("close", (code) => console.log("Exit code:", code));
