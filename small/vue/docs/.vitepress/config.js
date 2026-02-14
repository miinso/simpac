import { defineConfig } from 'vitepress';
import basicSsl from '@vitejs/plugin-basic-ssl';
import { katex } from '@mdit/plugin-katex';

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
    },
  },
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'API Usage', link: '/apis/api-usage' },
      { text: 'Blog Intro', link: '/posts/blogintro' },
      { text: 'Explicit', link: '/posts/explicit' },
      { text: 'Filesystem', link: '/apis/filesystem' },
      { text: 'Gizmo', link: '/apis/gizmo' },
      {
        text: 'Gallery',
        items: [
          { text: 'Inje Sinnam', link: '/gallery/inje-sinnam' }
        ]
      },
      { text: 'Labs', link: '/labs' },
      { text: 'Remote API', link: '/apis/remote-api' },
      { text: 'Repl API', link: '/apis/repl-api' },
      { text: 'Scripting', link: '/apis/scripting' },
      { text: 'Toggle 2', link: '/apis/toggle2' }
    ],
    sidebar: [
      {
        text: 'APIs',
        items: [
          { text: 'API Usage', link: '/apis/api-usage' },
          { text: 'Config API', link: '/apis/config-api' },
          { text: 'Filesystem', link: '/apis/filesystem' },
          { text: 'Gizmo', link: '/apis/gizmo' },
          { text: 'Remote API', link: '/apis/remote-api' },
          { text: 'Repl API', link: '/apis/repl-api' },
          { text: 'System API', link: '/apis/system-api' },
          { text: 'Scripting', link: '/apis/scripting' },
          { text: 'Toggle 2', link: '/apis/toggle2' }
        ]
      },
      {
        text: 'Posts',
        items: [
          { text: 'Explicit', link: '/posts/explicit' },
          { text: 'Blog Intro', link: '/posts/blogintro' }
        ]
      },
      {
        text: 'Labs',
        items: [
          { text: 'Labs Home', link: '/labs' },
          { text: 'Config Lab', link: '/labs/config' },
          { text: 'Tweak Lab', link: '/labs/tweak' },
          { text: 'Repl Lab', link: '/labs/repl' },
          { text: 'Toggle Lab', link: '/labs/toggle' },
          { text: 'Blocks', link: '/labs/blocks/' },
          { text: 'Blocks MK1', link: '/labs/blocks/mk1' }
        ]
      }
    ]
  }
});
