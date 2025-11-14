#pragma once
#include <QWidget>
#include <QBitArray>

class PixelGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit PixelGridWidget(QWidget* parent = nullptr);

    void setGridSize(int rows, int bytesPerRow); // обнулит сетку
    int rows()  const { return m_rows; }
    int cols()  const { return m_cols; }
    int bytesPerRow() const { return m_cols/8; }

    void setMsbFirst(bool v) { m_msbFirst = v; } // влияет на импорт/экспорт
    bool msbFirst() const { return m_msbFirst; }

    void setCellSize(int px);
    int  cellSize() const { return m_cell; }

    void clear();
    void invert();
    void shiftLeft();
    void shiftRight();
    void shiftUp();
    void shiftDown();

    bool importBytes(const QVector<quint8>& bytes, int bytesPerRow, bool msbFirst);
    QVector<quint8> exportBytes() const;
    QString exportCWithAscii() const; // "0x.., 0x..,  // ####  ..."

signals:
    void changed(); // пиксели/размер обновились

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

private:
    inline bool pixel(int r, int c) const { return m_bits.testBit(r*m_cols + c); }
    inline void setPixel(int r, int c, bool on) { m_bits.setBit(r*m_cols + c, on); }

    bool posToCell(const QPoint& p, int& r, int& c) const;

    int m_rows = 26;
    int m_cols = 16; // 2 байта * 8
    int m_cell = 18;
    int m_gap  = 1;
    QBitArray m_bits;
    bool m_msbFirst = true;

    bool m_drag = false;
    bool m_drawValue = true; // во время драга ставим 1 или 0
};
