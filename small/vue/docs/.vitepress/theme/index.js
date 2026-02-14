import DefaultTheme from 'vitepress/theme';
import Simpac from './components/Simpac.vue';
import { flecs } from './components/remote.js';
import * as imath from './components/imath.js';
import * as engine from './lib/index.js';
import 'katex/dist/katex.min.css';
import './styles.css';


export default {
  extends: DefaultTheme,
  async enhanceApp({ app }) {
    app.component('Simpac', Simpac);
    app.config.globalProperties.$flecs = flecs;
    app.config.globalProperties.$imath = imath;
    app.config.globalProperties.$engine = engine;
    if (typeof window !== 'undefined') {
      window.flecs = flecs;
      window.imath = imath;
      window.engine = engine;
    }
  }
};
