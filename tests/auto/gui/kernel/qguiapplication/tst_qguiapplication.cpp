/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QDebug>

class tst_QGuiApplication: public QObject
{
    Q_OBJECT

private slots:
    void displayName();
    void focusObject();
    void allWindows();
    void topLevelWindows();
    void abortQuitOnShow();
    void changeFocusWindow();
    void keyboardModifiers();
    void modalWindow();
    void quitOnLastWindowClosed();
};

void tst_QGuiApplication::displayName()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qguiapplication") };
    QGuiApplication app(argc, argv);
    QCOMPARE(::qAppName(), QString::fromLatin1("tst_qguiapplication"));
    QCOMPARE(QGuiApplication::applicationName(), QString::fromLatin1("tst_qguiapplication"));
    QCOMPARE(QGuiApplication::applicationDisplayName(), QString::fromLatin1("tst_qguiapplication"));
    QGuiApplication::setApplicationDisplayName("The GUI Application");
    QCOMPARE(QGuiApplication::applicationDisplayName(), QString::fromLatin1("The GUI Application"));
}

class DummyWindow : public QWindow
{
public:
    DummyWindow() : m_focusObject(0) {}

    virtual QObject *focusObject() const
    {
        return m_focusObject;
    }

    void setFocusObject(QObject *object)
    {
        m_focusObject = object;
        emit focusObjectChanged(object);
    }

    QObject *m_focusObject;
};


void tst_QGuiApplication::focusObject()
{
    int argc = 0;
    QGuiApplication app(argc, 0);

    QObject obj1, obj2, obj3;
    DummyWindow window1;
    DummyWindow window2;
    window1.show();

    QSignalSpy spy(&app, SIGNAL(focusObjectChanged(QObject *)));


    // verify active window focus propagates to qguiapplication
    QTest::qWaitForWindowShown(&window1);
    window1.requestActivateWindow();
    QTRY_COMPARE(app.focusWindow(), &window1);

    window1.setFocusObject(&obj1);
    QCOMPARE(app.focusObject(), &obj1);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    window1.setFocusObject(&obj2);
    QCOMPARE(app.focusObject(), &obj2);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    window2.setFocusObject(&obj3);
    QCOMPARE(app.focusObject(), &obj2); // not yet changed
    window2.show();
    QTest::qWaitForWindowShown(&window2);
    QTRY_COMPARE(app.focusWindow(), &window2);
    QCOMPARE(app.focusObject(), &obj3);
    QCOMPARE(spy.count(), 1);

    // focus change on unfocused window does not show
    spy.clear();
    window1.setFocusObject(&obj1);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(app.focusObject(), &obj3);
}

void tst_QGuiApplication::allWindows()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    QWindow *window1 = new QWindow;
    QWindow *window2 = new QWindow(window1);
    QVERIFY(app.allWindows().contains(window1));
    QVERIFY(app.allWindows().contains(window2));
    QCOMPARE(app.allWindows().count(), 2);
    delete window1;
    window1 = 0;
    window2 = 0;
    QVERIFY(!app.allWindows().contains(window2));
    QVERIFY(!app.allWindows().contains(window1));
    QCOMPARE(app.allWindows().count(), 0);
}

void tst_QGuiApplication::topLevelWindows()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    QWindow *window1 = new QWindow;
    QWindow *window2 = new QWindow(window1);
    QVERIFY(app.topLevelWindows().contains(window1));
    QVERIFY(!app.topLevelWindows().contains(window2));
    QCOMPARE(app.topLevelWindows().count(), 1);
    delete window1;
    window1 = 0;
    window2 = 0;
    QVERIFY(!app.topLevelWindows().contains(window2));
    QVERIFY(!app.topLevelWindows().contains(window1));
    QCOMPARE(app.topLevelWindows().count(), 0);
}

class ShowCloseShowWindow : public QWindow
{
    Q_OBJECT
public:
    ShowCloseShowWindow(bool showAgain, QWindow *parent = 0)
      : QWindow(parent), showAgain(showAgain)
    {
        QTimer::singleShot(0, this, SLOT(doClose()));
        QTimer::singleShot(500, this, SLOT(exitApp()));
    }

private slots:
    void doClose() {
        close();
        if (showAgain)
            show();
    }

