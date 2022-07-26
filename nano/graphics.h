/*
 * Nano Library
 *
 * Copyright (C) 2022, Meta-Sonic
 * All rights reserved.
 *
 * Proprietary and confidential.
 * Any unauthorized copying, alteration, distribution, transmission, performance,
 * display or other use of this material is strictly prohibited.
 *
 * Written by Alexandre Arsenault <alx.arsenault@gmail.com>
 */

#pragma once

/*!
 * @file      nano/graphics.h
 * @brief     nano graphics
 * @copyright Copyright (C) 2022, Meta-Sonic
 * @author    Alexandre Arsenault alx.arsenault@gmail.com
 * @date      Created 16/06/2022
 */

#include <nano/common.h>
#include <nano/geometry.h>

#include <filesystem>
#include <iomanip>
#include <string>
#include <string_view>
#include <vector>

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(warning, "-Weverything")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat")

namespace nano {

///
///
///
class color {
public:
  enum class format { argb, rgba, abgr };

  template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  struct float_rgba {
    T r, g, b, a;
  };

  template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  struct float_rgb {
    T r, g, b;
  };

  template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  struct float_grey_alpha {
    T grey, alpha;
  };

  NANO_INLINE_CXPR static color black() NANO_NOEXCEPT { return color(0x000000ff); }
  NANO_INLINE_CXPR static color white() NANO_NOEXCEPT { return color(0xffffffff); }
  NANO_INLINE_CXPR static color grey(std::uint8_t c) NANO_NOEXCEPT { return color(c, c, c, 0xFF); }

  color() NANO_NOEXCEPT = default;
  color(const color&) NANO_NOEXCEPT = default;
  color(color&&) NANO_NOEXCEPT = default;

  ~color() NANO_NOEXCEPT = default;

  color& operator=(const color&) NANO_NOEXCEPT = default;
  color& operator=(color&&) NANO_NOEXCEPT = default;

  NANO_INLINE_CXPR color(std::uint32_t rgba) NANO_NOEXCEPT;

  NANO_INLINE_CXPR color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) NANO_NOEXCEPT;

  NANO_INLINE_CXPR static color from_argb(std::uint32_t argb) NANO_NOEXCEPT;

  template <typename T>
  NANO_INLINE_CXPR color(const float_rgba<T>& rgba) NANO_NOEXCEPT;

  template <typename T>
  NANO_INLINE_CXPR color(const float_rgb<T>& rgb) NANO_NOEXCEPT;

  template <typename T>
  NANO_INLINE_CXPR color(const float_grey_alpha<T>& ga) NANO_NOEXCEPT;

  template <typename T, std::size_t Size, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  NANO_INLINE_CXPR color(const T (&data)[Size]) NANO_NOEXCEPT;

  template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  NANO_INLINE_CXPR color(const T* data, std::size_t size) NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR std::uint32_t& rgba() NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR std::uint32_t rgba() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR std::uint32_t argb() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR std::uint32_t abgr() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR float_rgba<float> f_rgba() const NANO_NOEXCEPT;

  template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
  NANO_NODC_INLINE_CXPR float_rgba<T> f_rgba() const NANO_NOEXCEPT;

  NANO_INLINE_CXPR color& red(std::uint8_t r) NANO_NOEXCEPT;
  NANO_INLINE_CXPR color& green(std::uint8_t g) NANO_NOEXCEPT;
  NANO_INLINE_CXPR color& blue(std::uint8_t b) NANO_NOEXCEPT;
  NANO_INLINE_CXPR color& alpha(std::uint8_t a) NANO_NOEXCEPT;

  template <format Format>
  NANO_INLINE_CXPR color& red(std::uint8_t r) NANO_NOEXCEPT {
    if constexpr (Format == format::rgba) {
      return red(r);
    }
    else if constexpr (Format == format::argb) {
      return green(r);
    }
    else if constexpr (Format == format::abgr) {
      return alpha(r);
    }
    return *this;
  }

  template <format Format>
  NANO_INLINE_CXPR color& green(std::uint8_t g) NANO_NOEXCEPT {
    if constexpr (Format == format::rgba) {
      return green(g);
    }
    else if constexpr (Format == format::argb) {
      return blue(g);
    }
    else if constexpr (Format == format::abgr) {
      return blue(g);
    }
    return *this;
  }

