import { defineConfig } from 'vitepress';
import basicSsl from '@vitejs/plugin-basic-ssl';
import { katex } from '@mdit/plugin-katex';
import { rctEmojiPlugin } from './plugins/rctEmoji.js';

const isVercelDev = process.env.VERCEL_DEV === '1';

export default defineConfig({
  vite: {
    plugins: isVercelDev ? [] : [basicSsl()],
    server: {
      https: false, // enable if needed for cross-origin isolation
      headers: {
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'credentialless',
        'Cross-Origin-Resource-Policy': 'cross-origin',
        'Access-Control-Allow-Origin': '*',
      },
    },
    assetsInclude: ['**/*.data'],
    worker: {
      format: 'es',
    },
  },
  title: 'Simpac Vue Testbed',
  description: 'Worker harness playground for Simpac WASM builds.',
  cleanUrls: true,
  ignoreDeadLinks: true,
  markdown: {
    config: (md) => {
      md.use(katex, {
        delimiters: 'dollars',
        throwOnError: false,
        strict: 'ignore',
        trust: true,
        macros: {
          '\\class': '\\htmlClass{#1}{#2}',
          '\\t': '\\htmlData{term=#1}{#2}',
          '\\p': '\\htmlData{param=#1}{#2}',
        },
      });
      md.use(rctEmojiPlugin);
    },
  },
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'API Usage', link: '/apis/api-usage' },
      { text: 'Blog Intro', link: '/posts/blogintro' },
      { text: 'Explicit', link: '/posts/explicit' },
      { text: 'FS API', link: '/apis/fs-api' },
      { text: 'Gizmo API', link: '/apis/gizmo-api' },
      {
        text: 'Gallery',
        items: [
          { text: 'Inje Sinnam', link: '/gallery/inje-sinnam' }
        ]
      },
      { text: 'Remote API', link: '/apis/remote-api' },
      { text: 'Repl API', link: '/apis/repl-api' },
      { text: 'Scripting API', link: '/apis/scripting-api' },
      { text: 'Toggle API', link: '/apis/toggle-api' }
    ],
    sidebar: [
      {
        text: 'APIs',
        items: [
          { text: 'API Usage', link: '/apis/api-usage' },
          { text: 'Config API', link: '/apis/config-api' },
          { text: 'FS API', link: '/apis/fs-api' },
          { text: 'Gizmo API', link: '/apis/gizmo-api' },
          { text: 'Remote API', link: '/apis/remote-api' },
          { text: 'Repl API', link: '/apis/repl-api' },
          { text: 'System API', link: '/apis/system-api' },
          { text: 'Scripting API', link: '/apis/scripting-api' },
          { text: 'Toggle API', link: '/apis/toggle-api' }
        ]
      },
      {
        text: 'Posts',
        items: [
          { text: 'Explicit', link: '/posts/explicit' },
          { text: 'Blog Intro', link: '/posts/blogintro' }
        ]
      }
    ]
  }
});
