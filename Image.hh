#ifndef __IMAGE_HH__
#define __IMAGE_HH__

#include <string>

class Color
{
public:
  Color() {};

  Color(uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
    v[0] = r; v[1] = g; v[2] = b; v[3] = a; n = 4;
  };
  Color(uint16_t r, uint16_t g, uint16_t b) {
    v[0] = r; v[1] = g; v[2] = b; n = 3;
  };
  Color(uint16_t g, uint16_t a) { v[0] = g; v[1] = a; };
  Color(uint16_t g) { v[0] = g; n = 1; };

  uint16_t v[5];
  int8_t   n;
};

class Image
{
public:
  Image(int32_t width, int32_t height, int8_t num_comp, int8_t bpc);
  Image(int32_t width, int32_t height, int8_t num_comp, int8_t bpc,
        const std::string& raster);
  ~Image();

  int32_t  getWidth()  const { return width; };
  int32_t  getHeight() const { return height; };
  int8_t   getNComps() const { return nComps; };
  int8_t   getBPC() const { return bpc; };

  std::string getPixelBytes() const;

  Color getPixel(int32_t x, int32_t y) const;
  void  putPixel(int32_t x, int32_t y, Color color);

protected:
  // It is sometimes very inconvinient to disallow modification of data.
  // Subclasses inherite Image class that read image file format such as PNG
  // within its constructor does not know required width, height, and other
  // values until they finish reading a file. Actual construction of Image
  // super class must be delayed until they finish to read input file.
  void reset(int32_t width, int32_t height,
             int8_t nComps, int8_t bpc, const std::string raster)
  {
    this->width  = width;
    this->height = height;
    this->nComps = nComps;
    this->bpc    = bpc;
    delete data;
    data = data_from_string(raster, width, height, nComps, bpc);
  };

private:
  static Color *data_from_string (const std::string raster,
                                  int32_t width, int32_t height,
                                  int8_t nComps, int8_t bpc);
  int32_t  width;
  int32_t  height;
  int8_t   nComps;
  int8_t   bpc;

  Color *data;
};

#endif // __IMAGE_HH__
