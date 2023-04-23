#pragma once

#include <functional>
#include <QObject>
#include <QHash>
#include <QList>
#include <QColor>
#include "qhexoptions.h"

struct QHexMetadataItem
{
    qint64 begin, end;
    QColor foreground, background;
    QString comment;
};

using QHexMetadataLine = QList<QHexMetadataItem>;

class QHexMetadata : public QObject
{
    Q_OBJECT

    private:
        using ClearMetadataCallback = std::function<bool(QHexMetadataItem&)>;

    private:
        explicit QHexMetadata(const QHexOptions* options, QObject *parent = nullptr);

    public:
        const QHexMetadataLine* find(qint64 line) const;
        QString getComment(qint64 line, qint64 column) const;
        void removeMetadata(qint64 line);
        void removeBackground(qint64 line);
        void removeForeground(qint64 line);
        void removeComments(qint64 line);
        void unhighlight(qint64 line);
        void clear();

    public:
        inline void setMetadata(qint64 begin, qint64 end, const QColor &fgcolor, const QColor &bgcolor, const QString &comment) { this->setMetadata({begin, end, fgcolor, bgcolor, comment}); }
        inline void setForeground(qint64 begin, qint64 end, const QColor &fgcolor) { this->setMetadata(begin, end, fgcolor, QColor(), QString()); }
        inline void setBackground(qint64 begin, qint64 end, const QColor &bgcolor) { this->setMetadata(begin, end, QColor(), bgcolor, QString()); }
        inline void setComment(qint64 begin, qint64 end, const QString& comment) { this->setMetadata(begin, end, QColor(), QColor(), comment); };
        inline void setMetadataSize(qint64 begin, qint64 length, const QColor &fgcolor, const QColor &bgcolor, const QString &comment) { this->setMetadata({begin, begin + length, fgcolor, bgcolor, comment}); }
        inline void setForegroundSize(qint64 begin, qint64 length, const QColor &fgcolor) { this->setForeground(begin, begin + length, fgcolor); }
        inline void setBackgroundSize(qint64 begin, qint64 length, const QColor &bgcolor) { this->setBackground(begin, begin + length, bgcolor); }
        inline void setCommentSize(qint64 begin, qint64 length, const QString& comment) { this->setComment(begin, begin + length, comment); };

    private:
        void copy(const QHexMetadata* metadata);
        void clearMetadata(qint64 line, ClearMetadataCallback&& cb);
        void setMetadata(const QHexMetadataItem& mi);
        void invalidate();

    Q_SIGNALS:
        void changed();
        void cleared();

    private:
        QHash<qint64, QHexMetadataLine> m_metadata;
        const QHexOptions* m_options;

    friend class QHexView;
};
