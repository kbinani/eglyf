# eglyf

An OpenType font tool that modifies OTF files to add support for [Egyptian Hieroglyph Format Controls](https://en.wikipedia.org/wiki/Egyptian_Hieroglyph_Format_Controls).

This project is inspired by and builds upon concepts from [Microsoft Font Tools](https://github.com/microsoft/font-tools).

> [!IMPORTANT]
> This tool is currently under development.

## About Egyptian Hieroglyph Format Controls

Egyptian Hieroglyph Format Controls are special Unicode characters (U+13430–U+1343F) that enable the proper layout of Egyptian hieroglyphic text. By adding support for these format controls to a font, this tool enables:

- Hieroglyph-specific layouts directly through the font's own capabilities, based on the standard OpenType features
- Rendering of hieroglyphic texts with widely-available text rendering engines

The key advantage is that once a font is processed with eglyf, it can properly display hieroglyphs with their unique layout requirements using only standard OpenType capabilities.

## Features

- Replaces GSUB and GPOS tables with necessary lookups for Egyptian Hieroglyph Format Controls and [Manuel de Codage](http://www.catchpenny.org/codage/) notation system
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
./eglyf_artefacts/eglyf --input input.ttf --output output.ttf --name "My EgyptHiero/Regular/My Egyptian Hieroglyphs Regular/MyEgyptianHieroglyphs-Regular"
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
- `--disable-mdc-subst`: Disable some GSUB lookups for Manuel de Codage support (optional)

## Disclaimer

This tool modifies font files but does not check the license terms of the input fonts. Users are responsible for ensuring that any modifications made to fonts comply with the original font's license terms. Some font licenses may not permit modification or may have specific requirements for derivative works.

## Dependencies

- [cxxopts](https://github.com/jarro2783/cxxopts)
- C++20 compatible compiler
- CMake 3.16+

## License

See the [LICENSE](LICENSE) file for details.
