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
    {
      isValid = true; colorSpaceType = png_colorspace_device;
      dpi_x = dpi_y = 72;
    };
  PNGImage(int32_t width, int32_t height, int8_t nComps, int8_t bpc,
           const std::string raster) :
      Image(width, height, nComps, bpc, raster)
    {
      isValid = true; colorSpaceType = png_colorspace_device;
      dpi_x = dpi_y = 72;
    };
  // Read data from file and construct PNGImage object.
  PNGImage(const std::string filename);
  // Save to file.
  int  save(const std::string filename) const;
  // Check if load image succeeded.
  bool valid() const { return isValid; };

  enum png_colorspace_type_e getColorSpaceType() { return colorSpaceType; };
  bool hasGamma() const { return colorSpace.hasGamma; };

  std::string getICCProfileName() const { return colorSpace.ICCP.name; };
  std::vector<unsigned char> getICCProfile() const
      { return colorSpace.ICCP.profile; };
  int   getsRGBIntent() const { return colorSpace.sRGB; };
  float getGamma() const { return colorSpace.gamma; };
  // 8 values for whitepoint and matrix
  std::vector<float> getChromaticity() const {
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

  float getResolutionX() const { return dpi_x; };
  float getResolutionY() const { return dpi_y; };
  void  setResolutionX(float dpi) { dpi_x = dpi; };
  void  setResolutionY(float dpi) { dpi_y = dpi; };
  void  setResolution (float x, float y) { dpi_x = x; dpi_y = y; };

private:

  bool isValid;

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

  float dpi_x, dpi_y;
};

#endif // __PNGIMAGE_HH__
