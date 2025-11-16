// pixelgridwidget.cpp
#include "pixelgridwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QImage>
#include <algorithm>

/** \brief Конструктор: включаем трекинг мыши и инициализируем сетку. */
PixelGridWidget::PixelGridWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    // Важно: setGridSize внутри не вызывает виртуальные методы (используем calcSizeHint()).
    setGridSize(m_rows, m_cols/8);
}

/** \brief Невиртуальный расчёт рекомендуемого размера (учитывает клетки и зазоры). */
QSize PixelGridWidget::calcSizeHint() const {
    int w = m_cols * (m_cell + m_gap) + m_gap;
    int h = m_rows * (m_cell + m_gap) + m_gap;
    return { std::max(200, w), std::max(150, h) };
}

/** \brief Виртуальный sizeHint(), делегирует в невиртуальный расчёт. */
QSize PixelGridWidget::sizeHint() const {
    return calcSizeHint();
}

/** \brief Установить новый размер сетки, полностью очистив содержимое. */
void PixelGridWidget::setGridSize(int rows, int bytesPerRow) {
    m_rows = std::max(1, rows);
    m_cols = std::max(1, bytesPerRow) * 8;

    m_bits.resize(m_rows * m_cols);
    m_bits.fill(false);

    updateGeometry();
    setMinimumSize(calcSizeHint());   // не трогаем виртуальные методы
    update();
    emit changed();
}

/** \brief Изменить размер сетки с сохранением пересекаемой области. */
void PixelGridWidget::resizeGridPreserve(int rows, int bytesPerRow) {
    const int newRows = std::max(1, rows);
    const int newCols = std::max(1, bytesPerRow) * 8;

    if (newRows == m_rows && newCols == m_cols)
        return;

    const int oldRows = m_rows;
    const int oldCols = m_cols;
    const QBitArray oldBits = m_bits;

    m_rows = newRows;
    m_cols = newCols;

    QBitArray newBits(m_rows * m_cols);
    newBits.fill(false);

    const int copyRows = std::min(oldRows, m_rows);
    const int copyCols = std::min(oldCols, m_cols);
    for (int r = 0; r < copyRows; ++r)
        for (int c = 0; c < copyCols; ++c)
            newBits.setBit(r * m_cols + c, oldBits.testBit(r * oldCols + c));

    m_bits = std::move(newBits);

    updateGeometry();
    setMinimumSize(calcSizeHint());
    update();
    emit changed();
}

/** \brief Установить размер клетки и обновить геометрию. */
void PixelGridWidget::setCellSize(int px) {
    m_cell = std::clamp(px, 6, 64);
    updateGeometry();
    setMinimumSize(calcSizeHint());
    update();
}

/** \brief Очистить всю сетку. */
void PixelGridWidget::clear() {
    m_bits.fill(false);
    update();
    emit changed();
}

/** \brief Инвертировать все пиксели. */
void PixelGridWidget::invert() {
    for (int i = 0; i < m_bits.size(); ++i)
        m_bits.setBit(i, !m_bits.testBit(i));
    update();
    emit changed();
}

/** \brief Сдвинуть изображение влево на 1 пиксель. */
void PixelGridWidget::shiftLeft() {
    for (int r = 0; r < m_rows; ++r) {
        int base = r * m_cols;
        for (int c = 0; c < m_cols - 1; ++c)
            m_bits.setBit(base + c, m_bits.testBit(base + c + 1));
        m_bits.clearBit(base + m_cols - 1);
    }
    update();
    emit changed();
}

/** \brief Сдвинуть изображение вправо на 1 пиксель. */
void PixelGridWidget::shiftRight() {
    for (int r = 0; r < m_rows; ++r) {
        int base = r * m_cols;
        for (int c = m_cols - 1; c > 0; --c)
            m_bits.setBit(base + c, m_bits.testBit(base + c - 1));
        m_bits.clearBit(base);
    }
    update();
    emit changed();
}

