XOVI_REPO ?= $(HOME)/github/asivery/xovi
name = rm-stylus

VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo "dev")

XOVI_VERSION = $(shell echo "$(VERSION)" | sed 's/^v//' | awk -F'[.-]' '{if (NF >= 3 && $$1 ~ /^[0-9]+$$/) printf "%d", ($$1 * 65536) + ($$2 * 256) + $$3; else print "0"}')

ARCH ?= armv7

QT_CFLAGS = $(shell pkg-config --cflags Qt6Gui Qt6Qml 2>/dev/null)
QT_PRIVATE = $(SDKTARGETSYSROOT)/usr/include/QtGui/$(shell pkg-config --modversion Qt6Gui 2>/dev/null)/QtGui
MOC = $(OECORE_NATIVE_SYSROOT)/usr/libexec/moc

CXXFLAGS = -D_GNU_SOURCE -fPIC -O2 -DVERSION=\"$(VERSION)\" -std=c++17 $(QT_CFLAGS) -I$(QT_PRIVATE)
CFLAGS = -D_GNU_SOURCE -fPIC -O2 -DVERSION=\"$(VERSION)\"
LDFLAGS = -lpthread

BUILD = build/$(ARCH)
OUTPUT = $(name)-$(ARCH).so

OBJECTS = $(BUILD)/stylus.o $(BUILD)/xovi.o

DOCKER_IMAGE_armv7 = eeems/remarkable-toolchain:latest-rm2
DOCKER_IMAGE_aarch64 = eeems/remarkable-toolchain:latest-rmpp
DOCKER_IMAGE = $(DOCKER_IMAGE_$(ARCH))

.PHONY: all all-armv7 all-aarch64 clean print-version

print-version:
	@echo "VERSION=$(VERSION)"
	@echo "XOVI_VERSION=$(XOVI_VERSION)"

all: all-armv7 all-aarch64

all-armv7:
	docker run --rm \
		-v "$(CURDIR):/work" \
		-v "$(XOVI_REPO):/work/xovi-repo" \
		-w /work \
		-e XOVI_REPO=/work/xovi-repo \
		-e VERSION=$(VERSION) \
		$(DOCKER_IMAGE_armv7) \
		bash -c '. /opt/codex/*/*/environment-setup-*; make ARCH=armv7 build'

all-aarch64:
	docker run --rm \
		-v "$(CURDIR):/work" \
		-v "$(XOVI_REPO):/work/xovi-repo" \
		-w /work \
		-e XOVI_REPO=/work/xovi-repo \
		-e VERSION=$(VERSION) \
		$(DOCKER_IMAGE_aarch64) \
		bash -c '. /opt/codex/*/*/environment-setup-*; make ARCH=aarch64 build'

build: $(OUTPUT)

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
	python3 $(XOVI_REPO)/util/xovigen.py -a $(ARCH) -o $(BUILD)/xovi.c -H $(BUILD)/xovi.h rm-stylus.xovi
	@sed -i.bak 's/const int EXTENSIONVERSION = [0-9]*/const int EXTENSIONVERSION = $(XOVI_VERSION)/' $(BUILD)/xovi.c && rm -f $(BUILD)/xovi.c.bak

clean:
	rm -f $(name)-*.so
	rm -rf build
