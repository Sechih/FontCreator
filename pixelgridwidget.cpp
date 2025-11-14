#include "pixelgridwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QImage>
#include <algorithm>

PixelGridWidget::PixelGridWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setGridSize(m_rows, m_cols/8);
}

void PixelGridWidget::setGridSize(int rows, int bytesPerRow) {
    m_rows = std::max(1, rows);
    m_cols = std::max(1, bytesPerRow) * 8;
    m_bits.resize(m_rows * m_cols);
    m_bits.fill(false);
    updateGeometry();
    setMinimumSize(sizeHint());
    update();
    emit changed();
}

void PixelGridWidget::setCellSize(int px) {
    m_cell = std::clamp(px, 6, 64);
    updateGeometry();
    setMinimumSize(sizeHint());
    update();
}

void PixelGridWidget::clear() {
    m_bits.fill(false);
    update();
    emit changed();
}

void PixelGridWidget::invert() {
    for (int i=0;i<m_bits.size();++i)
        m_bits.setBit(i, !m_bits.testBit(i));
    update();
    emit changed();
}

void PixelGridWidget::shiftLeft() {
    for (int r=0;r<m_rows;++r) {
        int base = r*m_cols;
        for (int c=0;c<m_cols-1;++c)
            m_bits.setBit(base+c, m_bits.testBit(base+c+1));
        m_bits.clearBit(base + m_cols-1);
    }
    update(); emit changed();
}
void PixelGridWidget::shiftRight() {
    for (int r=0;r<m_rows;++r) {
        int base = r*m_cols;
        for (int c=m_cols-1;c>0;--c)
            m_bits.setBit(base+c, m_bits.testBit(base+c-1));
        m_bits.clearBit(base);
    }
    update(); emit changed();
}
void PixelGridWidget::shiftUp() {
    for (int r=0;r<m_rows-1;++r)
        for (int c=0;c<m_cols;++c)
            m_bits.setBit(r*m_cols+c, m_bits.testBit((r+1)*m_cols+c));
    for (int c=0;c<m_cols;++c) m_bits.clearBit((m_rows-1)*m_cols + c);
    update(); emit changed();
}
void PixelGridWidget::shiftDown() {
    for (int r=m_rows-1;r>0;--r)
        for (int c=0;c<m_cols;++c)
            m_bits.setBit(r*m_cols+c, m_bits.testBit((r-1)*m_cols+c));
    for (int c=0;c<m_cols;++c) m_bits.clearBit(c);
    update(); emit changed();
}

bool PixelGridWidget::importBytes(const QVector<quint8>& bytes, int bpr, bool msbFirst) {
    if (bpr <= 0 || bytes.isEmpty() || (bytes.size() % bpr) != 0)
        return false;
    int h = bytes.size() / bpr;
    setGridSize(h, bpr);
    m_msbFirst = msbFirst;

    int idx = 0;
    for (int r=0; r<m_rows; ++r) {
        for (int b=0; b<bpr; ++b) {
            quint8 V = bytes[idx++];
            if (m_msbFirst) {
                for (int bit=7; bit>=0; --bit) {
                    int col = b*8 + (7 - bit);
                    setPixel(r, col, (V >> bit) & 1);
                }
            } else {
                for (int bit=0; bit<8; ++bit) {
                    int col = b*8 + bit;
                    setPixel(r, col, (V >> bit) & 1);
                }
            }
        }
    }
    setMinimumSize(sizeHint());
    update(); emit changed();
    return true;
}

