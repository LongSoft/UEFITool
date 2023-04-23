#pragma once

#include <QObject>
#include "qhexoptions.h"
#include "qhexutils.h"

class QHexView;

class QHexCursor : public QObject
{
    Q_OBJECT

    public:
        enum class Mode { Overwrite, Insert };

    private:
        explicit QHexCursor(const QHexOptions* options, QHexView *parent = nullptr);

    public:
        QHexView* hexView() const;
        Mode mode() const;
        qint64 line() const;
        qint64 column() const;
        qint64 offset() const;
        qint64 address() const;
        quint64 lineAddress() const;
        qint64 selectionStartOffset() const;
        qint64 selectionEndOffset() const;
        qint64 selectionLength() const;
        QHexPosition position() const;
        QHexPosition selectionStart() const;
        QHexPosition selectionEnd() const;
        QByteArray selectedBytes() const;
        bool hasSelection() const;
        bool isSelected(qint64 line, qint64 column) const;
        void setMode(Mode m);
        void move(qint64 offset);
        void move(qint64 line, qint64 column);
        void move(QHexPosition pos);
        void select(qint64 offset);
        void select(qint64 line, qint64 column);
        void select(QHexPosition pos);
        void selectSize(qint64 length);
        qint64 replace(const QVariant& oldvalue, const QVariant& newvalue, qint64 offset, QHexFindMode mode = QHexFindMode::Text, unsigned int options = QHexFindOptions::None, QHexFindDirection fd = QHexFindDirection::Forward) const;
        qint64 find(const QVariant& value, qint64 offset, QHexFindMode mode = QHexFindMode::Text, unsigned int options = QHexFindOptions::None, QHexFindDirection fd = QHexFindDirection::Forward) const;
        qint64 positionToOffset(QHexPosition pos) const;
        QHexPosition offsetToPosition(qint64 offset) const;

    public Q_SLOTS:
        void cut(bool hex = false);
        void copy(bool hex = false) const;
        void paste(bool hex = false);
        void selectAll();
        void removeSelection();
        void clearSelection();
        void switchMode();

    Q_SIGNALS:
        void positionChanged();
        void modeChanged();

    private:
        const QHexOptions* m_options;
        Mode m_mode{Mode::Overwrite};
        QHexPosition m_position{}, m_selection{};

    friend class QHexView;
};

