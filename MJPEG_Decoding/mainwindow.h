#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProcess>
#include <QTimer>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startFFmpeg();
    void captureFrame();
    void processOutput();

private:
    Ui::MainWindow *ui;
    QProcess *ffmpegProcess;
    QTimer *frameTimer;

};

#endif // MAINWINDOW_H
