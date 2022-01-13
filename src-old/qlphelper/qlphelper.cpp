#include <QRandomGenerator>

#include "../qlpconfig.h"
#include "qlphelper.h"

QLPHelper::QLPHelper(QStringList args, QObject* parent)
  : QObject(parent)
{
    room_url = args.at(0);
    record_file = args.at(1).compare("null") == 0 ? "" : args.at(1);
    if (args.at(2) == "true") {
        strict_stream = true;
    }
    if (args.at(3) == "true") {
        is_debug = true;
    }

    bool ok = false;
    auto fs = args.at(4).toDouble(&ok);
    if (ok == false || fs <= 0) {
        fs = QlpConfig::getInstance().readFontScale();
    }
    auto fa = args.at(5).toDouble(&ok);
    if (ok == false || (fa < 0 || fa > 1)) {
        fa = QlpConfig::getInstance().readFontAlpha();
    }

    stream_socket = QString("/tmp/qlp-%1").arg(QUuid::createUuid().toString());
    stream_port = QString::number(QRandomGenerator::system()->bounded(10000, 20000));
    danmaku_socket = QString("/tmp/qlp-%1").arg(QUuid::createUuid().toString());
    auto f = QString("/tmp/qlp-%1").arg(QUuid::createUuid().toString());
    QProcess::execute("mkfifo", QStringList() << f);
    ff2mpv_fifo = new QFile(f, this);

    ffmpeg_control = new FFmpegControl(stream_socket, stream_port, danmaku_socket, ff2mpv_fifo, record_file, is_debug, strict_stream, this);
    mpv_control = new MpvControl(ff2mpv_fifo, record_file, this);
    stream_finder = new StreamFinder(room_url, stream_socket, stream_port, this);
    danmaku_launcher =
      new DanmakuLauncher(room_url, danmaku_socket, fs, fa, QlpConfig::getInstance().readDanmakuSpeed(), args.at(6) == "true" ? true : false, this);

    connect(stream_finder, &StreamFinder::streamError, this, &QLPHelper::restart);
    connect(stream_finder, &StreamFinder::streamStart, danmaku_launcher, &DanmakuLauncher::onStreamStart);
    connect(stream_finder, &StreamFinder::streamStart, this, [this]() {
        QTimer::singleShot(500, this, &QLPHelper::setIdle);
    });
    connect(mpv_control, &MpvControl::requestReload, this, &QLPHelper::restart, Qt::QueuedConnection);
    connect(mpv_control, &MpvControl::resFetched, danmaku_launcher, &DanmakuLauncher::setScale);
    connect(mpv_control, &MpvControl::onQuality, stream_finder, &StreamFinder::setQuality);
    connect(mpv_control, &MpvControl::onFont, danmaku_launcher, &DanmakuLauncher::setFont);
    connect(mpv_control, &MpvControl::onFontScaleDelta, danmaku_launcher, &DanmakuLauncher::setFontScaleDelta);
    connect(mpv_control, &MpvControl::onSpeed, danmaku_launcher, &DanmakuLauncher::setSpeed);
    connect(mpv_control, &MpvControl::onToggleNick, danmaku_launcher, &DanmakuLauncher::setToggleNick);
    connect(stream_finder, &StreamFinder::titleMatched, mpv_control, &MpvControl::setTitle);
    connect(stream_finder, &StreamFinder::ready, ffmpeg_control, &FFmpegControl::onStreamReady);
}

QLPHelper::~QLPHelper()
{
    qCritical() << "Bye!";
    danmaku_launcher->stop();
    stream_finder->stop();
    ffmpeg_control->stop();

    ff2mpv_fifo->close();
    ff2mpv_fifo->remove();
}

void
QLPHelper::start()
{
    stream_finder->start();
    qDebug() << "streamer started";
    danmaku_launcher->start();
    qDebug() << "danmaku launcher started";
    ffmpeg_control->start();
    qDebug() << "ffmpeg started";
    mpv_control->start();
    qDebug() << "mpv started";
}

void
QLPHelper::restart()
{
    if (state == Idle) {
        state = Busy;
        // Be careful of the order of restart, due to ffmpeg's IO policy.
        // If the last input of ffmpeg is stucked, the whole ffmpeg will be blocked.
        stream_finder->stop();
        qDebug() << "streamer stopped";
        danmaku_launcher->stop();
        qDebug() << "danmaku launcher stopped";
        ffmpeg_control->stop();
        qDebug() << "ffmpeg stopped";
        QTimer::singleShot(500, this, [this]() {
            stream_finder->start();
            qDebug() << "streamer started";
            danmaku_launcher->start();
            qDebug() << "danmaku launcher started";
            ffmpeg_control->start();
            qDebug() << "ffmpeg started";
            mpv_control->restart();
            qDebug() << "mpv restarted";
        });
    }
}

void
QLPHelper::setIdle()
{
    state = Idle;
}
