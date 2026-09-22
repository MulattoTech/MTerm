import type { AstraBridge } from '../preload';

declare global {
  interface Window {
    astra: AstraBridge;
  }
}

export {};
