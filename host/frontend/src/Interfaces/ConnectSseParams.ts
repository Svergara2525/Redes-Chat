export interface ConnectSseParams {
  client_id: string;
  group_id: string;
  onMessage: (data: { client_id?: string; message: string }) => void;
  onError?: (error: Event) => void;
  onOpen?: () => void;
}
