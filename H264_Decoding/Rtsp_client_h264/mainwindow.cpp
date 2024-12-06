/*
 * FFmpeg RTSP Client
 *  H.264
 *  800*600
 *
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QLayout>

#include <QTimer>
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer(new QTimer(this))
    , frameTimer(new QTimer(this))
    , ffmpegProcess(new QProcess(this))
    , rtspUrl("") // 초기 URL 비우기
{
    ui->setupUi(this);

    // SetButton 체크 가능하도록 설정
    ui->setButton->setCheckable(true);
    ui->addressInput->setPlaceholderText("Enter RTSP server address (e.g., 0.0.0.0)");

    setupAddressInput();
    if (ui->addressInput->text().isEmpty()) {
        ui->addressInput->setPlaceholderText("Enter RTSP server address (e.g., 0.0.0.0)");
    }
    // 타이머로 날짜/시간 업데이트
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000); //1초마다 실행
    updateDateTime();// 초기 날짜/시간 표시

    connect(frameTimer, &QTimer::timeout, this, &MainWindow::captureFrame);
    //connect(ffmpegProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(captureFrame()));

    // AddressInput 내용 변경 시 SetButton 상태 해제
    connect(ui->addressInput, &QLineEdit::textChanged, this, [this]() {
        ui->setButton->setChecked(false);
    });

}

MainWindow::~MainWindow()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        ffmpegProcess->terminate();
        if (!ffmpegProcess->waitForFinished(3000)) { // 3초 대기
                        ffmpegProcess->kill(); // 종료 실패 시 강제 종료
                    }
    }
    delete ui;
}

void MainWindow::updateDateTime()
{
    QDateTime current = QDateTime::currentDateTime();

    // 요일 한글 변환
    QMap<int, QString> weekdays;
    weekdays[1] = "MON";
    weekdays[2] = "TUE";
    weekdays[3] = "WED";
    weekdays[4] = "THU";
    weekdays[5] = "FRI";
    weekdays[6] = "SAT";
    weekdays[7] = "SUN";

    QString weekday = weekdays[current.date().dayOfWeek()];

    // 오전/오후 판단
    QString ampm = current.time().hour() < 12 ? "AM" : "PM";

    // 12시간제로 변환
    int hour = current.time().hour() % 12;
    if (hour == 0) hour = 12;

    // 날짜/시간 문자열 생성
    QString dateTimeStr = QString::asprintf("%04d.%02d.%02d %s | %s %02d:%02d:%02d",
                            current.date().year(),
                            current.date().month(),
                            current.date().day(),
                            qPrintable(weekday),
                            qPrintable(ampm),
                            hour,
                            current.time().minute(),
                            current.time().second());

    ui->dateTimeLabel->setText(dateTimeStr);
}


/*RTSP스트림을 가져오고, 파이프로 출력*/
void MainWindow::startFFmpeg() {
    ui->setButton->setChecked(true); // Set 버튼 상태 해제

    if (ffmpegProcess->state() == QProcess::Running) {
        qDebug() << "FFmpeg is already running!";
        return;
    }

    if (rtspUrl.isEmpty()) {
        QMessageBox::warning(this, "Error", "RTSP URL is not set. Please set the address first.");
        return;
    }

    QString ffmpegPath = "/bin/ffmpeg"; // FFmpeg 실행 경로

    //FFMPEG 실행시 전달되는 옵션
    QStringList arguments = {
        "-protocol_whitelist","file,tcp,udp,rtp,rtsp",
        "-i",rtspUrl, //동적으로 설정된 RTSP URL
        "-vf", "fps=30",                    // 출력 프레임 속도 설정
        "-s", "800x600",                         // 출력 영상의 해상도 설정
        "-f","rawvideo",
        "-pix_fmt","rgb24",  // 픽셀 포맷: RGB888
        //"-pix_fmt", "yuvj420p",  // 색 공간 최적화
        "-loglevel","debug",
        "-"
    };


    ffmpegProcess->start(ffmpegPath, arguments);

    if (!ffmpegProcess->waitForStarted()) {
        qDebug() << "FFmpeg failed to start!";
        return;
    }
    frameTimer->start(33); // 30fps (1000ms /33 = 30ms)
    qDebug() << "FFmpeg started with URL:" << rtspUrl;
    setButtonStyle(ui->startButton, true);
    setButtonStyle(ui->stopButton, false);

}

