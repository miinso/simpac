module.exports = {
  darkMode: 'class',
  content: [
    './docs/**/*.{html,js,vue,ts,md}',
    './docs/.vitepress/**/*.{html,js,vue,ts,md}',
  ],
  plugins: [
    require('@tailwindcss/forms'),
    require('@tailwindcss/typography'),
  ],
};
