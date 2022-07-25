#include "nano/graphics.h"
#include <nano/objc.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <CoreText/CoreText.h>
#include <CoreServices/CoreServices.h>

#include <iostream>
//#include <thread>

#include <fstream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/proc_info.h>
#include <sys/param.h>
#include <libproc.h>

#define NANO_UTTypePNG CFSTR("public.png")
#define NANO_UTTypeJPEG CFSTR("public.jpeg")

namespace nano {
template <std::size_t N>
inline CFDictionaryRef create_cf_dictionary(CFStringRef const (&keys)[N], CFTypeRef const (&values)[N]) {
  return CFDictionaryCreate(kCFAllocatorDefault, reinterpret_cast<const void**>(&keys),
      reinterpret_cast<const void**>(&values), N, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

inline cf_unique_ptr<CFStringRef> create_cf_string_ptr(const char* str) {
  return cf_unique_ptr<CFStringRef>(CFStringCreateWithCString(kCFAllocatorDefault, str, kCFStringEncodingUTF8));
}

inline cf_unique_ptr<CFStringRef> create_cf_string_ptr(std::string_view str) {
  return cf_unique_ptr<CFStringRef>(CFStringCreateWithBytes(kCFAllocatorDefault,
      reinterpret_cast<const UInt8*>(str.data()), static_cast<CFIndex>(str.size()), kCFStringEncodingUTF8, false));
}

//
// namespace detail {
//
//  template <typename _CFType>
//  struct cf_object_deleter {
//    inline void operator()(std::add_pointer_t<std::remove_pointer_t<_CFType>> obj) const noexcept {
//        CFRelease(obj);
//    }
//  };
//
//
// template <class _CFType>
// using cf_unique_ptr_type = std::unique_ptr<std::remove_pointer_t<_CFType>, detail::cf_object_deleter<_CFType>>;
//
//
///// A unique_ptr for CFTypes.
///// The deleter will call CFRelease().
///// @remarks The _CFType can either be a CFTypeRef of the underlying type (e.g.
///// CFStringRef or __CFString).
/////          In other words, as opposed to std::unique_ptr<>, _CFType can also
/////          be a pointer.
// template <class _CFType>
// struct cf_unique_ptr : cf_unique_ptr_type<_CFType> {
//   using base = cf_unique_ptr_type<_CFType>;
//
//   inline cf_unique_ptr(_CFType ptr) : base(ptr) {}
//   cf_unique_ptr(cf_unique_ptr&&) = delete;
//   cf_unique_ptr& operator=(cf_unique_ptr&&) = delete;
//
//   inline operator _CFType() const {
//     return base::get();
//   }
//
//   template <class T, std::enable_if_t<std::is_convertible_v<_CFType, T>, std::nullptr_t> = nullptr>
//   inline T as() const {
//       return static_cast<T>(base::get());
//   }
// };
// } // namespace detail.

template <class _CFType>
using cf_ptr = nano::cf_unique_ptr<_CFType>;

double display::get_refresh_rate() {
  nano::cf_ptr<CGDisplayModeRef> mode = CGDisplayCopyDisplayMode(CGMainDisplayID());
  return mode ? CGDisplayModeGetRefreshRate(mode) : 0;
}

// DPI = [THP /TW + TVP / TL ] /2
//
// Where DPI is the average dots per inch
// THP is the total horizontal pixels
// TW is the total width (in)
// TVP is the total vertical pixels
// TL is the total length (in)
// nano::size<double> dsize = CGDisplayScreenSize(dis_id);
// double thp = actual_width;
// double tw = dsize.width * 0.0393701;
// std::cout << actual_width << " DPI " << 25.4 * (actual_width / dsize.width) << " " << thp / tw << std::endl;

double display::get_scale_factor() {
  CGDirectDisplayID main_id = CGMainDisplayID();
  nano::cf_ptr<CGDisplayModeRef> mode = CGDisplayCopyDisplayMode(main_id);

  if (!mode) {
    return 1;
  }

  std::size_t shown_width = CGDisplayPixelsWide(main_id);
  std::size_t actual_width = CGDisplayModeGetPixelWidth(mode);
  return actual_width / static_cast<double>(shown_width);
}

struct image::pimpl {
  CGImageRef img = nullptr;
};

image::image() { m_pimpl = new pimpl; }

nano::size<double> image::get_dpi(const std::string& filepath) {
  nano::cf_ptr<CGDataProviderRef> dataProvider = CGDataProviderCreateWithFilename(filepath.c_str());

  if (!dataProvider) {
    return { 0.0, 0.0 };
  }

  nano::cf_ptr<CGImageSourceRef> imageRef = CGImageSourceCreateWithDataProvider(dataProvider, nullptr);

  if (!imageRef) {
    return { 0.0, 0.0 };
  }

  nano::cf_ptr<CFDictionaryRef> imagePropertiesDict = CGImageSourceCopyPropertiesAtIndex(imageRef, 0, nullptr);

  if (!imagePropertiesDict) {
    return { 0.0, 0.0 };
  }

  CFNumberRef dpiWidth = nullptr;
  if (!CFDictionaryGetValueIfPresent(
          imagePropertiesDict, kCGImagePropertyDPIWidth, reinterpret_cast<const void**>(&dpiWidth))) {
    return { 0.0, 0.0 };
  }

  CFNumberRef dpiHeight = nullptr;
  if (!CFDictionaryGetValueIfPresent(
          imagePropertiesDict, kCGImagePropertyDPIHeight, reinterpret_cast<const void**>(&dpiHeight))) {
    return { 0.0, 0.0 };
  }

  CFNumberType ftype = kCFNumberFloat32Type;
  if (CFNumberGetType(dpiWidth) != ftype || CFNumberGetType(dpiHeight) != ftype) {
    return { 0.0, 0.0 };
  }

  float w_dpi = 0;
  float h_dpi = 0;
  CFNumberGetValue(dpiWidth, ftype, (void*)&w_dpi);
  CFNumberGetValue(dpiHeight, ftype, (void*)&h_dpi);

  return { w_dpi, h_dpi };
}

inline void nameee() {
  struct kinfo_proc* process = NULL;
  size_t proc_buf_size;
  int st, proc_count;
  constexpr int NAME_LEN = 4;
  int name[NAME_LEN] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
  pid_t pid;

  st = sysctl(name, NAME_LEN, NULL, &proc_buf_size, NULL, 0);
  process = (kinfo_proc*)malloc(proc_buf_size);
  st = sysctl(name, NAME_LEN, process, &proc_buf_size, NULL, 0);

  proc_count = proc_buf_size / sizeof(struct kinfo_proc);
  while (st < proc_count) {
    pid = process[st].kp_proc.p_pid;
    printf("pid: %d  name: %s\n", pid, process[st].kp_proc.p_comm);
    st++;
  }
}

inline void nameee(pid_t pid) {
  char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
  proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));

  char nameBuffer[256];

  int position = strlen(pathBuffer);
  while (position >= 0 && pathBuffer[position] != '/') {
    position--;
  }

  strcpy(nameBuffer, pathBuffer + position + 1);

  printf("path: %s\n\nname:%s\n\n", pathBuffer, nameBuffer);
}

image::image(const std::string& filepath, type img_type) {
  m_pimpl = new pimpl;

  nano::cf_ptr<CGDataProviderRef> dataProvider = CGDataProviderCreateWithFilename(filepath.c_str());
  assert(dataProvider != nullptr);

  if (!dataProvider) {
    return;
  }

  switch (img_type) {
  case type::png:
    m_pimpl->img = CGImageCreateWithPNGDataProvider(dataProvider, nullptr, false, kCGRenderingIntentDefault);
    break;

  case type::jpeg:
    m_pimpl->img = CGImageCreateWithJPEGDataProvider(dataProvider, nullptr, false, kCGRenderingIntentDefault);
    break;
  }
}

image::image(const nano::size<std::size_t>& size, std::size_t bitsPerComponent, std::size_t bitsPerPixel,
    std::size_t bytesPerRow, format fmt, const std::uint8_t* buffer) {
  m_pimpl = new pimpl;
  CGBitmapInfo bmp_info = 0;

  switch (fmt) {
  case format::alpha:
    bmp_info = kCGImageAlphaOnly;
    break;

  case format::argb:
    bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Little;
    break;

  case format::bgra:
    bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Big;
    break;

  case format::rgb:
    bmp_info = kCGImageAlphaNone;
    break;

  case format::rgba:
    bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Little;
    break;

  case format::abgr:
    bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Big;
    break;

  case format::rgbx:
    bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Little;
    break;

  case format::xbgr:
    bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Big;
    break;

  case format::xrgb:
    bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Little;
    break;

  case format::bgrx:
    bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Big;
    break;

  case format::float_alpha:
    bmp_info = kCGImageAlphaOnly | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case format::float_argb:
    bmp_info = kCGImageAlphaFirst | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case format::float_rgb:
    bmp_info = kCGImageAlphaNone | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case format::float_rgba:
    bmp_info = kCGImageAlphaLast | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;
  }

  nano::cf_ptr<CGDataProviderRef> dataProvider(
      CGDataProviderCreateWithData(nullptr, buffer, bytesPerRow * size.height, nullptr));
  nano::cf_ptr<CGColorSpaceRef> colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));

  m_pimpl->img = CGImageCreate(size.width, size.height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpace,
      bmp_info, dataProvider, nullptr, false, kCGRenderingIntentDefault);
}

