#include "nano/graphics.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
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

namespace nano {

namespace detail {

  template <typename _CFType>
  struct cf_object_deleter {
    inline void operator()(std::add_pointer_t<std::remove_pointer_t<_CFType>> obj) const noexcept {
        CFRelease(obj);
    }
  };


template <class _CFType>
using cf_unique_ptr_type = std::unique_ptr<std::remove_pointer_t<_CFType>, detail::cf_object_deleter<_CFType>>;


/// A unique_ptr for CFTypes.
/// The deleter will call CFRelease().
/// @remarks The _CFType can either be a CFTypeRef of the underlying type (e.g.
/// CFStringRef or __CFString).
///          In other words, as opposed to std::unique_ptr<>, _CFType can also
///          be a pointer.
template <class _CFType>
struct cf_unique_ptr : cf_unique_ptr_type<_CFType> {
  using base = cf_unique_ptr_type<_CFType>;

  inline cf_unique_ptr(_CFType ptr) : base(ptr) {} 
  cf_unique_ptr(cf_unique_ptr&&) = delete;
  cf_unique_ptr& operator=(cf_unique_ptr&&) = delete;

  inline operator _CFType() const {
    return base::get();
  }
  
  template <class T, std::enable_if_t<std::is_convertible_v<_CFType, T>, std::nullptr_t> = nullptr>
  inline T as() const {
      return static_cast<T>(base::get());
  }
};
} // namespace detail.


template <class _CFType>
using cf_ptr = detail::cf_unique_ptr<_CFType>;

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
//nano::size<double> dsize = CGDisplayScreenSize(dis_id);
//double thp = actual_width;
//double tw = dsize.width * 0.0393701;
//std::cout << actual_width << " DPI " << 25.4 * (actual_width / dsize.width) << " " << thp / tw << std::endl;


double display::get_scale_factor() {
  CGDirectDisplayID main_id = CGMainDisplayID();
  nano::cf_ptr<CGDisplayModeRef> mode =CGDisplayCopyDisplayMode(main_id);

  if(!mode) {
    return 1;
  }
  
  std::size_t shown_width = CGDisplayPixelsWide(main_id);
    std::size_t actual_width = CGDisplayModeGetPixelWidth(mode);
  return actual_width / static_cast<double>(shown_width);
}

struct image::pimpl {
  CGImageRef img = nullptr;
};
 
image::image() {
  m_pimpl = new pimpl;
}


nano::size<double> image::get_dpi(const std::string& filepath)
{
  nano::cf_ptr<CGDataProviderRef> dataProvider = CGDataProviderCreateWithFilename(filepath.c_str());

  if (!dataProvider) {
    return {0.0, 0.0};
  }
  
  nano::cf_ptr<CGImageSourceRef> imageRef = CGImageSourceCreateWithDataProvider(dataProvider, nullptr);
  
  if (!imageRef) {
    return {0.0, 0.0};
  }
  
  nano::cf_ptr<CFDictionaryRef> imagePropertiesDict = CGImageSourceCopyPropertiesAtIndex(imageRef, 0, nullptr);
  
  if (!imagePropertiesDict) {
    return {0.0, 0.0};
  }

  CFNumberRef dpiWidth = nullptr;
  if(!CFDictionaryGetValueIfPresent(imagePropertiesDict, kCGImagePropertyDPIWidth, reinterpret_cast<const void **>(&dpiWidth))) {
    return {0.0, 0.0};
  }
  
  CFNumberRef dpiHeight = nullptr;
  if(!CFDictionaryGetValueIfPresent(imagePropertiesDict, kCGImagePropertyDPIHeight, reinterpret_cast<const void **>(&dpiHeight))) {
    return {0.0, 0.0};
  }

  CFNumberType ftype = kCFNumberFloat32Type;
  if(CFNumberGetType(dpiWidth) != ftype || CFNumberGetType(dpiHeight) != ftype) {
    return {0.0, 0.0};
  }
  
  float w_dpi = 0;
  float h_dpi = 0;
  CFNumberGetValue(dpiWidth, ftype, (void*)&w_dpi);
  CFNumberGetValue(dpiHeight, ftype, (void*)&h_dpi);

  return {w_dpi, h_dpi};
}


inline void nameee() {
  struct kinfo_proc *process = NULL;
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
  char pathBuffer [PROC_PIDPATHINFO_MAXSIZE];
  proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));

  char nameBuffer[256];

  int position = strlen(pathBuffer);
  while(position >= 0 && pathBuffer[position] != '/')
  {
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

  nano::cf_ptr<CGDataProviderRef> dataProvider(CGDataProviderCreateWithData(nullptr, buffer, bytesPerRow * size.height, nullptr));
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

std::size_t image::width() const
{
  return is_valid() ? CGImageGetWidth(m_pimpl->img) : 0;
}

std::size_t image::height() const
{
  return is_valid() ? CGImageGetHeight(m_pimpl->img) : 0;
}

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
  return is_valid() ? image(nano::cf_ptr<CGImageRef>(CGImageCreateWithImageInRect(m_pimpl->img, r.convert<CGRect>())).as<handle>()) : image();
}

namespace {
  static inline nano::cf_ptr<CGContextRef> create_bitmap_context(const nano::size<std::size_t>& size) {
    return  CGBitmapContextCreate(nullptr, size.width, size.height, 8,
                                 size.width * 4, nano::cf_ptr<CGColorSpaceRef>(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB)), kCGImageAlphaPremultipliedLast);
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

bool image::save(const std::filesystem::path& filepath, type img_type) {
  if (!is_valid()) {
    return false;
  }
//  filepath.string();
//  std::string_view sv(filepath);
  
//    CFStringRef st = CFStringCreateWithCString(kCFAllocatorDefault, filepath.c_str(), kCFStringEncodingUTF8);
//    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, st, kCFURLPOSIXPathStyle, false);
//  CFRelease(st);
  
//  CFStringRef st = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)sv.data(), sv.size(), kCFStringEncodingUTF8, false);
//  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, st, kCFURLPOSIXPathStyle, false);
  
  nano::cf_ptr<CFURLRef> url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8*)filepath.c_str(), std::string_view(filepath.c_str()).size(), false);

  
  
  CFStringRef typeString = nullptr;

  switch (img_type) {
  case type::png:
      typeString = kUTTypePNG;
    break;

  case type::jpeg:
      typeString = kUTTypeJPEG;
    break;
  }

  nano::cf_ptr<CGImageDestinationRef> dest = CGImageDestinationCreateWithURL(url, typeString, 1, nullptr);
  CGImageDestinationAddImage(dest, m_pimpl->img, nullptr);
  bool result = CGImageDestinationFinalize(dest);

//  CGImageAlphaInfo ainfo = CGImageGetAlphaInfo(m_pimpl->img);
//  std::cout << "----- " << (std::uint32_t)ainfo << std::endl;
//
//  CGBitmapInfo bmp_info = CGImageGetBitmapInfo(m_pimpl->img);
//  std::cout << get_bits_per_pixel() << "----- " << (std::uint32_t)bmp_info << std::endl;
//
//  std::uint32_t fff = bmp_info & kCGBitmapAlphaInfoMask;
//  std::cout << "----- " << (std::uint32_t)fff << std::endl;

  return result;
}
} // namespace nano.