    void exitApp() {
      qApp->exit(1);
    }

private:
    bool showAgain;
};

void tst_QGuiApplication::abortQuitOnShow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    QWindow *window1 = new ShowCloseShowWindow(false);
    window1->show();
    QCOMPARE(app.exec(), 0);

    QWindow *window2 = new ShowCloseShowWindow(true);
    window2->show();
    QCOMPARE(app.exec(), 1);
}


class FocusChangeWindow: public QWindow
{
protected:
    virtual bool event(QEvent *ev)
    {
        if (ev->type() == QEvent::FocusAboutToChange)
            windowDuringFocusAboutToChange = qGuiApp->focusWindow();
        return QWindow::event(ev);
    }

    virtual void focusOutEvent(QFocusEvent *)
    {
        windowDuringFocusOut = qGuiApp->focusWindow();
    }

public:
    FocusChangeWindow() : QWindow(), windowDuringFocusAboutToChange(0), windowDuringFocusOut(0) {}

    QWindow *windowDuringFocusAboutToChange;
    QWindow *windowDuringFocusOut;
};

void tst_QGuiApplication::changeFocusWindow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);

    // focus is changed between FocusAboutToChange and FocusChanged
    FocusChangeWindow window1, window2;
    window1.show();
    window2.show();
    QTest::qWaitForWindowShown(&window1);
    QTest::qWaitForWindowShown(&window2);
    window1.requestActivateWindow();
    QTRY_COMPARE(app.focusWindow(), &window1);

    window2.requestActivateWindow();
    QTRY_COMPARE(app.focusWindow(), &window2);
    QCOMPARE(window1.windowDuringFocusAboutToChange, &window1);
    QCOMPARE(window1.windowDuringFocusOut, &window2);
}

