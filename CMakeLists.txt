cmake_minimum_required(VERSION 3.16)
project(eglyf VERSION 1.1.0)

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

add_subdirectory(deps/JUCE)

list(APPEND src_files
  src/defer.hpp
  src/Type.hpp
  src/Tag.hpp
  src/TableRecord.hpp
  src/TableDirectory.hpp
  src/FontFile.hpp
  src/io/InputStream.hpp
  src/io/OutputStream.hpp
  src/io/FileInputStream.hpp
  src/io/FileOutputStream.hpp
  src/io/ByteInputStream.hpp
  src/io/ByteOutputStream.hpp
  src/io/OffsetInputStream.hpp
  src/table/Table.hpp
  src/table/ReadonlyTable.hpp
  src/table/FontHeaderTable.hpp
  src/table/MaximumProfileTable.hpp
  src/table/IndexToLocationTable.hpp
  src/table/GlyphDataTable.hpp
  src/table/PostScriptTable.hpp
  src/table/OS2AndWindowsMetricsTable.hpp
  src/table/HorizontalHeaderTable.hpp
  src/table/HorizontalMetricsTable.hpp
  src/table/SubtableCollection.hpp
  src/table/GlyphSubstitutionTable.hpp
  src/table/ScriptList.hpp
  src/table/FeatureList.hpp
  src/table/LookupList.hpp
  src/table/coverage/Coverage1.hpp
  src/table/coverage/Coverage2.hpp
  src/table/coverage/Coverage.hpp
  src/table/coverage/CoverageReader.hpp
  src/table/ClassDef.hpp
  src/table/Subtable.hpp
  src/table/gsub/Single.hpp
  src/table/gsub/SubstitutionExtension.hpp
  src/table/gsub/Ligature.hpp
  src/table/gsub/Multiple.hpp
  src/table/SequenceLookup.hpp
  src/table/ChainedContexts.hpp
  src/table/gsub/ReverseChainedContextsSingle.hpp
  src/io/OffsetWriter.hpp
  src/table/gsub/Alternate.hpp
  include/eglyf.hpp
  src/status.hpp
  src/errno.hpp
  src/optional.hpp
  src/table/Contextual.hpp
  src/table/FeatureVariations.hpp
  src/table/gpos/SingleAdjustment.hpp
  src/table/GlyphPositioningTable.hpp
  src/table/gpos/PositioningExtension.hpp
  src/table/gpos/MarkToBaseAttachment.hpp
  src/table/gpos/Anchor.hpp
  src/table/gpos/MarkArray.hpp
  src/table/gpos/MarkToLigatureAttachmentPositioning.hpp
  src/table/gpos/MarkToMarkAttachmentPositioning.hpp
  src/table/gpos/ValueRecord.hpp
  src/table/gpos/PairAdjustmentPositioning.hpp
  src/table/gpos/CursiveAttachmentPositioning.hpp
  src/io/DataFragmentWriter.hpp
  src/table/GlyphDefinitionTable.hpp
  src/table/MarkGlyphSets.hpp
  src/table/VerticalHeaderTable.hpp
  src/table/VerticalMetricsTable.hpp
  src/table/CharacterToGlyphIndexMappingTable.hpp
  src/table/cmap/CmapSubtable.hpp
  src/table/cmap/ByteEncoding.hpp
  src/table/cmap/SegmentMappingToDeltaValues.hpp
  src/table/cmap/SegmentedCoverage.hpp
  src/table/cmap/UnicodeVariationSequences.hpp
)

juce_add_console_app(eglyf)
target_sources(eglyf PRIVATE ${src_files} src/main.cpp)
target_include_directories(eglyf PRIVATE src include)
target_link_libraries(eglyf
  PRIVATE
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)
if (MSVC)
  target_compile_definitions(eglyf
    PRIVATE
      NOMINMAX
      WIN32_LEAN_AND_MEAN
      JUCE_DISABLE_JUCE_VERSION_PRINTING=1
  )
else()
  target_compile_definitions(eglyf
    PRIVATE
      JUCE_DISABLE_JUCE_VERSION_PRINTING=1
  )
endif()

juce_add_console_app(resave)
target_sources(resave PRIVATE ${src_files} test/resave.cpp)
target_include_directories(resave PRIVATE src include)
target_link_libraries(resave
  PRIVATE
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)
if (MSVC)
  target_compile_definitions(resave
    PRIVATE
      NOMINMAX
      WIN32_LEAN_AND_MEAN
      JUCE_DISABLE_JUCE_VERSION_PRINTING=1
  )
else()
  target_compile_definitions(resave
    PRIVATE
      JUCE_DISABLE_JUCE_VERSION_PRINTING=1
  )
endif()

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files})
