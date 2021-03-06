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

#include "playlistview.h"

#include <QDebug>
#include <DMenu>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QStyleFactory>
#include <QResizeEvent>
#include <QStandardItemModel>

#include <DDialog>
#include <DDesktopServices>
#include <DScrollBar>
#include <DLabel>
#include "util/threadpool.h"
#include "util/pinyinsearch.h"

#include "../../core/metasearchservice.h"
#include "../helper/widgethellper.h"

#include "delegate/playitemdelegate.h"
#include "model/playlistmodel.h"

DWIDGET_USE_NAMESPACE

class PlayListViewPrivate
{
public:
    explicit PlayListViewPrivate(PlayListView *parent): q_ptr(parent) {}

    void addMedia(const MetaPtr meta);
    void removeSelection(QItemSelectionModel *selection);

    PlaylistModel      *model        = nullptr;
    PlayItemDelegate   *delegate     = nullptr;
    int                 themeType    = 1;
    MetaPtr             playing      = nullptr;
    bool                searchFlag   = true;

    MetaPtrList         playMetaPtrList;
    QPixmap              playingPixmap = QPixmap(":/mpimage/light/music1.svg");
    QPixmap              sidebarPixmap = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    QPixmap              albumPixmap   = QPixmap(":/mpimage/light/music_white_album_cover/music1.svg");

    PlayListView *q_ptr;
    Q_DECLARE_PUBLIC(PlayListView)
};

PlayListView::PlayListView(bool searchFlag, bool isPlayList, QWidget *parent)
    : DListView(parent), d_ptr(new PlayListViewPrivate(this))
{
    Q_D(PlayListView);
    m_IsPlayList = isPlayList;
    setObjectName("PlayListView");

    d->searchFlag = searchFlag;
    d->model = new PlaylistModel(0, 1, this);
    setModel(d->model);

    d->delegate = new PlayItemDelegate;
    setItemDelegate(d->delegate);

    setUniformItemSizes(true);

    setDragEnabled(true);
    //viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropOverwriteMode(false);
    //setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    //setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDragEnabled(true);
    setMovement(QListView::Free);

    setViewModeFlag(QListView::ListMode);
    setResizeMode(QListView::Adjust);
    setLayoutMode(QListView::Batched);
    setBatchSize(2000);

    setSelectionMode(QListView::ExtendedSelection);
    //setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &PlayListView::customContextMenuRequested,
            this, &PlayListView::requestCustomContextMenu);

    connect(this, &PlayListView::doubleClicked,
    this, [ = ](const QModelIndex & index) {
        MetaPtr meta = d->model->meta(index);
        if (meta == playlist()->playing()) {
            Q_EMIT resume(meta);
        } else {
            Q_EMIT playMedia(meta);
        }
    });
    m_ModelMake = new ModelMake();
    ThreadPool::instance()->moveToNewThread(m_ModelMake);
    connect(this, &PlayListView::modelMake, m_ModelMake, &ModelMake::onModelMake, Qt::QueuedConnection);
    connect(m_ModelMake, &ModelMake::addMeta, this, &PlayListView::onAddMeta, Qt::QueuedConnection);
    connect(m_ModelMake, &ModelMake::hideEmptyHits, this, [ = ](bool a) {
        if (d->playMetaPtrList.isEmpty()) {
            //emit got none data
            emit getSearchData(false);
        }
        Q_EMIT hideEmptyHits(a);
    });
}

PlayListView::~PlayListView()
{

}

MetaPtr PlayListView::activingMeta() const
{
    Q_D(const PlayListView);

    if (d->model->playlist().isNull()) {
        return MetaPtr();
    }

    if (d->playing == nullptr) {
        return d->model->playlist()->playing();
    } else {
        return d->playing;
    }
}

PlaylistPtr PlayListView::playlist() const
{
    Q_D(const PlayListView);
    return d->model->playlist();
}

QModelIndex PlayListView::findIndex(const MetaPtr meta)
{
    Q_ASSERT(!meta.isNull());
    Q_D(PlayListView);

    return d->model->findIndex(meta);
}

