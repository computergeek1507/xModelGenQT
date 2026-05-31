<p align="center">
  <img src="res/xmodel_gen.png" alt="xModelGen" width="128">
</p>

<h1 align="center">xModelGen</h1>

<p align="center">
  Turn a DXF or SVG drawing of mounting holes into a wired
  <a href="https://www.xlights.org">xLights</a> custom model.
</p>

---

## What it does

xModelGen reads a DXF or SVG file, finds the round holes in it (where the
LEDs/pixels go), works out the order to wire them, and exports an xLights
`.xmodel` you can import directly.

1. **Open a DXF or SVG** – in DXF, holes are detected from circles, arcs,
   circle‑like polylines, and loops of connected line segments, with block
   references (`INSERT`s) expanded to their real positions. In SVG, `<circle>`
   and near‑circular `<ellipse>` elements are used, with group/element
   `transform`s applied.
2. **Set the hole size** – the target diameter (mm) is configurable; only holes
   within ±1 mm of it are picked up. Units are honoured (DXF `$INSUNITS`; SVG
   `width` + `viewBox`, falling back to 1 unit = 1 mm).
3. **Choose a Mode** –
   - *Pick start*: click (or click‑and‑drag) a hole to set the Auto Wire start;
     it turns green.
   - *Manual wire*: click or drag across holes to wire them by hand (right‑click
     or **Undo Last** removes the last).
   - *Select section*: rubber‑band a box (or click holes) to pick a group, then
     **Wire Section**.
4. **Wire it** – pick a **Method** and hit **Auto Wire** (whole model) or
   **Wire Section** (selection only):
   - *Nearest‑first* gives the tidiest, shortest‑hop wiring.
   - *Warnsdorff* completes reliably from almost any start node.

   A recursive search finds an order that visits every hole within the wire gap,
   skipping and backtracking as needed; it runs behind a cancelable progress
   dialog. Wired holes turn blue and show their number, and manual picks and
   wired sections chain into one continuous run.
5. **Export xModel** – `File ▸ Export xModel` writes an xLights custom model,
   gridded to the hole spacing and numbered in wiring order.

## Web version

A cross‑platform Flutter port (web + Windows) lives in its own repo, with a
browser build you can try without installing anything:

- **Live demo:** https://computergeek1507.github.io/xmodelgen_flutter/
- **Source:** https://github.com/computergeek1507/xmodelgen_flutter

## Building (Windows)

Requirements: **Qt 5.15.2** (msvc2019_64), **CMake ≥ 3.20**, and Visual Studio.
spdlog and nlohmann/json are fetched automatically.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v142 `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build --config Release --target xmodel_gen
```

> **Note:** the `-T v142` toolset is required — Qt 5.15.2 uses
> `stdext::checked_array_iterator`, which the newest MSVC STL removed.

## Command line

For scripting/testing the app can run headlessly:

```
xmodel_gen <file.dxf|file.svg> [holeDiameterMm] [wireGapMm] [out.xmodel]
```

e.g. `xmodel_gen bulb.dxf 12 100 bulb.xmodel` detects 12 mm holes, wires with a
100 mm gap from the first hole, and exports the model.

## Releases

Tagged commits (`v*`) build a Windows installer via the
[release workflow](.github/workflows/release.yml) and attach it to a GitHub
release. You can also trigger the workflow manually to download the installer as
a build artifact.

## License

dxflib (under `dxflib/`) is licensed separately — see
`dxflib/gpl-2.0greater.txt` and `dxflib/dxflib_commercial_license.txt`.
