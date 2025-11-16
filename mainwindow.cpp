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
#include <QScrollArea>
#include <QImage>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!statusBar()) setStatusBar(new QStatusBar(this));
    auto *lbl = new QLabel(this);
    statusBar()->addPermanentWidget(lbl);
    lbl->setObjectName("lblStatus");

    // дефолты
    ui->sbBytesPerRow->setValue(2);
    ui->sbRows->setValue(26);
    ui->cbMsbFirst->setChecked(true);
    ui->slCell->setValue(18);

    // --- Делаем pixelGrid прямым widget() у saGrid ---
    ui->saGrid->setWidgetResizable(false);
    if (ui->saGrid->widget() != ui->pixelGrid) {
        QWidget* old = ui->saGrid->takeWidget(); // убрать auto-contents от Designer
        // вытащим сам pixelGrid из старого контейнера
        ui->pixelGrid->setParent(nullptr);
        ui->saGrid->setWidget(ui->pixelGrid);
        if (old && old != ui->pixelGrid) old->deleteLater(); // аккуратно убрать лишнее
    }
    ui->pixelGrid->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->pixelGrid->setMinimumSize(ui->pixelGrid->sizeHint());
    // --- конец блока скролла ---

    // первичная настройка сетки
    ui->pixelGrid->setGridSize(ui->sbRows->value(), ui->sbBytesPerRow->value());
    ui->pixelGrid->setMsbFirst(ui->cbMsbFirst->isChecked());
    ui->pixelGrid->setCellSize(ui->slCell->value());
    resizeGridWidgetToHint();

    // связи: редактор глифа
    connect(ui->btnApply,   &QPushButton::clicked, this, &MainWindow::applyGridFromControls);
    connect(ui->btnClear,   &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::clear);
    connect(ui->btnInvert,  &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::invert);
    connect(ui->btnShiftL,  &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftLeft);
    connect(ui->btnShiftR,  &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftRight);
    connect(ui->btnShiftU,  &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftUp);
    connect(ui->btnShiftD,  &QPushButton::clicked, ui->pixelGrid, &PixelGridWidget::shiftDown);

    connect(ui->cbMsbFirst, &QCheckBox::toggled,   ui->pixelGrid, &PixelGridWidget::setMsbFirst);
    connect(ui->slCell, &QSlider::valueChanged, this, [this](int){
        ui->pixelGrid->setCellSize(ui->slCell->value());
        resizeGridWidgetToHint();
        updateStatus();
    });

    connect(ui->btnImportText, &QPushButton::clicked, this, &MainWindow::importFromText);
    connect(ui->btnOpen,       &QPushButton::clicked, this, &MainWindow::openFileDialog);
    if (ui->btnPaste) {
        connect(ui->btnPaste, &QPushButton::clicked, this, [this]{
            ui->teInput->setPlainText(QApplication::clipboard()->text());
        });
    }

    connect(ui->btnExportC,     &QPushButton::clicked, this, &MainWindow::exportC);
    connect(ui->btnExportBytes, &QPushButton::clicked, this, &MainWindow::exportBytes);
    connect(ui->btnExportPy,    &QPushButton::clicked, this, &MainWindow::exportPy);
    connect(ui->btnCopyOut,     &QPushButton::clicked, [this]{
        QApplication::clipboard()->setText(ui->teOutput->toPlainText());
        statusBar()->showMessage("Скопировано в буфер обмена", 1500);
    });

    // изображения
    if (ui->btnImportBmp)
        connect(ui->btnImportBmp, &QPushButton::clicked, this, &MainWindow::importBmp);
    if (ui->btnExportBmp)
        connect(ui->btnExportBmp, &QPushButton::clicked, this, &MainWindow::exportBmp);

    // конвертор текста (вкладка)
    if (ui->plainTextEdit)
        connect(ui->plainTextEdit,   &QPlainTextEdit::textChanged, this, &MainWindow::onTextToHexChanged);
    if (ui->plainTextEdit_2)
        connect(ui->plainTextEdit_2, &QPlainTextEdit::textChanged, this, &MainWindow::onHexToTextChanged);

    connect(ui->pixelGrid, &PixelGridWidget::changed, this, &MainWindow::updateStatus);
    updateStatus();
}


MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::resizeGridWidgetToHint() {
    const QSize s = ui->pixelGrid->sizeHint();
    ui->pixelGrid->setMinimumSize(s);
    ui->pixelGrid->resize(s);
    ui->pixelGrid->updateGeometry();
}



void MainWindow::applyGridFromControls() {
    int bpr  = ui->sbBytesPerRow->value();
    int rows = ui->sbRows->value();
    ui->pixelGrid->resizeGridPreserve(rows, bpr);
    ui->pixelGrid->setMsbFirst(ui->cbMsbFirst->isChecked());
    resizeGridWidgetToHint();
    updateStatus();
}

/** \brief Парсер байтов из текста с поддержкой "0x"/"0X" и десятичного формата.
 *  \details Регистронезависимый матчинг префикса 0x/0X. Десятичные числа до 255.
 */
