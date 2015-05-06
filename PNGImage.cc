// TODO: Preserving sRGB, cHRM, gAMA, iCCP, iTXt

#include <iostream>

#include <string>
#include <vector>
#include <cassert>

#ifdef _WIN32
#  define FOPEN_RBIN_MODE "rb"
#  define FOPEN_WBIN_MODE "wb"
#else
#  define FOPEN_RBIN_MODE "r"
#  define FOPEN_WBIN_MODE "w"
#endif

#define PNG_NO_MNG_FEATURES
#define PNG_NO_PROGRESSIVE_READ

#include <png.h>

#include "PNGImage.hh"

PNGImage::PNGImage (const std::string filename) : Image(0, 0, 0, 0) // dummy
{
  FILE       *fp;
  png_structp png_ptr;
  png_infop   png_info_ptr;
  png_byte    color_type, bpc, nComps;
  png_uint_32 width, height, rowbytes;

  fp = fopen(filename.c_str(), FOPEN_RBIN_MODE);
  if(!fp) {
    std::cerr << "Could not open source image: " << filename << std::endl;
    return;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL ||
      (png_info_ptr = png_create_info_struct(png_ptr)) == NULL) {
    std::cerr << "Creating Libpng read/info struct failed." << std::endl;
    if (png_ptr)
      png_destroy_read_struct(&png_ptr, NULL, NULL);
    return;
  }

#if PNG_LIBPNG_VER >= 10603
  // ignore possibly incorrect CMF bytes
  png_set_option(png_ptr, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
#endif

  // Inititializing file IO.
  png_init_io(png_ptr, fp);

  // Read PNG info-header and get some info.
  png_read_info(png_ptr, png_info_ptr);
  color_type = png_get_color_type  (png_ptr, png_info_ptr);
  width      = png_get_image_width (png_ptr, png_info_ptr);
  height     = png_get_image_height(png_ptr, png_info_ptr);
  bpc        = png_get_bit_depth   (png_ptr, png_info_ptr);
  nComps     = png_get_channels    (png_ptr, png_info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    std::cerr << "Pallete color unsupported." << std::endl;
    return;
  }
  png_read_update_info(png_ptr, png_info_ptr);

  rowbytes = png_get_rowbytes(png_ptr, png_info_ptr);
  png_bytep stream_data_ptr = new png_byte[rowbytes * height];

  png_bytepp rows_p = new png_bytep[height];
  for (uint32_t i = 0; i < height; i++)
    rows_p[i] = stream_data_ptr + (rowbytes * i);
  png_read_image(png_ptr, rows_p);
  delete rows_p;

  png_read_end(png_ptr, NULL);

  // Cleanup
  if (png_info_ptr)
    png_destroy_info_struct(png_ptr, &png_info_ptr);
  if (png_ptr)
    png_destroy_read_struct(&png_ptr, NULL, NULL);
  fclose(fp);

  std::string raster(reinterpret_cast<const char *>(stream_data_ptr),
                     rowbytes * height);
  delete stream_data_ptr;

  // Actual timing of construction of Image base class is here.
  Image::reset(width, height, nComps, bpc, raster);
}

int
PNGImage::save (const std::string filename) const
{
  FILE       *fp;
  png_structp png_ptr;
  png_infop   png_info_ptr;
  png_byte    color_type = PNG_COLOR_TYPE_RGB;

  fp = fopen(filename.c_str(), FOPEN_WBIN_MODE);
  if(!fp) {
    std::cerr << "Could not open file for output: " << filename << std::endl;
    return -1;
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL ||
      (png_info_ptr = png_create_info_struct(png_ptr)) == NULL) {
    std::cerr << "Creating Libpng read/info struct failed." << std::endl;
    if (png_ptr)
      png_destroy_read_struct(&png_ptr, NULL, NULL);
    return -1;
  }

#if PNG_LIBPNG_VER >= 10603
  // ignore possibly incorrect CMF bytes
  png_set_option(png_ptr, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
#endif

  switch (getNComps()) {
  case 1:
    color_type = PNG_COLOR_TYPE_GRAY;
    break;
  case 2:
    color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
    break;
  case 3:
    color_type = PNG_COLOR_TYPE_RGB;
    break;
  case 4:
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    break;
  }
  // Inititializing file IO.
  png_init_io(png_ptr, fp);
  // Write header (8 bit colour depth)
  png_set_IHDR(png_ptr, png_info_ptr, getWidth(), getHeight(),
               getBPC(), // BPC of original image data
               color_type,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, png_info_ptr);

  png_bytep row = new png_byte[getWidth() * (getBPC() / 8) * getNComps()];
  if (getBPC() == 8) {
    for (int32_t j = 0; j < getHeight(); j++) {
      for (int32_t i = 0; i < getWidth(); i++) {
        Color pixel = getPixel(i, j);
        for (int c = 0; c < getNComps(); c++) {
          row[getNComps() * i + c] = 255 * ((float) pixel.v[c] / 65535.);
        }
      }
      png_write_row(png_ptr, row);
    }
  } else {
    for (int32_t j = 0; j < getHeight(); j++) {
      for (int32_t i = 0; i < getWidth(); i++) {
        Color   pixel = getPixel(i, j);
        int32_t pos   = 2 * getNComps() * i;
        for (int c = 0; c < getNComps(); c++) {
          uint16_t val = pixel.v[c];
          row[pos + 2 * c] = (val >> 8) & 0xff;
          row[pos + 2 * c + 1] =  val & 0xff;
        }
      }
      png_write_row(png_ptr, row);
    }
  }
  delete row;

  png_write_end(png_ptr, NULL);
  if (png_info_ptr)
    png_free_data(png_ptr, png_info_ptr, PNG_FREE_ALL, -1);
  if (png_ptr)
    png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
  fclose(fp);

  return 0;
}
