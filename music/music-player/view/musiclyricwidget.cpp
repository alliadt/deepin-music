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

#include "musiclyricwidget.h"

#include <QDebug>
#include <QFile>
#include <QScrollArea>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QStringListModel>
#include <QAbstractItemDelegate>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

#include <DPalette>
#include <DPushButton>

#include "../core/util/lyric.h"
#include "../core/musicsettings.h"
#include "../core/metasearchservice.h"

#include "widget/cover.h"
#include "widget/searchmetalist.h"
#include "widget/searchmetaitem.h"
#include "widget/searchlyricswidget.h"
#include "widget/lyriclabel.h"
#include "widget/musicimagebutton.h"
#include "musicsettings.h"
#include <DGuiApplicationHelper>
#include <DBlurEffectWidget>
DGUI_USE_NAMESPACE

//static const int LyricLineHeight = 40;
static const QString defaultLyric = "No Lyric";

class MusicLyricWidgetPrivate
{
public:
    explicit MusicLyricWidgetPrivate(MusicLyricWidget *parent): q_ptr(parent) {}

    MetaPtr             activingMeta;

    Cover               *m_cover              = nullptr;

    SearchLyricsWidget  *searchLyricsWidget   = nullptr;
    LyricLabel          *lyricview            = nullptr;
    DLabel              *nolyric              = nullptr;

    MusicImageButton         *serachbt = nullptr;

    QString             defaultCover = ":/common/image/cover_max.svg";

    bool               serachflag = false;

    QImage              backgroundimage;

    DBlurEffectWidget *backgroundW = nullptr;
    MusicLyricWidget *q_ptr;
    Q_DECLARE_PUBLIC(MusicLyricWidget)
};

MusicLyricWidget::MusicLyricWidget(QWidget *parent)
    : DWidget(parent), d_ptr(new MusicLyricWidgetPrivate(this))
{
    Q_D(MusicLyricWidget);

    setAutoFillBackground(true);
    auto palette = this->palette();
    palette.setColor(DPalette::Background, QColor("#F8F8F8"));
    setPalette(palette);

    d->backgroundW = new DBlurEffectWidget(this);
//    d->backgroundW->setBlurEnabled(true);
//    d->backgroundW->setMode(DBlurEffectWidget::GaussianBlur);

    auto mainlayout = new QHBoxLayout(this);
    mainlayout->setMargin(0);
    mainlayout->setSpacing(0);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(20, 20, 20, 20);

    auto musicDir =  QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
    d->searchLyricsWidget = new SearchLyricsWidget(musicDir.first());
    d->searchLyricsWidget->hide();

    d->m_cover = new Cover;
    d->m_cover->setFixedSize(200, 200);
    d->m_cover->setObjectName("LyricCover");

    m_leftLayout = new QHBoxLayout();
    m_leftLayout->setContentsMargins(120, 0, 140, 0);
    m_leftLayout->addWidget(d->m_cover, Qt::AlignLeft | Qt::AlignVCenter);
    m_leftLayout->addWidget(d->searchLyricsWidget);

    d->lyricview = new LyricLabel(false);
    d->nolyric = new DLabel();
    d->nolyric->setAlignment(Qt::AlignCenter);
    d->nolyric->setText(tr("No lyrics yet"));
    QPalette nolyr = d->nolyric->palette();
    nolyr.setColor(QPalette::WindowText, QColor(85, 85, 85, 102));
    d->nolyric->setPalette(nolyr);

    auto searchlayout = new QVBoxLayout();
    d->serachbt = new MusicImageButton(":/mpimage/light/normal/search_normal.svg",
                                       ":/mpimage/light/normal/search_normal.svg",
                                       ":/mpimage/light/normal/search_normal.svg");
    d->serachbt->setProperty("typeName", true);
    d->serachbt->setPropertyPic("typeName", false, ":/mpimage/light/normal/back_normal.svg",
                                ":/mpimage/light/normal/back_normal.svg",
                                ":/mpimage/light/normal/back_normal.svg");
    d->serachbt->setFixedSize(48, 48);
    d->serachbt->hide();

    searchlayout->addWidget(d->serachbt);
    searchlayout->addStretch();
    searchlayout->setContentsMargins(58, 18, 34, 484);

    layout->addLayout(m_leftLayout, 0);
    layout->addWidget(d->lyricview, 10);
    layout->addWidget(d->nolyric, 10);
    d->nolyric->hide();
    layout->addLayout(searchlayout, 0);

    d->backgroundW->setLayout(layout);
    mainlayout->addWidget(d->backgroundW);
//    bool themeFlag = false;
//    int themeType = MusicSettings::value("base.play.theme").toInt(&themeFlag);
//    if (!themeFlag)
//        themeType = 1;
    int themeType = DGuiApplicationHelper::instance()->themeType();
    slotTheme(themeType);

    connect(d->serachbt, &DPushButton::clicked, this, &MusicLyricWidget::onsearchBt);
    connect(d->searchLyricsWidget, &SearchLyricsWidget::lyricPath, this, &MusicLyricWidget::slotonsearchresult);
}

