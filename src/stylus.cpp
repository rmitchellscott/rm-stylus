#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <linux/input.h>

#include <QObject>
#include <QSocketNotifier>
#include <QThread>
#include <qpa/qwindowsysteminterface.h>

extern "C" {
#include "xovi.h"
}

#ifndef VERSION
#define VERSION "unknown"
#endif

#define INPUT_DEVICES "/proc/bus/input/devices"
#define WACOM_NAME "Wacom I2C Digitizer"

static void debug_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[rm-stylus]: ");
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);
}

static char* findPenDevice() {
    FILE *f = fopen(INPUT_DEVICES, "r");
    if (!f) {
        debug_log("Cannot open %s\n", INPUT_DEVICES);
        return nullptr;
    }

    static char device_path[64];
    char line[256];
    bool found_wacom = false;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "N: Name=\"", 9) == 0) {
            found_wacom = (strstr(line, WACOM_NAME) != nullptr);
        } else if (found_wacom && strncmp(line, "H: Handlers=", 12) == 0) {
            char *event = strstr(line, "event");
            if (event) {
                int event_num;
                if (sscanf(event, "event%d", &event_num) == 1) {
                    snprintf(device_path, sizeof(device_path), "/dev/input/event%d", event_num);
                    fclose(f);
                    return device_path;
                }
            }
        }
    }

    fclose(f);
    debug_log("Could not find %s device\n", WACOM_NAME);
    return nullptr;
}

class StylusHandler : public QObject {
    Q_OBJECT
public:
    StylusHandler(const QString &device, QObject *parent = nullptr)
        : QObject(parent), m_fd(-1), m_notifier(nullptr), m_device(device) {

        m_fd = open(device.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
        if (m_fd < 0) {
            debug_log("Failed to open %s: %s\n", qPrintable(device), strerror(errno));
            return;
        }
        debug_log("Opened pen device: %s\n", qPrintable(device));

        m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, &StylusHandler::readData);
        debug_log("Handler ready\n");
    }

    ~StylusHandler() {
        if (m_fd >= 0)
            close(m_fd);
    }

private slots:
    void readData() {
        struct input_event ev;
        while (read(m_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                QEvent::Type qEvent = ev.value != 0 ? QEvent::KeyPress : QEvent::KeyRelease;
                if (ev.code == BTN_STYLUS) {
                    debug_log("BTN_STYLUS %s -> Ctrl+U\n", ev.value ? "press" : "release");
                    QWindowSystemInterface::handleKeyEvent(nullptr, qEvent, Qt::Key_U, Qt::ControlModifier);
                } else if (ev.code == BTN_TOOL_RUBBER) {
                    debug_log("BTN_TOOL_RUBBER %s -> Ctrl+T\n", ev.value ? "press" : "release");
                    QWindowSystemInterface::handleKeyEvent(nullptr, qEvent, Qt::Key_T, Qt::ControlModifier);
                }
            }
        }
    }

private:
    int m_fd;
    QSocketNotifier *m_notifier;
    QString m_device;
};

class StylusThread : public QThread {
public:
    StylusThread(const QString &device, QObject *parent = nullptr)
        : QThread(parent), m_device(device), m_handler(nullptr) {
    }

    ~StylusThread() {
        quit();
        wait();
    }

protected:
    void run() override {
        debug_log("Thread started\n");
        m_handler = new StylusHandler(m_device);
        exec();
        delete m_handler;
        m_handler = nullptr;
        debug_log("Thread exiting\n");
    }

private:
    QString m_device;
    StylusHandler *m_handler;
};

static StylusThread *thread = nullptr;

extern "C" void _xovi_construct() {
    debug_log("Extension loading (version %s)\n", VERSION);

    char* pen_device = findPenDevice();
    if (!pen_device) {
        debug_log("Failed to find pen device\n");
        return;
    }

    thread = new StylusThread(QString::fromLocal8Bit(pen_device));
    thread->start();
    debug_log("Extension loaded\n");
}

#include "stylus.moc"