  template <format Format>
  NANO_INLINE_CXPR color& blue(std::uint8_t b) NANO_NOEXCEPT {
    if constexpr (Format == format::rgba) {
      return blue(b);
    }
    else if constexpr (Format == format::argb) {
      return alpha(b);
    }
    else if constexpr (Format == format::abgr) {
      return green(b);
    }
    return *this;
  }

  NANO_NODC_INLINE_CXPR std::uint8_t red() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR std::uint8_t green() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR std::uint8_t blue() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR std::uint8_t alpha() const NANO_NOEXCEPT;

  template <typename T>
  NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> red() const NANO_NOEXCEPT;

  template <typename T>
  NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> green() const NANO_NOEXCEPT;

  template <typename T>
  NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> blue() const NANO_NOEXCEPT;

  template <typename T>
  NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> alpha() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR float f_red() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR float f_green() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR float f_blue() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR float f_alpha() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR std::uint8_t operator[](std::size_t index) const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR bool is_opaque() const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR bool is_transparent() const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR color darker(float amount) const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR color brighter(float amount) const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR color with_red(std::uint8_t red) const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR color with_green(std::uint8_t green) const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR color with_blue(std::uint8_t blue) const NANO_NOEXCEPT;
  NANO_NODC_INLINE_CXPR color with_alpha(std::uint8_t alpha) const NANO_NOEXCEPT;

  /// mu should be between [0, 1]
  NANO_NODC_INLINE_CXPR color operator*(float mu) const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR bool operator==(const color& c) const NANO_NOEXCEPT;

  NANO_NODC_INLINE_CXPR bool operator!=(const color& c) const NANO_NOEXCEPT;

  template <class CharT, class TraitsT>
  inline friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, nano::color c);

private:
  std::uint32_t m_rgba;
};

static_assert(std::is_trivial<color>::value, "nano::color must remain a trivial type");

///
///
///
class image {
public:
  enum class type { png, jpeg };

  enum class format {
    alpha,

    argb,
    bgra,

    rgb,

    rgba,
    abgr,

    rgbx,
    xbgr,

    xrgb,
    bgrx,

    float_alpha,
    float_argb,
    float_rgb,
    float_rgba
  };

  using handle = void*;

  image();
  image(const std::string& filepath, type fmt);
  image(handle native_img);

  image(const nano::size<std::size_t>& size, std::size_t bitsPerComponent, std::size_t bitsPerPixel,
      std::size_t bytesPerRow, format fmt, const std::uint8_t* buffer = nullptr);

  image(const image& img);
  image(image&& img);

  ~image();

  image& operator=(const image& img);
  image& operator=(image&& img);

  bool is_valid() const;
  inline explicit operator bool() const { return is_valid(); }

  nano::size<std::size_t> get_size() const;
  std::size_t width() const;
  std::size_t height() const;

  inline nano::rect<std::size_t> get_rect() const { return { { 0, 0 }, get_size() }; }

  image get_sub_image(const nano::rect<std::size_t>& r) const;

  image make_copy();

  handle get_native_image() const;

  std::size_t get_bits_per_component() const;
  std::size_t get_bits_per_pixel() const;
  std::size_t get_bytes_per_row() const;

  const std::uint8_t* data() const;

  void copy_data(std::vector<std::uint8_t>& buffer) const;

  std::vector<std::uint8_t> get_data() const;

  image create_colored_image(const nano::color& color) const;

  bool save(const std::filesystem::path& filepath, type fmt);

  static nano::size<double> get_dpi(const std::string& filepath);

  struct pimpl;

private:
  pimpl* m_pimpl;
};

enum class text_alignment { left, center, right };

/// https://developer.apple.com/documentation/quartzcore/cashapelayer/1521905-linecap?language=objc
enum class line_join {
  /// The edges of the adjacent line segments are continued to meet at a sharp point.
  miter,

  /// A join with a rounded end. Core Graphics draws the line to extend beyond
  /// the endpoint of the path. The line ends with a semicircular arc with a
  /// radius of 1/2 the line’s width, centered on the endpoint.
  round,

