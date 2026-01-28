import { defineConfig } from 'vitepress';
import basicSsl from '@vitejs/plugin-basic-ssl';
import { katex } from '@mdit/plugin-katex';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const isVercelDev = process.env.VERCEL_DEV === '1';
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const docsRoot = path.resolve(__dirname, '..');

function extractTitle(contents, fallback) {
  const frontmatterMatch = contents.match(/^---[\s\S]*?^---/m);
  if (frontmatterMatch) {
    const titleMatch = frontmatterMatch[0].match(/^title:\s*(.+)\s*$/m);
    if (titleMatch) return titleMatch[1].trim().replace(/^["']|["']$/g, '');
  }
  return fallback;
}

function loadPages() {
  const entries = fs.readdirSync(docsRoot, { withFileTypes: true });
  return entries
    .filter((entry) => entry.isFile() && entry.name.endsWith('.md') && entry.name !== 'index.md')
    .map((entry) => {
      const slug = entry.name.replace(/\.md$/, '');
      const contents = fs.readFileSync(path.join(docsRoot, entry.name), 'utf8');
      const title = extractTitle(contents, slug);
      return { text: title, link: `/${slug}` };
    })
    .sort((a, b) => a.text.localeCompare(b.text));
}

const pageLinks = loadPages();

function createNavWatcher() {
  return {
    name: 'simpac-nav-watcher',
    configureServer(server) {
      const isPage = (file) => {
        if (!file || typeof file !== 'string') return false;
        if (!file.endsWith('.md')) return false;
        const normalized = file.split(path.sep).join('/');
        if (normalized.includes('/.vitepress/')) return false;
        return true;
      };

      const restart = () => {
        if (typeof server.restart === 'function') {
          server.restart();
        } else {
          server.ws.send({ type: 'full-reload' });
        }
      };

      const onChange = (file) => {
        if (isPage(file)) {
          restart();
        }
      };

      server.watcher.on('add', onChange);
      server.watcher.on('unlink', onChange);
      server.watcher.on('change', onChange);
    }
  };
}

export default defineConfig({
  vite: {
    plugins: isVercelDev ? [createNavWatcher()] : [basicSsl(), createNavWatcher()],
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
    nav: [{ text: 'Home', link: '/' }, ...pageLinks],
    sidebar: [
      {
        text: 'Posts',
        items: pageLinks
      }
    ]
  }
});
