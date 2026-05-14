export interface Message {
  id: string;
  author: string;
  text: string;
  group: string;
  createdAt: string | null;
  mine: boolean;
}