// CGImageRef __nullable CGImageCreate(size_t width, size_t height,
//     size_t bitsPerComponent, size_t bitsPerPixel, size_t bytesPerRow,
//     CGColorSpaceRef cg_nullable space, CGBitmapInfo bitmapInfo,
//     CGDataProviderRef cg_nullable provider,
//     const CGFloat * __nullable decode, bool shouldInterpolate,
//     CGColorRenderingIntent intent)
//     CG_AVAILABLE_STARTING(10.0, 2.0);

image::image(const image& img) {

  m_pimpl = new pimpl;
  m_pimpl->img = img.m_pimpl->img;

  if (m_pimpl->img) {
    CGImageRetain(m_pimpl->img);
  }
}

image::image(image&& img) {
  m_pimpl = new pimpl;
  m_pimpl->img = img.m_pimpl->img;
  img.m_pimpl->img = nullptr;
}

image::image(image::handle nativeImg) {

  m_pimpl = new pimpl;
  m_pimpl->img = reinterpret_cast<CGImageRef>(nativeImg);

  if (m_pimpl->img) {
    CGImageRetain(m_pimpl->img);
  }
}

image::~image() {
  if (m_pimpl->img) {
    CGImageRelease(m_pimpl->img);
  }

  delete m_pimpl;
}

image& image::operator=(const image& img) {

  if (m_pimpl->img == img.m_pimpl->img) {
    return *this;
  }

  if (m_pimpl->img) {
    CGImageRelease(m_pimpl->img);
  }

  m_pimpl->img = img.m_pimpl->img;

  if (m_pimpl->img) {
    CGImageRetain(m_pimpl->img);
  }

  return *this;
}

image& image::operator=(image&& img) {
  if (m_pimpl->img) {
    CGImageRelease(m_pimpl->img);
  }

  m_pimpl->img = img.m_pimpl->img;
  img.m_pimpl->img = nullptr;
  return *this;
}

image::handle image::get_native_image() const { return reinterpret_cast<handle>(m_pimpl->img); }

bool image::is_valid() const { return m_pimpl->img != nullptr; }

nano::size<std::size_t> image::get_size() const {
  if (!is_valid()) {
    return { 0, 0 };
  }

  return nano::size<std::size_t>(CGImageGetWidth(m_pimpl->img), CGImageGetHeight(m_pimpl->img));
}

std::size_t image::width() const { return is_valid() ? CGImageGetWidth(m_pimpl->img) : 0; }

std::size_t image::height() const { return is_valid() ? CGImageGetHeight(m_pimpl->img) : 0; }

