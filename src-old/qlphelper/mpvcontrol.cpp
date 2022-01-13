#include "mpvcontrol.h"

MpvControl::MpvControl(QFile* ff2mpv_fifo, QString record_file, QObject* parent)
  : QObject(parent)
{
    this->ff2mpv_fifo = ff2mpv_fifo;
    this->record_file = record_file;
    mpv_socket_path = QString("/tmp/qlp-%1").arg(QUuid::createUuid().toString());
    mpv_proc = new QProcess(this);
    mpv_socket = new QLocalSocket(this);
    connect(mpv_socket, &QLocalSocket::readyRead, this, &MpvControl::readMpvSocket);
    if (record_file.isEmpty()) {
        connect(mpv_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitStatus);
            QCoreApplication::exit(exitCode);
        });
    }
}

MpvControl::~MpvControl()
{
    mpv_proc->terminate();
    mpv_proc->waitForFinished(3000);
    QLocalServer::removeServer(mpv_socket_path);
}

void
MpvControl::start()
{
    if (record_file.isEmpty()) {
        mpv_proc->start("mpv", QStringList() << "--idle=yes"
                                             << "--player-operation-mode=pseudo-gui"
                                             << "--cache=yes"
                                             << "--cache-pause-initial=yes"
                                             << "--vf=lavfi=\"fps=60\""
                                             << "--input-ipc-server=" + mpv_socket_path << ff2mpv_fifo->fileName());
        auto t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this, t]() {
            if (mpv_socket->state() != QLocalSocket::ConnectedState) {
                mpv_socket->connectToServer(mpv_socket_path);
            } else {
                mpv_socket->write(QString("{ \"command\": [\"keybind\", \"alt+r\", \"script-message qlp:r\"] }\n"
                                          "{ \"command\": [\"keybind\", \"alt+z\", \"script-message qlp:fsdown\"] }\n"
                                          "{ \"command\": [\"keybind\", \"alt+x\", \"script-message qlp:fsup\"] }\n"
                                          "{ \"command\": [\"keybind\", \"alt+i\", \"script-message qlp:nick\"] }\n")
                                    .toUtf8());
                t->stop();
                t->deleteLater();
            }
        });
        t->start(1000);
    } else {
        mpv_proc->start("cp", QStringList() << ff2mpv_fifo->fileName() << genRecordFileName());
    }
}

void
MpvControl::restart()
{
    if (record_file.isEmpty()) {
        // do loadfile
        mpv_socket->write(QString("{ \"command\": [\"loadfile\", \"%1\"] }\n").arg(ff2mpv_fifo->fileName()).toUtf8());
    } else {
        mpv_proc->terminate();
        mpv_proc->waitForFinished();
        mpv_proc->start("cp", QStringList() << ff2mpv_fifo->fileName() << genRecordFileName());
    }
}

void
MpvControl::readMpvSocket()
{
    while (mpv_socket->canReadLine()) {
        auto tmp = mpv_socket->readLine();
        if (tmp.contains("qlp:")) {
            auto jobj = QJsonDocument::fromJson(tmp).object();
            auto parser = QlpCmdParser(jobj["args"].toArray()[0].toString());
            if (parser.getReload()) {
                emit requestReload();
            }
            if (parser.getQuality() != -1) {
                emit onQuality(parser.getQuality());
            }
            if (parser.getFs() != -1) {
                emit onFont(parser.getFs(), -1);
            }
            if (parser.getFa() != -1) {
                emit onFont(-1, parser.getFa());
            }
            if (parser.getSpeed() != -1) {
                emit onSpeed(parser.getSpeed());
            }
            if (parser.getFsUp()) {
                emit onFontScaleDelta(0.15);
            }
            if (parser.getFsDown()) {
                emit onFontScaleDelta(-0.15);
            }
            if (parser.getShowNick()) {
                emit onToggleNick();
            }
        } else if (tmp.contains("file-loaded")) {
            QTimer::singleShot(500, this, [this]() {
                this->mpv_socket->write(QString("{ \"command\": [\"get_property\", \"video-params\"] }\n").toUtf8());
            });
            emit reloaded();
        } else if (tmp.contains("pixelformat")) {
            auto jobj = QJsonDocument::fromJson(tmp).object();
            emit resFetched(jobj.value("data")["w"].toInt(1920), jobj.value("data")["h"].toInt(1080));
        }
    }
}

void
MpvControl::setTitle(QString title)
{
    room_title = title;
}

QString
MpvControl::genRecordFileName()
{
    if (record_file.right(4) == ".mkv" || record_file.right(4) == ".mp4" || record_file.right(4) == ".flv") {
        record_file.chop(4);
    }
    return record_file + "-" + QString::number(record_cnt++) + ".mkv";
}
