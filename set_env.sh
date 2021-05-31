#!/bin/sh

echo "1. Install default environment package (Ubuntu x86_64)"
echo "======================================================"
sudo apt-get update
sudo apt-get upgrade
sudo apt update
sudo apt upgrade
sudo apt-get install build-essential qtcreator qt5-default libudev-dev cmake libxt-dev libusb-1.0-0-dev 
sudo apt-get install meshlab -y
sudo apt-get install doxygen
sudo apt-get install qtmultimedia5-dev libqt5multimediawidgets5 libqt5multimedia5-plugins libqt5multimedia5
sudo apt install ubuntu-restricted-extras

sudo apt install python3.7
sudo apt-get install python3-pip
sudo apt-get install python3.7-venv
sudo apt install libpython3.7-dev
sudo apt-get install python3.7-dev
sudo apt install libx11-dev
sudo apt install libudev-dev
sudo apt install libglfw3
sudo apt install libglfw3-dev
sudo apt install libusb-1.0.0-dev
sudo apt install liblog4cplus-dev
sudo apt install cgroup-tools
sudo apt install libcgroup-dev
sudo apt-get install libjpeg9
wget -q -O /tmp/libpng12.deb http://mirrors.kernel.org/ubuntu/pool/main/libp/libpng/libpng12-0_1.2.54-1ubuntu1_amd64.deb
sudo  dpkg -i /tmp/libpng12.deb
sudo add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main"
sudo apt update
sudo apt install libjasper1 libjasper-dev


echo
echo
echo "2. Install Grphics card driver (depend graphics card vendor)"
echo "============================================================"
echo "List can install drivers:"
echo "!!! Please install recommended driver (choose [recommended] driver) as below command !!!"
echo "for example: sudo apt-get install nvidia-driver-450-server"
echo "!!! Must reboot after installation !!!"
echo
sudo ubuntu-drivers devices
echo
