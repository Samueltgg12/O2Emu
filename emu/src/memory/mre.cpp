/**
 * @file mre.cpp
 * @brief MRE (Memory & Rendering Engine) implementation
 */

#include <o2emu/logging/logger.h>
#include <o2emu/memory/mre.h>

namespace o2emu::memory {

MRE::MRE(Memory &memory) : memory_(memory) { reset(); }

void MRE::reset() {
  regs_.fill(0);
  framebuffer_configured_ = false;
  render_active_ = false;
  dma_active_ = false;

  // The CRIME Render Engine has no documented reset-time register defaults
  // beyond zero; the PROM/driver programs every page explicitly. The
  // interface-buffer control register reports empty (not full) after reset.
  regs_[INTFBUF_CTL / 4] = 0x00000000;
}

u32 MRE::read(u32 offset) const {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MRE::write(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  const u32 reg = offset / 4;

  switch (offset) {
  case INTFBUF_RESET:
    // Writing the interface-buffer reset clears the FIFO and control state.
    regs_[INTFBUF_CTL / 4] = 0;
    regs_[reg] = value;
    break;

  case PIXPIPE_FLUSH:
    // Flushing the pixel pipe completes any in-flight render.
    regs_[reg] = value;
    render_active_ = false;
    break;

  case MTE_FLUSH:
    // Flushing the MTE completes any in-flight transfer.
    regs_[reg] = value;
    dma_active_ = false;
    break;

  case SET_START_PTR:
    // Setting the start pointer begins rendering from the given address.
    regs_[reg] = value;
    render_active_ = true;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

void MRE::tick(u64 cycles) {
  // The RE has no documented free-running counters; this is a no-op hook
  // retained for the device tick interface.
  (void)cycles;
}

u32 MRE::interrupt_status() const {
  // The RE raises interrupts through the CRIME CPU interface (0x14000000),
  // not through a local mask/status pair. Report no pending RE interrupts.
  return 0;
}

void MRE::clear_interrupt(u32 bit) {
  // RE interrupts are cleared via the CRIME interrupt controller.
  (void)bit;
}

void MRE::start_render() {
  // Rendering is initiated by writing SET_START_PTR; this helper mirrors the
  // same state transition for callers that drive the RE directly.
  render_active_ = true;
}

void MRE::flush_render() {
  // Flushing the pixel pipe completes any in-flight render.
  render_active_ = false;
}

bool MRE::render_busy() const { return render_active_; }

void MRE::start_dma(u32 src, u32 dst, u32 size) {
  // Program the MTE source/destination and mark a transfer in flight.
  regs_[MTE_SRC0 / 4] = src;
  regs_[MTE_DST0 / 4] = dst;
  regs_[MTE_MODE / 4] = size; // size is carried in the mode word for now
  dma_active_ = true;
}

bool MRE::dma_busy() const { return dma_active_; }

void MRE::set_framebuffer(u32 phys_addr, u32 stride, u32 width, u32 height,
                          u32 depth) {
  fb_base_ = phys_addr;
  fb_stride_ = stride;
  fb_width_ = width;
  fb_height_ = height;
  fb_depth_ = depth;
  fb_format_ = 0; // format is not tracked by the RE; see GBE color mode
  framebuffer_configured_ = true;
}

u32 MRE::fb_base() const { return fb_base_; }
u32 MRE::fb_stride() const { return fb_stride_; }
u32 MRE::fb_width() const { return fb_width_; }
u32 MRE::fb_height() const { return fb_height_; }
u32 MRE::fb_depth() const { return fb_depth_; }
u32 MRE::fb_format() const { return fb_format_; }

MRE::~MRE() = default;

} // namespace o2emu::memory