// nano::size<int> image::get_scaled_size() const {
//   if (!is_valid()) {
//     return nano::size<int>::zero();
//   }
//
//   int w = static_cast<int>(CGImageGetWidth(m_pimpl->img));
//   int h = static_cast<int>(CGImageGetHeight(m_pimpl->img));
//   double ratio = 1.0 / m_scale_factor;
//
//   return nano::size<int>(static_cast<int>(w * ratio), static_cast<int>(h * ratio));
// }

std::size_t image::get_bits_per_component() const { return CGImageGetBitsPerComponent(m_pimpl->img); }

std::size_t image::get_bits_per_pixel() const { return CGImageGetBitsPerPixel(m_pimpl->img); }

std::size_t image::get_bytes_per_row() const { return CGImageGetBytesPerRow(m_pimpl->img); }

const std::uint8_t* image::data() const {
  if (!is_valid()) {
    return nullptr;
  }

  CGDataProviderRef dataProvider = CGImageGetDataProvider(m_pimpl->img);
  return CFDataGetBytePtr(nano::cf_ptr<CFDataRef>(CGDataProviderCopyData(dataProvider)));
}

void image::copy_data(std::vector<std::uint8_t>& buffer) const {
  if (!is_valid()) {
    return;
  }

  CGDataProviderRef dataProvider = CGImageGetDataProvider(m_pimpl->img);
  nano::cf_ptr<CFDataRef> cfData = CGDataProviderCopyData(dataProvider);

  std::size_t data_size = CFDataGetLength(cfData);
  buffer.resize(data_size);
  CFDataGetBytes(cfData, CFRangeMake(0, data_size), buffer.data());
}

std::vector<std::uint8_t> image::get_data() const {
  std::vector<std::uint8_t> buffer;
  copy_data(buffer);
  return buffer;
}

// std::uint8_t* image::data() {
//   CGDataProviderRef dataProvider = CGImageGetDataProvider(m_pimpl->img);
//   CFDataRef d = CGDataProviderCopyData(dataProvider);
//   CFMutableDataRef dd = CFDataCreateMutableCopy(kCFAllocatorDefault, CFDataGetLength(d), d);
//   std::uint8_t* bytes = CFDataGetMutableBytePtr(dd);
//   return bytes;
// }

image image::make_copy() {
  return is_valid() ? image(nano::cf_ptr<CGImageRef>(CGImageCreateCopy(m_pimpl->img)).as<handle>()) : image();
}

image image::get_sub_image(const nano::rect<std::size_t>& r) const {
  return is_valid()
      ? image(nano::cf_ptr<CGImageRef>(CGImageCreateWithImageInRect(m_pimpl->img, r.convert<CGRect>())).as<handle>())
      : image();
}

namespace {
  static inline nano::cf_ptr<CGContextRef> create_bitmap_context(const nano::size<std::size_t>& size) {
    return CGBitmapContextCreate(nullptr, size.width, size.height, 8, size.width * 4,
        nano::cf_ptr<CGColorSpaceRef>(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB)),
        kCGImageAlphaPremultipliedLast);
  }

  static inline image create_colored_image_impl(const nano::image& img, const nano::color& color) {
    nano::rect<int> rect = img.get_rect();

    nano::cf_ptr<CGContextRef> ctx = create_bitmap_context(rect.size);
    CGContextClipToMask(ctx, rect.convert<CGRect>(), reinterpret_cast<CGImageRef>(img.get_native_image()));

    CGContextSetRGBFillColor(
        ctx, color.red<CGFloat>(), color.green<CGFloat>(), color.blue<CGFloat>(), color.alpha<CGFloat>());
    CGContextFillRect(ctx, rect.convert<CGRect>());

    return image(reinterpret_cast<image::handle>(nano::cf_ptr<CGImageRef>(CGBitmapContextCreateImage(ctx)).get()));
  }

} // namespace.

image image::create_colored_image(const nano::color& color) const { return create_colored_image_impl(*this, color); }

namespace {
  static inline CFStringRef get_image_type_string(image::type img_type) {
    switch (img_type) {
    case image::type::png:
      return NANO_UTTypePNG;
      break;

    case image::type::jpeg:
      return NANO_UTTypeJPEG;
      break;
    }

    return NANO_UTTypePNG;
  }
} // namespace

bool image::save(const std::filesystem::path& filepath, type img_type) {
  if (!is_valid()) {
    return false;
  }

  nano::cf_ptr<CFURLRef> url = CFURLCreateFromFileSystemRepresentation(
      kCFAllocatorDefault, (const UInt8*)filepath.c_str(), std::string_view(filepath.c_str()).size(), false);

  nano::cf_ptr<CGImageDestinationRef> dest
      = CGImageDestinationCreateWithURL(url, get_image_type_string(img_type), 1, nullptr);
  CGImageDestinationAddImage(dest, m_pimpl->img, nullptr);
  return CGImageDestinationFinalize(dest);
}

