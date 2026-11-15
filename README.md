# Purple Polished Map

An unofficial fork of [Polished Map](https://github.com/Rangi42/polished-map) by Rangi: a map and tileset editor for [pokered](https://github.com/pret/pokered), [pokeyellow](https://github.com/pret/pokeyellow), [pokegold](https://github.com/pret/pokegold), [pokecrystal](https://github.com/pret/pokecrystal), and hacks based on them.

Polished Map's last release was 4.7.1, in 2021, and a few known bugs remain in it. This fork fixes those bugs first, then adds improvements. It is not affiliated with or endorsed by Rangi, so please report problems with this fork in [its own issue tracker](https://github.com/TheDreigon/purple-polished-map/issues), not to the original project.

All credit for Polished Map goes to Rangi and its contributors. The license ([LGPL v3](LICENSE.md)) and the original copyright are unchanged.

## What is different from Polished Map

- It is based on Polished Map's latest code (after 4.7.1, with FLTK 1.4.5), not on the 4.7.1 release.
- Moving the mouse over the map no longer lags on Windows 11 (fix by dannye, from [Polished Map pull request 112](https://github.com/Rangi42/polished-map/pull/112)).
- **build-windows.bat** builds it on Windows from the command line, with only the Build Tools for Visual Studio 2022.
- It has its own name, version, executable (**purple-polished-map.exe**) and settings file, so it can be used side by side with the original without sharing settings.
- It has [automated tests](tests/README.md) for the code that reads and writes maps, blocksets, tilesets and events.

More fixes are on the way. Each one is a separate commit that explains what it changes.

There are no downloadable builds yet. Follow the steps in [INSTALL.md](INSTALL.md) to build it from source.

This fork is developed with the help of an AI coding assistant (Claude, by Anthropic).

## About Polished Map

Polished Map works with [pokecrystal](https://github.com/pret/pokecrystal), [pokegold](https://github.com/pret/pokegold), [pokeyellow](https://github.com/pret/pokeyellow), [pokered](https://github.com/pret/pokered), and hacks including [Polished Crystal v2](https://github.com/Rangi42/polishedcrystal/tree/v2.2.0), [Red++ v3](https://github.com/TheFakeMateo/rpp-backup), [Orange](https://github.com/PiaCarrot/pokeorange), [Prism](http://www.pokemonprism.com/), and many more.

(For projects that use [256–512 tiles](https://github.com/pret/pokecrystal/wiki/Expand-tilesets-from-192-to-255-tiles) and [per-block attributes](https://github.com/pret/pokecrystal/wiki/Allow-tiles-to-have-different-attributes-in-different-blocks-\(including-X-and-Y-flip\)), including [Polished Crystal v3](https://github.com/Rangi42/polishedcrystal), [Red++ v4](https://github.com/TheFakeMateo/RedPlusPlus), [Coral](https://github.com/pkmncoraldev/polishedcoral), [Black and White 3: Genesis](https://github.com/AzureKeys/BW3G), and [Ancient Ruby](https://github.com/BloodlessNS/ancientruby), use the original [Polished Map++](https://github.com/Rangi42/polished-map/tree/plusplus). This fork does not include its changes.)

Inspired by [crowdmap](https://github.com/yenatch/crowdmap) (now defunct), but implemented with C++ and [FLTK](http://www.fltk.org/), and with more functions for graphics editing.

The [example/](example/) directory contains a minimal pokecrystal project with two test maps. **Kanto.180x135.kanto.blk** is a stitch of every Kanto overworld map (they all use the `kanto` tileset). **Johto.235x135.johto.blk** is a stitch of every Johto overworld map; Goldenrod and Azalea use the `johto_modern` tileset, so try switching tilesets with **Edit→Change Tileset…** or by pressing Ctrl+H.

Browse the menu items, toolbar buttons, and Help dialog to learn how to use it. And don't miss the mouse controls:

|                          | Blocks Mode   | Events Mode       | Edit Block          | Edit Tileset |
|--------------------------|---------------|-------------------|---------------------|--------------|
| **Click/drag**           | Place block   | Move event        | Place tile          | Place pixel  |
| **Middle drag**          | Scroll        | Scroll            |                     |              |
| **Right-click**          | Select block  | Edit event        | Select tile         | Select hue   |
| **Double-click**         |               | Open .asm file    |                     |              |
| **Ctrl+click**           | Replace block |                   | Place 2x2 tiles     | Replace hue  |
| **Shift+click**          | Flood fill    | Follow warp event | Place 2/2 tiles     | Flood fill   |
| **Ctrl+Shift+click**     |               |                   | Place 4x4 tiles     |              |
| **Alt+click**            | Swap blocks   |                   | Place 2+2 tiles     | Swap hues    |
| **Alt+Shift+click**      |               |                   | Place 4+4+4+4 tiles |              |
| **Ctrl+Alt+click**       |               |                   | Place 2-2 tiles     |              |
| **Ctrl+Alt+Shift+click** |               |                   | Place 4-4-4-4 tiles |              |

(All those "Edit Block" mouse controls are explained further in [doc/edit-block.md](doc/edit-block.md).)

More information about Polished Map is at [PokéCommunity](https://www.pokecommunity.com/showthread.php?t=425994).

![Screenshot of Polished Map](screenshot.png)