  /// A join with a squared-off end. Core Graphics draws the line to extend beyond
  /// the endpoint of the path, for a distance of 1/2 the line’s width.
  bevel

};

/// https://developer.apple.com/documentation/quartzcore/cashapelayer/1522147-linejoin?language=objc
enum class line_cap {
  /// A line with a squared-off end. Core Graphics draws the line to
  /// extend only to the exact endpoint of the path.
  /// This is the default.
  butt,

  /// A line with a rounded end. Core Graphics draws the line to extend beyond the
  /// endpoint of the path. The line ends with a semicircular arc with a radius
  /// of 1/2 the line’s width, centered on the endpoint.
  round,

  /// A line with a squared-off end. Core Graphics extends the line beyond the
  /// endpoint of the path for a distance equal to half the line width.
  square

};

///
///
///
class font {
public:
  using handle = const void*;
  struct filepath_tag {};

  font();

  ///
  font(const char* font_name, double font_size);

  ///
  font(const char* filepath, double font_size, filepath_tag);

  ///
  font(const std::uint8_t* data, std::size_t data_size, double font_size);

  font(const font& f);
  font(font&& f);

  ~font();

  font& operator=(const font& f);

  font& operator=(font&& f);

  ///
  bool is_valid() const noexcept;

  ///
  inline explicit operator bool() const noexcept { return is_valid(); }

  ///
  double get_height() const noexcept;

  ///
  double get_font_size() const noexcept;

  ///
  float get_string_width(std::string_view text) const;

  handle get_native_font() const noexcept;

  struct pimpl;

private:
  pimpl* m_pimpl;
};

///
///
///
class graphic_context {
public:
  using handle = void*;
  class scoped_state;

  graphic_context(handle nc, bool is_bitmap = false);

  graphic_context(const graphic_context&) = delete;
  graphic_context(graphic_context&&) = delete;

  ~graphic_context();

  graphic_context& operator=(const graphic_context&) = delete;
  graphic_context& operator=(graphic_context&&) = delete;

  static graphic_context create_bitmap_context(const nano::size<std::size_t>& size, image::format fmt);

  //  static graphic_context create_bitmap_context(const nano::size<std::size_t>& size, std::size_t bitsPerComponent,
  //                 std::size_t bytesPerRow, image::format fmt,   std::uint8_t* buffer = nullptr);

  /// CTM (current transformation matrix)
  /// clip region
  /// image interpolation quality
  /// line width
  /// line join
  /// miter limit
  /// line cap
  /// line dash
  /// flatness
  /// should anti-alias
  /// rendering intent
  /// fill color space
  /// stroke color space
  /// fill color
  /// stroke color
  /// alpha value
  /// font
  /// font size
  /// character spacing
  /// text drawing mode
  /// shadow parameters
  /// the pattern phase
  /// the font smoothing parameter
  /// blend mode
  void save_state();
  void restore_state();

  void begin_transparent_layer(float alpha);
  void end_transparent_layer();

  void translate(const nano::point<float>& pos);

  void clip();
  void clip_even_odd();
  void reset_clip();
  void clip_to_rect(const nano::rect<float>& rect);
  // void clip_to_path(const nano::path& p);
  // void clip_to_path_even_odd(const nano::path& p);
  void clip_to_mask(const nano::image& img, const nano::rect<float>& rect);

  void begin_path();
  void close_path();
  void add_rect(const nano::rect<float>& rect);
  // void add_path(const nano::path& p);

  nano::rect<float> get_clipping_rect() const;

  void set_line_width(float width);
  void set_line_join(line_join lj);
  void set_line_cap(line_cap lc);
  void set_line_style(float width, line_join lj, line_cap lc);

  void set_fill_color(const nano::color& c);
  void set_stroke_color(const nano::color& c);

  void fill_rect(const nano::rect<float>& r);
  void stroke_rect(const nano::rect<float>& r);
  void stroke_rect(const nano::rect<float>& r, float line_width);

  void stroke_line(const nano::point<float>& p0, const nano::point<float>& p1);

  void fill_ellipse(const nano::rect<float>& r);
  void stroke_ellipse(const nano::rect<float>& r);

  void fill_rounded_rect(const nano::rect<float>& r, float radius);