void PlayListView::setPlaying(const MetaPtr meta)
{
    Q_D(PlayListView);
    d->playing = meta;
}

void PlayListView::setViewModeFlag(QListView::ViewMode mode)
{
    if (mode == QListView::IconMode) {
        setIconSize(QSize(150, 150));
        setGridSize(QSize(-1, -1));
        setSpacing(20);
        setViewportMargins(-10, -10, -35, 10);
    } else {
        setIconSize(QSize(36, 36));
        setGridSize(QSize(-1, -1));
        setSpacing(0);
        setViewportMargins(0, 0, 8, 0);
    }
    setViewMode(mode);
}

MetaPtrList PlayListView::playMetaPtrList() const
{
    Q_D(const PlayListView);
    return d->playMetaPtrList;
}

void PlayListView::setThemeType(int type)
{
    Q_D(PlayListView);
    d->themeType = type;
}

int PlayListView::getThemeType() const
{
    Q_D(const PlayListView);
    return d->themeType;
}

void PlayListView::setPlayPixmap(QPixmap pixmap, QPixmap sidebarPixmap, QPixmap albumPixmap)
{
    Q_D(PlayListView);
    d->playingPixmap = pixmap;
    d->sidebarPixmap = sidebarPixmap;
    d->albumPixmap = albumPixmap;
    update();
}

QPixmap PlayListView::getPlayPixmap() const
{
    Q_D(const PlayListView);
    return d->playingPixmap;
}

QPixmap PlayListView::getSidebarPixmap() const
{
    Q_D(const PlayListView);
    return d->sidebarPixmap;
}

QPixmap PlayListView::getAlbumPixmap() const
{
    Q_D(const PlayListView);
    return d->sidebarPixmap;
}

QStandardItem *PlayListView::item(int row, int column) const
{
    Q_D(const PlayListView);

    return  d->model->item(row, column);
}

void PlayListView::setCurrentItem(QStandardItem *item)
{
    Q_D(PlayListView);

    setCurrentIndex(d->model->indexFromItem(item));
}

int PlayListView::rowCount()
{
    Q_D(const PlayListView);
    return d->model->rowCount();
}

QString PlayListView::firstHash()
{
    Q_D(const PlayListView);
    QString hashStr;

    if (d->model->rowCount() > 0) {
        auto index = d->model->index(0, 0);
        hashStr = d->model->data(index).toString();
    }

    return hashStr;
}

void PlayListView::onAddMeta(QString playListId, const MetaPtr meta)
{
    Q_D(PlayListView);
    if (playlist().isNull()) {
        return;
    }
    if (playListId == playlist()->id()) {
        //first time to get here
        if (d->playMetaPtrList.isEmpty()) {
            //emit data ready
            emit getSearchData(true);
        }
        d->addMedia(meta);
        d->playMetaPtrList.append(meta);
    }
}

void PlayListView::onMusicListRemoved(const MetaPtrList metalist)
{
    Q_D(PlayListView);

    setAutoScroll(false);
    //d->model->blockSignals(true);
    for (auto meta : metalist) {
        if (meta.isNull()) {
            continue;
        }

//        for (int i = 0; i < d->model->rowCount(); ++i) {
        for (int i = 0; i < d->playMetaPtrList.size(); ++i) {
            auto index = d->model->index(i, 0);
            auto itemHash = d->model->data(index).toString();
            if (itemHash == meta->hash) {
                d->model->removeRow(i);
            }
        }
    }
    //d->model->blockSignals(false);
    //updateScrollbar();
    setAutoScroll(true);
}

void PlayListView::onMusicError(const MetaPtr meta, int /*error*/)
{
    if (meta == nullptr) {
        return ;
    }

    update();
}

void PlayListView::onMusicListAdded(const MetaPtrList metalist)
{
    Q_D(PlayListView);
    setUpdatesEnabled(false);
    setAutoScroll(false);
    for (auto meta : metalist) {
        d->addMedia(meta);
        d->playMetaPtrList.append(meta);
    }
    setAutoScroll(true);
    setUpdatesEnabled(true);
    //updateScrollbar();
}

