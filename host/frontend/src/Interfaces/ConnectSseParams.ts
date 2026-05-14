export interface ConnectSseParams {
  client_id: string;
  group_id: string;
  onMessage: (data: { message: string }) => void;
  onError?: (error: Event) => void;
  onOpen?: () => void;
}
