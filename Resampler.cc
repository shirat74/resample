// Based on public domain code by Dale Schumacher
// THIS FILE IS IN THE PUBLIC DOMAIN

#include <math.h>

#include <string>
#include <vector>

#include "Resampler.hh"

//
// Interpolaion kernels: Box (Nearest Neighbor), Bilinear, B-spline,
//                       Bicubic, Lanczos3, Mitchell
//
#define BOX_SUPPORT 0.5
float Resampler::box_filter (float t)
{
  return (((t > -0.5) && (t <= 0.5)) ? 1.0 : 0.0);
}

#define BILINEAR_SUPPORT 1.0
float Resampler::bilinear_filter (float t)
{
  if (t < 0.0)
    t = -t;
  return (t < 1.0 ? 1.0 - t : 0);
}

#define B_SPLINE_SUPPORT 2.0
float Resampler::B_spline_filter (float t)
{
  if (t < 0.0)
    t = -t;
  if (t < 1.0) {
    return ((0.5 * t * t * t) - t * t + (2.0 / 3.0));
  } else if (t < 2) {
    t = 2 - t;
    return ((1.0 / 6.0) * (t * t * t));
  }
  return 0.0;
}

#define BICUBIC_SUPPORT 2.0
#define a -0.5
float Resampler::bicubic_filter (float t)
{
  if (t < 0.0)
    t = -t;
  if (t < 1.0)
    return ((a+2.0)*t*t*t - (a+3.0)*t*t + 1.0);
  else if (t < 2.0)
   return (a*t*t*t - 5.0*a*t*t + 8.0*a*t - 4.0*a);
  return 0.0;
}
#undef a

#ifndef sinc
#  define sinc(x) (((x) != 0.0) ? (sin((x) * M_PI) / ((x) * M_PI)) : 1.0)
#endif
#define LANCZOS3_SUPPORT 3.0
float Resampler::Lanczos3_filter (float t)
{
  if (t < 0)
   t = -t;
  return ((t < 3.0) ? (sinc(t) * sinc( t / 3.0)) : 0.0);
}

// Mitchell filter
#define  MITCHELL_SUPPORT 2.0
#define  B  (1.0 / 3.0)
#define  C  (1.0 / 3.0)
float Resampler::Mitchell_filter (float t)
{
  float tt = t*t;
  if (t < 0)
    t = -t;
  if (t < 1.0) {
    t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt))
        + ((-18.0 + 12.0 * B + 6.0 * C) * tt)
        + (6.0 - 2 * B));
    return (t / 6.0);
  } else if (t < 2.0) {
    t = (((-1.0 * B - 6.0 * C) * (t * tt))
        + ((6.0 * B + 30.0 * C) * tt)
        + ((-12.0 * B - 48.0 * C) * t)
        + (8.0 * B + 24 * C));
    return (t / 6.0);
  }
  return 0.0;
}
#undef B
#undef C

const struct filterItem Resampler::filters[] = {
  {"Box",       Resampler::box_filter,      BOX_SUPPORT},
  {"Bilinear",  Resampler::bilinear_filter, BILINEAR_SUPPORT},
  {"B-spline",  Resampler::B_spline_filter, B_SPLINE_SUPPORT},
  {"Bicubic",   Resampler::bicubic_filter,  BICUBIC_SUPPORT},
  {"Lanczos",   Resampler::Lanczos3_filter, LANCZOS3_SUPPORT},
  {"Mitchell",  Resampler::Mitchell_filter, MITCHELL_SUPPORT}
};
const int Resampler::NUM_FILTERS = (sizeof(filters) / sizeof(filters[0]));

// Resampler class
Resampler::Resampler ()
{
  Resampler("Bicubic"); // default to Bicubic
}

Resampler::Resampler (const std::string& filter)
{
  filter_fn = filters[0].func;
  support   = filters[0].support;
  for (int i = 0; i < NUM_FILTERS; i++) {
    if (filter == filters[i].name) {
      filter_fn = filters[i].func;
      support   = filters[i].support;
      break;
    }
  }
}

Resampler::~Resampler ()
{

}

