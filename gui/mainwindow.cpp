/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 */

#include "mainwindow.h"
#include "debuggerwidget.h"
#include "framebufferwidget.h"
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("O2Emu - SGI O2 (IP32) Emulator");
  resize(1200, 800);

  createMenus();
  createToolbars();
  createStatusBar();
  createDockWidgets();

  // Set up emulation timer
  connect(&emulation_timer_, &QTimer::timeout, this,
          &MainWindow::emulationLoop);
  emulation_timer_.setInterval(16); // ~60 FPS

  // Load settings
  QSettings settings;
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray());
  prom_path_ =
      settings.value("promPath", "samples/ip32prom.rev4.18.bin").toString();
  ram_mb_ = settings.value("ramMB", 256).toInt();
  debug_logging_ = settings.value("debugLogging", false).toBool();
}

MainWindow::~MainWindow() {
  shutdownEmulator();

  // Save settings
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  settings.setValue("promPath", prom_path_);
  settings.setValue("ramMB", ram_mb_);
  settings.setValue("debugLogging", debug_logging_);
}

void MainWindow::createMenus() {
  // File menu
  QMenu *fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction("&Open PROM...", this, &MainWindow::onOpenProm,
                      QKeySequence::Open);
  fileMenu->addAction("Open &Disk Image...", this, &MainWindow::onOpenDisk);
  fileMenu->addSeparator();
  fileMenu->addAction("&Save State...", this, &MainWindow::onSaveState,
                      QKeySequence::Save);
  fileMenu->addAction("&Load State...", this, &MainWindow::onLoadState,
                      QKeySequence::Open);
  fileMenu->addSeparator();
  fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

  // Emulation menu
  QMenu *emuMenu = menuBar()->addMenu("&Emulation");
  emuMenu->addAction("&Start", this, &MainWindow::onStart,
                     QKeySequence(Qt::Key_F5));
  emuMenu->addAction("&Pause", this, &MainWindow::onPause,
                     QKeySequence(Qt::Key_F6));
  emuMenu->addAction("&Stop", this, &MainWindow::onStop,
                     QKeySequence(Qt::Key_F7));
  emuMenu->addAction("&Reset", this, &MainWindow::onReset,
                     QKeySequence(Qt::Key_F8));
  emuMenu->addSeparator();
  emuMenu->addAction("&Step Instruction", this, &MainWindow::onStep,
                     QKeySequence(Qt::Key_F10));
  emuMenu->addAction("Run &Cycles...", this, &MainWindow::onRunCycles);

  // Debug menu
  QMenu *debugMenu = menuBar()->addMenu("&Debug");
  debugMenu->addAction("Show &Debugger", this,
                       [this]() { debugger_dock_->show(); });
  debugMenu->addAction("Show &Framebuffer", this,
                       [this]() { framebuffer_widget_->show(); });

  // Settings menu
  QMenu *settingsMenu = menuBar()->addMenu("&Settings");
  settingsMenu->addAction("&Preferences...", this, &MainWindow::onSettings);

  // Help menu
  QMenu *helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction("&About", this, &MainWindow::onAbout);
  helpMenu->addAction("About &Qt", qApp, &QApplication::aboutQt);
}

void MainWindow::createToolbars() {
  QToolBar *toolbar = addToolBar("Main");
  toolbar->setMovable(false);
  toolbar->setIconSize(QSize(24, 24));

  toolbar->addAction(QIcon::fromTheme("media-playback-start"), "Start", this,
                     &MainWindow::onStart);
  toolbar->addAction(QIcon::fromTheme("media-playback-pause"), "Pause", this,
                     &MainWindow::onPause);
  toolbar->addAction(QIcon::fromTheme("media-playback-stop"), "Stop", this,
                     &MainWindow::onStop);
  toolbar->addAction(QIcon::fromTheme("view-refresh"), "Reset", this,
                     &MainWindow::onReset);
  toolbar->addSeparator();
  toolbar->addAction(QIcon::fromTheme("go-next"), "Step", this,
                     &MainWindow::onStep);
  toolbar->addSeparator();
  toolbar->addAction(QIcon::fromTheme("document-open"), "Open PROM", this,
                     &MainWindow::onOpenProm);
  toolbar->addAction(QIcon::fromTheme("document-save"), "Save State", this,
                     &MainWindow::onSaveState);
}

