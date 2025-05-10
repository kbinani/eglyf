# eglyf

An OpenType font tool that modifies OTF files to add support for [Egyptian Hieroglyph Format Controls](https://en.wikipedia.org/wiki/Egyptian_Hieroglyph_Format_Controls).

This project is inspired by and builds upon concepts from [Microsoft Font Tools](https://github.com/microsoft/font-tools).

> [!IMPORTANT]
> This tool is currently under development.

## About Egyptian Hieroglyph Format Controls

Egyptian Hieroglyph Format Controls (U+13430–U+1343F) are Unicode characters that specify the layout of hieroglyphic text. While these controls are defined in the Unicode standard, they typically require specialized software to be interpreted correctly.

## What This Tool Does

eglyf enhances fonts to support these format controls:

- Enables hieroglyph-specific layouts through the font's native capabilities
- Allows proper rendering with widely-available text engines

Once processed with eglyf, fonts can correctly display hieroglyphs with their unique layouts using only standard OpenType features.

## Features

- Replaces the 'DFLT' script of GSUB and GPOS tables to add necessary lookups for Egyptian Hieroglyph Format Controls and [Manuel de Codage](http://www.catchpenny.org/codage/) notation system
- Modifies cmap, GDEF, and other tables to enable proper rendering of hieroglyphic text

## Steps to build

```
git clone https://github.com/kbinani/eglyf.git
cd eglyf
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build . --parallel
./eglyf_artefacts/eglyf --input input.ttf --output output.ttf --names "My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular"
```

## Usage

```
eglyf --input <input-font> --output <output-font> [options]
```

### Options

- `--input`: File path to the input font file (required)
- `--output`: File path where the modified font will be saved (required)
- `--names`: Custom name for the output font, formatted as "family/subFamily/fullName/postScriptName"
  - Example: `--names "My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular"`
- `--experimental-mdc-subst`: Disable some GSUB lookups for Manuel de Codage support ("on"/"off", default "off")

## Disclaimer

This tool modifies font files but does not check the license terms of the input fonts. Users are responsible for ensuring that any modifications made to fonts comply with the original font's license terms. Some font licenses may not permit modification or may have specific requirements for derivative works.

## Dependencies

- [cxxopts](https://github.com/jarro2783/cxxopts)
- C++20 compatible compiler
- CMake 3.16+

## Tested Fonts

The following fonts have been successfully tested with eglyf:

- [Noto Sans Egyptian Hieroglyphs](https://github.com/notofonts/egyptian-hieroglyphs)
- [NewGardiner](https://mjn.host.cs.st-andrews.ac.uk/egyptian/fonts/newgardiner.html)
- [JSeshFont](http://files.qenherkhopeshef.org/jsesh/JSeshFont.ttf)
- [EgyptianHiero](https://github.com/MKilani/Djehuty/blob/master/EgyptianHiero4.03.ttf)
- [Aaron](https://github.com/HieroglyphsEverywhere/Fonts/tree/master/Experimental)
- [SINUHE](https://github.com/somiyagawa/SINUHE-the-Hierotyper/tree/master/fonts/webfonts)

> [!IMPORTANT]
> When using these or other fonts, please ensure compliance with their respective licenses.

## License

See the [LICENSE](LICENSE) file for details.
