# Bibirble (C++) notes for Claude

@AGENTS.md

## Every pull request: Linux system requirements HTML

Every time a pull request goes out (when you open one, and again whenever you push more commits to an
open one), update `docs/linux-system-requirements.html` so it matches that build, commit it on the PR's
branch, and send the file to the user.

- **No styling:** no `<style>`, no `style=` attributes, no `class`/`id` attributes, no CSS, no scripts.
- **No metadata:** no `<!DOCTYPE>`, `<html>`, `<head>`, `<meta>`, `<title>` or `<link>`. Start straight
  with the content (`<h1>`), and use only plain tags: `h1`, `h2`, `p`, `ul`, `ol`, `li`, `code`.
- **Work the requirements out from the actual Linux build**, not from memory:
  - `readelf -d build/Bibirble | grep NEEDED`: the libraries it links to, and which packages provide them
    on Ubuntu/Debian/Mint, Fedora and Arch.
  - `objdump -T build/Bibirble | grep -o 'GLIBC_[0-9.]*' | sort -V -u | tail -1` (and the same for
    `GLIBCXX_`): the oldest glibc / libstdc++ it can run on, and so the oldest distro versions.
  - The CPU architecture, the minimum window size (`SetMinSize` in `BibirbleWindow.cpp`), disk space (the
    binary's size) and memory.
- Keep the sections: Computer, Linux version, Libraries to install (one install command per distro family),
  Running it, and What these requirements come from (the commit it was built from and the linked libraries).

## Documentation: write it the same way as the existing docs

Any agent writing or changing documentation must match how the existing docs are already written. Read the
neighbouring doc first and copy its shape; don't invent a new format.

- **Plain, short sentences** in everyday words. Say what the feature does for the player, then the file or
  function that does it (`src/StoryWorldMap.h/cpp`), in backticks.
- **README.md:** the project overview, layout tree, data format, features list (`- **Name**: what it does`)
  and notes. Update the matching entry rather than adding a new section.
- **AGENTS.md:** one entry per component (`- **Name** (`files`): what it owns`), the critical invariants
  (things kept in sync in several places) and the build/data pipeline commands.
- **CHANGES.md:** append one `## Title` section per change, with `-` bullets of what changed and why; never
  rewrite older sections.
- **docs/*.md** design docs (like `docs/STORY_MODE.md`): numbered `##` sections, **bold** key terms, and a
  new `##` section at the end for a new part of the design.
- **HTML docs** (like `docs/linux-system-requirements.html`): plain HTML with no styling and no metadata,
  exactly as described in the section above.
- Keep docs in sync with the code in the same commit, and keep Bibirble-web's docs matching when a change
  touches both games (its own rules are in Bibirble-web's `CLAUDE.md`).
