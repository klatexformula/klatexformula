# Apple Silicon development build

This branch builds KLatexFormula natively for ARM64 Macs, without Rosetta.
It uses Qt 5.15 with an updated macOS presentation. It is a development port, not an
official KLatexFormula release.

## macOS presentation

The main window uses the native system font and a dark neutral grayscale palette,
with one action row between the editor and preview and a compact export bar.
The default window is 400 × 360 logical pixels, matching the installed
original's approximate width. It uses 24-pixel controls, minimal margins,
and no extra headings or card frames. The original hand-drawn button icons retain
their paths, with light ink for dark controls. Subtle grayscale gradients soften
the window and editor background. Rounded editor, preview and Options surfaces
use contrasting fills without visible outlines. Buttons, inputs, hover, keyboard
focus and disabled states are also distinguished by background tone;
the selected Options tab uses dark text on a light gray fill.
Program-output and error-log text views use explicit dark backgrounds and light
text, including selected text. Their legacy fixed light palette is removed.
Scrollbars have zero visible extent throughout the application, including error
logs, editors, symbols and library views. Wheel, trackpad and keyboard scrolling
continue to use the existing scroll models.
The equation canvas stays white so exported black formulas remain visible.
The application uses Dark Aqua for its native title bar and menus. The title bar
uses a full-size content view so the same Qt background paints behind the native
title and traffic-light controls, without a separate fill or separator. Its appearance is
reapplied after Cocoa finishes showing, activating or resizing the window.
The layout reserves the native title-bar inset to keep the editor clear of the
window controls and retain the original compact content size.

The macOS app icon preserves the original mathematical f on a rounded tile,
with transparent outer corners and all standard Retina sizes. Its editable source
is `src/macosx/app-icon.svg`; regenerate the checked-in ICNS with
`bash scripts/generate-macos-icon.sh` on macOS with Qt 5 installed.

Options open alongside the editor instead of using the deprecated NSDrawer.
Saved drawer settings migrate to the inline layout. Quit remains available
in the native application menu and with Cmd-Q.

## Build

Use a native Apple Silicon terminal with Apple's Command Line Tools and
Homebrew installed:

```sh
brew install cmake qt@5 ghostscript
bash scripts/build-macos-arm64.sh
open build-arm64/src/klatexformula.app
```

Install MacTeX or BasicTeX separately for the LaTeX toolchain. The app finds
MacTeX in `/Library/TeX/texbin` and Homebrew tools in `/opt/homebrew/bin`,
including when launched from Finder with a minimal PATH. The app bundles its
Qt frameworks and other linked libraries, but does not bundle TeX or Ghostscript.

The script explicitly builds ARM64, packages dependencies, and ad-hoc signs
every Mach-O file, including Qt plug-ins. It then checks ARM64 availability,
individual signatures, bundle integrity, and that non-system linked libraries
resolve inside the app. It also runs the native command-line version check.
Run the verifier again on a copied or extracted bundle with:

```sh
python3 scripts/macos-bundle.py /path/to/klatexformula.app
```

The build uses the installed Homebrew libraries' deployment requirements.
Building on an older supported macOS is necessary to distribute to older Macs;
lowering CMAKE_OSX_DEPLOYMENT_TARGET alone cannot lower dependencies' minimum OS.

## Rendering smoke test

With MacTeX/BasicTeX and Ghostscript installed, test automatic discovery with
a fresh configuration and Finder-like PATH:

```sh
env PATH=/usr/bin:/bin:/usr/sbin:/sbin \
    KLF_HOME_CONFIG_DIR="$PWD/build-arm64/render-config" \
    build-arm64/src/klatexformula.app/Contents/MacOS/klatexformula \
    --latexinput='E=mc^2' --output="$PWD/build-arm64/equation.png"
test -s build-arm64/equation.png
```

Repeat with a `.pdf` output for vector export. Before a release, manually check
preview, symbols, history/library persistence, copy/paste and drag/drop into
the target applications. CI builds the app and verifies its binary bundle;
it does not install TeX or exercise those GUI interactions.

## Packaging and remaining work

GitHub Actions builds on the native ARM64 `macos-15` runner and uploads a ZIP
of the development app. Local ad-hoc signatures are not Developer ID signatures
or Apple notarization; downloaded builds may require approval in macOS Privacy
& Security. A public release needs Developer ID signing and notarization.
Sparkle is disabled to avoid the old updater and its upstream release feed.

Qt 5 is deprecated; Homebrew currently schedules disabling `qt@5` on
2027-05-19. This branch is a practical first native build. Migrating to Qt 6,
including replacing QtMacExtras, QMacPasteboardMime and QMacNativeWidget usage,
is separate work needed for long-term maintenance. Intel and Universal builds
have not been validated by this workflow.

References:
- [Apple's Rosetta support timeline](https://support.apple.com/en-us/102527)
- [Homebrew Qt 5 formula and deprecation status](https://formulae.brew.sh/formula/qt@5)
- [Qt 5.15 macOS architecture support](https://doc.qt.io/archives/qt-5.15/macos.html)