// bool image::save(const std::filesystem::path& filepath, type img_type) {
//   if (!is_valid()) {
//     return false;
//   }
////  filepath.string();
////  std::string_view sv(filepath);
//
////    CFStringRef st = CFStringCreateWithCString(kCFAllocatorDefault, filepath.c_str(), kCFStringEncodingUTF8);
////    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, st, kCFURLPOSIXPathStyle, false);
////  CFRelease(st);
//
////  CFStringRef st = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)sv.data(), sv.size(),
/// kCFStringEncodingUTF8, false); /  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, st,
/// kCFURLPOSIXPathStyle, false);
//
//  nano::cf_ptr<CFURLRef> url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const
//  UInt8*)filepath.c_str(), std::string_view(filepath.c_str()).size(), false);
//
//
//
//  CFStringRef typeString = nullptr;
//
//
//
//  switch (img_type) {
//  case type::png:
//      typeString = NANO_UTTypePNG;
//
//    break;
//
//  case type::jpeg:
//      typeString = NANO_UTTypeJPEG;
//    break;
//  }
//
//
////  char bbb[1024] = {0};
////  CFStringGetCString(kUTTypePNG, bbb, 1024, kCFStringEncodingUTF8);
////
////  std::cout << "DALKDJAKDJAKLDJLKJDA --- " << bbb << std::endl;
////
////  CFStringGetCString(kUTTypeJPEG, bbb, 1024, kCFStringEncodingUTF8);
////
////  std::cout << "DALKDJAKDJAKLDJLKJDA --- " << bbb << std::endl;
//
////  switch (img_type) {
////  case type::png:
////      typeString = kUTTypePNG;
////
////    break;
////
////  case type::jpeg:
////      typeString = kUTTypeJPEG;
////    break;
////  }
//
////  nano::cf_ptr<CGImageDestinationRef> dest = CGImageDestinationCreateWithURL(url, typeString, 1, properties);
//
//  float dpi = 150;
//  CFNumberRef dpi_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, reinterpret_cast<const void
//  *>(&dpi));
//
////  std::int64_t num2_value = 1000;
////  CFNumberRef num2 = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, reinterpret_cast<const void
///*>(&num2_value));
//
//
//
//
//  float ppm = 2;
//  CFNumberRef ppm_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, reinterpret_cast<const void
//  *>(&ppm)); nano::cf_unique_ptr<CFDictionaryRef> png_props(nano::create_cf_dictionary(
//      { kCGImagePropertyPNGTitle, kCGImagePropertyPNGAuthor }, { CFSTR("JOHN"), CFSTR("AlexA") }));
//
//
//
//
//  // , kCGImagePropertyDPIHeight, kCGImagePropertyDPIWidth, kCGImagePropertyPixelWidth, kCGImagePropertyPixelHeight
//  nano::cf_unique_ptr<CFDictionaryRef> properties(nano::create_cf_dictionary(
//      { kCGImagePropertyPNGDictionary , kCGImagePropertyDPIWidth, kCGImagePropertyDPIHeight}, { png_props.get(),
//      dpi_ref, dpi_ref}));
//
//
//
//
////  CGImageDestinationSetProperties(dest, properties);
//  nano::cf_ptr<CGImageDestinationRef> dest = CGImageDestinationCreateWithURL(url, typeString, 1, nullptr);
//
//
//  CGImageDestinationAddImage(dest, m_pimpl->img, properties);
//
//
////  CFNumberRef dpiHeight = nullptr;
////  if(!CFDictionaryGetValueIfPresent(imagePropertiesDict, kCGImagePropertyDPIHeight, reinterpret_cast<const void
///**>(&dpiHeight))) { /    return {0.0, 0.0}; /  }
//
//  bool result = CGImageDestinationFinalize(dest);
//
////  CGImageAlphaInfo ainfo = CGImageGetAlphaInfo(m_pimpl->img);
////  std::cout << "----- " << (std::uint32_t)ainfo << std::endl;
////
////  CGBitmapInfo bmp_info = CGImageGetBitmapInfo(m_pimpl->img);
////  std::cout << get_bits_per_pixel() << "----- " << (std::uint32_t)bmp_info << std::endl;
////
////  std::uint32_t fff = bmp_info & kCGBitmapAlphaInfoMask;
////  std::cout << "----- " << (std::uint32_t)fff << std::endl;
//
//  return result;
//}

//
// MARK: font
//

struct font::pimpl {
  pimpl(double ft)
      : font_size(ft) {}
  ~pimpl() = default;

  CTFontRef font;
  double font_size;
};

font::font() {
  m_pimpl = new pimpl(0);
  m_pimpl->font = nullptr;
}

font::font(const char* fontName, double fontSize) {

  CFStringRef cfName = CFStringCreateWithCString(kCFAllocatorDefault, fontName, kCFStringEncodingUTF8);

  m_pimpl = new pimpl(fontSize);
  m_pimpl->font = CTFontCreateWithName(cfName, static_cast<CGFloat>(fontSize), nullptr);

  CFRelease(cfName);

  //  m_native = (native*)CTFontCreateWithName(cfName, static_cast<CGFloat>(m_font_size), nullptr);
}

font::font(const char* filepath, double fontSize, filepath_tag) {

  m_pimpl = new pimpl(fontSize);
  m_pimpl->font = nullptr;

  CFStringRef cgFilepath = CFStringCreateWithCString(kCFAllocatorDefault, filepath, kCFStringEncodingUTF8);

  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cgFilepath, kCFURLPOSIXPathStyle, false);
  CFRelease(cgFilepath);

  // This function returns a retained reference to a CFArray of
  // CTFontDescriptorRef objects, or NULL on error. The caller is responsible
  // for releasing the array.
  CFArrayRef cfArray = CTFontManagerCreateFontDescriptorsFromURL(url);
  CFRelease(url);

  if (cfArray == nullptr) {
    m_pimpl->font_size = 0;
    return;
  }

  CFIndex arraySize = CFArrayGetCount(cfArray);
  if (arraySize == 0) {
    CFRelease(cfArray);
    m_pimpl->font_size = 0;
    return;
  }

  CTFontDescriptorRef desc = reinterpret_cast<CTFontDescriptorRef>(CFArrayGetValueAtIndex(cfArray, 0));

  m_pimpl->font = CTFontCreateWithFontDescriptor(desc, static_cast<CGFloat>(fontSize), nullptr);
  CFRelease(cfArray);
}

font::font(const std::uint8_t* data, std::size_t data_size, double font_size) {

  m_pimpl = new pimpl(font_size);
  m_pimpl->font = nullptr;

  if (!data || !data_size) {
    m_pimpl->font_size = 0;
    return;
  }

  CFDataRef cfData = CFDataCreate(nullptr, data, static_cast<CFIndex>(data_size));
  if (!cfData) {
    m_pimpl->font_size = 0;
    return;
  }

  CTFontDescriptorRef desc = CTFontManagerCreateFontDescriptorFromData(cfData);
  CFRelease(cfData);

  if (!desc) {
    m_pimpl->font_size = 0;
    return;
  }

  m_pimpl->font = CTFontCreateWithFontDescriptor(desc, static_cast<CGFloat>(font_size), nullptr);

  CFRelease(desc);
}

font::font(const font& f) {

  m_pimpl = new pimpl(f.get_font_size());
  m_pimpl->font = f.m_pimpl->font;

  if (m_pimpl->font) {
    CFRetain(m_pimpl->font);
  }
}

