/**
 * @file mainwindow.h
 * @brief Main window for O2Emu GUI
 */

#pragma once

#include <QMainWindow>
#include <QTimer>
#include <memory>

class FramebufferWidget;
class DebuggerWidget;
class QMenuBar;
class QToolBar;
class QStatusBar;
class QDockWidget;

namespace o2emu::cpu {
class ICpu;
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

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void setPromPath(const QString &path);
  void setRamSize(int mb);
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

private:
  void createMenus();
  void createToolbars();
  void createStatusBar();
  void createDockWidgets();
  void initializeEmulator();
  void shutdownEmulator();

  // Emulator components
  std::unique_ptr<o2emu::cpu::ICpu> cpu_;
  std::unique_ptr<o2emu::memory::Memory> memory_;
  std::unique_ptr<o2emu::firmware::PROMLoader> prom_loader_;
  std::unique_ptr<o2emu::system::Bus> bus_;

  // UI components
  FramebufferWidget *framebuffer_widget_ = nullptr;
  DebuggerWidget *debugger_widget_ = nullptr;
  QDockWidget *debugger_dock_ = nullptr;

  // Emulation timer
  QTimer emulation_timer_;
  bool running_ = false;
  u64 cycles_per_frame_ = 1000000; // Cycles per UI update

  // Settings
  QString prom_path_;
  int ram_mb_ = 256;
  bool debug_logging_ = false;
};