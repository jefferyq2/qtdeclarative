/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlprofilertypedevent_p.h"
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

QDataStream &operator>>(QDataStream &stream, QQmlProfilerTypedEvent &event)
{
    qint64 time;
    qint32 messageType;
    qint32 subtype;

    stream >> time >> messageType;

    if (messageType < 0 || messageType > QQmlProfilerDefinitions::MaximumMessage)
        messageType = QQmlProfilerDefinitions::MaximumMessage;

    QQmlProfilerDefinitions::RangeType rangeType = QQmlProfilerDefinitions::MaximumRangeType;
    if (!stream.atEnd()) {
        stream >> subtype;
        rangeType = static_cast<QQmlProfilerDefinitions::RangeType>(subtype);
        if (rangeType < 0 || rangeType > QQmlProfilerDefinitions::MaximumRangeType)
            rangeType = QQmlProfilerDefinitions::MaximumRangeType;
    } else {
        subtype = -1;
    }

    event.event.setTimestamp(time > 0 ? time : 0);
    event.event.setTypeIndex(-1);
    event.serverTypeId = 0;

    switch (messageType) {
    case QQmlProfilerDefinitions::Event: {
        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype);
        switch (subtype) {
        case QQmlProfilerDefinitions::StartTrace:
        case QQmlProfilerDefinitions::EndTrace: {
            QVarLengthArray<qint32> engineIds;
            while (!stream.atEnd()) {
                qint32 id;
                stream >> id;
                engineIds << id;
            }
            event.event.setNumbers<QVarLengthArray<qint32>, qint32>(engineIds);
            break;
        }
        case QQmlProfilerDefinitions::AnimationFrame: {
            qint32 frameRate, animationCount;
            qint32 threadId;
            stream >> frameRate >> animationCount;
            if (!stream.atEnd())
                stream >> threadId;
            else
                threadId = 0;

            event.event.setNumbers<qint32>({frameRate, animationCount, threadId});
            break;
        }
        case QQmlProfilerDefinitions::Mouse:
        case QQmlProfilerDefinitions::Key:
            int inputType = (subtype == QQmlProfilerDefinitions::Key
                             ? QQmlProfilerDefinitions::InputKeyUnknown
                             : QQmlProfilerDefinitions::InputMouseUnknown);
            if (!stream.atEnd())
                stream >> inputType;
            qint32 a = -1;
            if (!stream.atEnd())
                stream >> a;
            qint32 b = -1;
            if (!stream.atEnd())
                stream >> b;

            event.event.setNumbers<qint32>({inputType, a, b});
            break;
        }

        break;
    }
    case QQmlProfilerDefinitions::Complete: {
        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype);
        break;
    }
    case QQmlProfilerDefinitions::SceneGraphFrame: {
        QVarLengthArray<qint64> params;
        qint64 param;

        while (!stream.atEnd()) {
            stream >> param;
            params.push_back(param);
        }

        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype);
        event.event.setNumbers<QVarLengthArray<qint64>, qint64>(params);
        break;
    }
    case QQmlProfilerDefinitions::PixmapCacheEvent: {
        qint32 width = 0, height = 0, refcount = 0;
        QString filename;
        stream >> filename;
        if (subtype == QQmlProfilerDefinitions::PixmapReferenceCountChanged
                || subtype == QQmlProfilerDefinitions::PixmapCacheCountChanged) {
            stream >> refcount;
        } else if (subtype == QQmlProfilerDefinitions::PixmapSizeKnown) {
            stream >> width >> height;
            refcount = 1;
        }

        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype,
                    QQmlProfilerEventLocation(filename, 0, 0));
        event.event.setNumbers<qint32>({width, height, refcount});
        break;
    }
    case QQmlProfilerDefinitions::MemoryAllocation: {
        qint64 delta;
        stream >> delta;

        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype);
        event.event.setNumbers<qint64>({delta});
        break;
    }
    case QQmlProfilerDefinitions::RangeStart: {
        if (!stream.atEnd()) {
            qint64 typeId;
            stream >> typeId;
            if (stream.status() == QDataStream::Ok)
                event.serverTypeId = typeId;
            // otherwise it's the old binding type of 4 bytes
        }

        event.type = QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage, rangeType, -1);
        event.event.setRangeStage(QQmlProfilerDefinitions::RangeStart);
        break;
    }
    case QQmlProfilerDefinitions::RangeData: {
        QString data;
        stream >> data;

        event.type = QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage, rangeType, -1,
                                           QQmlProfilerEventLocation(), data);
        event.event.setRangeStage(QQmlProfilerDefinitions::RangeData);
        if (!stream.atEnd())
            stream >> event.serverTypeId;
        break;
    }
    case QQmlProfilerDefinitions::RangeLocation: {
        QString filename;
        qint32 line = 0;
        qint32 column = 0;
        stream >> filename >> line;

        if (!stream.atEnd()) {
            stream >> column;
            if (!stream.atEnd())
                stream >> event.serverTypeId;
        }

        event.type = QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage, rangeType, -1,
                                           QQmlProfilerEventLocation(filename, line, column));
        event.event.setRangeStage(QQmlProfilerDefinitions::RangeLocation);
        break;
    }
    case QQmlProfilerDefinitions::RangeEnd: {
        event.type = QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage, rangeType, -1);
        event.event.setRangeStage(QQmlProfilerDefinitions::RangeEnd);
        break;
    }
    default:
        event.event.setNumbers<char>({});
        event.type = QQmlProfilerEventType(
                    static_cast<QQmlProfilerDefinitions::Message>(messageType),
                    QQmlProfilerDefinitions::MaximumRangeType, subtype);
        break;
    }

    return stream;
}

QT_END_NAMESPACE
