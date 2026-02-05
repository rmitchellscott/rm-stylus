XOVI_REPO ?= $(HOME)/github/asivery/xovi
name = rm-stylus

VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo "dev")

XOVI_VERSION = $(shell echo "$(VERSION)" | sed 's/^v//' | awk -F'[.-]' '{if (NF >= 3 && $$1 ~ /^[0-9]+$$/) printf "%d", ($$1 * 65536) + ($$2 * 256) + $$3; else print "0"}')

QT_CFLAGS = $(shell pkg-config --cflags Qt6Gui 2>/dev/null)
QT_PRIVATE = $(SDKTARGETSYSROOT)/usr/include/QtGui/$(shell pkg-config --modversion Qt6Gui 2>/dev/null)/QtGui
MOC = $(OECORE_NATIVE_SYSROOT)/usr/libexec/moc

CXXFLAGS = -D_GNU_SOURCE -fPIC -O2 -DVERSION=\"$(VERSION)\" -std=c++17 $(QT_CFLAGS) -I$(QT_PRIVATE)
CFLAGS = -D_GNU_SOURCE -fPIC -O2 -DVERSION=\"$(VERSION)\"
LDFLAGS = -lpthread

BUILD = build
OUTPUT = $(name).so

OBJECTS = $(BUILD)/stylus.o $(BUILD)/xovi.o

.PHONY: all clean print-version

print-version:
	@echo "VERSION=$(VERSION)"
	@echo "XOVI_VERSION=$(XOVI_VERSION)"

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)

$(BUILD)/stylus.moc: src/stylus.cpp
	@mkdir -p $(BUILD)
	$(MOC) $(QT_CFLAGS) -I$(QT_PRIVATE) $< -o $@

$(BUILD)/stylus.o: src/stylus.cpp $(BUILD)/xovi.h $(BUILD)/stylus.moc
	$(CXX) $(CXXFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/xovi.o: $(BUILD)/xovi.c $(BUILD)/xovi.h
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/xovi.h: $(BUILD)/xovi.c
$(BUILD)/xovi.c: rm-stylus.xovi
	@mkdir -p $(BUILD)
	python3 $(XOVI_REPO)/util/xovigen.py -a armv7 -o $(BUILD)/xovi.c -H $(BUILD)/xovi.h rm-stylus.xovi
	@sed -i.bak 's/const int EXTENSIONVERSION = [0-9]*/const int EXTENSIONVERSION = $(XOVI_VERSION)/' $(BUILD)/xovi.c && rm -f $(BUILD)/xovi.c.bak

clean:
	rm -f $(name).so
	rm -rf build