QVector<quint8> MainWindow::parseBytes(const QString& text) {
    QString s = text;
    s.replace(QRegularExpression(R"(//[^\n\r]*)"), " "); // убрать // комменты
    QRegularExpression rx(
        R"(0x([0-9A-Fa-f]{1,2})|\b(\d{1,3})\b)",
        QRegularExpression::CaseInsensitiveOption  // <<< важно: 0x/0X
        );
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
                             QString("Число байтов (%1) не кратно байтам на строку (%2).")
                                 .arg(bytes.size()).arg(bpr));
        return;
    }
    int rows = bytes.size() / bpr;
    ui->sbRows->setValue(rows);
    if (!ui->pixelGrid->importBytes(bytes, bpr, msb)) {
        QMessageBox::warning(this, "Импорт", "Не удалось импортировать.");
        return;
    }
    resizeGridWidgetToHint();
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
/** \brief Экспорт только байтов в стиле "0xFE, 0x07, ..." (0x — нижний регистр). */
void MainWindow::exportBytes() {
    auto bytes = ui->pixelGrid->exportBytes();
    QStringList xs;
    xs.reserve(bytes.size());
    for (auto b : bytes) {
        const QString hex = QString::number(b, 16).rightJustified(2, QLatin1Char('0')).toUpper();
        xs << QString("0x%1").arg(hex);  // "0x" остаётся нижним, цифры — верхние
    }
    ui->teOutput->setPlainText(xs.join(", "));
    statusBar()->showMessage("Экспорт: только байты", 1500);
}

/** \brief Экспорт Python-списка байтов в стиле ["0xFE", ...] с 0x в нижнем регистре. */
void MainWindow::exportPy() {
    auto bytes = ui->pixelGrid->exportBytes();
    QStringList xs;
    xs.reserve(bytes.size());
    for (auto b : bytes) {
        const QString hex = QString::number(b, 16).rightJustified(2, QLatin1Char('0')).toUpper();
        xs << QString("0x%1").arg(hex);
    }
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

void MainWindow::importBmp() {
    const QString fn = QFileDialog::getOpenFileName(
        this, "Импорт изображения", QString(),
        "Images (*.bmp *.png *.jpg *.jpeg *.gif);;All files (*.*)");
    if (fn.isEmpty()) return;

    QImage img(fn);
    if (img.isNull()) {
        QMessageBox::warning(this, "Импорт", "Не удалось открыть изображение.");
        return;
    }

    if (!ui->pixelGrid->importFromImage(img, /*autoResize=*/true, /*threshold=*/128, /*invert=*/false)) {
        QMessageBox::warning(this, "Импорт", "Не удалось импортировать картинку.");
        return;
    }
    resizeGridWidgetToHint();
    updateStatus();
}

void MainWindow::exportBmp() {
    const QString fn = QFileDialog::getSaveFileName(
        this, "Экспорт BMP", "glyph.bmp", "BMP image (*.bmp)");
    if (fn.isEmpty()) return;

    QImage img = ui->pixelGrid->toQImage();
    if (!img.save(fn)) {
        QMessageBox::warning(this, "Экспорт", "Не удалось сохранить BMP.");
        return;
    }
    statusBar()->showMessage(QString("Сохранено: %1").arg(fn), 1500);
}

/** \brief Преобразовать байты в строку вида "\xNN\xAB..." (префикс \x — нижний регистр). */
QString MainWindow::bytesToEscapedHex(const QByteArray& bytes) const {
    QString out;
    out.reserve(bytes.size() * 4);
    for (unsigned char b : bytes) {
        const QString hex = QString::number(int(b), 16).rightJustified(2, QLatin1Char('0')).toUpper();
        out += "\\x" + hex;  // \x остаётся нижним, цифры — верхние
    }
    return out;
}

/** \brief Спарсить строку с \xNN / 0xNN / NN (регистронезависимо для \x/0x). */
QByteArray MainWindow::parseHexString(const QString& s) const {
    QByteArray res;
    QRegularExpression rx(
        R"(\\x([0-9A-Fa-f]{2})|0x([0-9A-Fa-f]{2})|([0-9A-Fa-f]{2}))",
        QRegularExpression::CaseInsensitiveOption   // <<< важно: \x/0x и \X/0X
        );
    auto it = rx.globalMatch(s);
    while (it.hasNext()) {
        auto m = it.next();
        QString hh = !m.captured(1).isEmpty() ? m.captured(1)
                     : !m.captured(2).isEmpty() ? m.captured(2)
                                                : m.captured(3);
        bool ok = false;
        int v = hh.toInt(&ok, 16);
        if (ok) res.append(char(v));
    }
    return res;
}

void MainWindow::onTextToHexChanged() {
    if (m_convBusy) return;
    if (!ui->plainTextEdit || !ui->plainTextEdit_2) return;
    m_convBusy = true;
    const QByteArray bytes = ui->plainTextEdit->toPlainText().toLocal8Bit(); // CP1251 на русской Windows
    const QString hex = bytesToEscapedHex(bytes);
    ui->plainTextEdit_2->blockSignals(true);
    ui->plainTextEdit_2->setPlainText(hex);
    ui->plainTextEdit_2->blockSignals(false);
    m_convBusy = false;
}

void MainWindow::onHexToTextChanged() {
    if (m_convBusy) return;
    if (!ui->plainTextEdit || !ui->plainTextEdit_2) return;
    m_convBusy = true;
    const QByteArray bytes = parseHexString(ui->plainTextEdit_2->toPlainText());
    const QString txt = QString::fromLocal8Bit(bytes); // обратно из CP1251 (или локальной ANSI) в Unicode
    ui->plainTextEdit->blockSignals(true);
    ui->plainTextEdit->setPlainText(txt);
    ui->plainTextEdit->blockSignals(false);
    m_convBusy = false;
}
