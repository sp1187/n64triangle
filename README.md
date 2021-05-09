# Interactive RDP triangle demo

This simple Nintendo 64 demo allows experimenting with the RDP triangle rasterizer and its weird command format using a normal N64 controller.
In addition to displaying the triangle itself and the parameter values, each individual parameter is visualized by colored lines that delineate the triangle shape.
With this you should be able to create some pretty funny shapes and maybe even develop an intuitive understanding of how the RDP triangle parameters work, who knows?

## Controls and usage

| Btn | Action |
| --- | ------ |
| L | Switch to previous variable |
| R | Switch to next variable |
| D ◀︎ | Decrease current variable |
| D ▶︎ | Increase current variable |
| A | Invert left/right major
| B | Flip triangle horizontally |

The variables are cycled in the order `YL`, `YM`, `YH`, `XL`, `XM`, `XH`, `DxLDy`, `DxMDy`, `DxHDy`.
The slope variables (`DxLDy`, `DxMDy` and `DxHDy`) change by steps of 0.125 while the positional variables change by steps of 1.

## Compilation
Install a `mips64-elf` GNU toolchain and the [`libdragon`](https://github.com/DragonMinded/libdragon) library into the folder pointed at by the `N64_INST` environment variable and run `make`.
