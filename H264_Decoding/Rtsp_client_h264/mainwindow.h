#ifndef STREAMING_H
#define STREAMING_H

#include <QWidget>
#include <QMainWindow>
#include <QLabel>
#include <QProcess>
#include <QTimer>
#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QProcess *ffmpegProcess;
    QTimer *frameTimer;
    QString rtspUrl;  // RTSP 서버 주소
    QByteArray *buffer;
    QByteArray incompleteBuffer; // 12.06 추가 : 중요!

private slots:
    // UI 업데이트
    void updateDateTime();

    // RTSP 및 FFmpeg 관련
    void setupAddressInput();
    bool eventFilter(QObject *watched, QEvent *event);
    void startFFmpeg();
    void stopFFmpeg();
    void captureFrame();
    void processOutput();

    // RTSP GUI 설정
    void on_setButton_clicked();    //RTSP IP주소 설정
    void rtsp_setting();            //
    void on_startButton_clicked();
    void on_stopButton_clicked();

    void setButtonStyle(QPushButton* button, bool isActive);
};

#endif // STREAMING_H