void tst_QGuiApplication::keyboardModifiers()
{
    int argc = 0;
    QGuiApplication app(argc, 0);

    QWindow *window = new QWindow;
    window->show();
    QTest::qWaitForWindowShown(window);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    // mouse events
    QPoint center = window->geometry().center();
    QTest::mouseEvent(QTest::MousePress, window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::mouseEvent(QTest::MouseRelease, window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::mouseEvent(QTest::MousePress, window, Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::mouseEvent(QTest::MouseRelease, window, Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // shortcut events
    QWindowSystemInterface::tryHandleSynchronousShortcutEvent(window, Qt::Key_5, Qt::MetaModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::MetaModifier);
    QWindowSystemInterface::tryHandleSynchronousShortcutEvent(window, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QWindowSystemInterface::tryHandleSynchronousShortcutEvent(window, Qt::Key_0, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // key events
    QTest::keyEvent(QTest::Press, window, Qt::Key_C);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::keyEvent(QTest::Release, window, Qt::Key_C);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    QTest::keyEvent(QTest::Press, window, Qt::Key_U, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::keyEvent(QTest::Release, window, Qt::Key_U, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    QTest::keyEvent(QTest::Press, window, Qt::Key_T);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::keyEvent(QTest::Release, window, Qt::Key_T);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    QTest::keyEvent(QTest::Press, window, Qt::Key_E, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::keyEvent(QTest::Release, window, Qt::Key_E, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // wheel events
    QPoint global = window->mapToGlobal(center);
    QPoint delta(0, 1);
    QWindowSystemInterface::handleWheelEvent(window, center, global, delta, delta, Qt::NoModifier);
    QWindowSystemInterface::sendWindowSystemEvents(app.eventDispatcher(), QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QWindowSystemInterface::handleWheelEvent(window, center, global, delta, delta, Qt::AltModifier);
    QWindowSystemInterface::sendWindowSystemEvents(app.eventDispatcher(), QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::AltModifier);
    QWindowSystemInterface::handleWheelEvent(window, center, global, delta, delta, Qt::ControlModifier);
    QWindowSystemInterface::sendWindowSystemEvents(app.eventDispatcher(), QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // touch events
    QList<const QTouchDevice *> touchDevices = QTouchDevice::devices();
    if (!touchDevices.isEmpty()) {
        QTouchDevice *touchDevice = const_cast<QTouchDevice *>(touchDevices.first());
        QTest::touchEvent(window, touchDevice).press(1, center).release(1, center);
        QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    }

    window->close();
    delete window;
}

class BlockableWindow : public QWindow
{
    Q_OBJECT
public:
    int blocked;

    inline BlockableWindow()
        : QWindow()
    {
        blocked = false;
    }

    bool event(QEvent *e)
    {
        switch (e->type()) {
        case QEvent::WindowBlocked:
            ++blocked;
            break;
        case QEvent::WindowUnblocked:
            --blocked;
            break;
        default:
            break;
        }
        return QWindow::event(e);
    }
};

void tst_QGuiApplication::modalWindow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);

    BlockableWindow *window1 = new BlockableWindow;

    BlockableWindow *window2 = new BlockableWindow;

    BlockableWindow *windowModalWindow1 = new BlockableWindow;
    windowModalWindow1->setTransientParent(window1);
    windowModalWindow1->setWindowModality(Qt::WindowModal);

    BlockableWindow *windowModalWindow2 = new BlockableWindow;
    windowModalWindow2->setTransientParent(windowModalWindow1);
    windowModalWindow2->setWindowModality(Qt::WindowModal);

    BlockableWindow *applicationModalWindow1 = new BlockableWindow;
    applicationModalWindow1->setWindowModality(Qt::ApplicationModal);

    // show the 2 windows, nothing is blocked
    window1->show();
    window2->show();
    QTest::qWaitForWindowShown(window1);
    QTest::qWaitForWindowShown(window2);
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show applicationModalWindow1, everything is blocked
    applicationModalWindow1->show();
    QCOMPARE(app.modalWindow(), applicationModalWindow1);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 1);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 1);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // everything is unblocked when applicationModalWindow1 is hidden
    applicationModalWindow1->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show the windowModalWindow1, only window1 is blocked
    windowModalWindow1->show();
    QCOMPARE(app.modalWindow(), windowModalWindow1);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show the windowModalWindow2, windowModalWindow1 is blocked as well
    windowModalWindow2->show();
    QCOMPARE(app.modalWindow(), windowModalWindow2);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow1, nothing is unblocked
    windowModalWindow1->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow2);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow2, windowModalWindow1 and window1 are unblocked
    windowModalWindow2->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show windowModalWindow1 again, window1 is blocked
    windowModalWindow1->show();
    QCOMPARE(app.modalWindow(), windowModalWindow1);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show windowModalWindow2 again, windowModalWindow1 is also blocked
    windowModalWindow2->show();
    QCOMPARE(app.modalWindow(), windowModalWindow2);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show applicationModalWindow1, everything is blocked
    applicationModalWindow1->show();
    QCOMPARE(app.modalWindow(), applicationModalWindow1);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 1);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 1);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide applicationModalWindow1, windowModalWindow1 and window1 are blocked
    applicationModalWindow1->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow2);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow2, window1 is blocked
    windowModalWindow2->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow1);
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow1, everything is unblocked
    windowModalWindow1->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    window2->hide();
    window1->hide();

    delete applicationModalWindow1;
    delete windowModalWindow2;
    delete windowModalWindow1;
    delete window2;
    delete window1;
}

void tst_QGuiApplication::quitOnLastWindowClosed()
{
    {
        int argc = 0;
        QGuiApplication app(argc, 0);

        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        QPointer<QWindow> mainWindow = new QWindow;
        QPointer<QWindow> dialog = new QWindow;

        dialog->setTransientParent(mainWindow);

        QVERIFY(app.quitOnLastWindowClosed());

        mainWindow->show();
        dialog->show();

        timer.start();
        QTimer::singleShot(1000, mainWindow, SLOT(close())); // This should quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));        // This makes sure we quit even if it didn't

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() < 15);      // Should be around 10 if closing caused the quit
    }
    {
        int argc = 0;
        QGuiApplication app(argc, 0);

        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        QPointer<QWindow> mainWindow = new QWindow;
        QPointer<QWindow> dialog = new QWindow;

        QVERIFY(!dialog->transientParent());
        QVERIFY(app.quitOnLastWindowClosed());

        mainWindow->show();
        dialog->show();

        timer.start();
        QTimer::singleShot(1000, mainWindow, SLOT(close())); // This should not quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() > 15);      // Should be around 20 if closing did not cause the quit
    }
}


QTEST_APPLESS_MAIN(tst_QGuiApplication)
#include "tst_qguiapplication.moc"