/** \brief Сдвинуть изображение вверх на 1 пиксель. */
void PixelGridWidget::shiftUp() {
    for (int r = 0; r < m_rows - 1; ++r)
        for (int c = 0; c < m_cols; ++c)
            m_bits.setBit(r * m_cols + c, m_bits.testBit((r + 1) * m_cols + c));
    for (int c = 0; c < m_cols; ++c)
        m_bits.clearBit((m_rows - 1) * m_cols + c);
    update();
    emit changed();
}

/** \brief Сдвинуть изображение вниз на 1 пиксель. */
void PixelGridWidget::shiftDown() {
    for (int r = m_rows - 1; r > 0; --r)
        for (int c = 0; c < m_cols; ++c)
            m_bits.setBit(r * m_cols + c, m_bits.testBit((r - 1) * m_cols + c));
    for (int c = 0; c < m_cols; ++c)
        m_bits.clearBit(c);
    update();
    emit changed();
}

/**
 * \brief Импорт массива байтов в сетку.
 * \param bytes     Последовательность байтов.
 * \param bpr       Байты на строку.
 * \param msbFirst  true — старший бит слева; false — младший слева.
 * \return Успех/неуспех.
 */
bool PixelGridWidget::importBytes(const QVector<quint8>& bytes, int bpr, bool msbFirst) {
    if (bpr <= 0 || bytes.isEmpty() || (bytes.size() % bpr) != 0)
        return false;

    const int h = bytes.size() / bpr;
    setGridSize(h, bpr);
    m_msbFirst = msbFirst;

    int idx = 0;
    for (int r = 0; r < m_rows; ++r) {
        for (int b = 0; b < bpr; ++b) {
            quint8 V = bytes[idx++];
            if (m_msbFirst) {
                for (int bit = 7; bit >= 0; --bit) {
                    int col = b * 8 + (7 - bit);
                    setPixel(r, col, (V >> bit) & 1);
                }
            } else {
                for (int bit = 0; bit < 8; ++bit) {
                    int col = b * 8 + bit;
                    setPixel(r, col, (V >> bit) & 1);
                }
            }
        }
    }

    setMinimumSize(calcSizeHint());
    update();
    emit changed();
    return true;
}

/** \brief Экспорт текущей сетки в массив байтов. */
QVector<quint8> PixelGridWidget::exportBytes() const {
    const int bpr = m_cols / 8;
    QVector<quint8> out;
    out.resize(m_rows * bpr);

    int idx = 0;
    for (int r = 0; r < m_rows; ++r) {
        for (int b = 0; b < bpr; ++b) {
            quint8 V = 0;
            for (int x = 0; x < 8; ++x) {
                const bool pix = pixel(r, b * 8 + x);
                if (m_msbFirst)
                    V |= (pix ? 1 : 0) << (7 - x);
                else
                    V |= (pix ? 1 : 0) << x;
            }
            out[idx++] = V;
        }
    }
    return out;
}

/**
 * \brief Экспорт в “C + ASCII” (список 0xNN и справа рисунок из '#').
 * \details Префикс всегда `0x` (нижний регистр), гекс-цифры — верхний регистр.
 */
QString PixelGridWidget::exportCWithAscii() const {
    QString out;
    const int bpr = m_cols / 8;

    for (int r = 0; r < m_rows; ++r) {
        QStringList hexes;
        hexes.reserve(bpr);

        for (int b = 0; b < bpr; ++b) {
            quint8 V = 0;
            for (int x = 0; x < 8; ++x) {
                const bool pix = pixel(r, b * 8 + x);
                if (m_msbFirst) V |= (pix ? 1 : 0) << (7 - x);
                else            V |= (pix ? 1 : 0) << x;
            }
            const QString hx = QString::number(V, 16).rightJustified(2, QLatin1Char('0')).toUpper();
            hexes << ("0x" + hx); // "0x" — нижний, цифры — верхние
        }

        QString art;
        art.reserve(m_cols);
        for (int c = 0; c < m_cols; ++c)
            art += pixel(r, c) ? QLatin1Char('#') : QLatin1Char(' ');

        // clazy: используем многоаргументную перегрузку вместо .arg(...).arg(...)
        out += QStringLiteral("    %1,  // %2\n").arg(hexes.join(", "), art);
    }
    return out;
}

