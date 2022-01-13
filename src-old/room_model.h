#ifndef ROOM_MODEL_H
#define ROOM_MODEL_H

#include <QObject>
#include <QtGui>
#include "sites.h"

namespace RM {

class Room
{
    Q_GADGET
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString cover READ cover)
    Q_PROPERTY(QString owner READ owner)
    Q_PROPERTY(int status READ status)
    Q_PROPERTY(QString url READ url)
    Q_PROPERTY(int like READ like)

public:
    Room(QString url, QString title, QString cover, QString owner, int status, int like) {
        m_url = url;
        m_cover = cover;
        m_title = title;
        m_owner = owner;
        m_status = status;
        m_like = like;
    }
    Room() = default;
    Room(const Room& other)=default;
    Room& operator=(const Room& other)=default;

    const QString& cover() {
        return m_cover;
    }
    const QString& title() const {
        return m_title;
    }
    const QString& url() const {
        return m_url;
    }
    const QString& owner() const {
        return m_owner;
    }
    const int& status() const {
        return m_status;
    }
    const int& like() const {
        return m_like;
    }


private:
    QString m_title;
    QString m_cover;
    QString m_owner;
    QString m_url;
    int m_status = 0;
    int m_like = 0;
};

class RoomModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSortFilterProxyModel* saved_model READ saved_model NOTIFY savedModelChanged)
    Q_PROPERTY(QSortFilterProxyModel* history_model READ history_model NOTIFY historyModelChanged)
    Q_DISABLE_COPY(RoomModel)

public:
    explicit RoomModel(QObject *parent = nullptr);
    ~RoomModel();
    void sort();

    QSortFilterProxyModel* saved_model() const;
    QSortFilterProxyModel* history_model() const;

    QStandardItemModel* base_model = nullptr;

signals:
    void savedModelChanged();
    void historyModelChanged();

private:
    QSortFilterProxyModel *m_saved_model = nullptr;
    QSortFilterProxyModel *m_history_model = nullptr;

};
}
#endif // ROOM_MODEL_H