MusicLyricWidget::~MusicLyricWidget()
{

}

void MusicLyricWidget::updateUI()
{
    Q_D(MusicLyricWidget);
    d->m_cover->setCoverPixmap(QPixmap(d->defaultCover));
    QImage cover(d->defaultCover);

    //cut image
    double windowScale = (width() * 1.0) / height();
    int imageWidth = static_cast<int>(cover.height() * windowScale) ;
    QImage coverImage;
    if (imageWidth > cover.width()) {
        int imageheight = static_cast<int>(cover.width() / windowScale);
        coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
    } else {
        int imageheight = cover.height();
        coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
    }
    d->backgroundW->setSourceImage(coverImage);
    d->backgroundimage = cover;
}

QString MusicLyricWidget::defaultCover() const
{
    Q_D(const MusicLyricWidget);
    return d->defaultCover;
}

void MusicLyricWidget::checkHiddenSearch(QPoint mousePos)
{
    Q_UNUSED(mousePos)
    //Q_D(MusicLyricWidget);

}

void MusicLyricWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(MusicLyricWidget);
    QImage cover(d->backgroundimage);

    //cut image
    double windowScale = (width() * 1.0) / height();
    int imageWidth = static_cast<int>(cover.height() * windowScale);
    QImage coverImage;
    if (imageWidth > cover.width()) {
        int imageheight = static_cast<int>(cover.width() / windowScale);
        coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
    } else {
        int imageheight = cover.height();
        coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
    }
    d->backgroundW->setSourceImage(coverImage);
    d->backgroundW->update();
    QWidget::resizeEvent(event);
}

void MusicLyricWidget::mousePressEvent(QMouseEvent *event)
{
    Q_D(MusicLyricWidget);
    if (d->serachflag && !d->searchLyricsWidget->rect().contains(event->pos())) {
        onsearchBt();
    }
    QWidget::mousePressEvent(event);
}

void MusicLyricWidget::onMusicPlayed(PlaylistPtr playlist, const MetaPtr meta)
{
    Q_D(MusicLyricWidget);
    Q_UNUSED(playlist);
    d->activingMeta = meta;

    QFileInfo fileInfo(meta->localPath);
    QString lrcPath = fileInfo.dir().path() + QDir::separator() + fileInfo.completeBaseName() + ".lrc";
    QFile file(lrcPath);
    if (!file.exists()) {
        d->nolyric->show();
        d->lyricview->hide();
    } else {
        d->nolyric->hide();
        d->lyricview->show();
    }
    d->lyricview->getFromFile(lrcPath);
    d->searchLyricsWidget->setSearchDir(fileInfo.dir().path() + QDir::separator());

    QImage cover(d->defaultCover);
    auto coverData = MetaSearchService::coverData(meta);
    if (coverData.length() > 0) {
        cover = QImage::fromData(coverData);
    }

    d->m_cover->setCoverPixmap(QPixmap::fromImage(cover));
    d->m_cover->update();
    d->backgroundimage = cover;
    //cut image
    double windowScale = (width() * 1.0) / height();
    int imageWidth = static_cast<int>(cover.height() * windowScale);
    QImage coverImage;
    if (imageWidth > cover.width()) {
        int imageheight = static_cast<int>(cover.width() / windowScale);
        coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
    } else {
        int imageheight = cover.height();
        coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
    }
    d->backgroundW->setSourceImage(coverImage);
    d->backgroundW->update();
}

void MusicLyricWidget::onMusicStop(PlaylistPtr playlist, const MetaPtr meta)
{
    Q_UNUSED(playlist)
    Q_D(MusicLyricWidget);

    QImage cover(d->defaultCover);
    auto coverData = MetaSearchService::coverData(meta);
    if (coverData.length() > 0) {
        cover = QImage::fromData(coverData);
    }
    d->m_cover->setCoverPixmap(QPixmap::fromImage(cover));
    d->m_cover->update();

    d->backgroundimage = cover;
    //cut image
    double windowScale = (width() * 1.0) / height();
    int imageWidth = static_cast<int>(cover.height() * windowScale);
    QImage coverImage;
    if (imageWidth > cover.width()) {
        int imageheight = static_cast<int>(cover.width() / windowScale);
        coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
    } else {
        int imageheight = cover.height();
        coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
    }
    d->backgroundW->setSourceImage(coverImage);
    d->backgroundW->update();

    d->lyricview->getFromFile("clearLyric");
}

