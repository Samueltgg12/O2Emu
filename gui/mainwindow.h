/**
 * @file mainwindow.h
 * @brief Main window for O2Emu GUI
 */

#pragma once

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <array>
#include <cstdint>
#include <memory>
#include <o2emu/cpu/cpu_interface.h>
#include <o2emu/system/network_config.h>

class FramebufferWidget;
class DebuggerWidget;
class QMenuBar;
class QToolBar;
class QStatusBar;
class QDockWidget;

namespace o2emu::cpu {
class CPU;
}
namespace o2emu::memory {
class Memory;
}
namespace o2emu::firmware {
class PROMLoader;
}
namespace o2emu::system {
class Bus;
}
namespace o2emu::graphics {
class GBEFramebuffer;
}
namespace o2emu::devices {
class MACE;
class PS2;
class SCSIController;
} // namespace o2emu::devices

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void setPromPath(const QString &path);
  void setRamSize(int mb);
  void setCpuType(o2emu::cpu::CPUType type);
  void setDebugLogging(bool enabled);

private slots:
  void onStart();
  void onPause();
  void onStop();
  void onReset();
  void onStep();
  void onRunCycles();
  void onOpenProm();
  void onOpenDisk();
  void onSaveState();
  void onLoadState();
  void onSettings();
  void onAbout();
  void updateUI();
  void emulationLoop();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  void createMenus();
  void createToolbars();
  void createStatusBar();
  void createDockWidgets();
  void initializeEmulator();
  void shutdownEmulator();
  void attachMedia(int target, const QString &path);
  void updateSlotConfiguration();
  void applyNetworkConfiguration();
  static bool isCdImage(const QString &path);

  // Emulator components
  std::unique_ptr<o2emu::cpu::CPU> cpu_;
  std::unique_ptr<o2emu::memory::Memory> memory_;
  std::unique_ptr<o2emu::firmware::PROMLoader> prom_loader_;
  std::unique_ptr<o2emu::system::Bus> bus_;
  o2emu::graphics::GBEFramebuffer *gbe_framebuffer_ = nullptr;
  o2emu::devices::MACE *mace_ = nullptr;
  o2emu::devices::PS2 *ps2_ = nullptr;
  o2emu::devices::SCSIController *scsi_ = nullptr;

  // UI components
  FramebufferWidget *framebuffer_widget_ = nullptr;
  DebuggerWidget *debugger_widget_ = nullptr;
  QDockWidget *debugger_dock_ = nullptr;

  // Status bar labels
  QLabel *cpu_status_label_ = nullptr;
  QLabel *cycle_label_ = nullptr;
  QLabel *pc_label_ = nullptr;
  QLabel *fps_label_ = nullptr;

  // Emulation timer
  QTimer emulation_timer_;
  bool running_ = false;
  uint64_t cycles_per_frame_ = 1000000; // Cycles per UI update

  // Settings
  QString prom_path_;
  int ram_mb_ = 256;
  o2emu::cpu::CPUType cpu_type_ = o2emu::cpu::CPUType::R10000;
  bool debug_logging_ = false;
  std::array<QString, 7> scsi_images_;
  o2emu::system::NetworkConfig network_config_;
};