// Calculate how much sorrounding pixels contribute
void
Resampler::setupContributorForDownsample (float   scale,
                                          int32_t dstSize,
                                          int32_t boundary)
{
  float supportSize = support / scale;

  contributor.resize(dstSize);

  for (int32_t i = 0; i < dstSize; i++) {
    contributor[i].n = 0;
    std::vector<Contrib> p((int) (supportSize * 2 + 1));
    contributor[i].p = p;
    float center = (float) i / scale;
    float left   = ceil (center - supportSize);
    float right  = floor(center + supportSize);
    float total  = 0.0; // Normalization required
    for (int32_t k = left; k <= right; k++) {
      total += (*filter_fn)((center - k) * scale);
    }
    for (int32_t j = left; j <= right; j++) {
      int32_t n = j;
      float weight = (*filter_fn)((center - j) * scale) / total;
      // reflect at boundary
      if (j < 0) {
        n = - j; // or periodic: dstSize - j
      } else if (j >= boundary) {
        n = (boundary - j) + boundary - 1; // j - boundary
      }
      int k = contributor[i].n++;
      contributor[i].p[k].pixel  = n;      // positio of kth contributor
      contributor[i].p[k].weight = weight; // weight
    }
  }
}

void
Resampler::setupContributorForUpsample (float   scale,
                                        int32_t dstSize,
                                        int32_t boundary)
{
  contributor.resize(dstSize);

  for (int32_t i = 0; i < dstSize; i++) {
    contributor[i].n = 0;
    std::vector<Contrib> p((int)(support * 2 + 1));
    contributor[i].p = p;
    float center = (float) i / scale;
    float left   = ceil (center - support);
    float right  = floor(center + support);
    for (int32_t j = left; j <= right; j++) {
      float   weight = (*filter_fn)(center - j);
      int32_t n = j;
      // reflect at boundary
      if (j < 0) {
        n = -j;
      } else if (j >= boundary) {
        n = (boundary - j) + boundary - 1;
      }
      int k = contributor[i].n++;
      contributor[i].p[k].pixel  = n;
      contributor[i].p[k].weight = weight;
    }
  }
}

#define MAP_IN_RANGE(A,L,H) ((A) <= (L) ? (L) : (A) <= (H) ? (A) : (H))
void
Resampler::resampleX (Image& dst, const Image& src) const
{
  for (int32_t k = 0; k < dst.getHeight(); k++) {
    for (int32_t i = 0; i < dst.getWidth(); i++) {
      Color pixel;
      for (int c = 0; c < src.getNComps(); c++) {
        float weight = 0.0;
        for (int j = 0; j < contributor[i].n; j++) {
          weight +=
            src.getPixel(contributor[i].p[j].pixel, k).v[c] *
                                        contributor[i].p[j].weight;
        }
        pixel.v[c] = (uint16_t) MAP_IN_RANGE(weight, 0, 65535u);
      }
      dst.putPixel(i, k, pixel);
    }
  }
}

void
Resampler::resampleY (Image& dst, const Image& src) const
{
  for (int32_t k = 0; k < dst.getWidth(); k++) {
    for (int32_t i = 0; i < dst.getHeight(); i++) {
      Color pixel;
      for (int c = 0; c < src.getNComps(); c++) {
        float weight = 0.0;
        for (int j = 0; j < contributor[i].n; j++) {
          weight +=
            src.getPixel(k, contributor[i].p[j].pixel).v[c] *
                                           contributor[i].p[j].weight;
        }
        pixel.v[c] = (uint16_t) MAP_IN_RANGE(weight, 0, 65535u);
      }
      dst.putPixel(k, i, pixel);
    }
  }
}

Image
Resampler::resampleImage (const Image& src, float xsize, float ysize)
{
  float xScale, yScale;

  Image dst((uint32_t) xsize, (uint32_t) ysize, src.getNComps(), src.getBPC());

  xScale = (float) xsize / (float) src.getWidth();
  yScale = (float) ysize / (float) src.getHeight();

  // create intermediate image to hold horizontal zoom
  Image tmp(dst.getWidth(), src.getHeight(), src.getNComps(), src.getBPC());
  if (xScale < 1.0)
    setupContributorForDownsample(xScale, dst.getWidth(), src.getWidth());
  else
    setupContributorForUpsample  (xScale, dst.getWidth(), src.getWidth());
  resampleX(tmp, src);

  if (yScale < 1.0)
    setupContributorForDownsample(yScale, dst.getHeight(), src.getHeight());
  else
    setupContributorForUpsample  (yScale, dst.getHeight(), src.getHeight());
  resampleY(dst, tmp);

  return  dst;
}
