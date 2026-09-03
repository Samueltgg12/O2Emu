/**
 * @file debuggerwidget.cpp
 * @brief Debugger widget implementation
 */

#include "debuggerwidget.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <iomanip>
#include <sstream>

DebuggerWidget::DebuggerWidget(QWidget *parent) : QWidget(parent) {
  createUI();
}

DebuggerWidget::~DebuggerWidget() = default;

void DebuggerWidget::createUI() {
  QVBoxLayout *main_layout = new QVBoxLayout(this);

  // Control bar
  QHBoxLayout *control_layout = new QHBoxLayout();
  refresh_btn_ = new QPushButton("Refresh");
  step_btn_ = new QPushButton("Step");
  run_btn_ = new QPushButton("Run");
  break_btn_ = new QPushButton("Break");
  break_btn_->setEnabled(false);

  control_layout->addWidget(refresh_btn_);
  control_layout->addWidget(step_btn_);
  control_layout->addWidget(run_btn_);
  control_layout->addWidget(break_btn_);
  control_layout->addStretch();

  main_layout->addLayout(control_layout);

  // Tabs
  tabs_ = new QTabWidget();
  main_layout->addWidget(tabs_);

  // Registers tab
  QWidget *reg_tab = new QWidget();
  QVBoxLayout *reg_layout = new QVBoxLayout(reg_tab);
  reg_tree_ = new QTreeWidget();
  reg_tree_->setHeaderLabels({"Register", "Value (Hex)", "Value (Dec)"});
  reg_tree_->header()->setStretchLastSection(false);
  reg_tree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  reg_tree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  reg_tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  reg_layout->addWidget(reg_tree_);
  tabs_->addTab(reg_tab, "Registers");

  // Memory tab
  QWidget *mem_tab = new QWidget();
  QVBoxLayout *mem_layout = new QVBoxLayout(mem_tab);

  QHBoxLayout *mem_ctrl = new QHBoxLayout();
  mem_ctrl->addWidget(new QLabel("Address:"));
  mem_addr_edit_ = new QLineEdit("0x00000000");
  mem_addr_edit_->setMaximumWidth(150);
  mem_ctrl->addWidget(mem_addr_edit_);
  mem_ctrl->addStretch();
  mem_layout->addLayout(mem_ctrl);

  mem_table_ = new QTableWidget(16, 17); // 16 rows, 16 cols + address
  mem_table_->setHorizontalHeaderLabels({"Addr", "0", "1", "2", "3", "4", "5",
                                         "6", "7", "8", "9", "A", "B", "C", "D",
                                         "E", "F"});
  mem_table_->verticalHeader()->setVisible(false);
  mem_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  mem_table_->horizontalHeader()->setDefaultSectionSize(40);
  mem_layout->addWidget(mem_table_);
  tabs_->addTab(mem_tab, "Memory");

  // Disassembly tab
  QWidget *disasm_tab = new QWidget();
  QVBoxLayout *disasm_layout = new QVBoxLayout(disasm_tab);

  QHBoxLayout *disasm_ctrl = new QHBoxLayout();
  disasm_ctrl->addWidget(new QLabel("Address:"));
  disasm_addr_edit_ = new QLineEdit("0x00000000");
  disasm_addr_edit_->setMaximumWidth(150);
  disasm_ctrl->addWidget(disasm_addr_edit_);
  disasm_ctrl->addStretch();
  disasm_layout->addLayout(disasm_ctrl);

  disasm_table_ = new QTableWidget(0, 3);
  disasm_table_->setHorizontalHeaderLabels(
      {"Address", "Instruction", "Mnemonic"});
  disasm_table_->verticalHeader()->setVisible(false);
  disasm_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  disasm_table_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  disasm_table_->horizontalHeader()->setSectionResizeMode(2,
                                                          QHeaderView::Stretch);
  disasm_layout->addWidget(disasm_table_);
  tabs_->addTab(disasm_tab, "Disassembly");

  // Breakpoints tab
  QWidget *bp_tab = new QWidget();
  QVBoxLayout *bp_layout = new QVBoxLayout(bp_tab);

  QHBoxLayout *bp_ctrl = new QHBoxLayout();
  add_bp_btn_ = new QPushButton("Add Breakpoint");
  remove_bp_btn_ = new QPushButton("Remove");
  bp_ctrl->addWidget(add_bp_btn_);
  bp_ctrl->addWidget(remove_bp_btn_);
  bp_ctrl->addStretch();
  bp_layout->addLayout(bp_ctrl);

  bp_table_ = new QTableWidget(0, 3);
  bp_table_->setHorizontalHeaderLabels({"Address", "Enabled", "Condition"});
  bp_table_->verticalHeader()->setVisible(false);
  bp_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  bp_table_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  bp_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  bp_layout->addWidget(bp_table_);
  tabs_->addTab(bp_tab, "Breakpoints");

  // Connections
  connect(refresh_btn_, &QPushButton::clicked, this,
          &DebuggerWidget::onRefresh);
  connect(step_btn_, &QPushButton::clicked, this, &DebuggerWidget::onStep);
  connect(run_btn_, &QPushButton::clicked, this, &DebuggerWidget::onRun);
  connect(break_btn_, &QPushButton::clicked, this, &DebuggerWidget::onBreak);
  connect(mem_addr_edit_, &QLineEdit::returnPressed, this,
          &DebuggerWidget::onMemoryAddressChanged);
  connect(disasm_addr_edit_, &QLineEdit::returnPressed, this,
          &DebuggerWidget::onDisasmAddressChanged);
  connect(add_bp_btn_, &QPushButton::clicked, [this]() {
    bool ok;
    QString addr = QInputDialog::getText(
        this, "Add Breakpoint", "Address (hex):", QLineEdit::Normal, "0x", &ok);
    if (ok && !addr.isEmpty()) {
      int row = bp_table_->rowCount();
      bp_table_->insertRow(row);
      bp_table_->setItem(row, 0, new QTableWidgetItem(addr));
      bp_table_->setItem(row, 1, new QTableWidgetItem("Yes"));
      bp_table_->setItem(row, 2, new QTableWidgetItem(""));
    }
  });
  connect(remove_bp_btn_, &QPushButton::clicked, [this]() {
    int row = bp_table_->currentRow();
    if (row >= 0)
      bp_table_->removeRow(row);
  });
}

