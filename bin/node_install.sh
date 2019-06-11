#!/bin/bash
#
# Set up an Etalon node machine.

set -o errexit

if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

source $HOME/etalon/bincommon_install.sh

# Pipework.
cd
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

echo "Done"
# sudo reboot
