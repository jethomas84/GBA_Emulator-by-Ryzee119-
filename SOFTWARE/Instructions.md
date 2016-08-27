Draft copy! Still working on this

## Software Setup
This following describes the software installation steps for my GBA mod.
The following sources were used:
https://github.com/adafruit/Adafruit-Retrogame  
https://github.com/notro/fbtft/wiki/  
https://github.com/WiringPi/WiringPi  
https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=39138  


### 1. OS Installation
1.	Download RetroPie from https://retropie.org.uk/
2.	The version I used is v3.8.1
3.	Flash the .img file to an SD card using [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager/).
4.	Once the image has been flashed to the SD card a **boot** partition should appear in your PC.
5.	Place *‘gbaemu-overlay.dtbo’* into the **boot/overlays** folder.
6.	In **boot/** create a new folder called *‘gba_mod’*
7.	In the **gba_mod** folder place the following programs/files:
  1.  retrogame
  2.  gpio_alt
  3.  fbcp
  4.  es_input.cfg
  5.  retroarch.cfg
8. Go to the [WiringPi Github repo](https://github.com/WiringPi/WiringPi) and clone a zip file of the project to your desktop. Unzip all files to a folder called *'wiringpi'*
9. Copy the 'wiringpi' folder to boot/gba_mod/ on the SD card.  
10.	Edit boot/config.txt with a editor that follows the linefeeds in a linux system (i.e notepad++) and include the following at the end of the file then save and close the file:
> hdmi_force_hotplug=1  
> hdmi_group=2  
> hdmi_mode=87  
> hdmi_cvt=320 240 60 1 0 0 0  
> dtoverlay=gbaemu-overlay

11.	That’s it. Eject the SD card and put it into your modded GBA-Rasp Pi.  

### 2. First power up
1.	If you power on the GBA at this point, the TFT should turn on but you won’t be able to see anything just yet. **Turn the GBA back off**
2.	Plug your modified Gameboy link cable in the link port on the modded GBA and the USB end into your PC. If you’re running Windows, the FTDI chip should be detected and automatically installed. If not, you can install the Virtual Com Port (VCP) drivers here: http://www.ftdichip.com/Drivers/VCP.htm. The chip should still be detected even if the GBA is off as this chip is powered from the USB bus.
3.	After it has installed confirm that a COM port appears in your Device Manager
4.	Download and install [FT_PROG](http://www.ftdichip.com/Support/Utilities.htm#FT_PROG)
5.	Once installed, open the program and go DEVICES>Scan and Parse. The FTDI chip should be detected.
6.	Search through the settings and set the following options in the EEPROM:
  1.	Enable the **Bus Powered** option
  2.	Set **Max Bus Power** to 500 mA
  3.	Enable **Pull-down IO Pins in USB Suspend**
  4.	Set CBUS3 output to **PWREN#**
  5.	Go to DEVICES>Program to write the settings to the FTDI chip.  
  6.	Close FT_Prog, Remove and reinsert the GBA link cable from the USB port. Confirm that the COM port reappears in your Device Manger. Note the COM Port Number. (i.e COM3)
7.	Open up a program that allows communication of the COM Port. I have used [Putty](http://www.chiark.greenend.org.uk/~sgtatham/putty/).
8.	Select **Serial** connection type. Type in your COM Port number and Speed of 115200. Hit Open.
9.	Now power on your GBA using the power switch. You should be presented with a message *’Uncompressing Linux… done, booting the kernel.’*. Wait for the login prompt to appear. This can take about a 1 minute or more.
10.	Default login is *pi* and password is *raspberry*.
11.	After first login, a First boot script will run, and reboot the system once again.
12.	Now you can issue commands to the Raspberry Pi Zero over the command line!

### 3. Configuration of System.
1.	Issue the following commands one at a time:
> sudo cp /boot/gba_mod/retrogame /usr/local/bin/retrogame  
> sudo cp /boot/gba_mod/gpio_alt /usr/local/bin/gpio_alt  
> sudo cp /boot/gba_mod/fbcp /usr/local/bin/fbcp  
> sudo cp /boot/gba_mod/retroarch.cfg /opt/retropie/configs/all/retroarch.cfg  
> sudo cp /boot/gba_mod/es_input.cfg /opt/retropie/configs/all/emulationstation/es_input.cfg  
> cd /boot/gba_mod/wiringpi  
> sudo ./build  

2. Enter the Rasp Pi config screen
>sudo raspi-config

3. Set the following options:  
  1. Disable overscan  
  2. Force 3.5mm audio (headphone)  
  3. Enable SPI  
  4. Exit raspi-config. There's no need to reboot just yet.

4.	To set the programs to begin at startup edit /etc/rc.local
> sudo nano /etc/rc.local

5. Add the following to the end of the file but **before** exit 0.
> fbcp &  
> retrogame &  
> gpio_alt -p 18 -f 5

6.	Setup virtual keyboard by the following:
> sudo nano /etc/udev/rules.d/10-retrogame.rules

   and add the following to this file:
> SUBSYSTEM=="input", ATTRS{name}=="retrogame", ENV{ID_INPUT_KEYBOARD}="1"

   Increase the font size in EmulationStation for readability by the following:
> sudo nano /etc/emulationstation/themes/carbon/carbon.xml

   Then scroll down to the ‘gamelist’ section and change any occurance of fontSize to 0.05.

7.	Reboot and you should have everything up and running!
8.	(Optional) In EmulationStation goto the ‘RetroPie’ menu item and explore all the options available to tweak how you like it.

### 4. Get ROMS onto it
1.	The easiest way to do this is to get a microUSB to USB adapter and connect a wifi dongle to the Raspberry Pi USB port. To setup wifi, do the following in the serial console:
> sudo nano /etc/wpa_supplicant/wpa_supplicant.conf

   Add the following and replace with your wifi info:
>network={  
>	ssid=”YOURWIFINAME”  
> 	psk=”YOURPASSWORD”  
>}  

2.	Reboot device and confirm that it connects to the wifi network. Check your IP by issuing the following into the serial console:
> ifconfig

3.	Located your IP under wlan0 and note this number.
4.	Download [WinSCP](https://winscp.net/eng/download.php). Setup a SFTP session using your IP address, Port 22 and username *pi* and password *raspberry*.
5.	Copy ROMS to their respective folders in **/home/pi/RetroPie/roms/**
6. Alternatively, if you're able to read the linux partition on your PC, your could remove the SD card and place it in your computer and copy them over directly.

### 5. Make console better (Optional)
1. This step makes the text on the console a bit easier to read on the small screen. In the serial console perform the following:
> sudo dpkg-reconfigure console-setup

   Select “UTF-8”, “Guess optimal character set”, “Terminus” and “6x12 (framebuffer only).”




