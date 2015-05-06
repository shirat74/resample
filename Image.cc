#include <string>
#include <math.h>

#include "Image.hh"

// Only 8 and 16 bpc supported
Image::Image (int32_t width, int32_t height, int8_t num_comp, int8_t bpc)
{
  this->width  = width;
  this->height = height;
  this->nComps = num_comp;
  this->bpc    = bpc;

  data = new Color[width*height];
}

Image::Image (int32_t width, int32_t height, int8_t nComps, int8_t bpc,
              const std::string& raster)
{
  this->width  = width;
  this->height = height;
  this->nComps = nComps;
  this->bpc    = bpc;

  data = data_from_string(raster, width, height, nComps, bpc);
}

Image::~Image ()
{
  delete data;
}

Color *
Image::data_from_string (const std::string raster,
                         int32_t width, int32_t height,
                         int8_t nComps, int8_t bpc)
{
  Color *data = new Color[width*height];

  size_t expected = width * height * nComps * (bpc / 8);
  std::string str = (raster.size() < expected) ?
      raster + std::string(expected - raster.size(), 0) : raster;
  switch (bpc) {
  case 8:
    for (int32_t i = 0; i < width * height; i++) {
      Color   pixel;
      int32_t pos = nComps * i;
      for (int c = 0; c < nComps; c++)
        pixel.v[c] = round(65535 * ((uint8_t) str.at(pos+c) / 255.0));
      data[i] = pixel;
    }
    break;
  case 16:
    for (int32_t i = 0; i < width * height; i++) {
      Color   pixel;
      int32_t pos = 2 * nComps * i;
      for (int c = 0; c < nComps; c++)
        pixel.v[c] = ((uint8_t) str.at(pos+2*c)) * 256  +
                                      (uint8_t) str.at(pos+2*c+1);
      data[i] = pixel;
    }
    break;
  }

  return  data;
}

// Periodic boundary...
Color
Image::getPixel (int32_t x, int32_t y) const
{
  if (x < 0) {
    while (x < 0)
      x += width;
  } else if (x >= width) {
    while (x >= width)
      x -= width;
  }
  if (y < 0) {
    while (y < 0)
      y += height;
  } else if (y >= height) {
    while (y >= height)
      y -= height;
  }
  return data[y * width + x];
}

void
Image::putPixel (int32_t x, int32_t y, Color value)
{
  if (x < 0) {
    while (x < 0)
      x += width;
  } else if (x >= width) {
    while (x >= width)
      x -= width;
  }
  if (y < 0) {
    while (y < 0)
      y += height;
  } else if (y >= height) {
    while (y >= height)
      y -= height;
  }
  data[y * width + x] = value;
}

std::string
Image::getPixelBytes () const
{
  std::string bytes(width * height * nComps * (bpc / 8), 0);
  for (int32_t j = 0; j < height; j++) {
    for (int32_t i = 0; i < width; i++) {
      Color  pixel = getPixel(i, j);
      size_t pos   = (bpc / 8) * nComps * (j * width + i);
      for (int c = 0; c < nComps; c++) {
        if (bpc == 8) {
          bytes[pos + c] = round(255 * (pixel.v[c] / 65535.0));
        } else {
          bytes[pos+2*c]   = (pixel.v[c] >> 8) & 0xff;
          bytes[pos+2*c+1] =  pixel.v[c] & 0xff;
        }
      }
    }
  }
  return bytes;
}
