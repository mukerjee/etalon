#!/bin/bash

echo "$HOME/sdrt/cloudlab/tune.sh" >> $HOME/.bashrc

# get docker
cd $HOME
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# get pipework
cd $HOME
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework
