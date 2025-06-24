# yarg-lang

Yarg-Lang is a project to experiment with a dynamic language targetting microcontrollers. It has not yet made a release suitable for wide use.

Yarg aims to be a dedicated language for Microcontroller firmware development. It offers:

  - An interactive, on-device, REPL
  - Direct hardware access
  - Interupt based and multi-core multiprocessing
  - Many other modern language conveniences.

Microcontrollers (such as the $4 Raspberry Pi Pico, or the ESP32 family) are powerful computers, yet we commonly develop software for them with languages like C that were designed when resources were far more scarce. Yarg aims to provide a richer language, spending some of these resources to achive that. Of course, if you want to use modern language features, many general purpose languages are available in 'Micro', 'Tiny' or other cut-down versions of their implementation for microcontroller use. These implementations are faced with choices when the resources available do not support the same implementation possible in their original general purpose implementation. Do they try to be compatible (but over-expensive), or do they document a limitation compared to the full language implementation? Yarg aims to remove the limitations these choices impose, and offer modern langauge features, by being dedicated to the task of Microcontroller development.

Many samples for starting projects include polling, (while 'true'; sleep(x); do-stuff;), which can be wasteful of energy. How long is x? Modern microprocessors are designed to be normally off, and to wake when something interesting is happening. Yarg is a language designed with this in mind from the start.

## Aims

  - A dynamic environment for on-device prototyping
  - Tooling to deploy working prototypes
  - Sufficient static typing to reasonably add device specific code without writing C
  - Interop with C libraries available on device

Not (yet) intended for use. Additional documentation on the [wiki][wiki]

[wiki]: https://github.com/jhmcaleely/yarg-lang/wiki

| dir | Description |
| :--- | :--- |
| `cyarg/` | yarg implementation in C |
| `hostyarg/` | host tooling for yarg maintenance |
| `tools/` | Miscellaneous tools |
| `vscode-yarg/` | A VS Code Language Extension for Yarg |
| `yarg/specimen/` | Samples of Yarg |
| `yarg/specimen/conway-life-display` | A Yarg implemention of: [jhmcaleely/conway-life-display](https://github.com/jhmcaleely/conway-life-display) |
| `yarg/specimen/todo` | Things that don't work yet |
| `yarg/test/` | A Test Suite |

## Name

[Cornish Yarg](https://en.wikipedia.org/wiki/Cornish_Yarg) is a cheese I enjoy.
