/**
 * @file debuggerwidget.h
 * @brief Debugger/inspector widget
 */

#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>
#include <memory>

namespace o2emu::cpu {
class ICpu;
}
namespace o2emu::memory {
class Memory;
}

class DebuggerWidget : public QWidget {
  Q_OBJECT

public:
  DebuggerWidget(QWidget *parent = nullptr);
  ~DebuggerWidget() override;

  void setCPU(o2emu::cpu::ICpu *cpu);
  void setMemory(o2emu::memory::Memory *memory);
  void clear();
  void update();

private slots:
  void onRefresh();
  void onStep();
  void onRun();
  void onBreak();
  void onMemoryAddressChanged();
  void onDisasmAddressChanged();

private:
  void createUI();
  void updateRegisters();
  void updateMemory();
  void updateDisassembly();
  void updateBreakpoints();

  o2emu::cpu::ICpu *cpu_ = nullptr;
  o2emu::memory::Memory *memory_ = nullptr;

  // UI components
  QTabWidget *tabs_ = nullptr;

  // Registers tab
  QTreeWidget *reg_tree_ = nullptr;

  // Memory tab
  QLineEdit *mem_addr_edit_ = nullptr;
  QTableWidget *mem_table_ = nullptr;

  // Disassembly tab
  QLineEdit *disasm_addr_edit_ = nullptr;
  QTableWidget *disasm_table_ = nullptr;

  // Breakpoints tab
  QTableWidget *bp_table_ = nullptr;
  QPushButton *add_bp_btn_ = nullptr;
  QPushButton *remove_bp_btn_ = nullptr;

  // Control buttons
  QPushButton *refresh_btn_ = nullptr;
  QPushButton *step_btn_ = nullptr;
  QPushButton *run_btn_ = nullptr;
  QPushButton *break_btn_ = nullptr;
};