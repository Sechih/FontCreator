#pragma once
#include <QMainWindow>
#include <QVector>

class QScrollArea;

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
    void applyGridFromControls();
    void importFromText();
    void openFileDialog();
    void exportC();
    void exportBytes();
    void exportPy();
    void updateStatus();

    // новое
    void importBmp();
    void exportBmp();
    void resizeGridWidgetToHint();

private:
    Ui::MainWindow* ui;
    static QVector<quint8> parseBytes(const QString& text);
};
