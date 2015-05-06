#ifndef __PNGIMAGE_HH__
#define __PNGIMAGE_HH__

#include <vector>
#include "Image.hh"

enum png_colorspace_type_e
{
  png_colorspace_device = 0, // none -- use device dependent
  png_colorspace_iccp,
  png_colorspace_srgb,
  png_colorspace_calibrated, // cHRM specified possibly with gAMA
  png_colorspace_gamma_only, // only gAMA specified
};

class PNGImage : public Image
{
public:
  PNGImage(int32_t width, int32_t height, int8_t nComps, int8_t bpc) :
    Image(width, height, nComps, bpc)
    { colorSpaceType = png_colorspace_device; };
  PNGImage(int32_t width, int32_t height, int8_t nComps, int8_t bpc,
           const std::string raster) :
      Image(width, height, nComps, bpc, raster)
    { colorSpaceType = png_colorspace_device; };
  // Read data from file and construct PNGImage object.
  PNGImage(const std::string filename);
  // Save to file.
  int save(const std::string filename) const;

  enum png_colorspace_type_e getColorSpaceType() { return colorSpaceType; };
  bool hasGamma() { return colorSpace.hasGamma; };

  std::string getICCProfileName() { return colorSpace.ICCP.name; };
  std::vector<unsigned char> getICCProfile()
      { return colorSpace.ICCP.profile; };
  int   getsRGBIntent() { return colorSpace.sRGB; };
  float getGamma() { return colorSpace.gamma; };
  // 8 values for whitepoint and matrix
  std::vector<float> getChromaticity() {
    std::vector<float> cal(8);
    cal[0] = colorSpace.calRGB.xw;
    cal[1] = colorSpace.calRGB.yw;
    cal[2] = colorSpace.calRGB.xr;
    cal[3] = colorSpace.calRGB.yr;
    cal[4] = colorSpace.calRGB.xg;
    cal[5] = colorSpace.calRGB.yg;
    cal[6] = colorSpace.calRGB.xb;
    cal[7] = colorSpace.calRGB.yb;
    return cal;
  }

  // The following easily introduces inconsistency.
  void setColorSpaceType(enum png_colorspace_type_e type)
      { colorSpaceType = type; };
  void setICCProfileName(std::string name) { colorSpace.ICCP.name = name; };
  void setICCProfile(std::vector<unsigned char> profile)
      { colorSpace.ICCP.profile = profile; };
  void setsRGBIntent(int intent) { colorSpace.sRGB = intent; };
  void setGamma(float gamma)
      { colorSpace.gamma = gamma; colorSpace.hasGamma = true; };
  // 8 values for whitepoint and matrix
  void setChromaticity(float xw, float yw, float xr, float yr,
                       float xg, float yg, float xb, float yb) {
    colorSpace.calRGB.xw = xw;
    colorSpace.calRGB.yw = yw;
    colorSpace.calRGB.xr = xr;
    colorSpace.calRGB.yr = yr;
    colorSpace.calRGB.xg = xg;
    colorSpace.calRGB.yg = yg;
    colorSpace.calRGB.xb = xb;
    colorSpace.calRGB.yb = yb;
  }

private:

  struct ICCP {
    std::string name;
    std::vector<unsigned char> profile;
  };
  struct CalRGB {
    float xw, yw, xr, yr, xg, yg, xb, yb;
  };

  // union { not working
  struct {
    int8_t        sRGB;   // rendering intent stored
    struct ICCP   ICCP;
    float         gamma;  // Gama Only
    struct CalRGB calRGB; // Calibrated: with cHRM and gAMA
    bool          hasGamma;
  } colorSpace;

  enum png_colorspace_type_e colorSpaceType;
};

#endif // __PNGIMAGE_HH__