font::font(font&& f) {
  m_pimpl = new pimpl(f.get_font_size());
  m_pimpl->font = f.m_pimpl->font;
  f.m_pimpl->font = nullptr;
}

font::~font() {
  if (m_pimpl->font) {
    CFRelease(m_pimpl->font);
    m_pimpl->font = nullptr;
  }

  delete m_pimpl;
}

font& font::operator=(const font& f) {
  if (m_pimpl->font == f.m_pimpl->font) {
    return *this;
  }

  if (m_pimpl->font) {
    CFRelease(m_pimpl->font);
  }

  m_pimpl->font = f.m_pimpl->font;

  if (m_pimpl->font) {
    CFRetain(m_pimpl->font);
  }

  return *this;
}

font& font::operator=(font&& f) {
  if (m_pimpl->font) {
    CFRelease(m_pimpl->font);
  }

  m_pimpl->font = f.m_pimpl->font;
  f.m_pimpl->font = nullptr;
  return *this;
}

bool font::is_valid() const noexcept { return m_pimpl->font != nullptr; }

double font::get_font_size() const noexcept { return m_pimpl->font_size; }

double font::get_height() const noexcept { return static_cast<double>(CTFontGetCapHeight(m_pimpl->font)); }

struct Advances {
  Advances(CTRunRef run, CFIndex numGlyphs)
      : advances(CTRunGetAdvancesPtr(run)) {

    if (advances == nullptr) {
      local.resize(static_cast<size_t>(numGlyphs));
      CTRunGetAdvances(run, CFRangeMake(0, 0), local.data());
      advances = local.data();
    }
  }

  const CGSize* advances;
  std::vector<CGSize> local;
};

struct Glyphs {
  Glyphs(CTRunRef run, CFIndex numGlyphs)
      : glyphs(CTRunGetGlyphsPtr(run)) {
    if (glyphs == nullptr) {
      local.resize(static_cast<size_t>(numGlyphs));
      CTRunGetGlyphs(run, CFRangeMake(0, 0), local.data());
      glyphs = local.data();
    }
  }

  const CGGlyph* glyphs;
  std::vector<CGGlyph> local;
};

float font::get_string_width(std::string_view text) const {
  if (text.empty()) {
    return 0;
  }

  nano::cf_unique_ptr<CFDictionaryRef> attributes(nano::create_cf_dictionary(
      { kCTFontAttributeName, kCTLigatureAttributeName }, { m_pimpl->font, kCFBooleanTrue }));

  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);

  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));

  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));
  CFArrayRef run_array = CTLineGetGlyphRuns(line.get());

  float x = 0;

  for (CFIndex i = 0; i < CFArrayGetCount(run_array); i++) {
    CTRunRef run = reinterpret_cast<CTRunRef>(CFArrayGetValueAtIndex(run_array, i));
    CFIndex length = CTRunGetGlyphCount(run);

    const Advances advances(run, length);

    for (CFIndex j = 0; j < length; j++) {
      x += static_cast<float>(advances.advances[j].width);
    }
  }

  return x;
}

font::handle font::get_native_font() const noexcept { return reinterpret_cast<font::handle>(m_pimpl->font); }

//
// MARK: graphic_context
//

class graphic_context::pimpl {
public:
  ~pimpl() = default;

  CGContextRef gc;
  bool is_bitmap;

  static inline void flip(CGContextRef c, float flipHeight) {
    CGContextConcatCTM(c, CGAffineTransformMake(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, flipHeight));
  }

  template <typename Fct, typename... Args>
  inline void draw(Fct&& fct, Args&&... args) {

    if (is_bitmap) {
      size_t height = CGBitmapContextGetHeight(gc);
      flip(gc, height);
      fct(gc, std::forward<Args>(args)...);
      flip(gc, height);
    }
    else {
      fct(gc, std::forward<Args>(args)...);
    }
  }

  //  if(is_bitmap()) {

  //    return;
  //  }
};

graphic_context::graphic_context(handle nc, bool is_bitmap) {
  m_pimpl = new pimpl;
  m_pimpl->gc = reinterpret_cast<CGContextRef>(nc);
  m_pimpl->is_bitmap = is_bitmap;
}

graphic_context::~graphic_context() {
  if (m_pimpl->is_bitmap) {
    CGContextRelease(m_pimpl->gc);
  }

  delete m_pimpl;
}

void graphic_context::save_state() { CGContextSaveGState(m_pimpl->gc); }

void graphic_context::restore_state() { CGContextRestoreGState(m_pimpl->gc); }

void graphic_context::begin_transparent_layer(float alpha) {
  save_state();
  CGContextSetAlpha(m_pimpl->gc, static_cast<CGFloat>(alpha));
  CGContextBeginTransparencyLayer(m_pimpl->gc, nullptr);
}

void graphic_context::end_transparent_layer() {
  CGContextEndTransparencyLayer(m_pimpl->gc);
  restore_state();
}

void graphic_context::translate(const nano::point<float>& pos) {
  CGContextTranslateCTM(m_pimpl->gc, static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y));
}

void graphic_context::clip() { CGContextClip(m_pimpl->gc); }

void graphic_context::clip_even_odd() { CGContextEOClip(m_pimpl->gc); }

void graphic_context::reset_clip() { CGContextResetClip(m_pimpl->gc); }

void graphic_context::clip_to_rect(const nano::rect<float>& rect) {
  CGContextClipToRect(m_pimpl->gc, static_cast<CGRect>(rect));
}

// void graphic_context::clip_to_path(const nano::Path& p)
// {
//     CGContextRef g = m_pimpl->gc;
//     CGContextBeginPath(g);
//     CGContextAddPath(g, (CGPathRef)p.GetnativePath());
//     CGContextClip(g);
// }

// void graphic_context::clip_to_path_even_odd(const nano::Path& p)
// {
//     CGContextRef g = m_pimpl->gc;
//     CGContextBeginPath(g);
//     CGContextAddPath(g, (CGPathRef)p.get_native_path());
//     CGContextEOClip(g);
// }

