import axios from "axios";
import { ConnectSseParams } from "../Interfaces/ConnectSseParams";
import { SendMessage } from "../Interfaces/SendMessage";

export const ConexionesApi = () => {
  const url = "";

  return {
    sendMessage: (jsonData: SendMessage) =>
      axios
        .post(url + "/api/sendMessage", { message: jsonData })
        .then((response) => response.data)
        .catch((error) => {
          console.error("Error sending message:", error);
          throw error;
        }),

    connectMessageStream: (paramsRec: ConnectSseParams) => {
      const streamUrl = new URL(url + "/api/messages/stream", window.location.origin);
      streamUrl.searchParams.set("client_id", paramsRec.client_id);
      streamUrl.searchParams.set("group_id", paramsRec.group_id);

      const source = new EventSource(streamUrl.toString());

      source.onopen = () => {
        paramsRec.onOpen?.();
      };

      source.onmessage = (event) => {
        try {
          paramsRec.onMessage(JSON.parse(event.data));
        } catch {
          paramsRec.onMessage({ message: event.data });
        }
      };

      source.onerror = (error) => {
        paramsRec.onError?.(error);
      };

      return source;
    },
  };
};
