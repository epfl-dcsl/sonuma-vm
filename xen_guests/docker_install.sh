#!/bin/bash
apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
apt-key fingerprint 0EBFCD88 | grep fingerprint | sed -n "s/.*\=\s*\([0-9]*[A-F]*\)/\1/p" | diff - <(echo "9DC8 5822 9FC7 DD38 854A E2D8 8D81 803C 0EBF CD88") > ".KeyFprintDiff.out"
if [ $? -ne 0 ];
then echo -e "\e[1;31m WARN: Key fingerprint from Docker did not match the validated online key...\n\tValidate them in \".KeyFprintDiff.out\"\e[0m";
fi
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
apt-get update
apt-get install docker-ce
docker run hello-world