void graphic_context::clip_to_mask(const nano::image& img, const nano::rect<float>& rect) {
  CGContextRef g = m_pimpl->gc;
  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  pimpl::flip(g, rect.height);
  CGContextClipToMask(
      g, static_cast<CGRect>(rect.with_position({ 0, 0 })), reinterpret_cast<CGImageRef>(img.get_native_image()));
  pimpl::flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::add_rect(const nano::rect<float>& rect) {
  CGContextAddRect(m_pimpl->gc, static_cast<CGRect>(rect));
}

// void graphic_context::add_path(const nano::Path& p)
// {
//     CGContextAddPath(m_pimpl->gc, (CGPathRef)p.GetNativePath());
// }

void graphic_context::begin_path() { CGContextBeginPath(m_pimpl->gc); }
void graphic_context::close_path() { CGContextClosePath(m_pimpl->gc); }

nano::rect<float> graphic_context::get_clipping_rect() const { return CGContextGetClipBoundingBox(m_pimpl->gc); }

void graphic_context::set_line_width(float width) { CGContextSetLineWidth(m_pimpl->gc, static_cast<CGFloat>(width)); }

void graphic_context::set_line_join(line_join lj) {
  switch (lj) {
  case line_join::miter:
    CGContextSetLineJoin(m_pimpl->gc, kCGLineJoinMiter);
    break;

  case line_join::round:
    CGContextSetLineJoin(m_pimpl->gc, kCGLineJoinRound);
    break;

  case line_join::bevel:
    CGContextSetLineJoin(m_pimpl->gc, kCGLineJoinBevel);
    break;
  }
}

void graphic_context::set_line_cap(line_cap lc) {
  switch (lc) {
  case line_cap::butt:
    CGContextSetLineCap(m_pimpl->gc, kCGLineCapButt);
    break;

  case line_cap::round:
    CGContextSetLineCap(m_pimpl->gc, kCGLineCapRound);
    break;

  case line_cap::square:
    CGContextSetLineCap(m_pimpl->gc, kCGLineCapSquare);
    break;
  }
}

void graphic_context::set_line_style(float width, line_join lj, line_cap lc) {
  set_line_width(width);
  set_line_join(lj);
  set_line_cap(lc);
}

void graphic_context::set_fill_color(const nano::color& c) {
  CGContextRef cg = m_pimpl->gc;
  CGColorRef color
      = CGColorCreateGenericRGB(c.red<CGFloat>(), c.green<CGFloat>(), c.blue<CGFloat>(), c.alpha<CGFloat>());
  CGContextSetFillColorWithColor(cg, color);
  CGColorRelease(color);
}

void graphic_context::set_stroke_color(const nano::color& c) {
  CGContextRef cg = m_pimpl->gc;
  CGColorRef color
      = CGColorCreateGenericRGB(c.red<CGFloat>(), c.green<CGFloat>(), c.blue<CGFloat>(), c.alpha<CGFloat>());
  CGContextSetStrokeColorWithColor(cg, color);
  CGColorRelease(color);
}

void graphic_context::fill_rect(const nano::rect<float>& r) {
  m_pimpl->draw([](CGContextRef g, const nano::rect<float>& rect) { CGContextFillRect(g, rect.convert<CGRect>()); }, r);
}

void graphic_context::stroke_rect(const nano::rect<float>& r) {
  m_pimpl->draw(
      [](CGContextRef g, const nano::rect<float>& rect) { CGContextStrokeRect(g, rect.convert<CGRect>()); }, r);
}

void graphic_context::stroke_rect(const nano::rect<float>& r, float lineWidth) {
  CGContextSetLineWidth(m_pimpl->gc, static_cast<CGFloat>(lineWidth));

  m_pimpl->draw(
      [](CGContextRef g, const nano::rect<float>& rect) { CGContextStrokeRect(g, rect.convert<CGRect>()); }, r);
}

void graphic_context::stroke_line(const nano::point<float>& p0, const nano::point<float>& p1) {
  m_pimpl->draw(
      [](CGContextRef g, const nano::point<float>& p0, const nano::point<float>& p1) {
        CGContextMoveToPoint(g, static_cast<CGFloat>(p0.x), static_cast<CGFloat>(p0.y));
        CGContextAddLineToPoint(g, static_cast<CGFloat>(p1.x), static_cast<CGFloat>(p1.y));
        CGContextStrokePath(g);
      },
      p0, p1);
}

void graphic_context::fill_ellipse(const nano::rect<float>& r) {
  m_pimpl->draw(
      [](CGContextRef g, const nano::rect<float>& rect) {
        CGContextAddEllipseInRect(g, rect.convert<CGRect>());
        CGContextFillPath(g);
      },
      r);
}

void graphic_context::stroke_ellipse(const nano::rect<float>& r) {
  m_pimpl->draw(
      [](CGContextRef g, const nano::rect<float>& rect) {
        CGContextAddEllipseInRect(g, rect.convert<CGRect>());
        CGContextStrokePath(g);
      },
      r);
}

void graphic_context::fill_rounded_rect(const nano::rect<float>& r, float radius) {

  CGPathRef path = CGPathCreateWithRoundedRect(
      r.convert<CGRect>(), static_cast<CGFloat>(radius), static_cast<CGFloat>(radius), nullptr);

  m_pimpl->draw(
      [](CGContextRef g, CGPathRef path) {
        CGContextBeginPath(g);
        CGContextAddPath(g, path);
        CGContextFillPath(g);
      },
      path);
  CGPathRelease(path);

  //  CGContextRef g = m_pimpl->gc;
  //  CGPathRef path = CGPathCreateWithRoundedRect(
  //      r.convert<CGRect>(), static_cast<CGFloat>(radius), static_cast<CGFloat>(radius), nullptr);
  //  CGContextBeginPath(g);
  //  CGContextAddPath(g, path);
  //  CGContextFillPath(g);
  //  CGPathRelease(path);
}

void graphic_context::stroke_rounded_rect(const nano::rect<float>& r, float radius) {

  CGPathRef path = CGPathCreateWithRoundedRect(
      r.convert<CGRect>(), static_cast<CGFloat>(radius), static_cast<CGFloat>(radius), nullptr);

  m_pimpl->draw(
      [](CGContextRef g, CGPathRef path) {
        CGContextBeginPath(g);
        CGContextAddPath(g, path);
        CGContextStrokePath(g);
      },
      path);

  CGPathRelease(path);
}

// void graphic_context::fill_path(const nano::Path& p)
// {
//     CGContextRef g = m_pimpl->gc;
//     CGContextAddPath(g, (CGPathRef)p.GetNativePath());
//     CGContextFillPath(g);
// }

// void graphic_context::fill_path(const nano::Path& p, const nano::rect<float>& rect)
// {
//     SaveState();
//     Translate(rect.position);

//     CGContextRef g = m_pimpl->gc;

//     CGContextScaleCTM(g, rect.width, rect.height);
//     CGContextAddPath(g, (CGPathRef)p.GetNativePath());
//     CGContextFillPath(g);

//     RestoreState();
// }

// void graphic_context::fill_path_with_shadow(
//     const nano::Path& p, float blur, const nano::color& shadow_color, const nano::size<float>& offset)
// {

//     SaveState();
//     CGContextRef g = m_pimpl->gc;
//     CGColorRef scolor = CGColorCreateGenericRGB(
//         shadow_color.fRed(), shadow_color.fGreen(), shadow_color.fBlue(), shadow_color.fAlpha());

//     CGContextSetShadowWithColor(g, (CGSize)offset, blur, scolor);
//     CGColorRelease(scolor);
//     FillPath(p);

//     RestoreState();
// }

// void graphic_context::stroke_path(const nano::Path& p)
// {
//     CGContextRef g = m_pimpl->gc;
//     CGContextAddPath(g, (CGPathRef)p.get_native_path());
//     CGContextStrokePath(g);
// }

void graphic_context::draw_image(const nano::image& img, const nano::point<float>& pos) {
  CGContextRef g = m_pimpl->gc;
  nano::rect<float> rect = { pos, img.get_size() };

  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  pimpl::flip(g, rect.height);

  CGContextDrawImage(
      g, static_cast<CGRect>(rect.with_position({ 0.0f, 0.0f })), reinterpret_cast<CGImageRef>(img.get_native_image()));

  pimpl::flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::draw_image(const nano::image& img, const nano::rect<float>& rect) {

  m_pimpl->draw(
      [](CGContextRef g, const nano::image& img, const nano::rect<float>& rect) {
        CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
        pimpl::flip(g, rect.height);
        CGContextDrawImage(g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(),
            reinterpret_cast<CGImageRef>(img.get_native_image()));
        pimpl::flip(g, rect.height);
        CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
      },
      img, rect);

  //  CGContextRef g = m_pimpl->gc;
  //  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  //  pimpl::flip(g, rect.height);
  //  CGContextDrawImage(
  //      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(),
  //      reinterpret_cast<CGImageRef>(img.get_native_image()));
  //  pimpl::flip(g, rect.height);
  //  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::draw_image(
    const nano::image& img, const nano::rect<float>& rect, const nano::rect<float>& clipRect) {
  CGContextRef g = m_pimpl->gc;
  save_state();
  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  clip_to_rect(clipRect);
  pimpl::flip(g, rect.height);
  CGContextDrawImage(
      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(), reinterpret_cast<CGImageRef>(img.get_native_image()));
  restore_state();
}

void graphic_context::draw_sub_image(
    const nano::image& img, const nano::rect<float>& rect, const nano::rect<float>& imgRect) {
  CGContextRef g = m_pimpl->gc;

  nano::image sImg = img.get_sub_image(imgRect);

  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  pimpl::flip(g, rect.height);
  CGContextDrawImage(
      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(), reinterpret_cast<CGImageRef>(sImg.get_native_image()));
  pimpl::flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

//
// MARK: Text.
//
NANO_INLINE_CXPR double k_default_mac_font_height = 11.0;

void graphic_context::draw_text(const nano::font& f, const std::string& text, const nano::point<float>& pos) {

  m_pimpl->draw(
      [](CGContextRef g, const nano::font& f, const std::string& text, const nano::point<float>& pos) {
        const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;

        nano::cf_unique_ptr<CFDictionaryRef> attributes(
            nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
                { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));

        nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
        nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
            CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
        nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));

        CGContextSetTextDrawingMode(g, kCGTextFill);
        CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
        CGContextSetTextPosition(g, static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y) + fontHeight);
        CTLineDraw(line.get(), g);
      },
      f, text, pos);

  //  const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;
  //  CGContextRef g = m_pimpl->gc;
  //
  //  nano::cf_unique_ptr<CFDictionaryRef> attributes(
  //      nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
  //          { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));
  //
  //  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
  //  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
  //      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
  //  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));
  //
  //  CGContextSetTextDrawingMode(g, kCGTextFill);
  //  CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
  //  CGContextSetTextPosition(g, static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y) + fontHeight);
  //  CTLineDraw(line.get(), g);
}