void PlayListView::onLocate(const MetaPtr meta)
{
    if (meta == nullptr)
        return;
    QModelIndex index = findIndex(meta);
    if (!index.isValid()) {
        return;
    }

    clearSelection();

    auto viewRect = QRect(QPoint(0, 0), size());
    if (!viewRect.intersects(visualRect(index))) {
        scrollTo(index, PlayListView::PositionAtCenter);
    }
    setCurrentIndex(index);
}

bool PlayListView::onMusiclistChanged(PlaylistPtr playlist)
{
    Q_D(PlayListView);
    if (playlist.isNull()) {
        qWarning() << "can not change to emptry playlist";
        d->model->removeRows(0, d->model->rowCount());
        d->playMetaPtrList.clear();
        d->model->setPlaylist(nullptr);
        return false;
    }
    if (playlist->searchStr().isEmpty() && playlist == d->model->playlist()
            && playlist->allmusic().size() == rowCount()) {
        bool flag = true;
        auto allMusic = playlist->allmusic();
        for (int i = 0; i < allMusic.size(); ++i) {
            auto curIndex = d->model->index(i, 0);
            if (!curIndex.isValid() || allMusic[i]->hash != d->model->data(d->model->index(i, 0)).toString()) {
                flag = false;
                break;
            }
        }
        if (flag)
            return  false;
    }

    setUpdatesEnabled(false);
    setModel(nullptr);
    d->model->removeRows(0, d->model->rowCount());
    d->playMetaPtrList.clear();

    QString searchStr = playlist->searchStr();
    if (!d->searchFlag)
        searchStr.clear();

    Q_EMIT modelMake(playlist, searchStr);
    setUpdatesEnabled(true);
    setModel(d->model);
    d->model->setPlaylist(playlist);
    return true;
}

void PlayListView::keyPressEvent(QKeyEvent *event)
{
    Q_D(PlayListView);
    switch (event->modifiers()) {
    case Qt::NoModifier:
        switch (event->key()) {
        case Qt::Key_Delete: {
            QItemSelectionModel *selection = this->selectionModel();
            d->removeSelection(selection);
        }
        break;
        case Qt::Key_Return: {
            QItemSelectionModel *selection = this->selectionModel();
            if (!selection->selectedRows().isEmpty()) {
                auto index = selection->selectedRows().first();
                if (d->model->meta(index) == playlist()->playing()) {
                    Q_EMIT resume(d->model->meta(index));
                } else {
                    Q_EMIT playMedia(d->model->meta(index));
                }
            }
        }
        break;
        }
        break;
    case Qt::ShiftModifier:
        switch (event->key()) {
        case Qt::Key_Delete:
            break;
        }
        break;
    case Qt::ControlModifier:
        switch (event->key()) {
        case Qt::Key_I:
            QItemSelectionModel *selection = this->selectionModel();
            if (selection->selectedRows().length() <= 0) {
                return;
            }
            auto index = selection->selectedRows().first();
            auto meta = d->model->meta(index);
            Q_EMIT showInfoDialog(meta);
            break;
        }
        break;
    //    case Qt::ControlModifier:
    //        switch (event->key()) {
    //        case Qt::Key_K:
    //            QItemSelectionModel *selection = this->selectionModel();
    //            if (selection->selectedRows().length() > 0) {
    //                MetaPtrList metalist;
    //                for (auto index : selection->selectedRows()) {
    //                    auto meta = d->model->meta(index);
    //                    metalist << meta;
    //                }
    //                if (!metalist.isEmpty())
    //                    Q_EMIT addMetasFavourite(metalist);
    //            }
    //            break;
    //        }
    //        break;
    //    case Qt::ControlModifier | Qt::ShiftModifier:
    //        switch (event->key()) {
    //        case Qt::Key_K:
    //            QItemSelectionModel *selection = this->selectionModel();
    //            if (selection->selectedRows().length() > 0) {
    //                MetaPtrList metalist;
    //                for (auto index : selection->selectedRows()) {
    //                    auto meta = d->model->meta(index);
    //                    metalist << meta;
    //                }
    //                if (!metalist.isEmpty())
    //                    Q_EMIT removeMetasFavourite(metalist);
    //            }
    //            break;
    //        }
    //        break;
    default:
        break;
    }

    QAbstractItemView::keyPressEvent(event);
}

