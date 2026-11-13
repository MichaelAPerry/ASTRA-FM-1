#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render one of ASTRA's guides (docs/*.md) as a page of the site, so the site and the repository say the same thing.

The Markdown in docs/ is the only source; this runs at site build time (tools/site_front.sh) and the HTML is not kept
in the repository. Only what the guides use is supported: # headings (with GitHub's anchors), paragraphs, - and 1.
lists, | tables |, **bold**, *italic*, `code` and [links](...). Standard library only.

  tools/md_page.py IN.md OUT.html "Page title"
"""
import html
import re
import sys

LINKS = {"GUIDE.md": "guide.html", "QUICKSTART.md": "start.html", "docs/GUIDE.md": "guide.html",
         "docs/QUICKSTART.md": "start.html"}
REPO = "https://github.com/MichaelAPerry/ASTRA-FM-1/blob/main/"


def anchor(text):
    """GitHub's heading anchor: lower case, punctuation dropped, spaces to hyphens."""
    t = re.sub(r"[^\w\- ]", "", text.lower())
    return t.replace(" ", "-")


def link(target):
    path, _, frag = target.partition("#")
    if path in LINKS:
        return LINKS[path] + ("#" + frag if frag else "")
    if path and not re.match(r"[a-z]+:", path):        # another file of the repository: its page on GitHub
        return REPO + ("docs/" if not path.startswith(("docs/", "../")) else "") + path.replace("../", "")
    return target


def inline(s):
    out, pos = [], 0
    for m in re.finditer(r"\[([^\]]+)\]\(([^)]+)\)", s):
        out.append(html.escape(s[pos:m.start()], quote=False))
        out.append('<a href="%s">%s</a>' % (html.escape(link(m.group(2))), html.escape(m.group(1), quote=False)))
        pos = m.end()
    out.append(html.escape(s[pos:], quote=False))
    t = "".join(out)
    t = re.sub(r"`([^`]+)`", r"<code>\1</code>", t)
    t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
    return re.sub(r"(?<![\w*])\*([^*\s][^*]*?)\*(?![\w*])", r"<i>\1</i>", t)


def body(md):
    lines, out, i = md.split("\n"), [], 0
    while i < len(lines):
        ln = lines[i]
        if not ln.strip():
            i += 1
            continue
        m = re.match(r"(#{1,4}) (.*)", ln)
        if m:
            n, text = len(m.group(1)), m.group(2).strip()
            out.append('<h%d id="%s">%s</h%d>' % (n, anchor(text), inline(text), n))
            i += 1
            continue
        if ln.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                i += 1
            rows = [r for r in rows if not all(re.fullmatch(r":?-{3,}:?", c) for c in r)]
            t = ['<div class="tablewrap"><table>']
            for k, r in enumerate(rows):
                tag = "th" if k == 0 else "td"
                t.append("<tr>" + "".join("<%s>%s</%s>" % (tag, inline(c), tag) for c in r) + "</tr>")
            out.append("\n".join(t) + "</table></div>")
            continue
        lm = re.match(r"\s*([-*]|\d+\.)\s+", ln)
        if lm:
            kind = "ol" if lm.group(1)[0].isdigit() else "ul"
            items = []
            while i < len(lines) and lines[i].strip():
                m2 = re.match(r"\s*([-*]|\d+\.)\s+(.*)", lines[i])
                if m2:
                    items.append(m2.group(2))
                elif items:
                    items[-1] += " " + lines[i].strip()
                i += 1
                if i < len(lines) and not lines[i].strip() and i + 1 < len(lines) and re.match(r"\s*([-*]|\d+\.)\s+", lines[i + 1]):
                    i += 1                                 # a list with blank lines between its items
            out.append("<%s>%s</%s>" % (kind, "".join("<li>%s</li>" % inline(x) for x in items), kind))
            continue
        para = []
        while i < len(lines) and lines[i].strip() and not re.match(r"(#{1,4} |\||\s*([-*]|\d+\.)\s)", lines[i]):
            para.append(lines[i].strip())
            i += 1
        out.append("<p>%s</p>" % inline(" ".join(para)))
    return "\n".join(out)


PAGE = """<!doctype html>
<!-- SPDX-License-Identifier: GPL-3.0-only -->
<!-- Generated from {src} by tools/md_page.py: edit the Markdown, not this page. -->
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{title}</title>
<link rel="icon" href="media/favicon.png">
<link rel="stylesheet" href="site.css">
</head>
<body><main class="doc">
<div class="top">
  <div class="brand"><a href="index.html">ASTRA <span>FM-1</span></a></div>
  <nav><a href="start.html">Start here</a><a href="guide.html">Guide</a><a href="index.html#play">Play</a><a href="install.html">Install</a></nav>
</div>
{body}
<footer>
  ASTRA FM-1 (working title) · a free, unofficial firmware for the M-VAVE FM-1, not associated with or endorsed by M-VAVE.
  Code: GPL-3.0-only, <a href="https://github.com/MichaelAPerry/ASTRA-FM-1">source</a>. No warranty.
</footer>
</main></body></html>
"""


def main():
    src, dst, title = sys.argv[1:4]
    with open(src, encoding="utf-8") as f:
        md = f.read()
    page = PAGE.format(src=html.escape(src), title=html.escape(title), body=body(md))
    with open(dst, "w", encoding="utf-8") as f:
        f.write(page)


if __name__ == "__main__":
    main()
