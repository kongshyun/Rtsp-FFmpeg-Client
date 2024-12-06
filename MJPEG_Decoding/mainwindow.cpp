#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <QLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      ffmpegProcess(new QProcess(this)),
      frameTimer(new QTimer(this)) {
    ui->setupUi(this);

    // 버튼 클릭으로 스트리밍 시작
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startFFmpeg);

    // 프레임 캡처를 위한 타이머 설정
    connect(frameTimer, &QTimer::timeout, this, &MainWindow::captureFrame);
}

MainWindow::~MainWindow() {
    delete ui;
    if (ffmpegProcess->state() == QProcess::Running) {
        ffmpegProcess->terminate();
        ffmpegProcess->waitForFinished();
    }
}

void MainWindow::startFFmpeg() {
    QString ffmpegPath = "/bin/ffmpeg"; // FFmpeg 실행 경로
    QStringList arguments = {
        "-fflags", "discardcorrupt",    // 손상된 프레임 무시
        "-i", "rtsp://192.168.0.39:8554/stream1", // RTSP URL
        "-vf", "fps=25",                    // 프레임 속도 설정
        "-s", "640x360",                         // 해상도 설정
        "-f", "image2pipe",                 // 이미지 파이프 형식
        "-vcodec", "mjpeg",                 // MJPEG 형식으로 디코딩
        "-q:v", "2",    // 품질 설정 (1~31, 낮을수록 품질이 좋음)
        "-pix_fmt", "yuvj420p",  // 색 공간 최적화
        "pipe:1"                            // 출력 파이프
    };

    ffmpegProcess->start(ffmpegPath, arguments);

    if (!ffmpegProcess->waitForStarted()) {
        qDebug() << "FFmpeg failed to start!";
        return;
    }

    frameTimer->start(40); // 25fps (1000ms / 25 = 40ms)
}

void MainWindow::captureFrame() {
    if (!ffmpegProcess->isReadable()) return;

    QByteArray frameData = ffmpegProcess->readAllStandardOutput();
    if (frameData.isEmpty()) return;

    // MJPEG 이미지를 QPixmap으로 변환
    QPixmap pixmap;
    if (!pixmap.loadFromData(frameData, "JPEG")) {
        qDebug() << "Failed to load frame data!";
        return;
    }

    QPixmap scaledPixmap = pixmap.scaled(
        ui->streamingLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    ui->streamingLabel->setPixmap(scaledPixmap);
    if (frameData.isEmpty()) {
        qDebug() << "No frame data received!";
        return;
    }
}

void MainWindow::processOutput() {
    QByteArray errorOutput = ffmpegProcess->readAllStandardError();
    if (!errorOutput.isEmpty()) {
            qDebug() << "FFmpeg Error:" << errorOutput;
        }
}
