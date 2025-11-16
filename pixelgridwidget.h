// pixelgridwidget.h
#pragma once
#include <QWidget>
#include <QBitArray>
#include <QImage>

/**
 * \brief Виджет редактирования пиксельной сетки глифа.
 * \details Хранит биты в QBitArray построчно, позволяет рисовать мышью,
 *          сдвигать, инвертировать, импортировать/экспортировать байты и изображения.
 */
class PixelGridWidget : public QWidget {
    Q_OBJECT
public:
    /** \brief Обычный конструктор Qt-виджета. */
    explicit PixelGridWidget(QWidget* parent = nullptr);

    /**
     * \brief Установить размер сетки, очистив содержимое.
     * \param rows        Количество строк (в пикселях).
     * \param bytesPerRow Байты в строке (ширина = bytesPerRow * 8).
     * \note Полностью сбрасывает текущий рисунок.
     */
    void setGridSize(int rows, int bytesPerRow);

    /**
     * \brief Изменить размер сетки с сохранением рисунка в общей области.
     * \param rows        Новое число строк.
     * \param bytesPerRow Новое число байт на строку.
     * \details Общая область (min(old,new)) копируется, добавленные зоны заполняются 0.
     */
    void resizeGridPreserve(int rows, int bytesPerRow);

    /// \name Параметры геометрии/состояния
    /// @{
    int rows()  const { return m_rows; }
    int cols()  const { return m_cols; }
    int bytesPerRow() const { return m_cols/8; }

    void setMsbFirst(bool v) { m_msbFirst = v; }
    bool msbFirst() const { return m_msbFirst; }

    void setCellSize(int px);
    int  cellSize() const { return m_cell; }
    /// @}

    /// \name Редактирование
    /// @{
    void clear();
    void invert();
    void shiftLeft();
    void shiftRight();
    void shiftUp();
    void shiftDown();
    /// @}

    /// \name Импорт/экспорт байтов и изображений
    /// @{
    bool importBytes(const QVector<quint8>& bytes, int bytesPerRow, bool msbFirst);
    QVector<quint8> exportBytes() const;
    QString exportCWithAscii() const;

    QImage toQImage() const;
    bool importFromImage(const QImage& src, bool autoResize = true, int threshold = 128, bool invert = false);
    /// @}

    /// \name Размерные подсказки для layout/scroll area
    /// @{
    QSize sizeHint() const override;
    /** \brief Минимальный хинт без виртуального вызова. */
    QSize minimumSizeHint() const override { return calcSizeHint(); }
    /// @}

signals:
    /** \brief Сигнал о том, что содержимое сетки изменилось. */
    void changed();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    /// \brief Прочитать бит пикселя (r,c).
    inline bool pixel(int r, int c) const { return m_bits.testBit(r*m_cols + c); }
    /// \brief Установить бит пикселя (r,c).
    inline void setPixel(int r, int c, bool on) { m_bits.setBit(r*m_cols + c, on); }
    /// \brief Преобразовать координату курсора в индексы ячейки.
    bool posToCell(const QPoint& p, int& r, int& c) const;

    /**
     * \brief Невиртуальный расчёт рекомендуемого размера виджета по текущей сетке.
     * \details Используем в местах, которые могут вызываться из конструктора,
     *          чтобы не трогать виртуальные методы.
     */
    QSize calcSizeHint() const;

    // --- Параметры сетки и хранение битов ---
    int       m_rows = 26;
    int       m_cols = 16; ///< 2 байта * 8 = 16 колонок по умолчанию
    int       m_cell = 18; ///< размер клетки в пикселях
    int       m_gap  = 1;  ///< зазор между клетками
    QBitArray m_bits;
    bool      m_msbFirst = true;

    // --- Временные состояния ввода мышью ---
    bool m_drag = false;
    bool m_drawValue = true;
};