void MainWindow::captureFrame() {
    while (ffmpegProcess->bytesAvailable() > 0) {
        // FFmpeg에서 데이터 읽기
        QByteArray frameData = ffmpegProcess->readAllStandardOutput();
        if (frameData.isEmpty()) {
            qDebug() << "No frame data received!";
            return;
        }
        //qDebug() << "frameData Size : " << frameData.size();
        // 이전에 누적된 데이터와 합치기
        incompleteBuffer.append(frameData);

        // 한 프레임 크기 계산 (RGB888)
        const int width = 800;
        const int height = 600;
        const int bytesPerPixel = 3;
        const int frameSize = width * height * bytesPerPixel;

        // 버퍼가 프레임 크기 이상인지 확인
        while (incompleteBuffer.size() >= frameSize) {
            QByteArray singleFrameData = incompleteBuffer.left(frameSize);
            incompleteBuffer.remove(0, frameSize);

            // QImage 생성
            QImage image(reinterpret_cast<const uchar*>(singleFrameData.data()),
                         width, height, QImage::Format_RGB888);

            // QLabel에 맞게 크기 조정
            QPixmap scaledPixmap = QPixmap::fromImage(image).scaled(
                ui->streamingLabel->size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );

            // QLabel에 QPixmap 설정
            ui->streamingLabel->setPixmap(scaledPixmap);
        }
    }
}


/*FFmpeg을 종료하고 마지막 프레임을 고정*/
void MainWindow::stopFFmpeg() {
    ui->setButton->setChecked(false); // Set 버튼 상태 해제
    if (ffmpegProcess->state() == QProcess::Running) {
        frameTimer->stop(); // 프레임 캡처 중지
        ffmpegProcess->terminate(); // 프로세스 종료 요청
        if (!ffmpegProcess->waitForFinished(3000)) { // 3초 대기
            ffmpegProcess->kill(); // 강제 종료
        }
        qDebug() << "FFmpeg process stopped.";
    } else {
        qDebug() << "FFmpeg is not running.";
    }

    setButtonStyle(ui->startButton, false);
    setButtonStyle(ui->stopButton, true);
}

/*FFmpeg 프로세스의 에러로그를 읽어서 출력*/
void MainWindow::processOutput() {
    QByteArray errorOutput = ffmpegProcess->readAllStandardError();
    if (!errorOutput.isEmpty()) {
            qDebug() << "FFmpeg Error:" << errorOutput;
        }
}

void MainWindow::on_setButton_clicked()
{
    rtsp_setting();
    setButtonStyle(ui->setButton, true);
    setButtonStyle(ui->startButton, false);
    setButtonStyle(ui->stopButton, false);
}

/*UI에서 입력된 RTSP주소 가져오기*/
void MainWindow::rtsp_setting(){
    // QLineEdit에서 IP 주소 가져오기
    QString inputAddress = ui->addressInput->text().trimmed();
    if (inputAddress.isEmpty()) {
        ui->setButton->setChecked(false); // 잘못된 입력 시 SetButton 상태 해제
        QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, "Error", "Please enter a valid IP address.", QMessageBox::Ok, this);
        msgBox->setStyleSheet("QLabel { color : black; }"); // 글씨 색상 변경
        msgBox->exec();
        delete msgBox; // 메모리 관리
        return;
    }

    // RTSP URL 생성
    rtspUrl = QString("rtsp://%1:8554/").arg(inputAddress);
    qDebug() << "RTSP URL set to:" << rtspUrl;

    QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "Success", "RTSP URL updated successfully.", QMessageBox::Ok, this);
    msgBox->setStyleSheet("QLabel { color : black; }"); // 글씨 색상 변경
    msgBox->exec();
    delete msgBox; // 메모리 관리

}

void MainWindow::on_startButton_clicked(){
    startFFmpeg();
}

void MainWindow::on_stopButton_clicked(){
    stopFFmpeg();
}


void MainWindow::setupAddressInput() {
    ui->addressInput->installEventFilter(this); // 이벤트 필터 등록
}
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->addressInput) {
        if (event->type() == QEvent::FocusIn) { // 입력 필드에 포커스가 들어올 때
            setButtonStyle(ui->setButton, false);
        }
    }
    return QWidget::eventFilter(watched, event); // 기본 동작 유지
}


//버튼 true, false에 따른 스타일 지정 함수
void MainWindow::setButtonStyle(QPushButton* button, bool isActive) {
    if (isActive) {
        button->setChecked(true);
        button->setStyleSheet("color: white; \
                              background-color:rgba(243,115,33,0.7); \
                              border:1px solid rgb(243,115,33); \
                              font: 500 9pt 'Quicksand Medium';");
    } else {
        button->setChecked(false);
        button->setStyleSheet("color: rgb(243,115,33); \
                              background:none; \
                              border:1px solid rgb(243,115,33); \
                              font: 500 9pt 'Quicksand Medium';");
    }
}
