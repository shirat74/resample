// A sample program for resampling PNG image.

#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include "PNGImage.hh"
#include "Resampler.hh"

static const char u[] = "\
usage: resample [-options] input.png output.png\n\
options:\n\
    -a          keep aspect ratio\n\
    -r          set resolution (dpi)\n\
    -x xsize    width of output image (in pixels)\n\
    -y ysize    height of output image\n\
    -f filter   filter type\n\
Available filters are:\n\
     b          Box\n\
     l          Biliner\n\
     B          B-spline\n\
     c          Bicubic\n\
     L          Lanczos\n\
     m          Mitchell\n\
";

void
usage()
{
  std::cerr << u << std::endl;
  exit(1);
}

int
main (int argc, char *argv[])
{
  extern int   optind;
  extern char *optarg;
  int32_t      xsize = 0, ysize = 0, dpi = 0;
  std::string  filter;
  std::string  dstfile, srcfile;
  bool         keep_aspect = false;
  int          error = 0;

  // process command line options.
  {
    int  c;
    while ((c = getopt(argc, argv, "r:x:y:af:V")) != EOF) {
      switch(c) {
      case 'a': keep_aspect = true;   break;
      case 'r': dpi   = atoi(optarg); break;
      case 'x': xsize = atoi(optarg); break;
      case 'y': ysize = atoi(optarg); break;
      case 'f':
        switch(*optarg) {
        case 'b': filter = "Box"      ; break;
        case 'l': filter = "Bilinear" ; break;
        case 'B': filter = "B-Spline" ; break;
        case 'c': filter = "Bicubic"  ; break;
        case 'L': filter = "Lanczos3" ; break;
        case 'm': filter = "Mitchell" ; break;
        default: usage();
        }
        break;
      case '?': usage();
      default:  usage();
      }
    }
  }
  if((argc - optind) != 2)
    usage();
  srcfile = argv[optind];
  dstfile = argv[optind + 1];

  PNGImage src(srcfile);
  if (!src.valid()) {
    std::cerr << "Loading PNG image \"" << srcfile << "\" failed." << std::endl;
    exit(2);
  }
  if (xsize > 0 && ysize > 0) {
    if (keep_aspect)
      std::cerr << "Ignoring -a option." << std::endl;
  } else if (keep_aspect) {
    if (xsize == 0)
      xsize = ysize * src.getWidth()  / src.getHeight();
    if (ysize == 0)
      ysize = xsize * src.getHeight() / src.getWidth();
  }
  if (xsize <= 0)
    xsize = src.getWidth();
  if (ysize <= 0)
    ysize = src.getHeight();

  Resampler resampler(filter);
  Image ras = resampler.resampleImage(src, xsize, ysize);
  PNGImage dst(ras.getWidth(), ras.getHeight(),
               ras.getNComps(), ras.getBPC(), ras.getPixelBytes());
  if (dpi > 0)
    dst.setResolution(dpi, dpi);
  else { // Copy resolution setting from source image.
    dst.setResolution(src.getResolutionX(), src.getResolutionY());
  }
  // Copy colorspace related information
  // There are currently no easy way to copy it.
  {
    enum png_colorspace_type_e type = src.getColorSpaceType();
    dst.setColorSpaceType(type);
    switch (type) {
    case png_colorspace_iccp:
      dst.setICCProfileName(src.getICCProfileName());
      dst.setICCProfile(src.getICCProfile());
      break;
    case png_colorspace_srgb:
      dst.setsRGBIntent(src.getsRGBIntent());
      break;
    case png_colorspace_calibrated:
      {
        std::vector<float> c = src.getChromaticity();
        dst.setChromaticity(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
        dst.setGamma(src.getGamma());
      }
      break;
    case png_colorspace_gamma_only:
      dst.setGamma(src.getGamma());
      break;
    case png_colorspace_device:
      // do nothing
      break;
    }
  }
  error = dst.save(dstfile);
  if (error) {
    std::cerr << "Could not save destination image: " << dstfile << std::endl;
    exit(2);
  }

  return 0;
}