void MainWindow::createStatusBar() {
  QStatusBar *status = statusBar();

  // CPU status
  cpu_status_label_ = new QLabel("CPU: Stopped");
  status->addWidget(cpu_status_label_);

  status->addPermanentWidget(new QLabel(" | "));

  // Cycle counter
  cycle_label_ = new QLabel("Cycles: 0");
  status->addPermanentWidget(cycle_label_);

  status->addPermanentWidget(new QLabel(" | "));

  // PC
  pc_label_ = new QLabel("PC: 0x00000000");
  status->addPermanentWidget(pc_label_);

  status->addPermanentWidget(new QLabel(" | "));

  // FPS
  fps_label_ = new QLabel("FPS: 0");
  status->addPermanentWidget(fps_label_);
}

void MainWindow::createDockWidgets() {
  // Framebuffer widget (central)
  framebuffer_widget_ = new FramebufferWidget(this);
  setCentralWidget(framebuffer_widget_);

  // Debugger dock
  debugger_widget_ = new DebuggerWidget(this);
  debugger_dock_ = new QDockWidget("Debugger", this);
  debugger_dock_->setWidget(debugger_widget_);
  debugger_dock_->setAllowedAreas(Qt::RightDockWidgetArea |
                                  Qt::BottomDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, debugger_dock_);
  debugger_dock_->hide();
}

void MainWindow::initializeEmulator() {
  // Initialize memory
  memory_ = std::make_unique<o2emu::memory::Memory>();
  memory_->init(ram_mb_);

  // Initialize CPU
  cpu_ = std::make_unique<o2emu::cpu::CPU>();
  cpu_->reset(o2emu::ip32::PROM_RESET_VECTOR);

  // Connect CPU to memory
  cpu_->set_memory_read_callback([this](u32 addr, u32 size) -> u32 {
    if (!memory_)
      return 0;
    switch (size) {
    case 1:
      return memory_->read8(addr);
    case 2:
      return memory_->read16(addr);
    case 4:
      return memory_->read32(addr);
    default:
      return 0;
    }
  });

  cpu_->set_memory_write_callback([this](u32 addr, u32 size, u32 value) {
    if (!memory_)
      return;
    switch (size) {
    case 1:
      memory_->write8(addr, value);
      break;
    case 2:
      memory_->write16(addr, value);
      break;
    case 4:
      memory_->write32(addr, value);
      break;
    }
  });

  // Load PROM
  prom_loader_ = std::make_unique<o2emu::firmware::PROMLoader>(*cpu_, *memory_);
  if (!prom_loader_->load_prom(prom_path_.toStdString())) {
    QMessageBox::critical(this, "Error", "Failed to load PROM: " + prom_path_);
    return;
  }

  prom_loader_->map_prom_sections();
  prom_loader_->init_cpu_for_prom();

  // Connect framebuffer widget
  framebuffer_widget_->setMemory(memory_.get());
  framebuffer_widget_->setCPU(cpu_.get());

  // Connect debugger
  debugger_widget_->setCPU(cpu_.get());
  debugger_widget_->setMemory(memory_.get());

  running_ = true;
  emulation_timer_.start();
  cpu_status_label_->setText("CPU: Running");
}

void MainWindow::shutdownEmulator() {
  emulation_timer_.stop();
  running_ = false;
  cpu_status_label_->setText("CPU: Stopped");
}

void MainWindow::onStart() {
  if (!cpu_) {
    initializeEmulator();
  } else if (!running_) {
    running_ = true;
    emulation_timer_.start();
    cpu_status_label_->setText("CPU: Running");
  }
}

void MainWindow::onPause() {
  if (running_) {
    running_ = false;
    emulation_timer_.stop();
    cpu_status_label_->setText("CPU: Paused");
  }
}

void MainWindow::onStop() {
  shutdownEmulator();
  cpu_.reset();
  memory_.reset();
  prom_loader_.reset();
  framebuffer_widget_->clear();
  debugger_widget_->clear();
}

void MainWindow::onReset() {
  if (cpu_) {
    cpu_->reset(o2emu::ip32::PROM_RESET_VECTOR);
    prom_loader_->init_cpu_for_prom();
    framebuffer_widget_->clear();
    debugger_widget_->clear();
  }
}

void MainWindow::onStep() {
  if (cpu_) {
    cpu_->step();
    updateUI();
  }
}

void MainWindow::onRunCycles() {
  bool ok;
  u64 cycles = QInputDialog::getInt(
      this, "Run Cycles", "Number of cycles:", 1000000, 1, 1000000000, 1, &ok);
  if (ok && cpu_) {
    cpu_->run(cycles);
    updateUI();
  }
}

void MainWindow::onOpenProm() {
  QString file =
      QFileDialog::getOpenFileName(this, "Open PROM Image", prom_path_,
                                   "Binary Files (*.bin);;All Files (*)");
  if (!file.isEmpty()) {
    prom_path_ = file;
    if (running_) {
      onStop();
      onStart();
    }
  }
}

void MainWindow::onOpenDisk() {
  QString file = QFileDialog::getOpenFileName(
      this, "Open Disk Image", "", "Disk Images (*.img *.iso);;All Files (*)");
  if (!file.isEmpty()) {
    // TODO: Attach disk image to SCSI controller
  }
}

void MainWindow::onSaveState() {
  QString file = QFileDialog::getSaveFileName(
      this, "Save State", "", "State Files (*.state);;All Files (*)");
  if (!file.isEmpty()) {
    // TODO: Implement save state
  }
}

void MainWindow::onLoadState() {
  QString file = QFileDialog::getOpenFileName(
      this, "Load State", "", "State Files (*.state);;All Files (*)");
  if (!file.isEmpty()) {
    // TODO: Implement load state
  }
}

void MainWindow::onSettings() {
  // TODO: Settings dialog
}

void MainWindow::onAbout() {
  QMessageBox::about(this, "About O2Emu",
                     "<h3>O2Emu v0.1.0</h3>"
                     "<p>SGI O2 (IP32 / Moosehead) Emulator</p>"
                     "<p>Based on exhaustive hardware research from Linux, "
                     "NetBSD, and leaked IRIX sources.</p>"
                     "<p>License: BSD 3-Clause</p>");
}

void MainWindow::updateUI() {
  if (cpu_) {
    cycle_label_->setText(QString("Cycles: %1").arg(cpu_->cycles_executed()));
    pc_label_->setText(
        QString("PC: 0x%1").arg(cpu_->state().pc, 8, 16, QChar('0')).toUpper());
  }

  if (framebuffer_widget_) {
    fps_label_->setText(QString("FPS: %1").arg(framebuffer_widget_->fps()));
  }
}

void MainWindow::emulationLoop() {
  if (!cpu_ || !running_)
    return;

  // Run emulation for a chunk of cycles
  cpu_->run(cycles_per_frame_);

  // Update framebuffer
  if (framebuffer_widget_) {
    framebuffer_widget_->updateFramebuffer();
  }

  // Update debugger
  if (debugger_widget_ && debugger_dock_->isVisible()) {
    debugger_widget_->update();
  }

  // Update UI periodically
  static int frame_count = 0;
  if (++frame_count % 60 == 0) {
    updateUI();
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  shutdownEmulator();
  event->accept();
}