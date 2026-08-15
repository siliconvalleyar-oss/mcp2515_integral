# Assets

## Fonts

This directory contains font resources for the SSD1306 display.

### Available Fonts
- `font6x8.h` - 6x8 pixel font (default, space efficient)
- `font8x16.h` - 8x16 pixel font (larger, more readable)

### Usage
```cpp
#include "font6x8.h"

// Render text
display->drawText("Hello", 0, 0, Font::FONT_6x8);
```

## Images

- `boot_logo.h` - Boot screen logo (128x32 bitmap)
- `icons.h` - Menu icons (16x16 sprites)

## Conversion Tools

To convert images to C arrays:
```bash
# Using ImageMagick
convert input.png -resize 128x32! -depth 1 gray:output.raw

# Using xxd
xxd -i output.raw > boot_logo.h
```