void graphic_context::draw_text(
    const nano::font& f, const std::string& text, const nano::rect<float>& rect, nano::text_alignment alignment) {

  m_pimpl->draw(
      [](CGContextRef g, const nano::font& f, const std::string& text, const nano::rect<float>& rect,
          nano::text_alignment alignment) {
        const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;

        nano::cf_unique_ptr<CFDictionaryRef> attributes(
            nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
                { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));

        nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
        nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
            CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
        nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));

        nano::point<float> textPos = { 0.0f, 0.0f };

        // Left.
        switch (alignment) {
        case nano::text_alignment::left: {
          textPos = nano::point<float>(rect.x, rect.y + (rect.height + static_cast<float>(fontHeight)) * 0.5f);
        } break;

        case nano::text_alignment::center: {
          float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
          textPos = rect.position
              + nano::point<float>(rect.width - textWidth, rect.height + static_cast<float>(fontHeight)) * 0.5f;
        } break;

        case nano::text_alignment::right: {
          float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
          textPos = rect.position
              + nano::point<float>(rect.width - textWidth, (rect.height + static_cast<float>(fontHeight)) * 0.5f);
        }

        break;
        }

        CGContextSetTextDrawingMode(g, kCGTextFill);
        CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
        CGContextSetTextPosition(g, static_cast<CGFloat>(textPos.x), static_cast<CGFloat>(textPos.y));
        CTLineDraw(line.get(), g);
      },
      f, text, rect, alignment);

  //
  //  const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;
  //  CGContextRef g = m_pimpl->gc;
  //
  //  nano::cf_unique_ptr<CFDictionaryRef> attributes(
  //      nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
  //          { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));
  //
  //  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
  //  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
  //      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
  //  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));
  //
  //  nano::point<float> textPos = { 0.0f, 0.0f };
  //
  //  // Left.
  //  switch (alignment) {
  //  case nano::text_alignment::left: {
  //    textPos = nano::point<float>(rect.x, rect.y + (rect.height + static_cast<float>(fontHeight)) * 0.5f);
  //  } break;
  //
  //  case nano::text_alignment::center: {
  //    float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
  //    textPos = rect.position
  //        + nano::point<float>(rect.width - textWidth, rect.height + static_cast<float>(fontHeight)) * 0.5f;
  //  } break;
  //
  //  case nano::text_alignment::right: {
  //    float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
  //    textPos = rect.position
  //        + nano::point<float>(rect.width - textWidth, (rect.height + static_cast<float>(fontHeight)) * 0.5f);
  //  }
  //
  //  break;
  //  }
  //
  //  CGContextSetTextDrawingMode(g, kCGTextFill);
  //  CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
  //  CGContextSetTextPosition(g, static_cast<CGFloat>(textPos.x), static_cast<CGFloat>(textPos.y));
  //  CTLineDraw(line.get(), g);
}