void PlayListView::keyboardSearch(const QString &search)
{
    Q_UNUSED(search);
    // Disable keyborad serach
    //    qDebug() << search;
    //    QAbstractItemView::keyboardSearch(search);
}

void PlayListViewPrivate::addMedia(const MetaPtr meta)
{
    for (int i = 0; i < model->rowCount(); ++i) {
        auto hash = model->data(model->index(i, 0)).toString();
        if (hash == meta->hash)
            return;
    }
    QStandardItem *newItem = new QStandardItem;
    QPixmap cover(":/common/image/cover_max.svg");
    auto coverData = MetaSearchService::coverData(meta);
    if (coverData.length() > 0) {
        cover = QPixmap::fromImage(QImage::fromData(coverData));
    }
    if (cover.width() > 160 || cover.height() > 160)
        cover = cover.scaled(QSize(160, 160));
    QIcon icon = QIcon(cover);
    newItem->setIcon(icon);
    model->appendRow(newItem);

    auto row = model->rowCount() - 1;
    QModelIndex index = model->index(row, 0, QModelIndex());
    model->setData(index, meta->hash);
}

void PlayListViewPrivate::removeSelection(QItemSelectionModel *selection)
{
    Q_ASSERT(selection != nullptr);
    Q_Q(PlayListView);

    MetaPtrList metalist;
    for (auto index : selection->selectedRows()) {
        auto meta = model->meta(index);
        metalist << meta;
    }
    Q_EMIT q->removeMusicList(metalist);
}

