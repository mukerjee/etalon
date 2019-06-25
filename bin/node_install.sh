#!/bin/bash
#
# Set up an Etalon node machine.
#
# The first argument is the new hostname.

set -o errexit

NEW_HOSTNAME=$1

# Validate.
if [ -z $NEW_HOSTNAME ]; then
    echo "Error: Must provide a hostname!"
    exit 1
fi
if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

source $HOME/etalon/bin/common_install.sh $NEW_HOSTNAME

sudo apt update
sudo apt install -y iputils-arping

# Start the rpyc daemon.
echo "Starting rpyc daemon..."
sudo systemctl enable /etalon/rpycd/rpycd.service

# Pipework.
echo "Installing Pipework..."
cd
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

# Give SSH access to the switch.
cat /etalon/vhost/config/ssh/id_rsa.pub >> $HOME/.ssh/authorized_keys

# Do this last because afterwards apt complains and prevents packages from being
# installed.
source $HOME/etalon/bin/kernel_install.sh

# Fix permissions of ~/.config. Do this last because something else is setting
# the owner to "root".
if [ -d $HOME/.config ]; then
    sudo chown -R `whoami`:`whoami` $HOME/.config
fi

echo "Done"
sudo reboot
