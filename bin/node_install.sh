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

# Pipework.
cd
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

echo "Done"
sudo reboot
