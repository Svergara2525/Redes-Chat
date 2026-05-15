import { useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import { Group } from "./Interfaces/Group";
import { Message } from "./Interfaces/Message";
import { ConexionesApi } from "./ConexionApi/ConexionApi";

const GROUPS: Group[] = [
  { id: "Grupo 1", name: "Grupo 1" },
  { id: "Grupo 2", name: "Grupo 2" },
];

function createClientId() {
  const randomValues = new Uint32Array(2);
  window.crypto?.getRandomValues?.(randomValues);

  return `frontend-${Date.now()}-${randomValues[0].toString(16)}-${randomValues[1].toString(16)}`;
}

function App() {
  const api = useMemo(() => ConexionesApi(), []);
  const clientId = useMemo(() => createClientId(), []);
  const [selectedGroup, setSelectedGroup] = useState(GROUPS[0].id);
  const [message, setMessage] = useState("");
  const [messages, setMessages] = useState<Message[]>([]);
  const [error, setError] = useState("");
  const messagesEndRef = useRef<HTMLDivElement | null>(null);

  const selectedGroupName = useMemo(
    () => GROUPS.find((group) => group.id === selectedGroup)?.name ?? "",
    [selectedGroup],
  );

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  useEffect(() => {
    const groupId = selectedGroup === "Grupo 1" ? "1" : "2";

    const source = api.connectMessageStream({
      client_id: clientId,
      group_id: groupId,
      onOpen: () => {
        setError("");
      },
      onMessage: (data) => {
        const receivedMessage = data.message.trim();

        if (!receivedMessage || receivedMessage === "Connected to Back") {
          return;
        }

        setMessages((currentMessages) => [
          ...currentMessages,
          {
            id: `remote-${Date.now()}-${currentMessages.length}`,
            author: "Servidor",
            text: receivedMessage,
            group: selectedGroup,
            createdAt: new Date().toISOString(),
            mine: false,
          },
        ]);
      },
      onError: () => {
        setError("Error escuchando mensajes del backend.");
      },
    });

    return () => {
      source.close();
    };
  }, [api, clientId, selectedGroup]);

  async function handleSubmit() {
    const trimmedMessage = message.trim();
    if (!trimmedMessage) {
      return;
    }

    const optimisticMessage: Message = {
      id: `local-${Date.now()}`,
      author: "Tu",
      text: trimmedMessage,
      group: selectedGroup,
      createdAt: new Date().toISOString(),
      mine: true,
    };

    try {
      await api.sendMessage({
        client_id: clientId,
        group_id: selectedGroup === "Grupo 1" ? "1" : "2",
        message: trimmedMessage,
      });

      setMessages((currentMessages) => [...currentMessages, optimisticMessage]);
      setMessage("");
      setError("");
    } catch {
      setError("No se pudo enviar el mensaje.");
    }
  }

  return (
    <main className="chat-shell">
      <aside className="groups-panel" aria-label="Grupos del chat">
        <div className="brand-block">
          <span className="brand-dot" aria-hidden="true" />
          <div>
            <h1>Chat</h1>
            <p>Mensajeria por grupos</p>
          </div>
        </div>

        <nav className="groups-list">
          {GROUPS.map((group) => (
            <button
              className={group.id === selectedGroup ? "active" : ""}
              key={group.id}
              onClick={() => setSelectedGroup(group.id)}
              type="button"
            >
              <span>{group.name}</span>
              <small>{group.id === selectedGroup ? "Activo" : "Grupo"}</small>
            </button>
          ))}
        </nav>
      </aside>

      <section className="chat-panel" aria-label={`Chat ${selectedGroupName}`}>
        <header className="chat-header">
          <div>
            <p>Grupo seleccionado</p>
            <h2>{selectedGroupName}</h2>
          </div>
        </header>

        {error ? <div className="error-banner">{error}</div> : null}

        <div className="messages-window">
          {messages.length > 0 ? (
            messages.map((item) => (
              <article
                className={`message-bubble ${item.mine ? "mine" : ""}`}
                key={item.id}
              >
                <div className="message-meta">
                  <strong>{item.author}</strong>
                </div>
                <p>{item.text}</p>
              </article>
            ))
          ) : (
            <div>No hay mensajes</div>
          )}
          <div ref={messagesEndRef} />
        </div>

        <div className="composer">
          <label className="composer-input">
            <span>Mensaje</span>
            <textarea
              onKeyDown={(event) => {
                if (event.key === "Enter" && !event.shiftKey) {
                  event.preventDefault();
                  handleSubmit();
                }
              }}
              onChange={(event) => setMessage(event.target.value)}
              placeholder={`Escribe en ${selectedGroupName}`}
              rows={3}
              value={message}
            />
          </label>
          <button
            disabled={!message.trim()}
            onClick={() => {
              handleSubmit();
            }}
            type="button"
          >
            Enviar
          </button>
        </div>
      </section>
    </main>
  );
}

export default App;
