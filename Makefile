SOURCE_CODE=trashcan.c
PLUGIN_NAME=trashcan.so
CONFIG_DIR=~/.config/trashcan
INSTALLED_PLUGIN=$(shell find / -name "deskno.so" 2>/dev/null)
ifneq ($(INSTALLED_PLUGIN),)
PLUGINS_DIR = $(shell dirname $(INSTALLED_PLUGIN))
else
$(error LXDE probably not present on this system.)
endif

all: $(PLUGIN_NAME)
	cp -R config $(CONFIG_DIR)

${PLUGIN_NAME}: $(SOURCE_CODE)
	gcc -Wall `pkg-config --cflags glib-2.0 gtk+-2.0` \
	    -shared -fPIC $(SOURCE_CODE) -o $(PLUGIN_NAME) `pkg-config --libs lxpanel`

clean:
	rm -f $(PLUGIN_NAME)
	rm -rf $(CONFIG_DIR)

install: trashcan.so
	cp $(PLUGIN_NAME) $(PLUGINS_DIR)

uninstall:
	rm -f $(PLUGINS_DIR)/$(PLUGIN_NAME)
	rm -f $(PLUGIN_NAME)
	rm -rf $(CONFIG_DIR)
