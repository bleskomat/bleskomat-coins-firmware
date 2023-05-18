## Usage
#
#   $ make install     # run install process(es)
#   $ make compile     # compile the device firmware
#   $ make upload      # compile then upload the device firmware
#   $ make monitor     # start the serial monitor
#

## Variables
BAUDRATE ?= 115200
DEVICE ?= /dev/ttyUSB0
FONTS=./assets/fonts
PLATFORM=espressif32
SCRIPTS=./scripts

## Phony targets - these are for when the target-side of a definition
# (such as "install" below) isn't a file but instead just a label. Declaring
# it as phony ensures that it always run, even if a file by the same name
# exists.
.PHONY: install\
compile\
upload\
monitor\
.git-commit-hash

install:
	pio pkg install --platform ${PLATFORM}

compile: .git-commit-hash
	pio run

upload: .git-commit-hash
	sudo chown ${USER}:${USER} ${DEVICE}
	pio run --upload-port ${DEVICE} --target upload

monitor:
	sudo chown ${USER}:${USER} ${DEVICE}
	pio device monitor --baud ${BAUDRATE} --port ${DEVICE}

.git-commit-hash:
	-git rev-parse HEAD 2>/dev/null && git rev-parse HEAD > .git-commit-hash

fonts: fontconvert
	$(SCRIPTS)/generate-font-header-files.sh "$(FONTS)/CheckbookLightning/CheckbookLightning.ttf" 32-122 20,24,26,28,30,32
	$(SCRIPTS)/generate-font-header-files.sh "$(FONTS)/Courier Prime Code/Courier Prime Code.ttf" 32-382 8,9,10,12,14,16,18,20,22,24,26,28

fontconvert: ./tools/Adafruit-GFX-Library/fontconvert/fontconvert

./tools/Adafruit-GFX-Library/fontconvert/fontconvert:
	cd ./tools/Adafruit-GFX-Library/fontconvert && make fontconvert
