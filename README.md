# rm-stylus
[![rm1](https://img.shields.io/badge/rM1-supported-green)](https://remarkable.com/store/remarkable)
[![rm2](https://img.shields.io/badge/rM2-supported-green)](https://remarkable.com/store/remarkable-2)
[![rmpp](https://img.shields.io/badge/rMPP-supported-green)](https://remarkable.com/products/remarkable-paper/pro)
[![rmppm](https://img.shields.io/badge/rMPPM-supported-green)](https://remarkable.com/products/remarkable-paper/pro-move)

An [xovi](https://github.com/asivery/xovi) extension that exposes stylus button events as a QML type for reMarkable tablets.

Registers `RmStylus` as a QML component that QMD extensions can import and use to react to raw stylus button press/release events.

Inspired by [mb1986/remarkable-stylus](https://github.com/mb1986/remarkable-stylus)

## QML API

```qml
import net.rmitchellscott.RmStylus 1.0

RmStylus {
    // BTN_STYLUS
    onButtonPressed: { }
    onButtonReleased: { }

    // BTN_STYLUS2
    onButton2Pressed: { }
    onButton2Released: { }

    // BTN_TOOL_RUBBER=
    onRubberActivated: { }
    onRubberDeactivated: { }
}
```

### Properties

| Property | Type | Description |
|---|---|---|
| `buttonHeld` | bool | True while BTN_STYLUS is physically held |
| `button2Held` | bool | True while BTN_STYLUS2 is physically held |
| `rubberActive` | bool | True while the eraser end is in use |

## Installation

**Via [Vellum package manager](https://github/vellum-dev/vellum)**

`vellum add rm-stylus`

**Manually**

Download from the latest release and copy to `~/xovi/extensions.d/`

## Building

Requires [eeems/remarkable-toolchain](https://github.com/Eeems-Org/remarkable-toolchain) Docker image:

```bash
make all
```

## License

MIT