void PlayListView::showContextMenu(const QPoint &pos,
                                   PlaylistPtr selectedPlaylist,
                                   PlaylistPtr favPlaylist,
                                   QList<PlaylistPtr> newPlaylists)
{

    Q_D(PlayListView);
    QItemSelectionModel *selection = this->selectionModel();

    if (selection->selectedRows().length() <= 0) {
        return;
    }

    QPoint globalPos = this->mapToGlobal(pos);

    DMenu playlistMenu;
    auto newvar = QVariant::fromValue(PlaylistPtr());

    PlaylistPtr curPlaylist = nullptr;
    for (auto playlist : newPlaylists) {
        if (playlist->id() == PlayMusicListID) {
            curPlaylist = playlist;
            auto act = playlistMenu.addAction(tr("Play queue"));
            act->setData(QVariant::fromValue(curPlaylist));
            playlistMenu.addSeparator();
            break;
        }
    }
    if (selectedPlaylist != favPlaylist || this->playlist()->id() == "musicResult") {
        //        auto act = playlistMenu.addAction(favPlaylist->displayName());
        auto act = playlistMenu.addAction(tr("My favorites"));
        bool flag = true;
        for (auto &index : selection->selectedRows()) {
            auto meta = d->model->meta(index);
            if (!favPlaylist->contains(meta)) {
                flag = false;
            }
        }
        if (flag == true) {
            act->setEnabled(false);
        } else {
            act->setEnabled(true);
        }
        act->setData(QVariant::fromValue(favPlaylist));
        playlistMenu.addSeparator();
    }
    auto createPlaylist = playlistMenu.addAction(tr("Add to new playlist"));
    //    auto font = createPlaylist->font();
    //    font.setWeight(QFont::DemiBold);
    //    createPlaylist->setFont(font);
    createPlaylist->setData(newvar);
    playlistMenu.addSeparator();

    for (auto playlist : newPlaylists) {
        if (playlist == nullptr) {
            continue;
        }

        if (playlist->id() == PlayMusicListID) {
            curPlaylist = playlist;
            continue;
        }
        QFont font(playlistMenu.font());
        QFontMetrics fm(font);

        auto text = fm.elidedText(QString(playlist->displayName().replace("&", "&&")),
                                  Qt::ElideMiddle, 160);
        auto act = playlistMenu.addAction(text);
        act->setData(QVariant::fromValue(playlist));
    }

    playlistMenu.addSeparator();

    connect(&playlistMenu, &DMenu::triggered, this, [ = ](QAction * action) {
        auto playlist = action->data().value<PlaylistPtr >();
        qDebug() << playlist;
        MetaPtrList metalist;
        for (auto &index : selection->selectedRows()) {
            auto meta = d->model->meta(index);
            if (!meta.isNull()) {
                metalist << meta;
            }
        }
        Q_EMIT addToPlaylist(playlist, metalist);
    });

    bool singleSelect = (1 == selection->selectedRows().length());

    DMenu myMenu;
    QAction *playAction = nullptr;
    QAction *pauseAction = nullptr;
    if (singleSelect) {

        auto activeMeta = activingMeta();
        auto meta = d->model->meta(selection->selectedRows().first());

        if (d->model->playlist()->playingStatus() && activeMeta == meta) {

            if (rowCount() == 1 && meta->invalid) {
                playAction = myMenu.addAction(tr("Play"));
                if (meta->invalid)
                    playAction->setEnabled(false);
            } else {

                pauseAction = myMenu.addAction(tr("Pause"));
                if (meta->invalid)
                    pauseAction->setEnabled(false);
            }
        } else {
            playAction = myMenu.addAction(tr("Play"));
            if (meta->invalid)
                playAction->setEnabled(false);
        }
    }

    myMenu.addAction(tr("Add to playlist"))->setMenu(&playlistMenu);
    myMenu.addSeparator();

    QAction *displayAction = nullptr;
    if (singleSelect) {
        displayAction = myMenu.addAction(tr("Display in file manager"));
    }
    QAction *removeAction = nullptr;
    if (m_IsPlayList) {
        removeAction = myMenu.addAction(tr("Remove from play queue"));
    } else {
        removeAction = myMenu.addAction(tr("Remove from playlist"));
    }
    auto deleteAction = myMenu.addAction(tr("Delete from local disk"));

    QAction *songAction = nullptr;

    DMenu textCodecMenu;
    if (singleSelect) {
        auto index = selection->selectedRows().first();
        auto meta = d->model->meta(index);
        QList<QByteArray> codecList = DMusic::detectMetaEncodings(meta);

        if (!codecList.contains("UTF-8")) {
            codecList.push_front("UTF-8");
        }
        if (QLocale::system().name() == "zh_CN") {
            if (codecList.contains("GB18030")) {
                codecList.removeAll("GB18030");
            }

            if (!codecList.isEmpty()) {
                codecList.push_front("GB18030");
            }
        }

        for (auto codec : codecList) {

            auto act = textCodecMenu.addAction(codec);

            act->setCheckable(true);

            if (codec == meta->codec) {
                act->setChecked(true);
            }

            act->setData(QVariant::fromValue(codec));
        }

        if (codecList.length() > 1) {
            myMenu.addSeparator();
            myMenu.addAction(tr("Encoding"))->setMenu(&textCodecMenu);
        }

        myMenu.addSeparator();
        songAction = myMenu.addAction(tr("Song info"));

        connect(&textCodecMenu, &DMenu::triggered, this, [ = ](QAction * action) {
            auto codec = action->data().toByteArray();

            auto preTitle = meta->title;
            auto preArtist = meta->artist;
            auto preAlbum = meta->album;

            meta->updateCodec(codec);
            meta->codec = codec;

            if (preTitle != meta->title || preArtist != meta->artist || preAlbum != meta->album)
                Q_EMIT updateMetaCodec(preTitle, preArtist, preAlbum, meta);
        });
    }

    if (playAction) {
        connect(playAction, &QAction::triggered, this, [ = ](bool) {
            auto index = selection->selectedRows().first();
            if (d->model->meta(index) == playlist()->playing()) {
                Q_EMIT resume(d->model->meta(index));
            } else {
                Q_EMIT playMedia(d->model->meta(index));
            }
        });
    }

    if (pauseAction) {
        connect(pauseAction, &QAction::triggered, this, [ = ](bool) {
            auto index = selection->selectedRows().first();
            Q_EMIT pause(d->model->meta(index));
        });
    }

    if (displayAction) {
        connect(displayAction, &QAction::triggered, this, [ = ](bool) {
            auto index = selection->selectedRows().first();
            auto meta = d->model->meta(index);
            auto dirUrl = QUrl::fromLocalFile(meta->localPath);
            Dtk::Widget::DDesktopServices::showFileItem(dirUrl);
        });
    }

    if (removeAction) {
        connect(removeAction, &QAction::triggered, this, [ = ](bool) {
            MetaPtrList metalist;
            for (auto index : selection->selectedRows()) {
                auto meta = d->model->meta(index);
                metalist << meta;
            }
            if (metalist.isEmpty())
                return ;

            Dtk::Widget::DDialog warnDlg(this);
            warnDlg.setTextFormat(Qt::RichText);
            warnDlg.addButton(tr("Cancel"), true, Dtk::Widget::DDialog::ButtonNormal);
            int deleteFlag = warnDlg.addButton(tr("Remove"), false, Dtk::Widget::DDialog::ButtonWarning);

            if (1 == metalist.length()) {
                auto meta = metalist.first();
                warnDlg.setMessage(QString(tr("Are you sure you want to remove %1?")).arg(meta->title));
            } else {
                warnDlg.setMessage(QString(tr("Are you sure you want to remove the selected %1 songs?").arg(metalist.length())));
            }

            warnDlg.setIcon(QIcon::fromTheme("deepin-music"));
            if (deleteFlag == warnDlg.exec()) {
                d->removeSelection(selection);
            }
        });
    }

    if (deleteAction) {
        connect(deleteAction, &QAction::triggered, this, [ = ](bool) {
            bool containsCue = false;
            MetaPtrList metalist;
            for (auto index : selection->selectedRows()) {
                auto meta = d->model->meta(index);
                if (!meta->cuePath.isEmpty()) {
                    containsCue = true;
                }
                metalist << meta;
            }

            Dtk::Widget::DDialog warnDlg(this);
            warnDlg.setTextFormat(Qt::RichText);
            warnDlg.addButton(tr("Cancel"), true, Dtk::Widget::DDialog::ButtonNormal);
            int deleteFlag = warnDlg.addButton(tr("Delete"), false, Dtk::Widget::DDialog::ButtonWarning);

            auto cover = QImage(QString(":/common/image/del_notify.svg"));
            if (1 == metalist.length()) {
                auto meta = metalist.first();
                auto coverData = MetaSearchService::coverData(meta);
                if (coverData.length() > 0) {
                    cover = QImage::fromData(coverData);
                }
                warnDlg.setMessage(QString(tr("Are you sure you want to delete %1?")).arg(meta->title));
            } else {
//                warnDlg.setTitle(QString(tr("Are you sure you want to delete the selected %1 songs?")).arg(metalist.length()));
                DLabel *t_titleLabel = new DLabel(this);
                t_titleLabel->setForegroundRole(DPalette::TextTitle);
                DLabel *t_infoLabel = new DLabel(this);
                t_infoLabel->setForegroundRole(DPalette::TextTips);
                t_titleLabel->setText(tr("Are you sure you want to delete the selected %1 songs?").arg(metalist.length()));
                t_infoLabel->setText(tr("The song files contained will also be deleted"));
                warnDlg.addContent(t_titleLabel, Qt::AlignHCenter);
                warnDlg.addContent(t_infoLabel, Qt::AlignHCenter);
                warnDlg.addSpacing(20);
            }

            if (containsCue && false) {
                DLabel *t_titleLabel = new DLabel(this);
                t_titleLabel->setForegroundRole(DPalette::TextTitle);
                DLabel *t_infoLabel = new DLabel(this);
                t_infoLabel->setForegroundRole(DPalette::TextTips);
                t_titleLabel->setText(tr("Are you sure you want to delete the selected %1 songs?").arg(metalist.length()));
                t_infoLabel->setText(tr("The song files contained will also be deleted"));
                warnDlg.addContent(t_titleLabel, Qt::AlignHCenter);
                warnDlg.addContent(t_infoLabel, Qt::AlignHCenter);
                warnDlg.addSpacing(20);
            }
            auto coverPixmap =  QPixmap::fromImage(WidgetHelper::cropRect(cover, QSize(64, 64)));

            warnDlg.setIcon(QIcon::fromTheme("deepin-music"));
            if (deleteFlag == warnDlg.exec()) {
                Q_EMIT deleteMusicList(metalist);
            }
        });
    }

    if (songAction) {
        connect(songAction, &QAction::triggered, this, [ = ](bool) {
            auto index = selection->selectedRows().first();
            auto meta = d->model->meta(index);
            Q_EMIT showInfoDialog(meta);
        });
    }

    myMenu.exec(globalPos);
}