void DebuggerWidget::setCPU(o2emu::cpu::ICpu *cpu) { cpu_ = cpu; }

void DebuggerWidget::setMemory(o2emu::memory::Memory *memory) {
  memory_ = memory;
}

void DebuggerWidget::clear() {
  reg_tree_->clear();
  mem_table_->clearContents();
  disasm_table_->setRowCount(0);
  bp_table_->setRowCount(0);
}

void DebuggerWidget::update() {
  if (cpu_) {
    updateRegisters();
    updateDisassembly();
  }
  if (memory_) {
    updateMemory();
  }
}

void DebuggerWidget::onRefresh() { update(); }

void DebuggerWidget::onStep() {
  if (cpu_) {
    cpu_->step();
    update();
  }
}

void DebuggerWidget::onRun() {
  if (cpu_) {
    run_btn_->setEnabled(false);
    break_btn_->setEnabled(true);
    // TODO: Run in background thread
  }
}

void DebuggerWidget::onBreak() {
  run_btn_->setEnabled(true);
  break_btn_->setEnabled(false);
  // TODO: Signal CPU to break
}

void DebuggerWidget::onMemoryAddressChanged() { updateMemory(); }

void DebuggerWidget::onDisasmAddressChanged() { updateDisassembly(); }

void DebuggerWidget::updateRegisters() {
  if (!cpu_)
    return;

  reg_tree_->clear();

  // GPRs
  QTreeWidgetItem *gpr_root = new QTreeWidgetItem(reg_tree_, {"GPRs", "", ""});
  for (int i = 0; i < 32; ++i) {
    std::stringstream ss;
    ss << "$" << i;
    if (i == 0)
      ss << " (zero)";
    else if (i == 1)
      ss << " (at)";
    else if (i == 2)
      ss << " (v0)";
    else if (i == 3)
      ss << " (v1)";
    else if (i >= 4 && i <= 7)
      ss << " (a" << (i - 4) << ")";
    else if (i >= 8 && i <= 15)
      ss << " (t" << (i - 8) << ")";
    else if (i >= 16 && i <= 23)
      ss << " (s" << (i - 16) << ")";
    else if (i >= 24 && i <= 25)
      ss << " (t" << (i - 16) << ")";
    else if (i == 26)
      ss << " (k0)";
    else if (i == 27)
      ss << " (k1)";
    else if (i == 28)
      ss << " (gp)";
    else if (i == 29)
      ss << " (sp)";
    else if (i == 30)
      ss << " (fp/s8)";
    else if (i == 31)
      ss << " (ra)";

    QString name = QString::fromStdString(ss.str());
    uint32_t val = cpu_->gpr(i);
    QString hex = QString("0x%1").arg(val, 8, 16, QChar('0')).toUpper();
    QString dec = QString::number(val);

    new QTreeWidgetItem(gpr_root, {name, hex, dec});
  }
  gpr_root->setExpanded(true);

  // Special registers
  QTreeWidgetItem *special_root =
      new QTreeWidgetItem(reg_tree_, {"Special", "", ""});
  uint32_t pc_val = cpu_->pc();
  QString pc_hex = QString("0x%1").arg(pc_val, 8, 16, QChar('0')).toUpper();
  // HI/LO not directly accessible via ICpu, show as 0 for now
  QString hi_hex = "0x00000000";
  QString lo_hex = "0x00000000";
  new QTreeWidgetItem(special_root, {"PC", pc_hex, QString::number(pc_val)});
  new QTreeWidgetItem(special_root, {"HI", hi_hex, "0"});
  new QTreeWidgetItem(special_root, {"LO", lo_hex, "0"});
  special_root->setExpanded(true);

  // CP0 registers
  QTreeWidgetItem *cp0_root = new QTreeWidgetItem(reg_tree_, {"CP0", "", ""});
  uint32_t status = cpu_->cp0_reg(12);
  uint32_t cause = cpu_->cp0_reg(13);
  uint32_t epc = cpu_->cp0_reg(14);
  uint32_t badvaddr = cpu_->cp0_reg(8);
  uint32_t count = cpu_->cp0_reg(9);
  uint32_t compare = cpu_->cp0_reg(11);
  uint32_t config = cpu_->cp0_reg(16);
  uint32_t prid = cpu_->cp0_reg(15);

  QString status_hex = QString("0x%1").arg(status, 8, 16, QChar('0')).toUpper();
  QString cause_hex = QString("0x%1").arg(cause, 8, 16, QChar('0')).toUpper();
  QString epc_hex = QString("0x%1").arg(epc, 8, 16, QChar('0')).toUpper();
  QString badvaddr_hex =
      QString("0x%1").arg(badvaddr, 8, 16, QChar('0')).toUpper();
  QString count_hex = QString("0x%1").arg(count, 8, 16, QChar('0')).toUpper();
  QString compare_hex =
      QString("0x%1").arg(compare, 8, 16, QChar('0')).toUpper();
  QString config_hex = QString("0x%1").arg(config, 8, 16, QChar('0')).toUpper();
  QString prid_hex = QString("0x%1").arg(prid, 8, 16, QChar('0')).toUpper();

  new QTreeWidgetItem(cp0_root,
                      {"Status", status_hex, QString::number(status)});
  new QTreeWidgetItem(cp0_root, {"Cause", cause_hex, QString::number(cause)});
  new QTreeWidgetItem(cp0_root, {"EPC", epc_hex, QString::number(epc)});
  new QTreeWidgetItem(cp0_root,
                      {"BadVAddr", badvaddr_hex, QString::number(badvaddr)});
  new QTreeWidgetItem(cp0_root, {"Count", count_hex, QString::number(count)});
  new QTreeWidgetItem(cp0_root,
                      {"Compare", compare_hex, QString::number(compare)});
  new QTreeWidgetItem(cp0_root,
                      {"Config", config_hex, QString::number(config)});
  new QTreeWidgetItem(cp0_root, {"PRId", prid_hex, QString::number(prid)});
  cp0_root->setExpanded(true);

  // FPU registers
  QTreeWidgetItem *fpu_root = new QTreeWidgetItem(reg_tree_, {"FPU", "", ""});
  QString fcr0_hex =
      QString("0x%1").arg(state.fcr0, 8, 16, QChar('0')).toUpper();
  QString fcr31_hex =
      QString("0x%1").arg(state.fcr31, 8, 16, QChar('0')).toUpper();
  new QTreeWidgetItem(fpu_root,
                      {"FCR0", fcr0_hex, QString::number(state.fcr0)});
  new QTreeWidgetItem(fpu_root,
                      {"FCR31", fcr31_hex, QString::number(state.fcr31)});
  for (int i = 0; i < 32; ++i) {
    QString name = QString("FPR%1").arg(i);
    QString hex =
        QString("0x%1").arg(state.fpr_u[i], 8, 16, QChar('0')).toUpper();
    float fval = state.fpr_s[i];
    QString dec = QString::number(fval);
    new QTreeWidgetItem(fpu_root, {name, hex, dec});
  }
  fpu_root->setExpanded(false);
}