  void stroke_rounded_rect(const nano::rect<float>& r, float radius);
  // void fill_quad(const nano::quad& q);
  // void stroke_quad(const nano::quad& q);

  // void fill_path(const nano::path& p);
  // void fill_path(const nano::path& p, const nano::rect<float>& rect);
  // void fill_path_with_shadow(
  // const nano::path& p, float blur, const nano::color& shadow_color, const nano::size<float>& offset);

  // void stroke_path(const nano::path& p);

  void draw_image(const nano::image& img, const nano::point<float>& pos);
  void draw_image(const nano::image& img, const nano::rect<float>& r);
  void draw_image(const nano::image& img, const nano::rect<float>& r, const nano::rect<float>& clip_rect);

  void draw_sub_image(const nano::image& img, const nano::rect<float>& r, const nano::rect<float>& img_rect);

  void draw_text(const nano::font& f, const std::string& text, const nano::point<float>& pos);

  void draw_text(
      const nano::font& f, const std::string& text, const nano::rect<float>& rect, nano::text_alignment alignment);

  // void set_shadow(float blur, const nano::color& shadow_color, const nano::size<float>& offset);

  bool is_bitmap() const noexcept;

  nano::image create_image();

  handle get_handle() const noexcept;

  struct pimpl;

private:
  pimpl* m_pimpl;
};

class display {
public:
  static double get_scale_factor();
  static double get_refresh_rate();
};

//------------------------

//
// MARK: - color -
//

namespace detail {
  template <typename T>
  static inline std::uint32_t color_float_component_to_uint32(T f) {
    return static_cast<std::uint32_t>(std::floor(f * 255));
  }

  enum color_shift : std::uint32_t {
    color_shift_red = 24,
    color_shift_green = 16,
    color_shift_blue = 8,
    color_shift_alpha = 0
  };

  enum color_bits : std::uint32_t {
    color_bits_red = 0xff000000,
    color_bits_green = 0x00ff0000,
    color_bits_blue = 0x0000ff00,
    color_bits_alpha = 0x000000ff
  };
} // namespace detail.

NANO_INLINE_CXPR color::color(std::uint32_t rgba) NANO_NOEXCEPT : m_rgba(rgba) {}

NANO_INLINE_CXPR color::color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) NANO_NOEXCEPT
    : m_rgba((std::uint32_t(a) << detail::color_shift_alpha) | (std::uint32_t(b) << detail::color_shift_blue)
          | (std::uint32_t(g) << detail::color_shift_green) | (std::uint32_t(r) << detail::color_shift_red)) {}

