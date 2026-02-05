#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <dlfcn.h>
#include <linux/input.h>

#include <QObject>
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QtQml>

extern "C" {
#include "xovi.h"
}

#ifndef VERSION
#define VERSION "unknown"
#endif

#define INPUT_DEVICES "/proc/bus/input/devices"
#define WACOM_NAME "Wacom I2C Digitizer"
#define ELAN_NAME "Elan marker input"

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
    bool found_pen = false;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "N: Name=\"", 9) == 0) {
            found_pen = (strstr(line, WACOM_NAME) != nullptr ||
                         strstr(line, ELAN_NAME) != nullptr);
        } else if (found_pen && strncmp(line, "H: Handlers=", 12) == 0) {
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
    debug_log("Could not find pen device\n");
    return nullptr;
}

class StylusHandler : public QObject {
    Q_OBJECT
public:
    StylusHandler(const QString &device, QObject *parent = nullptr)
        : QObject(parent), m_fd(-1), m_notifier(nullptr), m_rubberActive(false) {

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

signals:
    void buttonPressed();
    void buttonReleased();
    void button2Pressed();
    void button2Released();
    void rubberActivated();
    void rubberDeactivated();

private slots:
    void readData() {
        struct input_event ev;
        while (read(m_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type != EV_KEY)
                continue;
            if (ev.code == BTN_STYLUS) {
                if (ev.value) {
                    debug_log("BTN_STYLUS press\n");
                    emit buttonPressed();
                } else {
                    debug_log("BTN_STYLUS release\n");
                    emit buttonReleased();
                }
            } else if (ev.code == BTN_STYLUS2) {
                if (ev.value) {
                    debug_log("BTN_STYLUS2 press\n");
                    emit button2Pressed();
                } else {
                    debug_log("BTN_STYLUS2 release\n");
                    emit button2Released();
                }
            } else if (ev.code == BTN_TOOL_RUBBER) {
                bool active = (ev.value != 0);
                if (active != m_rubberActive) {
                    m_rubberActive = active;
                    if (active) {
                        debug_log("BTN_TOOL_RUBBER activated\n");
                        emit rubberActivated();
                    } else {
                        debug_log("BTN_TOOL_RUBBER deactivated\n");
                        emit rubberDeactivated();
                    }
                }
            }
        }
    }

private:
    int m_fd;
    QSocketNotifier *m_notifier;
    bool m_rubberActive;
};

static StylusHandler *g_handler = nullptr;
static QString g_devicePath;

class RmStylus : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool buttonHeld READ buttonHeld NOTIFY buttonHeldChanged)
    Q_PROPERTY(bool button2Held READ button2Held NOTIFY button2HeldChanged)
    Q_PROPERTY(bool rubberActive READ rubberActive NOTIFY rubberActiveChanged)
public:
    explicit RmStylus(QObject *parent = nullptr)
        : QObject(parent), m_buttonHeld(false), m_button2Held(false), m_rubberActive(false) {
        if (!g_handler && !g_devicePath.isEmpty()) {
            g_handler = new StylusHandler(g_devicePath, QCoreApplication::instance());
        }
        if (!g_handler)
            return;
        connect(g_handler, &StylusHandler::buttonPressed, this, [this]() {
            m_buttonHeld = true;
            emit buttonHeldChanged();
            emit buttonPressed();
        });
        connect(g_handler, &StylusHandler::buttonReleased, this, [this]() {
            m_buttonHeld = false;
            emit buttonHeldChanged();
            emit buttonReleased();
        });
        connect(g_handler, &StylusHandler::button2Pressed, this, [this]() {
            m_button2Held = true;
            emit button2HeldChanged();
            emit button2Pressed();
        });
        connect(g_handler, &StylusHandler::button2Released, this, [this]() {
            m_button2Held = false;
            emit button2HeldChanged();
            emit button2Released();
        });
        connect(g_handler, &StylusHandler::rubberActivated, this, [this]() {
            m_rubberActive = true;
            emit rubberActiveChanged();
            emit rubberActivated();
        });
        connect(g_handler, &StylusHandler::rubberDeactivated, this, [this]() {
            m_rubberActive = false;
            emit rubberActiveChanged();
            emit rubberDeactivated();
        });
    }

    bool buttonHeld() const { return m_buttonHeld; }
    bool button2Held() const { return m_button2Held; }
    bool rubberActive() const { return m_rubberActive; }

signals:
    void buttonPressed();
    void buttonReleased();
    void button2Pressed();
    void button2Released();
    void rubberActivated();
    void rubberDeactivated();
    void buttonHeldChanged();
    void button2HeldChanged();
    void rubberActiveChanged();

private:
    bool m_buttonHeld;
    bool m_button2Held;
    bool m_rubberActive;
};

extern "C" char _xovi_shouldLoad() {
    if (!dlsym(RTLD_DEFAULT, "_Z21qRegisterResourceDataiPKhS0_S0_")) {
        debug_log("Not a GUI application, refusing to load\n");
        return 0;
    }
    return 1;
}

extern "C" void _xovi_construct() {
    debug_log("Extension loading (version %s)\n", VERSION);

    qmlRegisterType<RmStylus>("net.rmitchellscott.RmStylus", 1, 0, "RmStylus");

    char* pen_device = findPenDevice();
    if (!pen_device) {
        debug_log("Failed to find pen device\n");
        return;
    }

    g_devicePath = QString::fromLocal8Bit(pen_device);
    debug_log("Extension loaded\n");
}

#include "stylus.moc"
