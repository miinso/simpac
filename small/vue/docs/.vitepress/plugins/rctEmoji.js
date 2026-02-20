const RCT_PEEP_SHORTCODES = Object.freeze({
  bad: '/emoji/rct/peep-bad.png',
  barf: '/emoji/rct/peep-barf.gif',
  best: '/emoji/rct/peep-best.png',
  better: '/emoji/rct/peep-better.png',
  bored: '/emoji/rct/peep-bored.png',
  good: '/emoji/rct/peep-good.png',
  meh: '/emoji/rct/peep-meh.png',
  rage: '/emoji/rct/peep-rage.gif',
  sick: '/emoji/rct/peep-sick.png',
  sicker: '/emoji/rct/peep-sicker.gif',
  worse: '/emoji/rct/peep-worse.png',
  worst: '/emoji/rct/peep-worst.png',
  zzz: '/emoji/rct/peep-zzz.png',
});

export function rctEmojiPlugin(md) {
  md.inline.ruler.before('emphasis', 'rct_emoji', (state, silent) => {
    const start = state.pos;

    if (state.src.charCodeAt(start) !== 0x3a) {
      return false;
    }

    const match = /^:([a-z0-9_+-]+):/.exec(state.src.slice(start));
    if (!match) {
      return false;
    }

    const name = match[1].toLowerCase();
    const src = RCT_PEEP_SHORTCODES[name];
    if (!src) {
      return false;
    }

    if (!silent) {
      const token = state.push('html_inline', '', 0);
      token.content = `<img src="${src}" alt=":${name}:" title=":${name}:" class="peep-emoji" decoding="async" loading="lazy">`;
    }

    state.pos += match[0].length;
    return true;
  });
}
