import DefaultTheme from 'vitepress/theme';
import Simpac from './components/Simpac.vue';
import { flecs } from './components/remote.js';
import './styles.css';

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('Simpac', Simpac);
    app.config.globalProperties.$flecs = flecs;
    if (typeof window !== 'undefined') {
      window.flecs = flecs;
    }
  }
};
