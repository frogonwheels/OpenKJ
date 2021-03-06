#include "karaokefileinfo.h"
#include <QRegularExpression>
#include <QDebug>
#include <QTemporaryDir>
#include "tagreader.h"
#include "okarchive.h"
#include <QSqlQuery>

void KaraokeFileInfo::readTags()
{
    if (tagsRead)
        return;
    TagReader *tagReader = new TagReader(this);

    if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QString baseFn = fileName;
        QString mediaFile;
        baseFn.chop(3);
        if (QFile::exists(baseFn + "mp3"))
            mediaFile = baseFn + "mp3";
        else if (QFile::exists(baseFn + "Mp3"))
            mediaFile = baseFn + "Mp3";
        else if (QFile::exists(baseFn + "MP3"))
            mediaFile = baseFn + "MP3";
        else if (QFile::exists(baseFn + "mP3"))
            mediaFile = baseFn + "mP3";
        tagReader->setMedia(mediaFile);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        QString track = tagReader->getTrack();
        if (track != "")
        {
            tagSongid.append("-" + track);
        }
    }
    else if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        QTemporaryDir dir;
        archive.setArchiveFile(fileName);
        archive.checkAudio();
        QString audioFile = "temp" + archive.audioExtension();
        archive.extractAudio(dir.path(), audioFile);
        tagReader->setMedia(dir.path() + QDir::separator() + audioFile);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        duration = archive.getSongDuration();
        QString track = tagReader->getTrack();
        if (track != "")
        {
            tagSongid.append("-" + track);
        }
    }
    else
    {
        tagArtist = "Error";
        tagTitle = "Error";
        tagSongid = "Error";
        duration = 0;
        qWarning() << "KaraokeFileInfo::readTags() called on non zip or cdg file (" << fileName << ").  Something is terribly wrong...";
    }
    tagsRead = true;
    delete tagReader;
}

KaraokeFileInfo::KaraokeFileInfo(QObject *parent) : QObject(parent)
{
    duration = false;
    artistCaptureGroup = 0;
    titleCaptureGroup  = 0;
    songIdCaptureGroup = 0;
    useMetadata = false;
    tagsRead = false;
}

void KaraokeFileInfo::setFileName(QString filename)
{
    fileName = filename;
    fileBaseName = QFileInfo(filename).completeBaseName();
    tagArtist = "";
    tagTitle = "";
    tagSongid = "";
    duration = 0;
}

void KaraokeFileInfo::setPattern(SourceDir::NamingPattern pattern, QString path)
{
    useMetadata = false;
    int customPatternId = 0;
    switch (pattern)
    {
    case SourceDir::STA:
        setSongIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
        setArtistRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
        break;
    case SourceDir::SAT:
        setSongIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
        setTitleRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
        break;
    case SourceDir::ATS:
        setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
        setSongIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
        break;
    case SourceDir::TAS:
        setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
        setSongIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
        break;
    case SourceDir::AT:
        setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))");
        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
        break;
    case SourceDir::TA:
        setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))");
        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
        break;
    case SourceDir::S_T_A:
        setSongIdRegEx("^[^_]*_");
        setTitleRegEx("((?<=_)[^_]*_)");
        setArtistRegEx("([^_]+(?=_[^_]*(\\r?\\n|$)))");
        break;
    case SourceDir::METADATA:
        useMetadata = true;
        break;
    case SourceDir::CUSTOM:
        QSqlQuery query;
        query.exec("SELECT custompattern FROM sourcedirs WHERE path == \"" + path + "\"" );
        if (query.first())
        {
            customPatternId = query.value(0).toInt();
        }
        if (customPatternId < 1)
        {
            qCritical() << "Custom pattern set for path, but pattern ID is invalid!  Bailing out!";
            return;
        }
        query.exec("SELECT * FROM custompatterns WHERE patternid == " + QString::number(customPatternId));
        if (query.first())
        {
            setArtistRegEx(query.value("artistregex").toString(), query.value("artistcapturegrp").toInt());
            setTitleRegEx(query.value("titleregex").toString(), query.value("titlecapturegrp").toInt());
            setSongIdRegEx(query.value("discidregex").toString(), query.value("discidcapturegrp").toInt());
        }
        break;
    }
}

QString KaraokeFileInfo::getArtist()
{
    if (useMetadata)
    {
        readTags();
        return tagArtist;
    }
    QRegularExpression r(artistPattern);
    QRegularExpressionMatch match = r.match(fileBaseName);
    QString result = match.captured(artistCaptureGroup);
    result.replace("_", " ");
    return result;
}

QString KaraokeFileInfo::getTitle()
{
    if (useMetadata)
    {
        readTags();
        return tagTitle;
    }
    QRegularExpression r(titlePattern);
    QRegularExpressionMatch match = r.match(fileBaseName);
    QString result = match.captured(titleCaptureGroup);
    result.replace("_", " ");
    return result;
}

QString KaraokeFileInfo::getSongId()
{
    if (useMetadata)
    {
        readTags();
        return tagSongid;
    }
    QRegularExpression r(songIdPattern);
    QRegularExpressionMatch match = r.match(fileBaseName);
    QString result = match.captured(songIdCaptureGroup);
    result.replace("_", " ");
    return result;
}

int KaraokeFileInfo::getDuration()
{
    if (duration > 0)
        return duration;
    if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        archive.setArchiveFile(fileName);
        duration = archive.getSongDuration();
    }
    else if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
    {
        duration = ((QFile(fileName).size() / 96) / 75) * 1000;
    }
    else
    {
        TagReader reader;
        reader.setMedia(fileName);
        try
        {
            duration = reader.getDuration();
        }
        catch (...)
        {
            qWarning() << "KaraokeFileInfo unable to get duration for file: " << fileBaseName;
        }
    }
    return duration;
}
