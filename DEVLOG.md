# Dev log

Work done by other models (local or Codex), as the owner's CLAUDE.md asks: what, which model, how it was checked.

| Date | Task | Model | Checked by |
|---|---|---|---|
| 2026-11-12 | docs/GUIDE.md (new), README.md and site/index.html rewritten in plain words: a fact sheet drawn from core/app, then 3 rounds of section-by-section drafts and editor's notes | local, qwen3.5:9b (writer); Claude (fact sheet, notes, copy edit) | Every control and on-screen string checked against core/app/app.c, app_draw.c, core/synth/patch.c; site parses, placeholders intact |
| 2026-11-12 | docs/GUIDE.md: new sections "The astrology: reading the sky", "Shaping the sound: FM for beginners", "What ASTRA remembers" (owner's notes: mystical depth, FM and voices, does the birth chart persist) | local, qwen3.5:9b (writer); Claude (symbolism sheet, notes, copy edit) | Keywords, dignity phrases and 777 titles checked against core/chart/chart.c; voice stealing against core/fm/fm.c; mute, LEVEL/OUTPUT and persistence against core/synth/patch.c and core/app/app.c |
| 2026-11-12 | Guide condensed (8,000 to 5,100 words, sections merged), docs/QUICKSTART.md (new, 570 words), front page simplified; tools/md_page.py renders both guides into the site (start.html, guide.html) at build | local, qwen3.5:9b (condensing drafts, quick guide draft); Claude (notes, final cuts, converter) | Rendered pages parse; every link and anchor across index/start/guide resolves; site_front.sh syntax-checked (no package locally: the Pages run is the real test) |
| 2026-11-13 | docs/TRAILER.md (30-second comedy spot, stingers); the novelty claim and related-work credits on the site and README (from a Gemini prior-art report, its main sources checked) | local, qwen3.5:9b (three drafts of the spot); Claude (notes, final punch-up, claim wording) | Every screen line in the spot checked as one the firmware can print (core/chart/chart.c); claim limited to what the checked sources support |
