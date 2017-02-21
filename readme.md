The code for the 2017 robot.

It runs on a Raspberry Pi.

It uses OpenCV to identify the target.

**Compilation**
Install opencv 2.4.x 

Run `make camera` in either the gear or turret directory to compile.

**Running**

Run `make run` or `./build/camera` in either the gear or turret directory.

Running the `run_gear.sh` or `run_turret.sh` scripts call additional linux functions to prepare the script and then run it. This will need to be changed to run on different systems.

**Clean up build**

Run `make clean`

**Overclocking Raspberry Pi**
In /boot/config.txt, add the lines

arm_freq=1400
over_voltage=5
adram_freq=500

arm_freq refers to the MHz of the cpu, the other settings allow it to interface with the rest of the Pi successfully.

