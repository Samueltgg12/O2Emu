/**
 * @file main.cpp
 * @brief GUI entry point for O2Emu
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QMessageBox>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <iostream>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // Set Fusion style before creating QApplication
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  // Set up OpenGL surface format for framebuffer widget
  QSurfaceFormat format;
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setSamples(4); // MSAA
  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);
  app.setApplicationName("O2Emu");
  app.setApplicationVersion("0.1.0");
  app.setOrganizationName("O2Emu Project");
  app.setWindowIcon(QIcon(":/appicon.png"));

  // Command line parser
  QCommandLineParser parser;
  parser.setApplicationDescription("O2Emu - SGI O2 (IP32) Emulator");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption promOption(QStringList() << "p" << "prom",
                                "PROM image file", "file",
                                "samples/ip32prom.rev4.18.bin");
  parser.addOption(promOption);

  QCommandLineOption memoryOption(QStringList() << "m" << "memory",
                                  "RAM size in MB", "MB", "256");
  parser.addOption(memoryOption);

  QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                 "Enable debug logging");
  parser.addOption(debugOption);

  parser.process(app);

  // Create main window
  MainWindow window;

  // Apply command line options
  if (parser.isSet(promOption)) {
    window.setPromPath(parser.value(promOption));
  }

  if (parser.isSet(memoryOption)) {
    bool ok;
    int mb = parser.value(memoryOption).toInt(&ok);
    if (ok && mb > 0 && mb <= 1024) {
      window.setRamSize(mb);
    }
  }

  if (parser.isSet(debugOption)) {
    window.setDebugLogging(true);
  }

  window.show();

  return app.exec();
}