/** \brief Сконвертировать в QImage (чёрный/белый). */
QImage PixelGridWidget::toQImage() const {
    QImage img(m_cols, m_rows, QImage::Format_ARGB32);
    img.fill(Qt::white);
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            if (pixel(r, c))
                img.setPixelColor(c, r, Qt::black);
    return img;
}

/**
 * \brief Импорт из изображения.
 * \param src         Исходное изображение.
 * \param autoResize  true — подогнать сетку под размеры картинки.
 * \param threshold   Порог бинаризации (0..255): <threshold → чёрный пиксель.
 * \param invert      Инвертировать после бинаризации.
 */
bool PixelGridWidget::importFromImage(const QImage& src, bool autoResize, int threshold, bool invert) {
    if (src.isNull()) return false;

    QImage g = src.convertToFormat(QImage::Format_Grayscale8);
    const int w = g.width();
    const int h = g.height();

    if (autoResize) {
        const int bpr = (w + 7) / 8;
        setGridSize(h, bpr);
    }

    const int useW = std::min(m_cols, w);
    const int useH = std::min(m_rows, h);

    clear();

    for (int y = 0; y < useH; ++y) {
        for (int x = 0; x < useW; ++x) {
            const int gray = qGray(g.pixel(x, y));
            bool on = (gray < threshold);
            if (invert) on = !on;
            setPixel(y, x, on);
        }
    }

    setMinimumSize(calcSizeHint());
    update();
    emit changed();
    return true;
}

/** \brief Отрисовка сетки и клеток. */
void PixelGridWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    const int cw = m_cell, ch = m_cell;
    const int gap = m_gap;

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            const int x = gap + c * (cw + gap);
            const int y = gap + r * (ch + gap);
            const QRect cell(x, y, cw, ch);
            p.fillRect(cell, pixel(r, c) ? Qt::black : Qt::white);
            p.setPen(QColor(220, 220, 220));
            p.drawRect(cell);
        }
    }
}

/** \brief Координаты мыши → индексы клетки. */
bool PixelGridWidget::posToCell(const QPoint& pt, int& r, int& c) const {
    const int cw = m_cell, ch = m_cell, gap = m_gap;
    const int col = (pt.x() - gap) / (cw + gap);
    const int row = (pt.y() - gap) / (ch + gap);
    if (row < 0 || col < 0 || row >= m_rows || col >= m_cols)
        return false;
    r = row;
    c = col;
    return true;
}

/** \brief Нажатие мыши — начинаем рисование/стирание. */
void PixelGridWidget::mousePressEvent(QMouseEvent* e) {
    int r, c;
    if (!posToCell(e->pos(), r, c)) return;
    if (e->button() == Qt::LeftButton)  m_drawValue = true;
    if (e->button() == Qt::RightButton) m_drawValue = false;
    setPixel(r, c, m_drawValue);
    m_drag = true;
    update();
    emit changed();
}

/** \brief Перемещение мыши при зажатой кнопке — продолжаем рисование. */
void PixelGridWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!m_drag) return;
    int r, c;
    if (!posToCell(e->pos(), r, c)) return;
    setPixel(r, c, m_drawValue);
    update();
}

/** \brief Отпускание кнопки мыши — завершаем рисование. */
void PixelGridWidget::mouseReleaseEvent(QMouseEvent*) {
    if (m_drag) {
        m_drag = false;
        emit changed();
    }
}
