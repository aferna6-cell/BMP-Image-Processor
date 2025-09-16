# BMP Image Processor

A comprehensive C program for reading and manipulating 24-bit BMP (bitmap) images. Supports header analysis, edge detection filtering, and Gaussian noise addition using advanced mathematical algorithms.

## Features

### 1. Read Operation
- Parses BMP file headers (14-byte header + 40-byte info header)
- Extracts and displays all header information
- Outputs pixel data in RGB format with padding analysis
- Handles little-endian byte ordering correctly

### 2. Edge Detection
- Applies a 3×3 convolution filter for edge detection
- Uses discrete convolution with kernel:
  - Handles boundary pixels appropriately
- Outputs processed image as `<filename>-edge.bmp`

### 3. Gaussian Noise Addition
- Implements Box-Muller transform for true Gaussian distribution
- User-configurable standard deviation (5-20)
- Adds noise to each RGB channel independently
- Outputs noisy image as `<filename>-noise.bmp`

## Technical Implementation

### BMP Format Handling
- Manual byte-by-byte header parsing (avoids struct alignment issues)
- Proper handling of row padding (BMP rows must be 4-byte aligned)
- Support for 24-bit uncompressed BMP images
- BGR to RGB color channel reordering

### Mathematical Algorithms
- **Box-Muller Transform**: Converts uniform random variables to Gaussian distribution
- **Discrete Convolution**: 2D spatial filtering for edge detection
- **Pixel Clamping**: Ensures values stay within [0,255] range

### Memory Management
- Dynamic 2D array allocation for pixel storage
- Proper memory cleanup to prevent leaks
- Efficient row-by-row processing for large images

## Compilation
```bash
gcc -Wall -g -std=c99 prog6.c -o bmp_processor -lm
