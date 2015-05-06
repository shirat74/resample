#ifndef __PNGIMAGE_HH__
#define __PNGIMAGE_HH__

#include <vector>
#include "Image.hh"

class PNGImage : public Image
{
public:
  PNGImage(int32_t width, int32_t height, int8_t nComps, int8_t bpc) :
    Image(width, height, nComps, bpc)
    { };
  PNGImage(int32_t width, int32_t height, int8_t nComps, int8_t bpc,
           const std::string raster) :
      Image(width, height, nComps, bpc, raster)
    { };
  PNGImage(const std::string filename);

  int save(const std::string filename) const;

  bool issRGB() const { return sRGB >= 0 ? true : false; };
  bool hasICCProfile() const { return iCCP.empty() ? false : true; };
  std::string getICCProfile() const { return iCCP; };
  bool  hasGamma() const { return validGamma(gAMA) ? true : false; };
  float getGamma() const { return gAMA; };
  std::vector<std::string> getiText() const { return iTXt;};

private:
  int8_t        sRGB;
  std::string   iCCP;
  float         gAMA;
  struct cHRM {
    bool  hascHRM;
    float xw, yw, xr, yr, xg, yg, xb, yb;
  };
  struct cHRM   cHRM;
  std::vector<std::string> iTXt;

  static bool validGamma(float gamma) { return gamma > 0.0 ? true : false; };
};

#endif // __PNGIMAGE_HH__
