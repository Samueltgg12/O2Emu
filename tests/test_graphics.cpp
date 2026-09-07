/**
 * @file test_graphics.cpp
 * @brief Graphics subsystem unit tests
 */

#include <gtest/gtest.h>
#include <o2emu/graphics/display_engine.h>
#include <o2emu/graphics/framebuffer.h>
#include <o2emu/graphics/ice.h>
#include <o2emu/graphics/microprocessor.h>

using namespace o2emu::graphics;

TEST(Microprocessor, BasicOperation) {
  Microprocessor mp;

  // Test register access
  mp.write(Microprocessor::MICRO_CONTROL, Microprocessor::CTRL_ENABLE);
  EXPECT_EQ(mp.read(Microprocessor::MICRO_CONTROL),
            Microprocessor::CTRL_ENABLE);

  // Test revision
  EXPECT_EQ(mp.read(Microprocessor::MICRO_REVISION), 0x00010000);
}

TEST(Microprocessor, CommandFIFO) {
  Microprocessor mp;
  mp.write(Microprocessor::MICRO_CONTROL, Microprocessor::CTRL_ENABLE);

  // Push commands
  mp.push_command(0x00000000); // NOP
  mp.push_command(0x00000001); // VERTEX

  EXPECT_FALSE(mp.fifo_empty());
  EXPECT_FALSE(mp.fifo_full());

  // Process
  mp.draw_primitive(Microprocessor::CMD_NOP);
  EXPECT_TRUE(mp.fifo_empty());
}

TEST(Microprocessor, VertexProcessing) {
  Microprocessor mp;
  mp.write(Microprocessor::MICRO_CONTROL, Microprocessor::CTRL_ENABLE);

  mp.process_vertex(1.0f, 2.0f, 3.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);

  EXPECT_TRUE(mp.busy());
}

TEST(Microprocessor, MatrixStack) {
  Microprocessor mp;
  mp.write(Microprocessor::MICRO_CONTROL, Microprocessor::CTRL_ENABLE);

  float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  mp.load_matrix(identity);
  mp.push_matrix();
  mp.pop_matrix();

  // Should not crash
}

TEST(Microprocessor, Lighting) {
  Microprocessor mp;
  mp.write(Microprocessor::MICRO_CONTROL, Microprocessor::CTRL_ENABLE);

  float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
  float diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float position[4] = {0.0f, 0.0f, 1.0f, 0.0f};

  mp.set_light(0, true, ambient, diffuse, specular, position, nullptr, nullptr);

  // Should not crash
}

TEST(ICE, BasicOperation) {
  ICE ice;

  // VICE_ID reads back the chip revision/ID (reset value 0xE1 per spec).
  EXPECT_EQ(ice.read32(ICE::VICE_ID), 0xE1);

  // A generic chip register write/read round-trips.
  ice.write32(ICE::MSP_CTL_STAT, 0xDEADBEEF);
  EXPECT_EQ(ice.read32(ICE::MSP_CTL_STAT), 0xDEADBEEF);
}

TEST(ICE, ResetDefaults) {
  ICE ice;

  // BSP FIFO control/status resets to 0x05 per spec.
  EXPECT_EQ(ice.read32(ICE::BSP_FIFO_CTL_STAT), 0x05);

  // DMA channel control/status resets to 0x10 per spec.
  EXPECT_EQ(ice.read32(ICE::DMA_CTL_CH1), 0x10);
  EXPECT_EQ(ice.read32(ICE::DMA_STAT_CH1), 0x10);
  EXPECT_EQ(ice.read32(ICE::DMA_CTL_CH2), 0x10);
  EXPECT_EQ(ice.read32(ICE::DMA_STAT_CH2), 0x10);
}

TEST(ICE, InterruptEnableAndReset) {
  ICE ice;

  // Enable an interrupt bit, then clear it via the write-1-to-clear reset.
  ice.write32(ICE::VICE_INT_EN, 0x1);
  ice.write32(ICE::BSP_SW_INT, 0x1); // raise software interrupt
  EXPECT_NE(ice.interrupt_status(), 0u);

  ice.write32(ICE::VICE_INT_RESET, 0x1);
  EXPECT_EQ(ice.interrupt_status(), 0u);
}

TEST(ICE, SubWordAccess) {
  ICE ice;

  // 16-bit and 8-bit accessors operate on the correct byte lanes.
  ice.write32(ICE::MSP_PC, 0x12345678);
  EXPECT_EQ(ice.read16(ICE::MSP_PC), 0x5678);
  EXPECT_EQ(ice.read8(ICE::MSP_PC), 0x78);
}

TEST(DisplayEngine, BasicOperation) {
  DisplayEngine de;

  de.write(DisplayEngine::DE_CONTROL, DisplayEngine::CTRL_ENABLE);
  EXPECT_EQ(de.read(DisplayEngine::DE_CONTROL), DisplayEngine::CTRL_ENABLE);

  EXPECT_EQ(de.read(DisplayEngine::DE_REVISION), 0x00010000);
}

TEST(DisplayEngine, VideoTiming) {
  DisplayEngine de;

  de.set_timing(800, 640, 656, 752, 525, 480, 490, 492, false);

  u32 h_total, h_display, h_sync_start, h_sync_end;
  u32 v_total, v_display, v_sync_start, v_sync_end;
  de.get_timing(&h_total, &h_display, &h_sync_start, &h_sync_end, &v_total,
                &v_display, &v_sync_start, &v_sync_end);

  EXPECT_EQ(h_total, 800);
  EXPECT_EQ(h_display, 640);
  EXPECT_EQ(v_total, 525);
  EXPECT_EQ(v_display, 480);
}

