/*
 * Copyright (C) 2016 ~ 2018 Wuhan Deepin Technology Co., Ltd.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QMap>
#include <QSharedPointer>

#include <playlistmeta.h>

class PlayMusicType
{
public:
    QString         name;
    QString         extraName;
    QByteArray      icon;
    qint64          timestamp   = 0;    // addTime;
    PlaylistMeta    playlistMeta;
};
typedef QSharedPointer<PlayMusicType>   PlayMusicTypePtr;
typedef QList<PlayMusicTypePtr>         PlayMusicTypePtrList;

class PlayMusicTypePtrListData
{
public:
    QStringList              sortMetas;

    QMap<QString, PlayMusicTypePtr>   metas;

    int     sortType    = 0;
    int     orderType   = 0;
};

Q_DECLARE_METATYPE(PlayMusicType)
Q_DECLARE_METATYPE(PlayMusicTypePtr)
Q_DECLARE_METATYPE(PlayMusicTypePtrList)
Q_DECLARE_METATYPE(PlayMusicTypePtrListData)

class Playlist : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive)
public:
    explicit Playlist(const PlaylistMeta &musiclistinfo, QObject *parent = nullptr);

    enum SortType {
        SortByAddTime  = 0,
        SortByTitle,
        SortByArtist,
        SortByAblum,
        SortByCustom,
    };
    Q_ENUM(SortType)

    enum OrderType {
        Ascending = 0,
        Descending,
    };
    Q_ENUM(OrderType)

public:
    //! public interface
    QString id() const;
    QString displayName() const;
    QString icon() const;
    bool readonly() const;
    bool editmode() const;
    bool hide() const;
    bool isEmpty() const;
    bool canNext() const;
    int length() const;
    int sortType() const;
    uint sortID() const;

    bool active() const;
    void setActive(bool active);

    const MetaPtr first() const;
    const MetaPtr prev(const MetaPtr meta) const;
    const MetaPtr next(const MetaPtr info) const;
    const MetaPtr shufflePrev(const MetaPtr meta);
    const MetaPtr shuffleNext(const MetaPtr meta);
    const MetaPtr music(int index) const;
    const MetaPtr music(const QString &id) const;
    const MetaPtr playing() const;
    bool playingStatus() const;
    void setPlayingStatus(bool Status);

    int index(const QString &hash);
    bool isLast(const MetaPtr meta) const;
    bool contains(const MetaPtr meta) const;
    MetaPtrList allmusic() const;
    PlayMusicTypePtrList playMusicTypePtrList() const;

    void play(const MetaPtr meta);
    void reset(const MetaPtrList);

    void changePlayMusicTypeOrderType();

    void setSearchStr(const QString &str);
    QString searchStr() const;

    void setViewMode(const int &mode);
    int viewMode() const;

    void clearTypePtr();
    void appendMusicTypePtrListData(PlayMusicTypePtr  musicTypePtr);

public slots:
    void setDisplayName(const QString &name);
    void appendMusicList(const MetaPtrList metalist);
    MetaPtr removeMusicList(const MetaPtrList metalist);
    MetaPtr removeOneMusic(const MetaPtr meta);
    void updateMeta(const MetaPtr meta);
    void sortBy(Playlist::SortType sortType);
    void resort();
    void saveSort(QMap<QString, int> hashIndexs);

    void metaListToPlayMusicTypePtrList(Playlist::SortType sortType, const MetaPtrList metalist);
    void playMusicTypeToMeta(QString name = "", QStringList sortMetas = QStringList());
    void sortPlayMusicTypePtrListData(int sortType);

public:
    void load();

signals:
    void musiclistAdded(const MetaPtrList metalist);
    void musiclistRemoved(const MetaPtrList metalist);
    void removed();
    void displayNameChanged(QString displayName);

private:
    Q_DISABLE_COPY(Playlist)

    enum class ShuffleHistoryState {
        Empty,
        Prev,
        Next,
    };

    PlaylistMeta playlistMeta;
    PlayMusicTypePtrListData playMusicTypePtrListData;
    MetaPtrList shuffleList;
    MetaPtrList shuffleHistory;
    ShuffleHistoryState shuffleHistoryState = ShuffleHistoryState::Empty;
    int shuffleSeed;
    QString     searchData;
    int         viewModeFlag = 0;

    const MetaPtr shuffleTurn(bool forward);
};

typedef QSharedPointer<Playlist> PlaylistPtr;

Q_DECLARE_METATYPE(PlaylistPtr)
Q_DECLARE_METATYPE(QList<PlaylistPtr>)

extern const QString AlbumMusicListID;
extern const QString ArtistMusicListID;
extern const QString AllMusicListID;
extern const QString FavMusicListID;
extern const QString SearchMusicListID;
extern const QString PlayMusicListID;
extern const QString AlbumCandListID;
extern const QString MusicCandListID;
extern const QString ArtistCandListID;
extern const QString AlbumResultListID;
extern const QString MusicResultListID;
extern const QString ArtistResultListID;


