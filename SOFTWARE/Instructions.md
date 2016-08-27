Draft copy! Still working on this

## Software Setup
This following describes the software installation steps for my GBA mod.
The following sources were used:
https://learn.adafruit.com/running-opengl-based-games-and-emulators-on-adafruit-pitft-displays/pitft-setup
https://github.com/notro/fbtft/wiki/
### 1. OS Installation
1.	Download RetroPie from https://retropie.org.uk/
2.	The version I used v3.8.1
3.	Flash the .img file to an SD card using [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager/).
4.	Once the image has been flashed to the SD card a **boot** partition should appear in your PC.
5.	Place *‘gbaemu-overlay.dtbo’* into the **boot/overlays** folder.
6.	In **boot/** create a new folder called *‘gba_mod’*
7.	In the **gba_mod** folder place the following programs/files:  
a.  retrogame  
b.  gpio_alt  
c.  fbcp  
d.  es_input.cfg  
e.  retroarch.cfg  
f.  wiringpi *(folder)*  

8.	Edit boot/config.txt with a editor that follows the linefeeds in a linux sysem (i.e notepad++) and include the following at the end of the file then save and close the file:
> hdmi_force_hotplug=1  
> hdmi_group=2  
> hdmi_mode=87  
> hdmi_cvt=320 240 60 1 0 0 0  
> dtoverlay=gbaemu-overlay

9.	That’s it. Eject the SD card and put it into your modded GBA-Rasp Pi.  

### 2. First power up
1.	If you power on the GBA at this point, the TFT should turn white but you won’t be able to see anything just yet. **Turn it back off**
2.	Plug your modified Gameboy link cable in the link port on the modded GBA and the USB end into your PC. If you’re running Windows, the FTDI chip should be detected and automatically installed. If not, you can install the Virtual Com Port (VCP) drivers here: http://www.ftdichip.com/Drivers/VCP.htm. The chip should still be detected even if the GBA is off as this chip is powered from the USB bus.
3.	After it has installed confirm that a COM port appears in your Device Manager
4.	Download and install [FT_PROG](http://www.ftdichip.com/Support/Utilities.htm#FT_PROG)
5.	Once installed, open the program and go DEVICES>Scan and Parse. The FTDI chip should be detected.
6.	Search through the settings and set the following options in the EEPROM:  
a.	Enable the **Bus Powered** option  
b.	Set **Max Bus Power** to 500 mA  
c.	Enable **Pull-down IO Pins in USB Suspend**  
d.	Set CBUS3 output to **PWREN#**  
e.	Go to DEVICES>Program to write the settings to the FTDI chip.  
f.	Close FT_Prog, Remove and reinsert the GBA link cable from the USB port. Confirm that the COM port reappears in your Device Manger. Note the COM Port Number. (i.e COM3)
7.	Open up a program that allows communication of the COM Port. I have used [Putty](http://www.chiark.greenend.org.uk/~sgtatham/putty/).
8.	Select **Serial** connection type. Type in your COM Port number and Speed of 115200. Hit Open.
9.	Now power on your GBA using the power switch. You should be presented with a message *’Uncompressing Linux… done, booting the kernel.’*. Wait for the login prompt to appear. This can take about a 1 minute or more.
10.	Default login is *pi* and password is *raspberry*
11.	Now you can issue commands to the Raspberry Pi Zero over the command line!

### 3. Configuration of System.
1.	Issue the following commands one at a time:
> sudo cp /boot/gba_mod/retrogame /usr/local/bin/retrogame  
> sudo cp /boot/gba_mod/gpio_alt /usr/local/bin/gpio_alt  
> sudo cp /boot/gba_mod/fbcp /usr/local/bin/fbcp  
> sudo cp /boot/gba_mod/es_input.cfg /opt/retropie/configs/all/retroarch.cfg  
> sudo cp /boot/gba_mod/retroarch.cfg /opt/retropie/configs/all/emulationstation/es_input.cfg  
> cd /boot/gba_mod/wiringpi  
> sudo ./build  

2. Enter the Rasp Pi config screen:
>sudo raspi-config

Set the following options:  
a. Disable overscan  
b. Force 3.5mm audio (headphone)  
c. Enable SPI  

2.	To set the programs to begin at startup edit /etc/rc.local
> sudo nano /etc/rc.local

Add the following to the end of the file but **before** exit 0.
> fbcp &  
> retrogame &  
> gpio_alt -p 18 -f 5

3.	Setup virtual keyboard by the following:
> sudo nano /etc/udev/rules.d/10-gbaemu.rules

and add the following to this file:

> SUBSYSTEM=="input", ATTRS{name}=="gbaemu", ENV{ID_INPUT_KEYBOARD}="1"

4.	Increase the font size in EmulationStation for readability by the following:

> sudo nano /etc/emulationstation/themes/carbon/carbon.xml

Then scroll down to the ‘gamelist’ section and change any occurance of fontSize to 0.05.

5.	Reboot and you should have everything up and running!
6.	In EmulationStation goto the ‘RetroPie’ menu item and explore all the options available to tweak how you like it.

### Get ROMS onto it
1.	The easiest way to do this is to  get a microUSB to USB adapter and connect a wifi dongle to the Raspberry Pi USB port. To setup wifi, do the following in the serial console:

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




![GitHub Logo](/images/logo.png)

