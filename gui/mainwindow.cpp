/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 */

#include "mainwindow.h"
#include "debuggerwidget.h"
#include "framebufferwidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDataStream>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <o2emu/cpu/cpu.h>
#include <o2emu/cpu/cpu_interface.h>
#include <o2emu/devices/mace/mace.h>
#include <o2emu/devices/ps2.h>
#include <o2emu/devices/rtc.h>
#include <o2emu/devices/scsicontroller.h>
#include <o2emu/devices/uart.h>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/graphics/gbe_framebuffer.h>
#include <o2emu/memory/memory.h>
#include <o2emu/system/bus.h>

using o2emu::u64;

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

  // Default PROM path: find samples directory relative to executable
  QString default_prom_path = QCoreApplication::applicationDirPath() +
                              "/../../samples/ip32prom.rev4.18.bin";
  prom_path_ = settings.value("promPath", default_prom_path).toString();
  ram_mb_ = settings.value("ramMB", 256).toInt();
  cpu_type_ = static_cast<o2emu::cpu::CPUType>(
      settings.value("cpuType", static_cast<int>(cpu_type_)).toInt());
  debug_logging_ = settings.value("debugLogging", false).toBool();
  for (int target = 0; target < 7; ++target) {
    scsi_images_[target] =
        settings.value(QString("scsi%1").arg(target), QString()).toString();
  }
}

MainWindow::~MainWindow() {
  shutdownEmulator();

  // Save settings
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  settings.setValue("promPath", prom_path_);
  settings.setValue("ramMB", ram_mb_);
  settings.setValue("cpuType", static_cast<int>(cpu_type_));
  settings.setValue("debugLogging", debug_logging_);
  for (int target = 0; target < 7; ++target) {
    settings.setValue(QString("scsi%1").arg(target), scsi_images_[target]);
  }
}

void MainWindow::setPromPath(const QString &path) {
  prom_path_ = path;
  if (running_) {
    onStop();
    onStart();
  }
}

void MainWindow::setRamSize(int mb) {
  ram_mb_ = mb;
  if (running_) {
    onStop();
    onStart();
  }
}

void MainWindow::setCpuType(o2emu::cpu::CPUType type) {
  cpu_type_ = type;
  if (running_) {
    onStop();
    onStart();
  }
}

void MainWindow::setDebugLogging(bool enabled) {
  debug_logging_ = enabled;
  // TODO: Configure logging system based on debug_logging_
}

void MainWindow::createMenus() {
  // File menu
  QMenu *fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction("&Open PROM...", QKeySequence::Open, this,
                      &MainWindow::onOpenProm);
  fileMenu->addAction("Open &Disk Image...", QKeySequence(), this,
                      &MainWindow::onOpenDisk);
  fileMenu->addSeparator();
  fileMenu->addAction("&Save State...", QKeySequence::Save, this,
                      &MainWindow::onSaveState);
  fileMenu->addAction("&Load State...", QKeySequence::Open, this,
                      &MainWindow::onLoadState);
  fileMenu->addSeparator();
  fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);

  // Emulation menu
  QMenu *emuMenu = menuBar()->addMenu("&Emulation");
  emuMenu->addAction("&Start", QKeySequence(Qt::Key_F5), this,
                     &MainWindow::onStart);
  emuMenu->addAction("&Pause", QKeySequence(Qt::Key_F6), this,
                     &MainWindow::onPause);
  emuMenu->addAction("&Stop", QKeySequence(Qt::Key_F7), this,
                     &MainWindow::onStop);
  emuMenu->addAction("&Reset", QKeySequence(Qt::Key_F8), this,
                     &MainWindow::onReset);
  emuMenu->addSeparator();
  emuMenu->addAction("&Step Instruction", QKeySequence(Qt::Key_F10), this,
                     &MainWindow::onStep);
  emuMenu->addAction("Run &Cycles...", QKeySequence(), this,
                     &MainWindow::onRunCycles);

  // Debug menu
  QMenu *debugMenu = menuBar()->addMenu("&Debug");
  debugMenu->addAction("Show &Debugger", QKeySequence(), this,
                       [this]() { debugger_dock_->show(); });
  debugMenu->addAction("Show &Framebuffer", QKeySequence(), this,
                       [this]() { framebuffer_widget_->show(); });

  // Settings menu
  QMenu *settingsMenu = menuBar()->addMenu("&Settings");
  settingsMenu->addAction("&Preferences...", QKeySequence(), this,
                          &MainWindow::onSettings);

  // Help menu
  QMenu *helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction("&About", QKeySequence(), this, &MainWindow::onAbout);
  helpMenu->addAction("About &Qt", QKeySequence(), qApp,
                      &QApplication::aboutQt);
}

void MainWindow::createToolbars() {
  QToolBar *toolbar = addToolBar("Main");
  toolbar->setMovable(false);
  toolbar->setIconSize(QSize(24, 24));

  toolbar->addAction(QIcon::fromTheme("media-playback-start"), "Start",
                     QKeySequence(), this, &MainWindow::onStart);
  toolbar->addAction(QIcon::fromTheme("media-playback-pause"), "Pause",
                     QKeySequence(), this, &MainWindow::onPause);
  toolbar->addAction(QIcon::fromTheme("media-playback-stop"), "Stop",
                     QKeySequence(), this, &MainWindow::onStop);
  toolbar->addAction(QIcon::fromTheme("view-refresh"), "Reset", QKeySequence(),
                     this, &MainWindow::onReset);
  toolbar->addSeparator();
  toolbar->addAction(QIcon::fromTheme("go-next"), "Step", QKeySequence(), this,
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

  // Initialize system bus
  bus_ = std::make_unique<o2emu::system::Bus>();

  // Initialize CPU (R10000 for O2/IP32)
  cpu_ = o2emu::cpu::create_cpu(cpu_type_, bus_.get());
  cpu_->reset(o2emu::ip32::PROM_RESET_VECTOR);

  // Connect CPU to memory via bus
  bus_->attach_memory(memory_.get());

  // Attach the CRM display plane and MACE I/O devices before starting PROM.
  auto gbe_framebuffer = std::make_unique<o2emu::graphics::GBEFramebuffer>();
  gbe_framebuffer_ = gbe_framebuffer.get();
  bus_->attach_device(std::move(gbe_framebuffer));

  auto mace = std::make_unique<o2emu::devices::MACE>(*memory_);
  mace_ = mace.get();
  bus_->attach_device(std::move(mace));

  auto ps2 = std::make_unique<o2emu::devices::PS2>(0x1F320000, 5, 6);
  ps2_ = ps2.get();
  bus_->attach_device(std::move(ps2));
  bus_->attach_device(std::make_unique<o2emu::devices::UART>(0x1F390000, 7));
  bus_->attach_device(std::make_unique<o2emu::devices::UART>(0x1F398000, 8));
  bus_->attach_device(std::make_unique<o2emu::devices::RTC>(0x1F3A0000));
  auto scsi = std::make_unique<o2emu::devices::SCSIController>();
  scsi_ = scsi.get();
  bus_->attach_device(std::move(scsi));
  updateSlotConfiguration();

  // Load PROM
  prom_loader_ =
      std::make_unique<o2emu::firmware::PROMLoader>(bus_.get(), cpu_.get());
  if (!prom_loader_->load_prom(prom_path_.toStdString())) {
    QMessageBox::critical(this, "Error", "Failed to load PROM: " + prom_path_);
    return;
  }

  prom_loader_->execute_bootstrap();

  // Connect framebuffer widget
  framebuffer_widget_->setMemory(memory_.get());
  framebuffer_widget_->setGBEFramebuffer(gbe_framebuffer_);
  framebuffer_widget_->setPS2(ps2_);
  framebuffer_widget_->setCPU(cpu_.get());

  // Connect debugger
  debugger_widget_->setCPU(cpu_.get());
  debugger_widget_->setMemory(memory_.get());

  running_ = true;
  emulation_timer_.start();
  cpu_status_label_->setText("CPU: Running (R10000)");
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
  bus_.reset();
  gbe_framebuffer_ = nullptr;
  mace_ = nullptr;
  ps2_ = nullptr;
  scsi_ = nullptr;
  cpu_.reset();
  memory_.reset();
  prom_loader_.reset();
  framebuffer_widget_->clear();
  debugger_widget_->clear();
}

void MainWindow::onReset() {
  if (cpu_) {
    cpu_->reset(o2emu::ip32::PROM_RESET_VECTOR);
    prom_loader_->execute_bootstrap();
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

bool MainWindow::isCdImage(const QString &path) {
  const QString suffix = QFileInfo(path).suffix().toLower();
  return suffix == "iso" || suffix == "bin" || suffix == "cue" ||
         suffix == "ccd" || suffix == "mds" || suffix == "mdf";
}

void MainWindow::attachMedia(int target, const QString &path) {
  if (target < 0 || target >= 7)
    return;
  if (!path.isEmpty() && !QFileInfo(path).isFile()) {
    QMessageBox::warning(this, "SCSI media",
                         "The selected image does not exist.");
    return;
  }
  scsi_images_[target] = path;
  if (scsi_) {
    scsi_->detach_device(target, 0);
    if (!path.isEmpty())
      scsi_->attach_device(target, 0, path.toStdString());
  }
}

void MainWindow::updateSlotConfiguration() {
  if (!scsi_)
    return;
  for (int target = 0; target < 7; ++target) {
    scsi_->detach_device(target, 0);
    if (!scsi_images_[target].isEmpty())
      scsi_->attach_device(target, 0, scsi_images_[target].toStdString());
  }
}

void MainWindow::onOpenDisk() {
  QString file = QFileDialog::getOpenFileName(
      this, "Open Disk Image", "",
      "Disk/CD images (*.raw *.chd *.img *.iso *.bin *.cue *.ccd *.mds "
      "*.mdf);;All files (*)");
  if (!file.isEmpty()) {
    bool ok = false;
    const int default_target = isCdImage(file) ? 6 : 1;
    const int target = QInputDialog::getInt(
        this, "SCSI target", "Insert image into SCSI drive:", default_target, 0,
        6, 1, &ok);
    if (ok)
      attachMedia(target, file);
  }
}

void MainWindow::onSaveState() {
  QString file = QFileDialog::getSaveFileName(
      this, "Save State", "", "State Files (*.state);;All Files (*)");
  if (!file.isEmpty()) {
    if (!cpu_ || !memory_) {
      QMessageBox::warning(this, "Save State", "The emulator is not running.");
      return;
    }
    QFile state_file(file);
    if (!state_file.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(this, "Save State", state_file.errorString());
      return;
    }
    QDataStream stream(&state_file);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << quint32(0x4F324553) << quint32(1) << qint32(ram_mb_)
           << qint32(static_cast<int>(cpu_type_));
    const auto &state = cpu_->state();
    stream.writeRawData(reinterpret_cast<const char *>(state.gpr),
                        sizeof(state.gpr));
    stream << state.pc << state.next_pc << state.hi << state.lo;
    stream << quint32(memory_->size());
    stream.writeRawData(reinterpret_cast<const char *>(memory_->data()),
                        static_cast<int>(memory_->size()));
    for (const auto &image : scsi_images_)
      stream << image;
  }
}

void MainWindow::onLoadState() {
  QString file = QFileDialog::getOpenFileName(
      this, "Load State", "", "State Files (*.state);;All Files (*)");
  if (!file.isEmpty()) {
    QFile state_file(file);
    if (!state_file.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(this, "Load State", state_file.errorString());
      return;
    }
    QDataStream stream(&state_file);
    stream.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0;
    quint32 version = 0;
    qint32 saved_ram = 0;
    qint32 saved_cpu = 0;
    stream >> magic >> version >> saved_ram >> saved_cpu;
    if (magic != 0x4F324553 || version != 1 || saved_ram <= 0) {
      QMessageBox::critical(this, "Load State", "Invalid O2Emu state file.");
      return;
    }

    onStop();
    ram_mb_ = saved_ram;
    cpu_type_ = static_cast<o2emu::cpu::CPUType>(saved_cpu);
    initializeEmulator();
    if (!cpu_ || !memory_) {
      QMessageBox::critical(this, "Load State", "Could not initialize state.");
      return;
    }
    auto &state = cpu_->state();
    stream.readRawData(reinterpret_cast<char *>(state.gpr), sizeof(state.gpr));
    stream >> state.pc >> state.next_pc >> state.hi >> state.lo;
    quint32 memory_size = 0;
    stream >> memory_size;
    if (memory_size != memory_->size()) {
      QMessageBox::critical(this, "Load State", "State memory size mismatch.");
      return;
    }
    stream.readRawData(reinterpret_cast<char *>(memory_->data()),
                       static_cast<int>(memory_size));
    for (auto &image : scsi_images_)
      stream >> image;
    updateSlotConfiguration();
    framebuffer_widget_->updateFramebuffer();
  }
}

void MainWindow::onSettings() {
  QDialog dialog(this);
  dialog.setWindowTitle("O2Emu Configuration");
  auto *layout = new QFormLayout(&dialog);

  auto *cpu_combo = new QComboBox(&dialog);
  cpu_combo->addItem("R5000", static_cast<int>(o2emu::cpu::CPUType::R5000));
  cpu_combo->addItem("R10000", static_cast<int>(o2emu::cpu::CPUType::R10000));
  cpu_combo->addItem("R12000", static_cast<int>(o2emu::cpu::CPUType::R12000));
  cpu_combo->setCurrentIndex(cpu_combo->findData(static_cast<int>(cpu_type_)));
  layout->addRow("CPU", cpu_combo);

  auto *ram_spin = new QSpinBox(&dialog);
  ram_spin->setRange(8, 1024);
  ram_spin->setSingleStep(8);
  ram_spin->setValue(ram_mb_);
  layout->addRow("RAM (MiB)", ram_spin);

  std::array<QLineEdit *, 7> edits{};
  for (int target = 0; target < 7; ++target) {
    auto *row = new QWidget(&dialog);
    auto *row_layout = new QHBoxLayout(row);
    edits[target] = new QLineEdit(scsi_images_[target], row);
    auto *browse = new QPushButton("Browse", row);
    auto *remove = new QPushButton("Remove", row);
    row_layout->addWidget(edits[target]);
    row_layout->addWidget(browse);
    row_layout->addWidget(remove);
    layout->addRow(QString("SCSI %1").arg(target), row);
    connect(browse, &QPushButton::clicked, &dialog, [&, target]() {
      const QString path = QFileDialog::getOpenFileName(
          &dialog, QString("SCSI %1 image").arg(target), QString(),
          "Disk/CD images (*.raw *.chd *.img *.iso *.bin *.cue *.ccd *.mds "
          "*.mdf);;All files (*)");
      if (!path.isEmpty())
        edits[target]->setText(path);
    });
    connect(remove, &QPushButton::clicked, &dialog,
            [&, target]() { edits[target]->clear(); });
  }

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addRow(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted)
    return;

  const auto new_cpu_type =
      static_cast<o2emu::cpu::CPUType>(cpu_combo->currentData().toInt());
  const int new_ram_mb = ram_spin->value();
  const bool restart =
      cpu_ && (new_cpu_type != cpu_type_ || new_ram_mb != ram_mb_);
  cpu_type_ = new_cpu_type;
  ram_mb_ = new_ram_mb;
  for (int target = 0; target < 7; ++target) {
    scsi_images_[target] = edits[target]->text();
    if (!scsi_images_[target].isEmpty() &&
        !QFileInfo(scsi_images_[target]).isFile()) {
      QMessageBox::warning(
          this, "SCSI media",
          QString("SCSI %1 image does not exist.").arg(target));
      scsi_images_[target].clear();
    }
  }
  if (restart) {
    onStop();
    onStart();
  } else if (cpu_) {
    updateSlotConfiguration();
  }
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