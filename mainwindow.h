#pragma once
#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class PixelGridWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Глиф-редактор
    void applyGridFromControls();
    void importFromText();
    void openFileDialog();
    void exportC();
    void exportBytes();
    void exportPy();
    void importBmp();
    void exportBmp();
    void updateStatus();
    void resizeGridWidgetToHint();

    // Конвертор текста (вкладка)
    void onTextToHexChanged();   // plainTextEdit  -> plainTextEdit_2
    void onHexToTextChanged();   // plainTextEdit_2 -> plainTextEdit

private:
    Ui::MainWindow* ui;

    // Вспомогательное: парс/формат \xNN
    QString    bytesToEscapedHex(const QByteArray& bytes) const;
    QByteArray parseHexString(const QString& s) const;

    // Гард от рекурсий при взаимном обновлении полей
    bool m_convBusy = false;

    static QVector<quint8> parseBytes(const QString& text);
};
