[![rm1](https://img.shields.io/badge/rM1-supported-green)](https://remarkable.com/store/remarkable)
[![rm2](https://img.shields.io/badge/rM2-supported-green)](https://remarkable.com/store/remarkable-2)
# rm-stylus

An [xovi](https://github.com/asivery/xovi) extension that converts stylus button presses to keyboard shortcuts for reMarkable tablets.

- BTN_STYLUS (side button) → Ctrl+U (undo)
- BTN_TOOL_RUBBER (eraser end) → Ctrl+T (toggle eraser)

Works with [shortcutsToChooseEraser.qmd](https://github.com/ingatellent/xovi-qmd-extensions/tree/main?tab=readme-ov-file#shortcutstochooseeraseqmd) for eraser switching.

Inspired by [mb1986/remarkable-stylus](https://github.com/mb1986/remarkable-stylus)

## Installation

**Via [Vellum package manager](https://github/vellum-dev/vellum)**

`vellum add rm-stylus`

**Manually**
Download from the latest release and copy to `~/xovi/extensions.d/`

## Building

Requires [eeems/remarkable-toolchain](https://github.com/Eeems-Org/remarkable-toolchain) Docker image:

```bash
docker run --rm -v "$(pwd):/work" -v "/path/to/xovi:/xovi" -w /work \
  -e XOVI_REPO=/xovi eeems/remarkable-toolchain:latest-rm2 \
  bash -c '. /opt/codex/*/*/environment-setup-*; make ARCH=armv7 all'
```

## License

MIT