void MusicLyricWidget::onProgressChanged(qint64 value, qint64 /*length*/)
{
    Q_D(MusicLyricWidget);

    d->lyricview->postionChanged(value);
}

void MusicLyricWidget::onLyricChanged(const MetaPtr meta, const DMusic::SearchMeta &search,  const QByteArray &lyricData)
{
    Q_UNUSED(search)
    Q_UNUSED(lyricData)
    Q_D(MusicLyricWidget);
    if (d->activingMeta != meta) {
        return;
    }
}

void MusicLyricWidget::onCoverChanged(const MetaPtr meta,  const DMusic::SearchMeta &search, const QByteArray &coverData)
{
    Q_UNUSED(search)
    Q_D(MusicLyricWidget);
    if (d->activingMeta != meta) {
        return;
    }

    QImage cover(d->defaultCover);
    if (coverData.length() > 0) {
        cover = QImage::fromData(coverData);
    }
    d->backgroundimage = cover;
    d->m_cover->setCoverPixmap(QPixmap::fromImage(cover));
    d->m_cover->update();

    //cut image
    double windowScale = (width() * 1.0) / height();
    int imageWidth = static_cast<int>(cover.height() * windowScale);
    QImage coverImage;
    if (imageWidth > cover.width()) {
        int imageheight = static_cast<int>(cover.width() / windowScale);
        coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
    } else {
        int imageheight = cover.height();
        coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
    }
    d->backgroundW->setSourceImage(coverImage);
    d->backgroundW->update();
}

void MusicLyricWidget::setDefaultCover(QString defaultCover)
{
    Q_D(MusicLyricWidget);
    d->defaultCover = defaultCover;
}

void MusicLyricWidget::onUpdateMetaCodec(const MetaPtr /*meta*/)
{
    //Q_D(MusicLyricWidget);

//    if (d->m_playingMusic == meta) {
//        d->m_playingMusic.title = meta.title;
//        d->m_playingMusic.artist = meta.artist;
//        d->m_playingMusic.album = meta.album;
    //    }
}

void MusicLyricWidget::onsearchBt()
{
    Q_D(MusicLyricWidget);
    d->serachflag = !d->serachflag;
    if (d->serachflag) {
        d->serachbt->setProperty("typeName", false);
        d->m_cover->hide();
        d->searchLyricsWidget->show();
        if (d->activingMeta != nullptr) {
            d->searchLyricsWidget->setDefault(d->activingMeta->title, d->activingMeta->artist);
        } else {
            d->searchLyricsWidget->setDefault("", "");
        }

        m_leftLayout->setContentsMargins(51, 21, 51, 19);
    } else {
        d->serachbt->setProperty("typeName", true);
        d->m_cover->show();
        d->searchLyricsWidget->hide();
        m_leftLayout->setContentsMargins(120, 190, 140, 160);
    }
    d->serachbt->update();
}

void MusicLyricWidget::slotonsearchresult(QString path)
{
    Q_D(MusicLyricWidget);
    d->lyricview->getFromFile(path);
}

void MusicLyricWidget::slotTheme(int type)
{
    Q_D(MusicLyricWidget);

    if (type == 0)
        type = DGuiApplicationHelper::instance()->themeType();
    QString rStr;
    if (type == 1) {
        rStr = "light";
    } else {
        rStr = "dark";
    }
    if (type == 1) {
        auto palette = this->palette();
        palette.setColor(DPalette::Background, QColor("#F8F8F8"));
        setPalette(palette);

        QColor backMaskColor(255, 255, 255, 140);
        d->backgroundW->setMaskColor(backMaskColor);
    } else {
        auto palette = this->palette();
        palette.setColor(DPalette::Background, QColor("#252525"));
        setPalette(palette);

        QColor backMaskColor(37, 37, 37, 140);
        d->backgroundW->setMaskColor(backMaskColor);
    }
    d->serachbt->setPropertyPic(QString(":/mpimage/%1/normal/search_normal.svg").arg(rStr),
                                QString(":/mpimage/%1/normal/search_normal.svg").arg(rStr),
                                QString(":/mpimage/%1/normal/search_normal.svg").arg(rStr));

    d->serachbt->setPropertyPic("typeName", false, QString(":/mpimage/%1/normal/back_normal.svg").arg(rStr),
                                QString(":/mpimage/%1/normal/back_normal.svg").arg(rStr),
                                QString(":/mpimage/%1/normal/back_normal.svg").arg(rStr));

    d->searchLyricsWidget->setThemeType(type);
    d->lyricview->slotTheme(type);
}

void MusicLyricWidget::onContextSearchFinished(const QString &context, const QList<DMusic::SearchMeta> &metalist)
{
    //Q_D(MusicLyricWidget);

    //TODO: check context
    Q_UNUSED(context);
    Q_UNUSED(metalist)
}