void DebuggerWidget::updateMemory() {
  if (!memory_)
    return;

  bool ok;
  u32 addr = mem_addr_edit_->text().toUInt(&ok, 0);
  if (!ok)
    addr = 0;

  // Align to 16 bytes
  addr &= ~0xF;

  for (int row = 0; row < 16; ++row) {
    u32 row_addr = addr + row * 16;

    // Address column
    QTableWidgetItem *addr_item = mem_table_->item(row, 0);
    if (!addr_item) {
      addr_item = new QTableWidgetItem();
      mem_table_->setItem(row, 0, addr_item);
    }
    addr_item->setText(
        QString("0x%1").arg(row_addr, 8, 16, QChar('0')).toUpper());
    addr_item->setFlags(addr_item->flags() & ~Qt::ItemIsEditable);

    // Data columns
    for (int col = 0; col < 16; ++col) {
      u32 byte_addr = row_addr + col;
      u8 value = memory_->read8(byte_addr);

      QTableWidgetItem *item = mem_table_->item(row, col + 1);
      if (!item) {
        item = new QTableWidgetItem();
        mem_table_->setItem(row, col + 1, item);
      }
      item->setText(QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      item->setTextAlignment(Qt::AlignCenter);
    }
  }
}

void DebuggerWidget::updateDisassembly() {
  if (!cpu_)
    return;

  bool ok;
  u32 addr = disasm_addr_edit_->text().toUInt(&ok, 0);
  if (!ok)
    addr = cpu_->state().pc;

  disasm_table_->setRowCount(0);

  for (int i = 0; i < 50; ++i) {
    char buffer[256];
    cpu_->disassemble(addr, buffer, sizeof(buffer));

    int row = disasm_table_->rowCount();
    disasm_table_->insertRow(row);

    QTableWidgetItem *addr_item = new QTableWidgetItem(
        QString("0x%1").arg(addr, 8, 16, QChar('0')).toUpper());
    addr_item->setFlags(addr_item->flags() & ~Qt::ItemIsEditable);
    disasm_table_->setItem(row, 0, addr_item);

    // Get raw instruction
    u32 instr = 0;
    if (memory_) {
      instr = memory_->read32(addr);
    }
    QTableWidgetItem *instr_item = new QTableWidgetItem(
        QString("0x%1").arg(instr, 8, 16, QChar('0')).toUpper());
    instr_item->setFlags(instr_item->flags() & ~Qt::ItemIsEditable);
    disasm_table_->setItem(row, 1, instr_item);

    QTableWidgetItem *mnemonic_item = new QTableWidgetItem(QString(buffer));
    mnemonic_item->setFlags(mnemonic_item->flags() & ~Qt::ItemIsEditable);
    disasm_table_->setItem(row, 2, mnemonic_item);

    // Highlight current PC
    if (addr == cpu_->state().pc) {
      for (int c = 0; c < 3; ++c) {
        disasm_table_->item(row, c)->setBackground(QBrush(Qt::yellow));
      }
    }

    addr += 4;
  }
}