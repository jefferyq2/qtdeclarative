// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtTest/qsignalspy.h>
#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qboxlayout.h>
#endif
#ifdef QT_QUICKWIDGETS_LIB
#include <QtQuickWidgets/qquickwidget.h>
#endif

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_QQuickShortcut : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickShortcut();

private slots:
    void standardShortcuts_data();
    void standardShortcuts();
    void shortcuts_data();
    void shortcuts();
    void sequence_data();
    void sequence();
    void context_data();
    void context();
    void contextChange_data();
    void contextChange();
    void matcher_data();
    void matcher();
    void multiple_data();
    void multiple();
    void embedded_data();
    void embedded();
#ifdef QT_QUICKWIDGETS_LIB
    void quickWidgetShortcuts_data();
    void quickWidgetShortcuts();
#endif
};

Q_DECLARE_METATYPE(Qt::Key)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)

static const bool EnabledShortcut = true;
static const bool DisabledShortcut = false;

static QVariant shortcutMap(const QVariant &sequence, Qt::ShortcutContext context, bool enabled = true, bool autoRepeat = true)
{
    QVariantMap s;
    s["sequence"] = sequence;
    s["enabled"] = enabled;
    s["context"] = context;
    s["autoRepeat"] = autoRepeat;
    return s;
}

static QVariant shortcutMap(const QVariant &key, bool enabled = true, bool autoRepeat = true)
{
    return shortcutMap(key, Qt::WindowShortcut, enabled, autoRepeat);
}

tst_QQuickShortcut::tst_QQuickShortcut()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickShortcut::standardShortcuts_data()
{
    QTest::addColumn<QKeySequence::StandardKey>("standardKey");
    QTest::newRow("Close") << QKeySequence::Close;
    QTest::newRow("Cut") << QKeySequence::Cut;
    QTest::newRow("NextChild") << QKeySequence::NextChild;
    QTest::newRow("PreviousChild") << QKeySequence::PreviousChild;
    QTest::newRow("FindNext") << QKeySequence::FindNext;
    QTest::newRow("FindPrevious") << QKeySequence::FindPrevious;
    QTest::newRow("FullScreen") << QKeySequence::FullScreen;
}

