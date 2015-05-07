// TODO: Preserving sRGB, cHRM, gAMA, iCCP, iTXt
// strlen
#include <string.h>
#include <stdio.h>

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

// Creating an instance with data read from file
PNGImage::PNGImage (const std::string filename) : Image(0, 0, 0, 0) // dummy
{
  FILE       *fp;
  png_structp png_ptr;
  png_infop   png_info_ptr;
  png_byte    color_type, bpc, nComps;
  png_uint_32 width, height, rowbytes;

  fp = fopen(filename.c_str(), FOPEN_RBIN_MODE);
  if(!fp) {
    isValid = false;
    return;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL ||
      (png_info_ptr = png_create_info_struct(png_ptr)) == NULL) {
    if (png_ptr)
      png_destroy_read_struct(&png_ptr, NULL, NULL);
    isValid = false;
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
    isValid = false;
    return;
  }
  png_read_update_info(png_ptr, png_info_ptr);

  // Read colorspace information.
  bool hassRGB, hasiCCP, hasgAMA, hascHRM;
  hassRGB = hasiCCP = hasgAMA = hascHRM = false;
  // Precedence in this order.
  if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_iCCP)) {
    png_charpp  name    = NULL;
    int         compression_type = 0;
    png_bytepp  profile = NULL;
    png_uint_32 proflen = 0;
    if (png_get_iCCP(png_ptr, png_info_ptr,
                     name, &compression_type, profile, &proflen)) {
      colorSpace.ICCP.name = std::string(*name, strlen(*name));
      colorSpace.ICCP.profile.resize(proflen);
      for (size_t i = 0; i < proflen; i++)
        colorSpace.ICCP.profile[i] = (*profile)[i];
      colorSpaceType = png_colorspace_iccp;
      hasiCCP = true;
    }
  } else if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_sRGB)) {
    int  intent;
    if (png_get_sRGB(png_ptr, png_info_ptr, &intent)) {
      colorSpace.sRGB = intent;
      colorSpaceType  = png_colorspace_srgb;
      hassRGB = true;
    }
  } else if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_cHRM)) {
    double xw, yw, xr, yr, xg, yg, xb, yb;
    if (png_get_cHRM(png_ptr, png_info_ptr, &xw, &yw,
                     &xr, &yr, &xg, &yg, &xb, &yb)) {
      colorSpace.calRGB.xw = xw;
      colorSpace.calRGB.yw = yw;
      colorSpace.calRGB.xr = xr;
      colorSpace.calRGB.yr = yr;
      colorSpace.calRGB.xg = xg;
      colorSpace.calRGB.yg = yg;
      colorSpace.calRGB.xb = xb;
      colorSpace.calRGB.yb = yb;
      colorSpace.gamma = 2.2;    // maybe wrong but PhotoShop assume this?
      colorSpaceType = png_colorspace_calibrated;
      hascHRM = true;
    }
  }
  if (!hasiCCP && !hassRGB &&
      png_get_valid(png_ptr, png_info_ptr, PNG_INFO_gAMA)) {
    double G;
    if (png_get_gAMA(png_ptr, png_info_ptr, &G)) {
      colorSpace.gamma = 1.0 / G;
      if (!hascHRM)
        colorSpaceType = png_colorspace_gamma_only;
      colorSpace.hasGamma = hasgAMA = true;
    }
  }

  // Read raster image data.
  rowbytes = png_get_rowbytes(png_ptr, png_info_ptr);
  png_bytep stream_data_ptr = new png_byte[rowbytes * height];

  png_bytepp rows_p = new png_bytep[height];
  for (uint32_t i = 0; i < height; i++)
    rows_p[i] = &(stream_data_ptr[rowbytes * i]);
  png_read_image(png_ptr, rows_p);
  delete rows_p;

  // Reading file finished.
  png_read_end(png_ptr, NULL);

  // Cleanup.
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
  if(!fp)
    return -1;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL ||
      (png_info_ptr = png_create_info_struct(png_ptr)) == NULL) {
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

  // Write colorspace related information.
  switch (colorSpaceType) {
  case png_colorspace_gamma_only:
   png_set_gAMA(png_ptr, png_info_ptr, 1.0 / colorSpace.gamma);
   break;
  case png_colorspace_calibrated:
    if (colorSpace.hasGamma)
      png_set_gAMA(png_ptr, png_info_ptr, 1.0 / colorSpace.gamma);
    png_set_cHRM(png_ptr, png_info_ptr,
                 colorSpace.calRGB.xw, colorSpace.calRGB.yw,
                 colorSpace.calRGB.xr, colorSpace.calRGB.yr,
                 colorSpace.calRGB.xg, colorSpace.calRGB.yg,
                 colorSpace.calRGB.xb, colorSpace.calRGB.yb);
    break;
  case png_colorspace_srgb:
    png_set_sRGB(png_ptr, png_info_ptr, colorSpace.sRGB);
    break;
  case png_colorspace_iccp:
    if (colorSpace.ICCP.profile.size() > 0) {
      png_set_iCCP(png_ptr, png_info_ptr,
                   colorSpace.ICCP.name.c_str(),
                   PNG_COMPRESSION_TYPE_DEFAULT,
                   colorSpace.ICCP.profile.data(),
                   colorSpace.ICCP.profile.size());
    }
    break;
  default:
    assert(colorSpaceType == png_colorspace_device);
    break;
  }
  // Resolution convert ppi to pix-per-meter
  png_set_pHYs(png_ptr, png_info_ptr,
               (dpi_x * 10000 + 127) / 254, // add 127 to round to nearest int.
               (dpi_y * 10000 + 127) / 254,
               PNG_RESOLUTION_METER);

  png_write_info(png_ptr, png_info_ptr);

  // Write raster image body.
  png_bytep row = new png_byte[getWidth() * (getBPC() / 8) * getNComps()];
  if (getBPC() == 8) {
    for (int32_t j = 0; j < getHeight(); j++) {
      for (int32_t i = 0; i < getWidth(); i++) {
        Color pixel = getPixel(i, j);
        for (int c = 0; c < getNComps(); c++) {
          row[getNComps() * i + c] = 255 * ((float) pixel.v[c] / 65535.) + .5;
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