NANO_INLINE_CXPR color color::from_argb(std::uint32_t argb) NANO_NOEXCEPT {
  const color c(argb);
  return color(c.green(), c.blue(), c.alpha(), c.red());
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_rgba<T>& rgba) NANO_NOEXCEPT {
  std::uint32_t ur = detail::color_float_component_to_uint32(rgba.r);
  std::uint32_t ug = detail::color_float_component_to_uint32(rgba.g);
  std::uint32_t ub = detail::color_float_component_to_uint32(rgba.b);
  std::uint32_t ua = detail::color_float_component_to_uint32(rgba.a);
  m_rgba = (ua << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
      | (ur << detail::color_shift_red);
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_rgb<T>& rgb) NANO_NOEXCEPT {
  std::uint32_t ur = detail::color_float_component_to_uint32(rgb.r);
  std::uint32_t ug = detail::color_float_component_to_uint32(rgb.g);
  std::uint32_t ub = detail::color_float_component_to_uint32(rgb.b);
  m_rgba = (255 << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
      | (ur << detail::color_shift_red);
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_grey_alpha<T>& ga) NANO_NOEXCEPT {
  std::uint32_t u = detail::color_float_component_to_uint32(ga.grey);
  std::uint32_t ua = detail::color_float_component_to_uint32(ga.alpha);
  m_rgba = (ua << detail::color_shift_alpha) | (u << detail::color_shift_blue) | (u << detail::color_shift_green)
      | (u << detail::color_shift_red);
}

template <typename T, std::size_t Size, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_INLINE_CXPR color::color(const T (&data)[Size]) NANO_NOEXCEPT : color(&data[0], Size) {}

template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_INLINE_CXPR color::color(const T* data, std::size_t size) NANO_NOEXCEPT {
  switch (size) {
  case 2: {
    std::uint32_t u = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t a = detail::color_float_component_to_uint32(data[1]);
    m_rgba = (a << detail::color_shift_alpha) | (u << detail::color_shift_blue) | (u << detail::color_shift_green)
        | (u << detail::color_shift_red);
  }
    return;
  case 3: {
    std::uint32_t ur = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t ug = detail::color_float_component_to_uint32(data[1]);
    std::uint32_t ub = detail::color_float_component_to_uint32(data[2]);
    m_rgba = (255 << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
        | (ur << detail::color_shift_red);
  }
    return;
  case 4: {
    std::uint32_t ur = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t ug = detail::color_float_component_to_uint32(data[1]);
    std::uint32_t ub = detail::color_float_component_to_uint32(data[2]);
    std::uint32_t ua = detail::color_float_component_to_uint32(data[3]);
    m_rgba = (ua << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
        | (ur << detail::color_shift_red);
  }
    return;
  }
}

NANO_NODC_INLINE_CXPR std::uint32_t& color::rgba() NANO_NOEXCEPT { return m_rgba; }

NANO_NODC_INLINE_CXPR std::uint32_t color::rgba() const NANO_NOEXCEPT { return m_rgba; }

NANO_NODC_INLINE_CXPR std::uint32_t color::argb() const NANO_NOEXCEPT {
  using u32 = std::uint32_t;
  return (u32(blue()) << detail::color_shift_alpha) | (u32(green()) << detail::color_shift_blue)
      | (u32(red()) << detail::color_shift_green) | (u32(alpha()) << detail::color_shift_red);
}

NANO_NODC_INLINE_CXPR std::uint32_t color::abgr() const NANO_NOEXCEPT {
  using u32 = std::uint32_t;
  return (u32(red()) << detail::color_shift_alpha) | (u32(green()) << detail::color_shift_blue)
      | (u32(blue()) << detail::color_shift_green) | (u32(alpha()) << detail::color_shift_red);
}

NANO_NODC_INLINE_CXPR color::float_rgba<float> color::f_rgba() const NANO_NOEXCEPT {
  return { f_red(), f_green(), f_blue(), f_alpha() };
}

template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_NODC_INLINE_CXPR color::float_rgba<T> color::f_rgba() const NANO_NOEXCEPT {
  return { static_cast<T>(f_red()), static_cast<T>(f_green()), static_cast<T>(f_blue()), static_cast<T>(f_alpha()) };
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::red() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_red());
  }
  else {
    return static_cast<T>(red());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::green() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_green());
  }
  else {
    return static_cast<T>(green());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::blue() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_blue());
  }
  else {
    return static_cast<T>(blue());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::alpha() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_alpha());
  }
  else {
    return static_cast<T>(alpha());
  }
}

NANO_INLINE_CXPR color& color::red(std::uint8_t r) NANO_NOEXCEPT {
  //  *(((std::uint8_t*)(&m_rgba)) + 3) = r;

  m_rgba = (m_rgba & ~detail::color_bits_red) | (std::uint32_t(r) << detail::color_shift_red);
  return *this;
}

NANO_INLINE_CXPR color& color::green(std::uint8_t g) NANO_NOEXCEPT {
  //  *(((std::uint8_t*)(&m_rgba)) + 2) = g;

  m_rgba = (m_rgba & ~detail::color_bits_green) | (std::uint32_t(g) << detail::color_shift_green);
  return *this;
}

NANO_INLINE_CXPR color& color::blue(std::uint8_t b) NANO_NOEXCEPT {
  //  *(((std::uint8_t*)(&m_rgba)) + 1) = b;

  m_rgba = (m_rgba & ~detail::color_bits_blue) | (std::uint32_t(b) << detail::color_shift_blue);
  return *this;
}

NANO_INLINE_CXPR color& color::alpha(std::uint8_t a) NANO_NOEXCEPT {
  //  *((std::uint8_t*)(&m_rgba)) = a;

  m_rgba = (m_rgba & ~detail::color_bits_alpha) | (std::uint32_t(a) << detail::color_shift_alpha);
  return *this;
}

