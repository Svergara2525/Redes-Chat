import { useEffect, useMemo, useRef, useState } from "react";
import "./App.css";

const SEND_ENDPOINT =
  import.meta.env.VITE_SEND_ENDPOINT ?? "/api/messages/send";
const RECEIVE_ENDPOINT =
  import.meta.env.VITE_RECEIVE_ENDPOINT ?? "/api/messages/receive";

const GROUPS = [
  { id: "general", name: "Grupo 1" },
  { id: "redes", name: "Grupo 2" },
];

function normalizeMessages(payload) {
  const messages = Array.isArray(payload) ? payload : payload?.messages;

  if (!Array.isArray(messages)) {
    return [];
  }

  return messages.map((message, index) => ({
    id: message.id ?? `${message.group ?? "msg"}-${index}`,
    author: message.author ?? message.sender ?? "Usuario",
    text: message.text ?? message.message ?? message.content ?? "",
    group: message.group ?? message.groupId ?? "",
    createdAt: message.createdAt ?? message.timestamp ?? null,
    mine: Boolean(message.mine),
  }));
}

function formatTime(value) {
  if (!value) {
    return "";
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return "";
  }

  return new Intl.DateTimeFormat("es", {
    hour: "2-digit",
    minute: "2-digit",
  }).format(date);
}

function App() {
  const [selectedGroup, setSelectedGroup] = useState(GROUPS[0].id);
  const [message, setMessage] = useState("");
  const [messages, setMessages] = useState([]);
  const [isLoading, setIsLoading] = useState(false);
  const [isSending, setIsSending] = useState(false);
  const [error, setError] = useState("");
  const messagesEndRef = useRef(null);

  const selectedGroupName = useMemo(
    () => GROUPS.find((group) => group.id === selectedGroup)?.name ?? "",
    [selectedGroup],
  );

  useEffect(() => {
    let ignore = false;

    async function fetchMessages({ silent = false } = {}) {
      if (!silent) {
        setIsLoading(true);
      }

      try {
        const url = new URL(RECEIVE_ENDPOINT, window.location.origin);
        url.searchParams.set("group", selectedGroup);

        const response = await fetch(url);
        if (!response.ok) {
          throw new Error("No se pudieron cargar los mensajes.");
        }

        const payload = await response.json();
        if (!ignore) {
          setMessages(normalizeMessages(payload));
          setError("");
        }
      } catch (fetchError) {
        if (!ignore) {
          setError(fetchError.message);
          setMessages([]);
        }
      } finally {
        if (!ignore && !silent) {
          setIsLoading(false);
        }
      }
    }

    fetchMessages();
    const intervalId = window.setInterval(() => {
      fetchMessages({ silent: true });
    }, 5000);

    return () => {
      ignore = true;
      window.clearInterval(intervalId);
    };
  }, [selectedGroup]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  async function handleSubmit(event) {
    event.preventDefault();

    const trimmedMessage = message.trim();
    if (!trimmedMessage || isSending) {
      return;
    }

    const optimisticMessage = {
      id: `local-${Date.now()}`,
      author: "Tu",
      text: trimmedMessage,
      group: selectedGroup,
      createdAt: new Date().toISOString(),
      mine: true,
    };

    setMessages((currentMessages) => [...currentMessages, optimisticMessage]);
    setMessage("");
    setIsSending(true);
    setError("");

    try {
      const response = await fetch(SEND_ENDPOINT, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          group: selectedGroup,
          message: trimmedMessage,
        }),
      });

      if (!response.ok) {
        throw new Error("No se pudo enviar el mensaje.");
      }
    } catch (sendError) {
      setError(sendError.message);
      setMessages((currentMessages) =>
        currentMessages.filter((item) => item.id !== optimisticMessage.id),
      );
      setMessage(trimmedMessage);
    } finally {
      setIsSending(false);
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
          <span className="status-pill">
            {isLoading ? "Cargando" : "Actualizacion cada 5s"}
          </span>
        </header>

        {error ? <div className="error-banner">{error}</div> : null}

        <div className="messages-window">
          {isLoading ? (
            <div className="empty-state">Cargando mensajes...</div>
          ) : messages.length > 0 ? (
            messages.map((item) => (
              <article
                className={`message-bubble ${item.mine ? "mine" : ""}`}
                key={item.id}
              >
                <div className="message-meta">
                  <strong>{item.author}</strong>
                  <span>{formatTime(item.createdAt)}</span>
                </div>
                <p>{item.text}</p>
              </article>
            ))
          ) : (
            <div>No hay mensajes</div>
          )}
          <div ref={messagesEndRef} />
        </div>

        <form className="composer" onSubmit={handleSubmit}>
          <label className="composer-input">
            <span>Mensaje</span>
            <textarea
              onChange={(event) => setMessage(event.target.value)}
              placeholder={`Escribe en ${selectedGroupName}`}
              rows="3"
              value={message}
            />
          </label>
          <button disabled={!message.trim() || isSending} type="submit">
            {isSending ? "Enviando..." : "Enviar"}
          </button>
        </form>
      </section>
    </main>
  );
}

export default App;
