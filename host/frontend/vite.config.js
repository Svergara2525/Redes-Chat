import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    host: "::",
    proxy: {
      "/api": {
        target: "http://[2001:720:1d10:fff0:0:1:0:2]:3001",
        changeOrigin: true,
      },
    },
  },
});
