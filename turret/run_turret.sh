#!/bin/sh

route add -net 10.17.68.0 netmask 255.255.255.0 dev eth0

v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` --set-fmt-video=width=640,height=480,pixelformat=YUYV
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c brightness=-64
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c contrast=64
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c saturation=128
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c hue=-3
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c white_balance_temperature_auto=0
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c gamma=72
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c gain=0
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c power_line_frequency=1
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c white_balance_temperature=4722
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c sharpness=3
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c backlight_compensation=1
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c exposure_auto=1
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c exposure_absolute=19
v4l2-ctl -d`/home/pi/2017-Camera-Code/whatcamami.pl T` -c exposure_auto_priority=1

sleep 50

until nice -n -1 /home/pi/2017-Camera-Code/turret/build/camera `/home/pi/2017-Camera-Code/whatcamami.pl T`; do
echo "crash! $?" >&2
sleep 1
done
