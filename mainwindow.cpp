#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pixelgridwidget.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QRegularExpression>
#include <QClipboard>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Если статусбар пуст — добавим метку
    if (!statusBar()) setStatusBar(new QStatusBar(this));
    auto *lbl = new QLabel(this);
    statusBar()->addPermanentWidget(lbl);
    lbl->setObjectName("lblStatus");

    // Значения по умолчанию (под ваш пример 2×26)
    ui->sbBytesPerRow->setValue(2);
    ui->sbRows->setValue(26);
    ui->cbMsbFirst->setChecked(true);
    ui->slCell->setValue(18);

    // Настроим сетку под спинбоксы
    ui->pixelGrid->setGridSize(ui->sbRows->value(), ui->sbBytesPerRow->value());
    ui->pixelGrid->setMsbFirst(ui->cbMsbFirst->isChecked());
    ui->pixelGrid->setCellSize(ui->slCell->value());

    // Связи
    connect(ui->btnApply,      &QPushButton::clicked, this, &MainWindow::applyGridFromControls);
    connect(ui->btnClear,      &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::clear);
    connect(ui->btnInvert,     &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::invert);
    connect(ui->btnShiftL,     &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftLeft);
    connect(ui->btnShiftR,     &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftRight);
    connect(ui->btnShiftU,     &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftUp);
    connect(ui->btnShiftD,     &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftDown);

    connect(ui->slCell,        &QSlider::valueChanged, ui->pixelGrid, &PixelGridWidget::setCellSize);
    connect(ui->cbMsbFirst,    &QCheckBox::toggled,    ui->pixelGrid, &PixelGridWidget::setMsbFirst);

    connect(ui->btnImportText, &QPushButton::clicked, this, &MainWindow::importFromText);
    connect(ui->btnOpen,       &QPushButton::clicked, this, &MainWindow::openFileDialog);

    // Вставка из буфера
    connect(ui->btnPaste,      &QPushButton::clicked, [this]{
        ui->teInput->setPlainText(QApplication::clipboard()->text());
        statusBar()->showMessage("Вставлено из буфера обмена", 1500);
    });

    connect(ui->btnExportC,     &QPushButton::clicked, this, &MainWindow::exportC);
    connect(ui->btnExportBytes, &QPushButton::clicked, this, &MainWindow::exportBytes);
    connect(ui->btnExportPy,    &QPushButton::clicked, this, &MainWindow::exportPy);
    connect(ui->btnCopyOut,     &QPushButton::clicked, [this]{
        QApplication::clipboard()->setText(ui->teOutput->toPlainText());
        statusBar()->showMessage("Скопировано в буфер обмена", 1500);
    });

    connect(ui->pixelGrid, &PixelGridWidget::changed, this, &MainWindow::updateStatus);
    updateStatus();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::applyGridFromControls() {
    int bpr = ui->sbBytesPerRow->value();
    int rows = ui->sbRows->value();
    ui->pixelGrid->setGridSize(rows, bpr);
    ui->pixelGrid->setMsbFirst(ui->cbMsbFirst->isChecked());
    updateStatus();
}

QVector<quint8> MainWindow::parseBytes(const QString& text) {
    // Уберём комментарии // до конца строки
    QString s = text;
    s.replace(QRegularExpression(R"(//[^\n\r]*)"), " ");
    // Ищем 0x.. или десятичные 0..255
    QRegularExpression rx(R"(0x([0-9A-Fa-f]{1,2})|\b(\d{1,3})\b)");
    QVector<quint8> out;
    auto it = rx.globalMatch(s);
    while (it.hasNext()) {
        auto m = it.next();
        bool ok=false; int v=0;
        if (m.captured(1).size()) v = m.captured(1).toInt(&ok,16);
        else                       v = m.captured(2).toInt(&ok,10);
        if (ok && v>=0 && v<=255) out.push_back(static_cast<quint8>(v));
    }
    return out;
}

void MainWindow::importFromText() {
    const int bpr  = ui->sbBytesPerRow->value();
    const bool msb = ui->cbMsbFirst->isChecked();
    QVector<quint8> bytes = parseBytes(ui->teInput->toPlainText());
    if (bytes.isEmpty()) {
        QMessageBox::warning(this, "Импорт", "Не найдено байтов в тексте.");
        return;
    }
    if (bytes.size() % bpr != 0) {
        QMessageBox::warning(this, "Импорт",
                             QString("Число байтов (%1) не кратно байтам на строку (%2).").arg(bytes.size()).arg(bpr));
        return;
    }
    int rows = bytes.size() / bpr;
    ui->sbRows->setValue(rows); // подгоняем высоту под факт
    if (!ui->pixelGrid->importBytes(bytes, bpr, msb)) {
        QMessageBox::warning(this, "Импорт", "Не удалось импортировать.");
        return;
    }
    updateStatus();
}

void MainWindow::openFileDialog() {
    QString fn = QFileDialog::getOpenFileName(this, "Открыть текст/заголовок", QString(),
                                              "Text/Headers (*.txt *.h *.hpp *.c *.cpp);;All files (*.*)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл.");
        return;
    }
    QTextStream ts(&f);
    const QString content = ts.readAll();
    ui->teInput->setPlainText(content);
    statusBar()->showMessage(QString("Загружено: %1 байт текста").arg(content.size()), 1500);
}

void MainWindow::exportC() {
    ui->teOutput->setPlainText(ui->pixelGrid->exportCWithAscii());
    statusBar()->showMessage("Экспорт: C + ASCII", 1500);
}

void MainWindow::exportBytes() {
    auto bytes = ui->pixelGrid->exportBytes();
    QStringList xs;
    xs.reserve(bytes.size());
    for (auto b : bytes) xs << QString("0x%1").arg(b,2,16,QLatin1Char('0')).toUpper();
    ui->teOutput->setPlainText(xs.join(", "));
    statusBar()->showMessage("Экспорт: только байты", 1500);
}

void MainWindow::exportPy() {
    auto bytes = ui->pixelGrid->exportBytes();
    QStringList xs;
    xs.reserve(bytes.size());
    for (auto b : bytes) xs << QString("0x%1").arg(b,2,16,QLatin1Char('0')).toUpper();
    ui->teOutput->setPlainText("[" + xs.join(", ") + "]");
    statusBar()->showMessage("Экспорт: Python list", 1500);
}

void MainWindow::updateStatus() {
    auto *lbl = statusBar()->findChild<QLabel*>("lblStatus");
    if (!lbl) return;
    lbl->setText(QString("%1×%2 бит, %3 байт/строка, MSB слева: %4, клетка: %5")
                     .arg(ui->pixelGrid->cols())
                     .arg(ui->pixelGrid->rows())
                     .arg(ui->pixelGrid->bytesPerRow())
                     .arg(ui->pixelGrid->msbFirst() ? "да" : "нет")
                     .arg(ui->pixelGrid->cellSize()));
}
