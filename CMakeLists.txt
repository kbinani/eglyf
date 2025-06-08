cmake_minimum_required(VERSION 3.16)
project(eglyf VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_definitions(
    /DUNICODE
    /D_UNICODE
    /source-charset:utf-8
    /we4828
    /wd4100 # Suppress "unreferenced formal parameter" warnings
  )
  add_compile_options(/d1trimfile:${CMAKE_SOURCE_DIR}/)
else()
  add_compile_options(
    -Wno-shorten-64-to-32
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=
  )
endif()

add_subdirectory(deps/cxxopts)
add_subdirectory(deps/harfbuzz)
add_subdirectory(deps/doctest)

list(APPEND src_files
  src/Type.hpp
  src/Tag.hpp
  src/TableRecord.hpp
  src/TableDirectory.hpp
  src/Font.hpp
  src/io/InputStream.hpp
  src/io/OutputStream.hpp
  src/io/FileInputStream.hpp
  src/io/FileOutputStream.hpp
  src/io/ByteInputStream.hpp
  src/io/ByteOutputStream.hpp
  src/io/OffsetInputStream.hpp
  src/table/Table.hpp
  src/table/ReadonlyTable.hpp
  src/table/head/FontHeaderTable.hpp
  src/table/maxp/MaximumProfileTable.hpp
  src/table/loca/IndexToLocationTable.hpp
  src/table/glyf/GlyphDataTable.hpp
  src/table/post/PostScriptTable.hpp
  src/table/os2/OS2AndWindowsMetricsTable.hpp
  src/table/hhea/HorizontalHeaderTable.hpp
  src/table/hmtx/HorizontalMetricsTable.hpp
  src/table/g/SubtableCollection.hpp
  src/table/g/sub/GlyphSubstitutionTable.hpp
  src/table/g/ScriptList.hpp
  src/table/g/FeatureList.hpp
  src/table/g/LookupList.hpp
  src/table/Coverage.hpp
  src/table/ClassDef.hpp
  src/table/g/Subtable.hpp
  src/table/g/sub/Single.hpp
  src/table/g/sub/SubstitutionExtension.hpp
  src/table/g/sub/Ligature.hpp
  src/table/g/sub/Multiple.hpp
  src/table/g/SequenceLookup.hpp
  src/table/g/ChainedContexts.hpp
  src/table/g/sub/ReverseChainedContextsSingle.hpp
  src/io/OffsetWriter.hpp
  src/table/g/sub/Alternate.hpp
  include/eglyf.hpp
  src/Status.hpp
  src/Optional.hpp
  src/table/g/Contextual.hpp
  src/table/g/FeatureVariations.hpp
  src/table/g/pos/SingleAdjustment.hpp
  src/table/g/pos/GlyphPositioningTable.hpp
  src/table/g/pos/PositioningExtension.hpp
  src/table/g/pos/MarkToBaseAttachment.hpp
  src/table/g/pos/Anchor.hpp
  src/table/g/pos/MarkArray.hpp
  src/table/g/pos/MarkToLigatureAttachment.hpp
  src/table/g/pos/MarkToMarkAttachment.hpp
  src/table/g/pos/ValueRecord.hpp
  src/table/g/pos/PairAdjustment.hpp
  src/table/g/pos/CursiveAttachment.hpp
  src/io/DataFragmentWriter.hpp
  src/table/gdef/GlyphDefinitionTable.hpp
  src/table/gdef/MarkGlyphSets.hpp
  src/table/vhea/VerticalHeaderTable.hpp
  src/table/vmtx/VerticalMetricsTable.hpp
  src/table/cmap/CharacterToGlyphIndexMappingTable.hpp
  src/table/cmap/CmapSubtable.hpp
  src/table/cmap/ByteEncoding.hpp
  src/table/cmap/SegmentMappingToDeltaValues.hpp
  src/table/cmap/SegmentedCoverage.hpp
  src/table/cmap/UnicodeVariationSequences.hpp
  src/table/cmap/TrimmedTableMapping.hpp
  src/table/name/NamingTable.hpp
  src/editor/Editor.hpp
  src/editor/VtpParser.hpp
  src/GlyphNames.hpp
  src/Unicode.hpp
  src/table/g/pos/Attachment.hpp
  src/editor/PlaceholderGlyph.hpp
  src/gfx/QuadraticBezier.hpp
  src/editor/CartoucheGlyph.hpp
  src/gfx/QuadraticEquation.hpp
  src/gfx/Line.hpp
  src/gfx/Point.hpp
  src/gfx/Shape.hpp
  src/gfx/Path.hpp
  src/gfx/Vec.hpp
  src/gfx/Rect.hpp
  src/gfx/Transform.hpp
  src/editor/Insertion.hpp
  src/editor/Config.hpp
  src/editor/Glyph.hpp
  src/editor/Group.hpp
  src/editor/SizeVariants.hpp
  src/res/template.hpp.in
  src/io/File.hpp
  src/io/Endianness.hpp
  src/editor/ShadingGlyph.hpp
  src/editor/Transformer.hpp
  src/table/cff/Type.hpp
  src/table/cff/CompactFontFormatTable.hpp
  src/table/cff/Index.hpp
  src/table/cff/Dict.hpp
  src/table/cff/StdStrings.hpp
)

function(resource input output varname)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${input} content)

  set(chunk_size 16000) # workaround for MSVC C2026
  string(LENGTH "${content}" len)
  set(chunks "")
  set(offset 0)
  set(variable_name "${varname}")

  while(offset LESS len)
    math(EXPR remaining "${len}-${offset}")
    if(remaining GREATER chunk_size)
      set(size ${chunk_size})
    else()
      set(size ${remaining})
    endif()
    string(SUBSTRING "${content}" ${offset} ${size} part)
    string(APPEND chunks "R\"(${part})\"\n")
    math(EXPR offset "${offset}+${size}")
  endwhile()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/res/template.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${output}
    @ONLY)
endfunction()

resource(res/EgyptianText_200.vtp src/editor/Vtp.hpp vtp)

add_executable(eglyf)
target_sources(eglyf PRIVATE ${src_files} src/main.cpp)
target_include_directories(eglyf PRIVATE src include)
target_link_libraries(eglyf PRIVATE cxxopts)

add_executable(eglyf-tests)
target_sources(eglyf-tests PRIVATE ${src_files} test/main.cpp)
target_include_directories(eglyf-tests PRIVATE src include deps/harfbuzz/src deps/doctest/doctest)
target_link_libraries(eglyf-tests PRIVATE harfbuzz)

if (MSVC)
  foreach(target IN ITEMS eglyf eglyf-tests)
    target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(${target} PRIVATE /bigobj)
  endforeach ()
endif()

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files};src/main.cpp;test/main.cpp)
