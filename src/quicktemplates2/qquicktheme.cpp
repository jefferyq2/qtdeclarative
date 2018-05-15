/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktheme_p.h"
#include "qquicktheme_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QScopedPointer<QQuickTheme> QQuickThemePrivate::instance;

static QPlatformTheme::Font platformFont(QQuickTheme::Scope scope)
{
    switch (scope) {
    case QQuickTheme::Button: return QPlatformTheme::PushButtonFont;
    case QQuickTheme::CheckBox: return QPlatformTheme::CheckBoxFont;
    case QQuickTheme::ComboBox: return QPlatformTheme::ComboMenuItemFont;
    case QQuickTheme::GroupBox: return QPlatformTheme::GroupBoxTitleFont;
    case QQuickTheme::ItemView: return QPlatformTheme::ItemViewFont;
    case QQuickTheme::Label: return QPlatformTheme::LabelFont;
    case QQuickTheme::ListView: return QPlatformTheme::ListViewFont;
    case QQuickTheme::Menu: return QPlatformTheme::MenuFont;
    case QQuickTheme::MenuBar: return QPlatformTheme::MenuBarFont;
    case QQuickTheme::RadioButton: return QPlatformTheme::RadioButtonFont;
    case QQuickTheme::SpinBox: return QPlatformTheme::EditorFont;
    case QQuickTheme::Switch: return QPlatformTheme::CheckBoxFont;
    case QQuickTheme::TabBar: return QPlatformTheme::TabButtonFont;
    case QQuickTheme::TextArea: return QPlatformTheme::EditorFont;
    case QQuickTheme::TextField: return QPlatformTheme::EditorFont;
    case QQuickTheme::ToolBar: return QPlatformTheme::ToolButtonFont;
    case QQuickTheme::ToolTip: return QPlatformTheme::TipLabelFont;
    case QQuickTheme::Tumbler: return QPlatformTheme::ItemViewFont;
    default: return QPlatformTheme::SystemFont;
    }
}

static QPlatformTheme::Palette platformPalette(QQuickTheme::Scope scope)
{
    switch (scope) {
    case QQuickTheme::Button: return QPlatformTheme::ButtonPalette;
    case QQuickTheme::CheckBox: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::ComboBox: return QPlatformTheme::ComboBoxPalette;
    case QQuickTheme::GroupBox: return QPlatformTheme::GroupBoxPalette;
    case QQuickTheme::ItemView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Label: return QPlatformTheme::LabelPalette;
    case QQuickTheme::ListView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Menu: return QPlatformTheme::MenuPalette;
    case QQuickTheme::MenuBar: return QPlatformTheme::MenuBarPalette;
    case QQuickTheme::RadioButton: return QPlatformTheme::RadioButtonPalette;
    case QQuickTheme::SpinBox: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::Switch: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::TabBar: return QPlatformTheme::TabBarPalette;
    case QQuickTheme::TextArea: return QPlatformTheme::TextEditPalette;
    case QQuickTheme::TextField: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::ToolBar: return QPlatformTheme::ToolButtonPalette;
    case QQuickTheme::ToolTip: return QPlatformTheme::ToolTipPalette;
    case QQuickTheme::Tumbler: return QPlatformTheme::ItemViewPalette;
    default: return QPlatformTheme::SystemPalette;
    }
}

const QFont *QQuickThemePrivate::resolveThemeFont(QQuickTheme::Scope scope)
{
    Q_Q(QQuickTheme);
    if (!hasResolvedFonts) {
        q->resolveFonts(defaultFont ? *defaultFont : QFont());
        hasResolvedFonts = true;
        defaultFont.reset();
    }
    return q->font(scope);
}

const QPalette *QQuickThemePrivate::resolveThemePalette(QQuickTheme::Scope scope)
{
    Q_Q(QQuickTheme);
    if (!hasResolvedPalettes) {
        q->resolvePalettes(defaultPalette ? *defaultPalette : QPalette());
        hasResolvedPalettes = true;
        defaultPalette.reset();
    }
    return q->palette(scope);
}

QQuickTheme::QQuickTheme()
    : d_ptr(new QQuickThemePrivate)
{
    d_ptr->q_ptr = this;
}

QQuickTheme::~QQuickTheme()
{
}

QQuickTheme *QQuickTheme::instance()
{
    return QQuickThemePrivate::instance.data();
}

QFont QQuickTheme::themeFont(Scope scope)
{
    const QFont *font = nullptr;
    if (QQuickTheme *theme = instance())
        font = QQuickThemePrivate::get(theme)->resolveThemeFont(scope);
    else if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        font = theme->font(platformFont(scope));

    if (font) {
        QFont f = *font;
        if (scope == System)
            f.resolve(0);
        return f;
    }

    return QFont();
}

QPalette QQuickTheme::themePalette(Scope scope)
{
    const QPalette *palette = nullptr;
    if (QQuickTheme *theme = instance())
        palette = QQuickThemePrivate::get(theme)->resolveThemePalette(scope);
    else if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        palette = theme->palette(platformPalette(scope));

    if (palette) {
        QPalette f = *palette;
        if (scope == System)
            f.resolve(0);
        return f;
    }

    return QPalette();
}

const QFont *QQuickTheme::font(Scope scope) const
{
    Q_D(const QQuickTheme);
    Q_UNUSED(scope)
    return d->defaultFont.data();
}

const QPalette *QQuickTheme::palette(Scope scope) const
{
    Q_D(const QQuickTheme);
    Q_UNUSED(scope)
    return d->defaultPalette.data();
}

void QQuickTheme::resolveFonts(const QFont &defaultFont)
{
    Q_UNUSED(defaultFont)
}

void QQuickTheme::resolvePalettes(const QPalette &defaultPalette)
{
    Q_UNUSED(defaultPalette)
}

QT_END_NAMESPACE