graphic_context::handle graphic_context::get_handle() const noexcept { return reinterpret_cast<handle>(m_pimpl->gc); }

bool graphic_context::is_bitmap() const noexcept { return m_pimpl->is_bitmap; }

nano::image graphic_context::create_image() {
  if (!is_bitmap()) {
    return image();
  }

  CGImageRef img_ref = CGBitmapContextCreateImage(m_pimpl->gc);
  nano::image img(img_ref);
  CGImageRelease(img_ref);
  return img;
}

graphic_context graphic_context::create_bitmap_context(const nano::size<std::size_t>& size, image::format fmt) {
  CGBitmapInfo bmp_info = 0;

  switch (fmt) {
  case image::format::alpha:
    bmp_info = kCGImageAlphaOnly;
    break;

  case image::format::argb:
    bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Little;
    break;

  case image::format::bgra:
    bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Big;
    break;

  case image::format::rgb:
    bmp_info = kCGImageAlphaNone;
    break;

  case image::format::rgba:
    bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Little;
    break;

  case image::format::abgr:
    bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Big;
    break;

  case image::format::rgbx:
    bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Little;
    break;

  case image::format::xbgr:
    bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Big;
    break;

  case image::format::xrgb:
    bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Little;
    break;

  case image::format::bgrx:
    bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Big;
    break;

  case image::format::float_alpha:
    bmp_info = kCGImageAlphaOnly | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case image::format::float_argb:
    bmp_info = kCGImageAlphaFirst | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case image::format::float_rgb:
    bmp_info = kCGImageAlphaNone | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;

  case image::format::float_rgba:
    bmp_info = kCGImageAlphaLast | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
    break;
  }

  nano::cf_ptr<CGColorSpaceRef> colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));
  return graphic_context(CGBitmapContextCreate(nullptr, size.width, size.height, 8, 4 * size.width, colorSpace,
                             kCGImageAlphaPremultipliedLast),
      true);
}
// graphic_context graphic_context::create_bitmap_context(const nano::size<std::size_t>& size, std::size_t
// bitsPerComponent,
//                std::size_t bytesPerRow, image::format fmt,   std::uint8_t* buffer)
//
//{
//   CGBitmapInfo bmp_info = 0;
//
//   switch (fmt) {
//     case image::format::alpha:
//     bmp_info = kCGImageAlphaOnly;
//     break;
//
//     case image::format::argb:
//     bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::bgra:
//     bmp_info = kCGImageAlphaFirst | kCGImageByteOrder32Big;
//     break;
//
//     case image::format::rgb:
//     bmp_info = kCGImageAlphaNone;
//     break;
//
//     case image::format::rgba:
//     bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::abgr:
//     bmp_info = kCGImageAlphaLast | kCGImageByteOrder32Big;
//     break;
//
//     case image::format::rgbx:
//     bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::xbgr:
//     bmp_info = kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Big;
//     break;
//
//     case image::format::xrgb:
//     bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::bgrx:
//     bmp_info = kCGImageAlphaNoneSkipFirst | kCGImageByteOrder32Big;
//     break;
//
//     case image::format::float_alpha:
//     bmp_info = kCGImageAlphaOnly | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::float_argb:
//     bmp_info = kCGImageAlphaFirst | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::float_rgb:
//     bmp_info = kCGImageAlphaNone | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
//     break;
//
//     case image::format::float_rgba:
//     bmp_info = kCGImageAlphaLast | kCGBitmapFloatComponents | kCGImageByteOrder32Little;
//     break;
//   }
//
////  CGContextRef __nullable CGBitmapContextCreate(void * __nullable data,
////      size_t width, size_t height, size_t bitsPerComponent, size_t bytesPerRow,
////      CGColorSpaceRef cg_nullable space, uint32_t bitmapInfo)
////
//   nano::cf_ptr<CGColorSpaceRef> colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));
//
//
//  if (buffer) {
//    return graphic_context(CGBitmapContextCreate(reinterpret_cast<void*>( buffer), size.width, size.height,
//    bitsPerComponent, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedLast), true);
//
//  }
//
////  CGBitmapContextCreateImage(ctx))
//  return graphic_context(CGBitmapContextCreate(nullptr, size.width, size.height, bitsPerComponent, bytesPerRow,
//  colorSpace, kCGImageAlphaPremultipliedLast), true);
//}

} // namespace nano.