QVector<quint8> PixelGridWidget::exportBytes() const {
    int bpr = m_cols / 8;
    QVector<quint8> out;
    out.resize(m_rows * bpr);
    int idx = 0;
    for (int r=0; r<m_rows; ++r) {
        for (int b=0; b<bpr; ++b) {
            quint8 V = 0;
            for (int x=0; x<8; ++x) {
                bool pix = pixel(r, b*8 + x);
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

QString PixelGridWidget::exportCWithAscii() const {
    QString out;
    int bpr = m_cols/8;
    for (int r=0; r<m_rows; ++r) {
        QStringList hexes;
        for (int b=0; b<bpr; ++b) {
            quint8 V = 0;
            for (int x=0; x<8; ++x) {
                bool pix = pixel(r, b*8 + x);
                if (m_msbFirst) V |= (pix ? 1 : 0) << (7 - x);
                else            V |= (pix ? 1 : 0) << x;
            }
            hexes << QString("0x%1").arg(V, 2, 16, QLatin1Char('0')).toUpper();
        }
        QString art;
        art.reserve(m_cols);
        for (int c=0;c<m_cols;++c) art += pixel(r,c) ? QLatin1Char('#') : QLatin1Char(' ');
        out += QString("    %1,  // %2\n").arg(hexes.join(", ")).arg(art);
    }
    return out;
}

QImage PixelGridWidget::toQImage() const {
    QImage img(m_cols, m_rows, QImage::Format_ARGB32);
    img.fill(Qt::white);
    for (int r=0; r<m_rows; ++r)
        for (int c=0; c<m_cols; ++c)
            if (pixel(r,c))
                img.setPixelColor(c, r, Qt::black);
    return img;
}

bool PixelGridWidget::importFromImage(const QImage& src, bool autoResize, int threshold, bool invert) {
    if (src.isNull()) return false;

    QImage g = src.convertToFormat(QImage::Format_Grayscale8);
    int w = g.width();
    int h = g.height();

    if (autoResize) {
        int bpr = (w + 7) / 8;
        setGridSize(h, bpr);
    }
    int useW = std::min(m_cols, w);
    int useH = std::min(m_rows, h);

    clear();

    for (int y=0; y<useH; ++y) {
        for (int x=0; x<useW; ++x) {
            int gray = qGray(g.pixel(x, y));
            bool on = (gray < threshold);
            if (invert) on = !on;
            setPixel(y, x, on);
        }
    }
    update(); emit changed();
    return true;
}

QSize PixelGridWidget::sizeHint() const {
    int w = m_cols * (m_cell + m_gap) + m_gap;
    int h = m_rows * (m_cell + m_gap) + m_gap;
    return { std::max(200, w), std::max(150, h) };
}

void PixelGridWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    const int cw = m_cell, ch = m_cell;
    const int gap = m_gap;

    for (int r=0; r<m_rows; ++r) {
        for (int c=0; c<m_cols; ++c) {
            int x = gap + c*(cw+gap);
            int y = gap + r*(ch+gap);
            QRect cell(x, y, cw, ch);
            p.fillRect(cell, pixel(r,c) ? Qt::black : Qt::white);
            p.setPen(QColor(220,220,220));
            p.drawRect(cell);
        }
    }
}

bool PixelGridWidget::posToCell(const QPoint& pt, int& r, int& c) const {
    int cw = m_cell, ch = m_cell, gap = m_gap;
    int col = (pt.x() - gap) / (cw + gap);
    int row = (pt.y() - gap) / (ch + gap);
    if (row < 0 || col < 0 || row >= m_rows || col >= m_cols)
        return false;
    r = row; c = col; return true;
}

void PixelGridWidget::mousePressEvent(QMouseEvent* e) {
    int r,c;
    if (!posToCell(e->pos(), r, c)) return;
    if (e->button() == Qt::LeftButton)  m_drawValue = true;
    if (e->button() == Qt::RightButton) m_drawValue = false;
    setPixel(r, c, m_drawValue);
    m_drag = true;
    update(); emit changed();
}

void PixelGridWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!m_drag) return;
    int r,c;
    if (!posToCell(e->pos(), r, c)) return;
    setPixel(r, c, m_drawValue);
    update();
}

void PixelGridWidget::mouseReleaseEvent(QMouseEvent*) {
    if (m_drag) { m_drag = false; emit changed(); }
}