void tst_QQuickShortcut::standardShortcuts()
{
    QFETCH(QKeySequence::StandardKey, standardKey);

    QQmlApplicationEngine engine;

    engine.load(testFileUrl("multiple.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QObject *shortcut = window->property("shortcut").value<QObject *>();
    QVERIFY(shortcut);

    // create list of shortcuts
    QVariantList shortcuts;
    shortcuts.push_back(standardKey);
    shortcut->setProperty("sequences", shortcuts);

    // test all:
    QList<QKeySequence> allsequences = QKeySequence::keyBindings(standardKey);
    for (const QKeySequence &s : allsequences) {
        window->setProperty("activated", false);
        QTest::keySequence(window, s);
        QCOMPARE(window->property("activated").toBool(), true);
    }
}

void tst_QQuickShortcut::shortcuts_data()
{
    QTest::addColumn<QVariantList>("shortcuts");
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("activatedShortcut");
    QTest::addColumn<QString>("ambiguousShortcut");

    QVariantList shortcuts;
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "M" << "";
    QTest::newRow("Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "Alt+M" << "";
    QTest::newRow("Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "Ctrl+M" << "";
    QTest::newRow("Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+M" << "";
    QTest::newRow("Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "Ctrl+Alt+M" << "";
    QTest::newRow("+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "+" << "";
    QTest::newRow("F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "F1" << "";
    QTest::newRow("Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+F1" << "";

    // ambiguous
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("?M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "M";
    QTest::newRow("?Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "" << "Alt+M";
    QTest::newRow("?Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "" << "Ctrl+M";
    QTest::newRow("?Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "Shift+M";
    QTest::newRow("?Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "" << "Ctrl+Alt+M";
    QTest::newRow("?+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "+";
    QTest::newRow("?F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "F1";
    QTest::newRow("?Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "Shift+F1";

    // disabled
    shortcuts.clear();
    shortcuts << shortcutMap("M", DisabledShortcut)
              << shortcutMap("Alt+M", DisabledShortcut)
              << shortcutMap("Ctrl+M", DisabledShortcut)
              << shortcutMap("Shift+M", DisabledShortcut)
              << shortcutMap("Ctrl+Alt+M", DisabledShortcut)
              << shortcutMap("+", DisabledShortcut)
              << shortcutMap("F1", DisabledShortcut)
              << shortcutMap("Shift+F1", DisabledShortcut);

    QTest::newRow("!M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "" << "";
    QTest::newRow("!Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "" << "";
    QTest::newRow("!Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "";
    QTest::newRow("!Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "" << "";
    QTest::newRow("!+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "";

    // unambigous because others disabled
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("/M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "M" << "";
    QTest::newRow("/Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "Alt+M" << "";
    QTest::newRow("/Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "Ctrl+M" << "";
    QTest::newRow("/Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+M" << "";
    QTest::newRow("/Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "Ctrl+Alt+M" << "";
    QTest::newRow("/+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "+" << "";
    QTest::newRow("/F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "F1" << "";
    QTest::newRow("/Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+F1" << "";
}

void tst_QQuickShortcut::shortcuts()
{
    QFETCH(QVariantList, shortcuts);
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, activatedShortcut);
    QFETCH(QString, ambiguousShortcut);

    QQmlApplicationEngine engine;
    engine.load(testFileUrl("shortcuts.qml"));

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    window->setProperty("shortcuts", shortcuts);

    QTest::keyPress(window, key, modifiers);
    QTest::keyRelease(window, key, modifiers);
    QCOMPARE(window->property("activatedShortcut").toString(), activatedShortcut);
    QCOMPARE(window->property("ambiguousShortcut").toString(), ambiguousShortcut);
}

void tst_QQuickShortcut::sequence_data()
{
    QTest::addColumn<QVariantList>("shortcuts");
    QTest::addColumn<Qt::Key>("key1");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers1");
    QTest::addColumn<Qt::Key>("key2");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers2");
    QTest::addColumn<Qt::Key>("key3");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers3");
    QTest::addColumn<Qt::Key>("key4");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers4");
    QTest::addColumn<QString>("activatedShortcut");
    QTest::addColumn<QString>("ambiguousShortcut");

    QVariantList shortcuts;
    shortcuts << shortcutMap("Escape,W")
              << shortcutMap("Ctrl+X,Ctrl+C")
              << shortcutMap("Shift+Ctrl+4,Space")
              << shortcutMap("Alt+T,Ctrl+R,Shift+J,H");

    QTest::newRow("Escape,W") << shortcuts << Qt::Key_Escape << Qt::KeyboardModifiers(Qt::NoModifier)
                                           << Qt::Key_W << Qt::KeyboardModifiers(Qt::NoModifier)
                                           << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                           << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                           << "Escape,W" << "";

    QTest::newRow("Ctrl+X,Ctrl+C") << shortcuts << Qt::Key_X << Qt::KeyboardModifiers(Qt::ControlModifier)
                                                << Qt::Key_C << Qt::KeyboardModifiers(Qt::ControlModifier)
                                                << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                                << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                                << "Ctrl+X,Ctrl+C" << "";

    QTest::newRow("Shift+Ctrl+4,Space") << shortcuts << Qt::Key_4 << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::ShiftModifier)
                                                     << Qt::Key_Space << Qt::KeyboardModifiers(Qt::NoModifier)
                                                     << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                                     << Qt::Key(0) << Qt::KeyboardModifiers(Qt::NoModifier)
                                                     << "Shift+Ctrl+4,Space" << "";

    QTest::newRow("Alt+T,Ctrl+R,Shift+J,H") << shortcuts << Qt::Key_T << Qt::KeyboardModifiers(Qt::AltModifier)
                                                         << Qt::Key_R << Qt::KeyboardModifiers(Qt::ControlModifier)
                                                         << Qt::Key_J << Qt::KeyboardModifiers(Qt::ShiftModifier)
                                                         << Qt::Key_H << Qt::KeyboardModifiers(Qt::NoModifier)
                                                         << "Alt+T,Ctrl+R,Shift+J,H" << "";
}

void tst_QQuickShortcut::sequence()
{
    QFETCH(QVariantList, shortcuts);
    QFETCH(Qt::Key, key1);
    QFETCH(Qt::KeyboardModifiers, modifiers1);
    QFETCH(Qt::Key, key2);
    QFETCH(Qt::KeyboardModifiers, modifiers2);
    QFETCH(Qt::Key, key3);
    QFETCH(Qt::KeyboardModifiers, modifiers3);
    QFETCH(Qt::Key, key4);
    QFETCH(Qt::KeyboardModifiers, modifiers4);
    QFETCH(QString, activatedShortcut);
    QFETCH(QString, ambiguousShortcut);

    QQmlApplicationEngine engine;
    engine.load(testFileUrl("shortcuts.qml"));

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    window->setProperty("shortcuts", shortcuts);

    if (key1 != 0) {
        QTest::keyPress(window, key1, modifiers1);
        QTest::keyRelease(window, key1, modifiers1);
    }
    if (key2 != 0) {
        QTest::keyPress(window, key2, modifiers2);
        QTest::keyRelease(window, key2, modifiers2);
    }
    if (key3 != 0) {
        QTest::keyPress(window, key3, modifiers3);
        QTest::keyRelease(window, key3, modifiers3);
    }
    if (key4 != 0) {
        QTest::keyPress(window, key4, modifiers4);
        QTest::keyRelease(window, key4, modifiers4);
    }

    QCOMPARE(window->property("activatedShortcut").toString(), activatedShortcut);
    QCOMPARE(window->property("ambiguousShortcut").toString(), ambiguousShortcut);
}

void tst_QQuickShortcut::context_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<QVariantList>("activeWindowShortcuts");
    QTest::addColumn<QVariantList>("inactiveWindowShortcuts");
    QTest::addColumn<QString>("activeWindowActivatedShortcut");
    QTest::addColumn<QString>("inactiveWindowActivatedShortcut");
    QTest::addColumn<QString>("ambiguousShortcut");

    // APP: F1,(F2),F3,(F4) / WND: F7,(F8),F9,(F10)
    QVariantList activeWindowShortcuts;
    activeWindowShortcuts << shortcutMap("F1", Qt::ApplicationShortcut, EnabledShortcut)
                          << shortcutMap("F2", Qt::ApplicationShortcut, DisabledShortcut)
                          << shortcutMap("F3", Qt::ApplicationShortcut, EnabledShortcut)
                          << shortcutMap("F4", Qt::ApplicationShortcut, DisabledShortcut)
                          << shortcutMap("F7", Qt::WindowShortcut, EnabledShortcut)
                          << shortcutMap("F8", Qt::WindowShortcut, DisabledShortcut)
                          << shortcutMap("F9", Qt::WindowShortcut, EnabledShortcut)
                          << shortcutMap("F10", Qt::WindowShortcut, DisabledShortcut);

    // APP: F3,(F4),F5,(F6) / WND: F9,(F10),F11(F12)
    QVariantList inactiveWindowShortcuts;
    inactiveWindowShortcuts << shortcutMap("F3", Qt::ApplicationShortcut, EnabledShortcut)
                            << shortcutMap("F4", Qt::ApplicationShortcut, DisabledShortcut)
                            << shortcutMap("F5", Qt::ApplicationShortcut, EnabledShortcut)
                            << shortcutMap("F6", Qt::ApplicationShortcut, DisabledShortcut)
                            << shortcutMap("F9", Qt::WindowShortcut, EnabledShortcut)
                            << shortcutMap("F10", Qt::WindowShortcut, DisabledShortcut)
                            << shortcutMap("F11", Qt::WindowShortcut, EnabledShortcut)
                            << shortcutMap("F12", Qt::WindowShortcut, DisabledShortcut);

    // APP
    QTest::newRow("F1") << Qt::Key_F1 << activeWindowShortcuts << inactiveWindowShortcuts << "F1" << "" << "";
    QTest::newRow("F2") << Qt::Key_F2 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
    QTest::newRow("F3") << Qt::Key_F3 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "F3";
    QTest::newRow("F4") << Qt::Key_F4 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
    QTest::newRow("F5") << Qt::Key_F5 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "F5" << "";
    QTest::newRow("F6") << Qt::Key_F6 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";

    // WND
    QTest::newRow("F7") << Qt::Key_F7 << activeWindowShortcuts << inactiveWindowShortcuts << "F7" << "" << "";
    QTest::newRow("F8") << Qt::Key_F8 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
    QTest::newRow("F9") << Qt::Key_F9 << activeWindowShortcuts << inactiveWindowShortcuts << "F9" << "" << "";
    QTest::newRow("F10") << Qt::Key_F10 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
    QTest::newRow("F11") << Qt::Key_F11 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
    QTest::newRow("F12") << Qt::Key_F12 << activeWindowShortcuts << inactiveWindowShortcuts << "" << "" << "";
}

void tst_QQuickShortcut::context()
{
    QFETCH(Qt::Key, key);
    QFETCH(QVariantList, activeWindowShortcuts);
    QFETCH(QVariantList, inactiveWindowShortcuts);
    QFETCH(QString, activeWindowActivatedShortcut);
    QFETCH(QString, inactiveWindowActivatedShortcut);
    QFETCH(QString, ambiguousShortcut);

    QQmlApplicationEngine engine;

    engine.load(testFileUrl("shortcuts.qml"));
    QQuickWindow *inactiveWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(inactiveWindow);
    inactiveWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(inactiveWindow));
    inactiveWindow->setProperty("shortcuts", inactiveWindowShortcuts);

    engine.load(testFileUrl("shortcuts.qml"));
    QQuickWindow *activeWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().value(1));
    QVERIFY(activeWindow);
    activeWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(activeWindow));
    activeWindow->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(activeWindow));
    activeWindow->setProperty("shortcuts", activeWindowShortcuts);

    QTest::keyPress(activeWindow, key);
    QTest::keyRelease(activeWindow, key);

    QCOMPARE(activeWindow->property("activatedShortcut").toString(), activeWindowActivatedShortcut);
    QCOMPARE(inactiveWindow->property("activatedShortcut").toString(), inactiveWindowActivatedShortcut);
    QVERIFY(activeWindow->property("ambiguousShortcut").toString() == ambiguousShortcut
            || inactiveWindow->property("ambiguousShortcut").toString() == ambiguousShortcut);
}

typedef bool (*ShortcutContextMatcher)(QObject *, Qt::ShortcutContext);
extern ShortcutContextMatcher qt_quick_shortcut_context_matcher();
extern void qt_quick_set_shortcut_context_matcher(ShortcutContextMatcher matcher);

static ShortcutContextMatcher lastMatcher = nullptr;

static bool trueMatcher(QObject *, Qt::ShortcutContext)
{
    lastMatcher = trueMatcher;
    return true;
}

static bool falseMatcher(QObject *, Qt::ShortcutContext)
{
    lastMatcher = falseMatcher;
    return false;
}

Q_DECLARE_METATYPE(ShortcutContextMatcher)

void tst_QQuickShortcut::matcher_data()
{
    QTest::addColumn<ShortcutContextMatcher>("matcher");
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<QVariant>("shortcut");
    QTest::addColumn<QString>("activatedShortcut");

    ShortcutContextMatcher tm = trueMatcher;
    ShortcutContextMatcher fm = falseMatcher;

    QTest::newRow("F1") << tm << Qt::Key_F1 << shortcutMap("F1", Qt::ApplicationShortcut) << "F1";
    QTest::newRow("F2") << fm << Qt::Key_F2 << shortcutMap("F2", Qt::ApplicationShortcut) << "";
}

void tst_QQuickShortcut::matcher()
{
    QFETCH(ShortcutContextMatcher, matcher);
    QFETCH(Qt::Key, key);
    QFETCH(QVariant, shortcut);
    QFETCH(QString, activatedShortcut);

    ShortcutContextMatcher defaultMatcher = qt_quick_shortcut_context_matcher();
    QVERIFY(defaultMatcher);

    qt_quick_set_shortcut_context_matcher(matcher);
    QVERIFY(qt_quick_shortcut_context_matcher() == matcher);

    QQmlApplicationEngine engine(testFileUrl("shortcuts.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    window->setProperty("shortcuts", QVariantList() << shortcut);
    QTest::keyClick(window, key);

    QVERIFY(lastMatcher == matcher);
    QCOMPARE(window->property("activatedShortcut").toString(), activatedShortcut);

    qt_quick_set_shortcut_context_matcher(defaultMatcher);
}

void tst_QQuickShortcut::multiple_data()
{
    QTest::addColumn<QStringList>("sequences");
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<bool>("activated");

    // first
    QTest::newRow("Ctrl+X,(Shift+Del)") << (QStringList() << "Ctrl+X" << "Shift+Del") << Qt::Key_X << Qt::KeyboardModifiers(Qt::ControlModifier) << true << true;
    // second
    QTest::newRow("(Ctrl+X),Shift+Del") << (QStringList() << "Ctrl+X" << "Shift+Del") << Qt::Key_Delete << Qt::KeyboardModifiers(Qt::ShiftModifier) << true << true;
    // disabled
    QTest::newRow("(Ctrl+X,Shift+Del)") << (QStringList() << "Ctrl+X" << "Shift+Del") << Qt::Key_X << Qt::KeyboardModifiers(Qt::ControlModifier) << false << false;
}

void tst_QQuickShortcut::multiple()
{
    QFETCH(QStringList, sequences);
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(bool, enabled);
    QFETCH(bool, activated);

    QQmlApplicationEngine engine;

    engine.load(testFileUrl("multiple.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QObject *shortcut = window->property("shortcut").value<QObject *>();
    QVERIFY(shortcut);

    shortcut->setProperty("enabled", enabled);
    shortcut->setProperty("sequences", sequences);

    QTest::keyPress(window, key, modifiers);
    QTest::keyRelease(window, key, modifiers);

    QCOMPARE(window->property("activated").toBool(), activated);

    // check it still works after rotating the sequences
    QStringList rotatedSequences;
    for (int i = 1; i < sequences.size(); ++i)
        rotatedSequences.push_back(sequences[i]);
    if (sequences.size())
        rotatedSequences.push_back(sequences[0]);

    window->setProperty("activated", false);
    shortcut->setProperty("sequences", rotatedSequences);

    QTest::keyPress(window, key, modifiers);
    QTest::keyRelease(window, key, modifiers);
    QCOMPARE(window->property("activated").toBool(), activated);

    // check setting to no shortcuts
    QStringList emptySequence;

    window->setProperty("activated", false);
    shortcut->setProperty("sequences", emptySequence);

    QTest::keyPress(window, key, modifiers);
    QTest::keyRelease(window, key, modifiers);
    QCOMPARE(window->property("activated").toBool(), false);
}

void tst_QQuickShortcut::contextChange_data()
{
    multiple_data();
}
void tst_QQuickShortcut::contextChange()
{
    QFETCH(QStringList, sequences);
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(bool, enabled);
    QFETCH(bool, activated);

    QQmlApplicationEngine engine;

    engine.load(testFileUrl("multiple.qml"));
    QQuickWindow *inactiveWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    QVERIFY(inactiveWindow);
    inactiveWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(inactiveWindow));

    QObject *shortcut = inactiveWindow->property("shortcut").value<QObject *>();
    QVERIFY(shortcut);

    shortcut->setProperty("enabled", enabled);
    shortcut->setProperty("sequences", sequences);
    shortcut->setProperty("context", Qt::WindowShortcut);

    engine.load(testFileUrl("multiple.qml"));
    QQuickWindow *activeWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().value(1));
    QVERIFY(activeWindow);
    activeWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(activeWindow));
    activeWindow->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(activeWindow));

    QTest::keyPress(activeWindow, key, modifiers);
    QTest::keyRelease(activeWindow, key, modifiers);
    QCOMPARE(inactiveWindow->property("activated").toBool(), false);

    shortcut->setProperty("context", Qt::ApplicationShortcut);

    QTest::keyPress(activeWindow, key, modifiers);
    QTest::keyRelease(activeWindow, key, modifiers);
    QCOMPARE(inactiveWindow->property("activated").toBool(), activated);
}

void tst_QQuickShortcut::embedded_data()
{
    QTest::addColumn<Qt::Key>("testKey");
    QTest::addColumn<Qt::KeyboardModifiers>("testModifiers");
    QTest::addColumn<QString>("windowShortcutSequence");
    QTest::addColumn<QString>("applicationShortcutSequence");
    QTest::addColumn<bool>("windowShortcutActivated");
    QTest::addColumn<bool>("applicationShortcutActivated");

    QTest::newRow("windowActivated") << Qt::Key_W << (Qt::ControlModifier | Qt::AltModifier)
                                     << "Ctrl+Alt+W" << "Ctrl+Alt+A" << true << false;
    QTest::newRow("applicationActivated") << Qt::Key_A << (Qt::ControlModifier | Qt::AltModifier)
                                          << "Ctrl+Alt+W" << "Ctrl+Alt+A" << false << true;
}

void tst_QQuickShortcut::embedded()
{
#ifndef QT_WIDGETS_LIB
    QSKIP("Skipping due to QT_WIDGETS_LIB is not defined");
#else
    QFETCH(Qt::Key, testKey);
    QFETCH(Qt::KeyboardModifiers, testModifiers);
    QFETCH(QString, windowShortcutSequence);
    QFETCH(QString, applicationShortcutSequence);
    QFETCH(bool, windowShortcutActivated);
    QFETCH(bool, applicationShortcutActivated);

    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout {&window};
    QQuickView *quickView = new QQuickView;
    quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    quickView->setSource(testFileUrl("embedded.qml"));

    QWidget *container = QWidget::createWindowContainer(quickView);
    container->setMinimumSize(quickView->size());
    container->setFocusPolicy(Qt::TabFocus);

    QWidget *widget = new QWidget;
    // We will set focus to the widget and its default is NoFocus.
    widget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    layout->addWidget(widget);
    layout->addWidget(container);

    window.show();
    QTRY_VERIFY(window.isVisible());
    // The widget can get focused only when the including window has exposed.
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWindow *windowHandle = window.windowHandle();
    windowHandle->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(windowHandle));

    widget->setFocus();
    QTRY_VERIFY(widget->hasFocus());

    // If the window-context shortcut is expected to be activated,
    // then the QuickView window needs to be active.
    // On Linux, the embedded QuickView window is active immediately
    // after the containing window is active, but on Windows,
    // the embedded QuickView window is activated after a delay
    // once the containing window is active.
    if (windowShortcutActivated)
        QVERIFY(QTest::qWaitForWindowActive(quickView));

    QQuickItem *item = quickView->rootObject();
    QVERIFY(item);

    QObject *windowShortcut = item->property("shortcut").value<QObject *>();
    QVERIFY(windowShortcut);

    windowShortcut->setProperty("context", Qt::WindowShortcut);
    windowShortcut->setProperty("sequence", windowShortcutSequence);

    QTest::keyPress(&window, testKey, testModifiers);
    QTest::keyRelease(&window, testKey, testModifiers);
    QCOMPARE(item->property("activated").toBool(), windowShortcutActivated);

    quickView->requestActivate();
    QTRY_VERIFY(quickView->isActive());
    QVERIFY(quickView->isActive());

    item->setProperty("activated", false);
    QVERIFY(!item->property("activated").toBool());

    QTest::keyPress(&window, testKey, testModifiers);
    QTest::keyRelease(&window, testKey, testModifiers);
    QCOMPARE(item->property("activated").toBool(), windowShortcutActivated);

    QWidget otherWindow;
    QVBoxLayout *otherLayout = new QVBoxLayout {&otherWindow};
    QQuickView *otherQuickView = new QQuickView;
    otherQuickView->setResizeMode(QQuickView::SizeRootObjectToView);
    otherQuickView->setSource(testFileUrl("embedded.qml"));

    QWidget *otherContainer = QWidget::createWindowContainer(otherQuickView);
    otherContainer->setMinimumSize(quickView->size());
    otherContainer->setFocusPolicy(Qt::TabFocus);

    QWidget *otherWidget = new QWidget;
    otherLayout->addWidget(otherWidget);
    otherLayout->addWidget(otherContainer);

    otherWindow.show();
    QTRY_VERIFY(otherWindow.isVisible());
    QVERIFY(otherWindow.isVisible());

    // make sure that the (first) window is active
    quickView->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(quickView));

    QQuickItem *otherItem = otherQuickView->rootObject();
    QVERIFY(otherItem);

    QObject *applicationShortcut = otherItem->property("shortcut").value<QObject *>();
    QVERIFY(applicationShortcut);

    applicationShortcut->setProperty("context", Qt::ApplicationShortcut);
    applicationShortcut->setProperty("sequence", applicationShortcutSequence);

    QTest::keyPress(&otherWindow, testKey, testModifiers);
    QTest::keyRelease(&otherWindow, testKey, testModifiers);
    QCOMPARE(otherItem->property("activated").toBool(), applicationShortcutActivated);

    otherItem->setProperty("activated", false);
    otherWindow.close();
    QTRY_VERIFY(!otherWindow.isVisible());

    QTest::keyPress(&otherWindow, testKey, testModifiers);
    QTest::keyRelease(&otherWindow, testKey, testModifiers);
    QCOMPARE(otherItem->property("activated").toBool(), false);
#endif
}

#ifdef QT_QUICKWIDGETS_LIB
void tst_QQuickShortcut::quickWidgetShortcuts_data()
{
    QTest::addColumn<QVariantList>("shortcuts");
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("activatedShortcut");
    QTest::addColumn<QString>("ambiguousShortcut");

    QVariantList shortcuts;
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "M" << "";
    QTest::newRow("Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "Alt+M" << "";
    QTest::newRow("Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "Ctrl+M" << "";
    QTest::newRow("Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+M" << "";
    QTest::newRow("Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "Ctrl+Alt+M" << "";
    QTest::newRow("+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "+" << "";
    QTest::newRow("F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "F1" << "";
    QTest::newRow("Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+F1" << "";

    // ambiguous
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("?M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "M";
    QTest::newRow("?Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "" << "Alt+M";
    QTest::newRow("?Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "" << "Ctrl+M";
    QTest::newRow("?Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "Shift+M";
    QTest::newRow("?Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "" << "Ctrl+Alt+M";
    QTest::newRow("?+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "+";
    QTest::newRow("?F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "F1";
    QTest::newRow("?Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "Shift+F1";

    // disabled
    shortcuts.clear();
    shortcuts << shortcutMap("M", DisabledShortcut)
              << shortcutMap("Alt+M", DisabledShortcut)
              << shortcutMap("Ctrl+M", DisabledShortcut)
              << shortcutMap("Shift+M", DisabledShortcut)
              << shortcutMap("Ctrl+Alt+M", DisabledShortcut)
              << shortcutMap("+", DisabledShortcut)
              << shortcutMap("F1", DisabledShortcut)
              << shortcutMap("Shift+F1", DisabledShortcut);

    QTest::newRow("!M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "" << "";
    QTest::newRow("!Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "" << "";
    QTest::newRow("!Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "";
    QTest::newRow("!Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "" << "";
    QTest::newRow("!+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "" << "";
    QTest::newRow("!Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "" << "";

    // unambigous because others disabled
    shortcuts << shortcutMap("M")
              << shortcutMap("Alt+M")
              << shortcutMap("Ctrl+M")
              << shortcutMap("Shift+M")
              << shortcutMap("Ctrl+Alt+M")
              << shortcutMap("+")
              << shortcutMap("F1")
              << shortcutMap("Shift+F1");

    QTest::newRow("/M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::NoModifier) << "M" << "";
    QTest::newRow("/Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::AltModifier) << "Alt+M" << "";
    QTest::newRow("/Ctrl+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier) << "Ctrl+M" << "";
    QTest::newRow("/Shift+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+M" << "";
    QTest::newRow("/Ctrl+Alt+M") << shortcuts << Qt::Key_M << Qt::KeyboardModifiers(Qt::ControlModifier|Qt::AltModifier) << "Ctrl+Alt+M" << "";
    QTest::newRow("/+") << shortcuts << Qt::Key_Plus << Qt::KeyboardModifiers(Qt::NoModifier) << "+" << "";
    QTest::newRow("/F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::NoModifier) << "F1" << "";
    QTest::newRow("/Shift+F1") << shortcuts << Qt::Key_F1 << Qt::KeyboardModifiers(Qt::ShiftModifier) << "Shift+F1" << "";
}

void tst_QQuickShortcut::quickWidgetShortcuts()
{
    QFETCH(QVariantList, shortcuts);
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, activatedShortcut);
    QFETCH(QString, ambiguousShortcut);

    // ### Qt 6 figure out what to do with QQuickWidget - disabled for now
    QSKIP("Skipping due to QQuickWidget");

    QScopedPointer<QQuickWidget> quickWidget(new QQuickWidget);
    quickWidget->resize(300,300);

    QSignalSpy spy(qApp, &QGuiApplication::focusObjectChanged);

    quickWidget->setSource(testFileUrl("shortcutsRect.qml"));
    quickWidget->show();

    spy.wait();

    QVERIFY(qobject_cast<QQuickWidget*>(qApp->focusObject()) == quickWidget.data());

    QQuickItem* item = quickWidget->rootObject();
    item->setProperty("shortcuts", shortcuts);
    QTest::keyPress(quickWidget->quickWindow(), key, modifiers, 1500);
    QTest::keyRelease(quickWidget->quickWindow(), key, modifiers, 1600);
    QCOMPARE(item->property("activatedShortcut").toString(), activatedShortcut);
    QCOMPARE(item->property("ambiguousShortcut").toString(), ambiguousShortcut);
}
#endif // QT_QUICKWIDGETS_LIB

QTEST_MAIN(tst_QQuickShortcut)

#include "tst_qquickshortcut.moc"