void PlayListView::mouseMoveEvent(QMouseEvent *event)
{
    DListView::mouseMoveEvent(event);
}

void PlayListView::dragEnterEvent(QDragEnterEvent *event)
{
    DListView::dragEnterEvent(event);
}

void PlayListView::startDrag(Qt::DropActions supportedActions)
{
    Q_D(PlayListView);

    MetaPtrList list;
    for (auto index : selectionModel()->selectedIndexes()) {
        list << d->model->meta(index);
    }

    if (!selectionModel()->selectedIndexes().isEmpty())
        scrollTo(selectionModel()->selectedIndexes().first());
    setAutoScroll(false);
    DListView::startDrag(supportedActions);
    setAutoScroll(true);

    QMap<QString, int> hashIndexs;
    for (int i = 0; i < d->model->rowCount(); ++i) {
        auto index = d->model->index(i, 0);
        auto hash = d->model->data(index).toString();
        Q_ASSERT(!hash.isEmpty());
        hashIndexs.insert(hash, i);
    }
    d->model->playlist()->saveSort(hashIndexs);
    Q_EMIT customSort();

    QItemSelection selection;
    for (auto meta : list) {
        if (!meta.isNull()) {
            auto index = this->findIndex(meta);
            selection.append(QItemSelectionRange(index));
        }
    }
    if (!selection.isEmpty()) {
        selectionModel()->select(selection, QItemSelectionModel::Select);
    }
}


