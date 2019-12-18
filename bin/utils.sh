#!/bin/bash
#
# Common utility functions for setup scripts.

function hostname_validate {
    NEW_HOSTNAME="$1"
    if [ -z "$NEW_HOSTNAME" ]; then
        echo "Error: Must provide a hostname!"
        exit 1
    fi
}

function ubuntu_validate_version {
    UBUNTU_VERSION_SUPPORTED="$1"
    # Check that the Ubuntu version is correct.
    UBUNTU_VERSION_CURRENT="$(lsb_release -ds | grep -oP "Ubuntu \\K([0-9]+\\.[0-9]+)")"
    if [ "$UBUNTU_VERSION_CURRENT" != "$UBUNTU_VERSION_SUPPORTED" ]; then
        echo "Ubuntu version must be $SUPPORTED_VERSION, but is:" \
             "$UBUNTU_VERSION"
        exit
    fi
}

function ubuntu_validate_codename {
    UBUNTU_VERSION_SUPPORTED="$1"
    # Check that the Ubuntu version is correct.
    UBUNTU_VERSION_CURRENT="$(lsb_release -cs)"
    if [ "$UBUNTU_VERSION_CURRENT" != "$UBUNTU_SUPPORTED_VERSION" ]; then
        echo "Ubuntu version must be $SUPPORTED_VERSION, but is:" \
             "$UBUNTU_VERSION_CURRENT"
        exit
    fi
}