NANO_NODC_INLINE_CXPR std::uint8_t color::red() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_red) >> detail::color_shift_red);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::green() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_green) >> detail::color_shift_green);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::blue() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_blue) >> detail::color_shift_blue);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::alpha() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_alpha) >> detail::color_shift_alpha);
}

NANO_NODC_INLINE_CXPR float color::f_red() const NANO_NOEXCEPT { return red() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_green() const NANO_NOEXCEPT { return green() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_blue() const NANO_NOEXCEPT { return blue() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_alpha() const NANO_NOEXCEPT { return alpha() / 255.0f; }

NANO_NODC_INLINE_CXPR std::uint8_t color::operator[](std::size_t index) const NANO_NOEXCEPT {

  switch (index) {
  case 0:
    return red();
  case 1:
    return green();
  case 2:
    return blue();
  case 3:
    return alpha();
  }
  return red();
}

NANO_NODC_INLINE_CXPR bool color::is_opaque() const NANO_NOEXCEPT { return alpha() == 255; }
NANO_NODC_INLINE_CXPR bool color::is_transparent() const NANO_NOEXCEPT { return alpha() != 255; }

NANO_NODC_INLINE_CXPR color color::darker(float amount) const NANO_NOEXCEPT {
  amount = 1.0f - std::clamp<float>(amount, 0.0f, 1.0f);
  return color(std::uint8_t(red() * amount), std::uint8_t(green() * amount), std::uint8_t(blue() * amount), alpha());
}

NANO_NODC_INLINE_CXPR color color::brighter(float amount) const NANO_NOEXCEPT {
  const float ratio = 1.0f / (1.0f + cxpr::abs(amount));
  const float mu = 255 * (1.0f - ratio);

  return color(static_cast<std::uint8_t>(mu + ratio * red()), // r
      static_cast<std::uint8_t>(mu + ratio * green()), // g
      static_cast<std::uint8_t>(mu + ratio * blue()), // b
      alpha() // a
  );
}

NANO_NODC_INLINE_CXPR color color::with_red(std::uint8_t red) const NANO_NOEXCEPT {
  return color(red, green(), blue(), alpha());
}

NANO_NODC_INLINE_CXPR color color::with_green(std::uint8_t green) const NANO_NOEXCEPT {
  return color(red(), green, blue(), alpha());
}

NANO_NODC_INLINE_CXPR color color::with_blue(std::uint8_t blue) const NANO_NOEXCEPT {
  return color(red(), green(), blue, alpha());
}

NANO_NODC_INLINE_CXPR color color::with_alpha(std::uint8_t alpha) const NANO_NOEXCEPT {
  return color(red(), green(), blue(), alpha);
}

/// mu should be between [0, 1]
NANO_NODC_INLINE_CXPR color color::operator*(float mu) const NANO_NOEXCEPT {
  return color(static_cast<std::uint8_t>(red() * mu), static_cast<std::uint8_t>(green() * mu),
      static_cast<std::uint8_t>(blue() * mu), static_cast<std::uint8_t>(alpha() * mu));
}

NANO_NODC_INLINE_CXPR bool color::operator==(const color& c) const NANO_NOEXCEPT { return m_rgba == c.m_rgba; }

NANO_NODC_INLINE_CXPR bool color::operator!=(const color& c) const NANO_NOEXCEPT { return !operator==(c); }

template <class CharT, class TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, nano::color c) {
  std::ios_base::fmtflags flags(s.flags());
  s << CharT('#') << std::uppercase << std::hex << std::setfill(CharT('0')) << std::setw(8) << c.rgba();
  s.flags(flags);
  return s;
}

namespace colors {
  NANO_INLINE_CXPR nano::color transparent = 0x00000000;
  NANO_INLINE_CXPR nano::color black = 0x000000FF;
  NANO_INLINE_CXPR nano::color white = 0xFFFFFFFF;
  NANO_INLINE_CXPR nano::color red = 0xFF0000FF;
  NANO_INLINE_CXPR nano::color green = 0x00FF00FF;
  NANO_INLINE_CXPR nano::color blue = 0x0000FFFF;
} // namespace colors.
} // namespace nano.

NANO_CLANG_DIAGNOSTIC_POP()
