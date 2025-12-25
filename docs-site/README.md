# Hugo Documentation Site

This directory contains the Hugo-based documentation website for LeviathanDM.

## Local Development

### Prerequisites

- Hugo Extended v0.152.2+ ([download](https://gohugo.io/installation/))

### Running Locally

```bash
cd docs-site
hugo server --buildDrafts
```

Visit http://localhost:1313

### Build for Production

```bash
cd docs-site
hugo --minify
```

Output will be in `public/` directory.

## Theme

We use the [Hugo Book theme](https://github.com/alex-shpak/hugo-book) for clean, modern documentation.

## Structure

```
docs-site/
├── content/
│   ├── _index.md              # Homepage
│   └── docs/
│       ├── getting-started/   # Installation & setup
│       ├── features/          # Feature documentation
│       └── development/       # Developer docs
├── static/                    # Static assets
├── themes/hugo-book/          # Hugo Book theme (submodule)
└── hugo.toml                  # Configuration
```

## Adding Content

### New Page

```bash
hugo new content/docs/section/page.md
```

Edit the file and remove `draft: true` when ready.

### New Section

1. Create directory: `content/docs/new-section/`
2. Add `_index.md` in the directory
3. Add pages in the directory

## Deployment

The site is automatically deployed to GitHub Pages via GitHub Actions:

1. Push to `master` branch
2. GitHub Actions builds the site
3. Deploys to `https://leviathansystems.github.io/LeviathanDM/`

### Manual Deployment

To deploy manually:

```bash
cd docs-site
hugo --minify
# Upload public/ to your web server
```

## Configuration

Edit `hugo.toml` to change:
- Site title and description
- Base URL
- Theme settings
- Menu items
- Search settings

## Shortcodes

The Hugo Book theme provides useful shortcodes:

### Hints

```markdown
{{< hint info >}}
Information box
{{< /hint >}}

{{< hint warning >}}
Warning box
{{< /hint >}}

{{< hint danger >}}
Danger/error box
{{< /hint >}}
```

### Columns

```markdown
{{< columns >}}
Left column content
<--->
Right column content
{{< /columns >}}
```

### Buttons

```markdown
{{< button relref="/docs/page" >}}Click Me{{< /button >}}
```

### Tabs

```markdown
{{< tabs "uniqueid" >}}
{{< tab "Tab 1" >}}
Content for tab 1
{{< /tab >}}
{{< tab "Tab 2" >}}
Content for tab 2
{{< /tab >}}
{{< /tabs >}}
```

## Contributing

To improve documentation:

1. Edit files in `content/docs/`
2. Test locally with `hugo server`
3. Commit and push
4. GitHub Actions will deploy automatically

## Links

- [Hugo Documentation](https://gohugo.io/documentation/)
- [Hugo Book Theme](https://github.com/alex-shpak/hugo-book)
- [Markdown Guide](https://www.markdownguide.org/)
