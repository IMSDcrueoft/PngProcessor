# PNG Processor
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/IMSDcrueoft/PngProcessor)

A powerful PNG image processing tool with a wide range of image manipulation capabilities.

# Overview
PNG Processor is a C++ application designed for efficient PNG image processing. It provides numerous image manipulation functions including scaling, sharpening, color adjustments, filtering, and more. The tool is built on top of the LodePNG library for PNG encoding and decoding, with significant enhancements for advanced image processing operations.

# Features
PNG Processor offers a comprehensive set of image processing capabilities:

## Image Transformations
- Zoom/Scaling: Default sampling and bicubic convolution methods
- Sharpening: Laplace and Gauss-Laplace algorithms
### Color Adjustments:
- Tone mapping
- Vividness adjustment
- HSL (Hue, Saturation, Lightness) adjustments
- Reverse color
- Grayscale conversion
- Image Effects
### Filters:
- Surface blur
- Sobel edge enhancement
### Pixelation:
- Mosaic effects
- Interlaced scanning
### Color Reduction:
- Binarization
- Quaternization
- Hexadecimalization
- Image Manipulation
- Splitting/Cutting: Horizontal and block splitting
- Mixing: Combine multiple images
- Encryption: XOR-based image encryption
## Usage
The application is controlled via command-line arguments with the following general format:
```
PngProcessor [input_file] [mode] [parameters...]  
```
Where mode is one of the characters defined in the PngProcessingTools::Mode enum, representing different processing operations.

Technical Details
The application is built with performance in mind:

## Utilizes SIMD instructions for faster processing
Supports multi-threading for parallel image processing
- Handles various PNG color types and bit depths
- System Requirements
- C++17 compatible compiler
- 64-bit system
- Support for SSE2 instruction set
- Building from Source
- The project can be built using Visual Studio with the provided project files. Two build configurations are available:

Debug: PngProcessor_DevDebug64
Release: PngProcessor_Release64

# License
PNG Processor is provided 'as-is' under a permissive license that allows for both personal and commercial use, with the following restrictions:

Do not misrepresent the source of the software
Clearly identify modified versions
Preserve the license notice in all distributions
This project uses a modified version of LodePNG by Lode Vandevenne, and users must comply with its license requirements as well.

Version
Current version: 20220322

© 2021-2025 IMSDCrueoft
