import process from 'node:process';
import { fileURLToPath } from 'node:url';
import ui from '@nuxt/ui/vite';
import vue from '@vitejs/plugin-vue';
import { defineConfig } from 'vite';

const host = process.env.TAURI_DEV_HOST;
const path = (relative: string) => fileURLToPath(new URL(relative, import.meta.url));

// https://vite.dev/config/
export default defineConfig(() => ({
  plugins: [
    vue(),
    ui({
      // Each window picks its view from its label: no router.
      router: false,
      // Components are auto-imported (Nuxt UI and src/components, named after
      // their folder: lamp/ColorPicker.vue is <LampColorPicker>); functions
      // are imported explicitly.
      autoImport: false,
      components: { dts: 'src/components.d.ts', directoryAsNamespace: true },
      ui: {
        colors: {
          primary: 'brand',
          // The type lists only the Tailwind neutrals; `ink` is ours
          // (src/assets/css/palettes.css).
          neutral: 'ink' as 'neutral',
        },
      },
      // The app works offline and its CSP forbids other origins: the icons
      // it uses are bundled, never fetched.
      icon: { clientBundle: { scan: true } },
    }),
  ],

  resolve: {
    alias: { '@': path('./src') },
  },

  // Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
  //
  // 1. prevent Vite from obscuring rust errors
  clearScreen: false,
  // 2. tauri expects a fixed port, fail if that port is not available
  server: {
    port: 1420,
    strictPort: true,
    host: host || false,
    hmr: host
      ? {
          protocol: 'ws',
          host,
          port: 1421,
        }
      : undefined,
    watch: {
      // 3. tell Vite to ignore watching `src-tauri`
      ignored: ['**/src-tauri/**'],
    },
  },
}));
