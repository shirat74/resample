// Based on public domain code by Dale Schumacher
// THIS FILE IS IN THE PUBLIC DOMAIN

#ifndef __RESAMPLER_HH__
#define __RESAMPLER_HH__

#include <string>
#include <vector>
#include "Image.hh"

typedef struct
{
  int32_t pixel;
  float   weight;
} Contrib;

typedef struct
{
  int      n;
  std::vector<Contrib> p;
} ContribList;

struct filterList
{
  const char name[32];
  float (*func)(float);
  float support;
};

class Resampler
{
public:
  Resampler ();
  Resampler (const std::string& filter);
  ~Resampler();

  Image resampleImage(Image& src, float xsize, float ysize);

private:
  float (*filter_fn)(float);
  float support;
  std::vector<ContribList> contributor;

  void resampleX(Image& dst, Image& src) const;
  void resampleY(Image& dst, Image& src) const;
  std::vector<ContribList> setupContributorForDownsample (float scale,
                                       int32_t dstSize, int32_t boundary);
  std::vector<ContribList> setupContributorForUpsample   (float scale,
                                       int32_t dstSize, int32_t boundary);

  static float box_filter(float);
  static float bilinear_filter(float);
  static float B_spline_filter(float);
  static float bicubic_filter(float);
  static float Lanczos3_filter(float);
  static float Mitchell_filter(float);

  static const struct filterList filters[];
  static const int NUM_FILTERS;
};

#endif // __RESAMPLER_HH__
