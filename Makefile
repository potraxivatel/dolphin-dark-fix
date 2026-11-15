CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -fPIC
LDFLAGS ?= -shared -ldl

PREFIX ?= $(HOME)/.local
LIBDIR ?= $(PREFIX)/lib
BINDIR ?= $(PREFIX)/bin
APPDIR ?= $(PREFIX)/share/applications

TARGET = libdolphin_text_fix.so

all: $(TARGET)

$(TARGET): dolphin_text_fix.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

install: $(TARGET)
	install -d $(LIBDIR) $(BINDIR) $(APPDIR)
	install -m 755 $(TARGET) $(LIBDIR)/$(TARGET)
	@echo '#!/bin/sh' > $(BINDIR)/dolphin
	@echo 'export LD_PRELOAD="$(LIBDIR)/$(TARGET)$${LD_PRELOAD:+:$$LD_PRELOAD}"' >> $(BINDIR)/dolphin
	@echo 'exec /usr/bin/dolphin "$$@"' >> $(BINDIR)/dolphin
	chmod +x $(BINDIR)/dolphin
	@if [ -f /usr/share/applications/org.kde.dolphin.desktop ]; then \
		cp /usr/share/applications/org.kde.dolphin.desktop $(APPDIR)/org.kde.dolphin.desktop; \
		sed -i 's|^Exec=dolphin|Exec=$(BINDIR)/dolphin|g' $(APPDIR)/org.kde.dolphin.desktop; \
	fi
	@echo "Installation complete!"
	@echo "Make sure $(BINDIR) is in your PATH."

uninstall:
	rm -f $(LIBDIR)/$(TARGET)
	rm -f $(BINDIR)/dolphin
	rm -f $(APPDIR)/org.kde.dolphin.desktop
	@echo "Uninstalled successfully."

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean
