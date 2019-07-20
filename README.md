
## Introduction

trashcan-lxpanel-plugin is an LXDE lxpanel plugin which shows trash can in a panel,
instead of on the desktop, in order to anchor the placement of trash can in some
chosen panel at the periphery of the screen. Most useful when the Linux system is
run as a guest OS in a virtual machine, where one might resize the VM's window size.

The plugin has default icons for the trash can, both in empty and full condition,
but can display users' choice of own images in any formats such as PNG, JPG, ICO,
BMP, GIF, and SVG, if so desired.

<img src="https://github.com/dvertx/trashcan-lxpanel-plugin/blob/master/images/screenshot.jpg" alt="screenshot" align="middle" width="80%">

## Requirements

To successfully compile this plugin, some libraries are expected to have been
installed beforehand.

Ubuntu/Debian:

    $ sudo apt install libgtk2.0-dev lxpanel-dev libfm-dev

Fedora:

    $ sudo dnf install gtk2-devel lxpanel-devel libfm-devel

## Installation

The recommended way to install the plugin is through make:

    $ make
    $ sudo make install

### Author

- Name: Hertatijanto Hartono
- Email: dvertx [AT] gmail [DOT] com
