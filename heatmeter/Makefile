PORT = /dev/ttyACM0
burn:
	python reset.py $(PORT)
	sleep 2
	avrdude -patmega32u4 -cavr109 -P$(PORT) -b57600 -D -Uflash:w:./heatmeter.ino.hex:i
	picocom -b9600 $(PORT)

