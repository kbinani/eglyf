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
elseif (APPLE)
  add_compile_options(-Wno-shorten-64-to-32)
endif()

add_subdirectory(deps/JUCE)

list(APPEND src_files
  src/main.cpp
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
  src/table/GlyphSubstitutionTable.hpp
  src/table/ScriptList.hpp
  src/table/FeatureList.hpp
  src/table/LookupList.hpp
  src/table/coverage/Coverage1.hpp
  src/table/coverage/Coverage2.hpp
  src/table/coverage/Coverage.hpp
  src/table/coverage/CoverageReader.hpp
  src/table/ClassDef.hpp
  src/table/gsub/Subtable.hpp
  src/table/gsub/Single.hpp
  src/table/gsub/SubstitutionExtension.hpp
  src/table/gsub/SequenceLookup.hpp
  src/table/gsub/ChainedContextsSubstitution.hpp
)

juce_add_console_app(eglyf)
target_sources(eglyf PRIVATE ${src_files})
target_include_directories(eglyf PRIVATE src)
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
  )
endif()

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files})