TEST(DisplayEngine, Framebuffer) {
  DisplayEngine de;

  de.set_framebuffer(0x1000000, 640 * 4, 640, 480, DisplayEngine::FMT_32BPP);

  EXPECT_EQ(de.read(DisplayEngine::DE_FB_ADDR), 0x1000000);
  EXPECT_EQ(de.read(DisplayEngine::DE_FB_WIDTH), 640);
  EXPECT_EQ(de.read(DisplayEngine::DE_FB_HEIGHT), 480);
}

TEST(DisplayEngine, Cursor) {
  DisplayEngine de;

  de.set_cursor_position(100, 100);
  de.set_cursor_hotspot(8, 8);
  de.set_cursor_colors(0xFFFFFFFF, 0xFF000000, 0x00000000);
  de.enable_cursor(true);

  EXPECT_EQ(de.read(DisplayEngine::DE_CURSOR_POS_X), 100);
  EXPECT_EQ(de.read(DisplayEngine::DE_CURSOR_POS_Y), 100);
  EXPECT_TRUE(de.read(DisplayEngine::DE_CURSOR_CONTROL) &
              DisplayEngine::CURSOR_ENABLE);
}

TEST(DisplayEngine, VideoModes) {
  const auto *mode = DisplayEngine::get_video_mode("1024x768@60");
  ASSERT_NE(mode, nullptr);
  EXPECT_EQ(mode->width, 1024);
  EXPECT_EQ(mode->height, 768);
  EXPECT_EQ(mode->refresh_rate_hz, 60);

  mode = DisplayEngine::get_video_mode_by_resolution(1280, 1024, 60);
  ASSERT_NE(mode, nullptr);
  EXPECT_EQ(mode->width, 1280);
  EXPECT_EQ(mode->height, 1024);
}

TEST(Framebuffer, BasicOperation) {
  Framebuffer fb;

  fb.write(Framebuffer::FB_CONTROL, Framebuffer::CTRL_ENABLE);
  EXPECT_EQ(fb.read(Framebuffer::FB_CONTROL), Framebuffer::CTRL_ENABLE);

  EXPECT_EQ(fb.read(Framebuffer::FB_REVISION), 0x00010000);
}

TEST(Framebuffer, PixelAccess) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL, Framebuffer::CTRL_ENABLE);
  fb.set_dimensions(640, 480);
  fb.set_format(Framebuffer::FMT_32BPP);

  fb.write_pixel(100, 100, 0xFF0000FF); // Blue
  EXPECT_EQ(fb.read_pixel(100, 100), 0xFF0000FF);
}

TEST(Framebuffer, PixelFormats) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL, Framebuffer::CTRL_ENABLE);
  fb.set_dimensions(1600, 1200);

  fb.set_format(Framebuffer::FMT_8BPP);
  fb.write_pixel(1599, 1199, 0x0000007F);
  EXPECT_EQ(fb.read_pixel(1599, 1199), 0x0000007F);

  fb.set_format(Framebuffer::FMT_16BPP);
  fb.write_pixel(1599, 1199, 0x0000ABCD);
  EXPECT_EQ(fb.read_pixel(1599, 1199), 0x0000ABCD);

  fb.set_format(Framebuffer::FMT_24BPP);
  fb.write_pixel(1599, 1199, 0x00123456);
  EXPECT_EQ(fb.read_pixel(1599, 1199), 0x00123456);
}

TEST(Framebuffer, SpanOperations) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL, Framebuffer::CTRL_ENABLE);
  fb.set_dimensions(640, 480);
  fb.set_format(Framebuffer::FMT_32BPP);

  u32 colors[10];
  for (int i = 0; i < 10; i++)
    colors[i] = 0xFF000000 | (i << 16);

  fb.write_span(100, 100, 10, colors);

  u32 read_colors[10];
  fb.read_span(100, 100, 10, read_colors);

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(read_colors[i], colors[i]);
  }
}

TEST(Framebuffer, Clear) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL, Framebuffer::CTRL_ENABLE);
  fb.set_dimensions(640, 480);
  fb.set_format(Framebuffer::FMT_32BPP);

  fb.clear(0xFF00FF00); // Green

  EXPECT_EQ(fb.read_pixel(0, 0), 0xFF00FF00);
  EXPECT_EQ(fb.read_pixel(639, 479), 0xFF00FF00);
}

TEST(Framebuffer, DoubleBuffer) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL,
           Framebuffer::CTRL_ENABLE | Framebuffer::CTRL_DOUBLE_BUFFER);
  fb.set_dimensions(640, 480);
  fb.set_format(Framebuffer::FMT_32BPP);

  fb.set_front_buffer(0x1000000);
  fb.set_back_buffer(0x2000000);

  fb.swap_buffers();
  EXPECT_TRUE(fb.swap_pending());
}

TEST(Framebuffer, TileOperations) {
  Framebuffer fb;
  fb.write(Framebuffer::FB_CONTROL,
           Framebuffer::CTRL_ENABLE | Framebuffer::CTRL_TILE_MODE);
  fb.set_dimensions(640, 480);
  fb.set_format(Framebuffer::FMT_32BPP);
  fb.set_tile_config(Framebuffer::TILE_16x16, 32);

  u32 tile_data[16];
  for (int i = 0; i < 16; i++)
    tile_data[i] = 0xFF0000FF;

  fb.write_tile(0, 0, tile_data, 64);

  u32 read_data[16];
  fb.read_tile(0, 0, read_data, 64);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(read_data[i], 0xFF0000FF);
  }
}

TEST(Framebuffer, VideoModes) {
  const auto *mode = Framebuffer::get_video_mode("1600x1200@60");
  ASSERT_NE(mode, nullptr);
  EXPECT_EQ(mode->width, 1600);
  EXPECT_EQ(mode->height, 1200);
}