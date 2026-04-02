import type { VNode } from "vue";

export interface ItemProps {
  title: VNode | string;
  content: VNode | string;
  src: string;
}
