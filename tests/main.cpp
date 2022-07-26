#include <nano/test.h>
#include <nano/graphics.h>
#include <thread>

namespace {
TEST_CASE("nano.graphics", Color, "Color") {
  nano::color c = 0xFF00FFFF;
  EXPECT_EQ(c.red(), 0xFF);
  EXPECT_EQ(c.green(), 0);
  EXPECT_EQ(c.blue(), 0xFF);
  EXPECT_EQ(c.alpha(), 0xFF);

  EXPECT_EQ(c.f_red(), 1.0f);
  EXPECT_EQ(c.f_green(), 0.0f);
  EXPECT_EQ(c.f_blue(), 1.0f);
  EXPECT_EQ(c.f_alpha(), 1.0f);

  EXPECT_EQ(c.red<float>(), 1.0f);
  EXPECT_EQ(c.green<float>(), 0.0f);
  EXPECT_EQ(c.blue<float>(), 1.0f);
  EXPECT_EQ(c.alpha<float>(), 1.0f);

  nano::image img("/Users/alexandrearsenault/Desktop/vvv.png", nano::image::type::png);

  EXPECT_TRUE(img.is_valid());

  nano::image img_copy = img.get_sub_image({ img.width() - 300, img.height() - 300, 300, 300 });
  img_copy.save("/Users/alexandrearsenault/Desktop/vvvffff2.jpg", nano::image::type::jpeg);

  std::cout << "DPI " << nano::image::get_dpi("/Users/alexandrearsenault/Desktop/vvv.jpg") << std::endl;

  img.save("/Users/alexandrearsenault/Desktop/vvv2.jpg", nano::image::type::jpeg);

  //  {1200,1600}
  std::cout << img.get_size() << std::endl;

  std::vector<std::uint8_t> buffer;
  img.copy_data(buffer);

  std::vector<float> fbuffer;
  fbuffer.resize(img.get_size().width * img.get_size().height * 4);
  std::size_t fbuffer_index = 0;

  for (std::size_t j = 0; j < img.get_size().height; j++) {
    nano::color* cs = reinterpret_cast<nano::color*>(buffer.data() + j * img.get_bytes_per_row());

    for (std::size_t i = 0; i < img.get_size().width; i++) {
      // from buffer
      // r -> a
      // g -> b
      // b -> g
      // a -> r

      nano::color c = cs[i].abgr();
      fbuffer[fbuffer_index++] = c.f_red();
      fbuffer[fbuffer_index++] = c.f_green();
      fbuffer[fbuffer_index++] = c.f_blue();
      fbuffer[fbuffer_index++] = c.f_alpha();

      cs[i] = cs[i].abgr();

      //      cs[i].red<nano::color::format::abgr>(0);
      //      cs[i].green<nano::color::format::abgr>(0);

      //      cs[i] = cs[i].abgr();
      //      cs[i].blue(0);
      ////      cs[i].red(0);
      ////      cs[i].alpha(0);
      //
      //      cs[i] = cs[i].abgr();
    }
  }

  nano::image img2(img.get_size(), //
      img.get_bits_per_component(), //
      img.get_bits_per_pixel(), //
      img.get_bytes_per_row(),
      nano::image::format::rgba, //
      buffer.data());

  img2.save("/Users/alexandrearsenault/Desktop/vvv3.png", nano::image::type::png);

  std::size_t bitsPerComponent = 32;
  std::size_t bytePerPixel = sizeof(float) * 4;
  nano::size<std::size_t> fsize = img.get_size();

  nano::image img3(fsize.with_height(800), //
      bitsPerComponent, //
      bitsPerComponent * 4, //
      fsize.width * bytePerPixel,
      nano::image::format::float_rgba, //
      (reinterpret_cast<const std::uint8_t*>(fbuffer.data())) + (fsize.height - 800) * fsize.width * bytePerPixel);

  img3.save("/Users/alexandrearsenault/Desktop/vvv4.png", nano::image::type::png);

  //  nano::graphic_context gc = nano::graphic_context::create_bitmap_context({200, 200}, 8, 200 * 4,
  //  nano::image::format::rgba);
  nano::graphic_context gc = nano::graphic_context::create_bitmap_context({ 500, 500 }, nano::image::format::rgba);
  gc.set_fill_color(0xFF0000FF);
  gc.fill_rect({ 0, 0, 500, 500 });

  gc.set_fill_color(0xFF00FFFF);

  nano::rect<float> rect = { 20, 20, 200, 200 };
  gc.fill_rect(rect);

  gc.draw_image(img2, rect.reduced({ 10, 10 }));

  nano::font ft("arial", 14);
  gc.set_fill_color(0x000000FF);
  gc.draw_text(ft, "Alex", { 5, 5 });

  gc.draw_text(ft, "Alexandre", rect.reduced({ 10, 10 }), nano::text_alignment::center);

  nano::image cimg = gc.create_image();
  cimg.save("/Users/alexandrearsenault/Desktop/vvv343.png", nano::image::type::png);

  //  create_bitmap_context(const nano::size<std::size_t>& size, std::size_t bitsPerComponent,
  //                 std::size_t bytesPerRow, image::format fmt,   std::uint8_t* buffer)
}
} // namespace.

NANO_TEST_MAIN()
