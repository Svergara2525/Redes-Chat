export interface RawMessage {
  id?: string;
  author?: string;
  sender?: string;
  text?: string;
  message?: string;
  content?: string;
  group?: string;
  groupId?: string;
  createdAt?: string;
  timestamp?: string;
  mine?: boolean;
}