ModelMake::ModelMake(QObject *parent)
{

}

ModelMake::~ModelMake()
{

}

void ModelMake::onModelMake(PlaylistPtr playlist, QString searchStr)
{
    bool chineseFlag = false;
    for (auto ch : searchStr) {
        if (DMusic::PinyinSearch::isChinese(ch)) {
            chineseFlag = true;
            break;
        }
    }
    MetaPtr meta;
    QString id = playlist->id();
    int count = 0;
    for (auto meta : playlist->allmusic()) {
        if (searchStr.isEmpty()) {
            Q_EMIT addMeta(id, meta);
        } else {
            if (chineseFlag) {
                if (meta->title.contains(searchStr, Qt::CaseInsensitive)) {
                    Q_EMIT addMeta(id, meta);
                }
            } else {
                if (playlist->searchStr().size() == 1) {
                    auto curTextList = DMusic::PinyinSearch::simpleChineseSplit(meta->title);
                    if (!curTextList.isEmpty() && curTextList.first().contains(searchStr, Qt::CaseInsensitive)) {
                        Q_EMIT addMeta(id, meta);
                    }
                } else {
                    auto curTextList = DMusic::PinyinSearch::simpleChineseSplit(meta->title);
                    if (!curTextList.isEmpty() && curTextList.join("").contains(searchStr, Qt::CaseInsensitive)) {
                        Q_EMIT addMeta(id, meta);
                    }
                }
            }
        }
        if (count % 12 == 0) {
            QThread::msleep(50);
        }
        count ++;
    }

    Q_EMIT hideEmptyHits(!(count > 0));
}
