# Wraith

Changes:
- added [Documentation.md](Documentation.md)
- added [scripts/wraith.pl](scripts/wraith.pl) Generates hashes and salts for Wraith Botpack	
- always cloak os version (also added a few new versions)
- added botident option to conf which spoofs ident to whatever you want (by default it sets ident to ~/.ispoof file)
- few small changes so it compiles on Debian (tested ok with gcc 4.9, gcc6 does not seem to work)

__These changes are for this fork only.__

[![Build Status](https://travis-ci.org/wraith/wraith.png?branch=master)](https://travis-ci.org/wraith/wraith)

* http://wraith.botpack.net
* http://github.com/wraith/wraith
* @wraithbotpack

Wraith is an IRC channel management bot written purely in C/C++.
It has been in development since late 2003. It is based on
Eggdrop 1.6.12 but has since evolved into something much
different at its core. TCL and loadable modules are currently
not supported.

* Wraith aims to be a secure and easy to setup and manage botnet.
* A botnet can be setup in a matter of minutes and updated later with 1 command.
* Leaf bots save no files locally, but rather store configuration encrypted in
  their own binary.
* Hubs do not connect to IRC, and keep a local encrypted copy of the userfile.

All official sites, documentation, support venues, repositories and source urls
are referenced here.

For official release announcements send an email to:
  wraith-announce-subscribe@botpack.net

Download: http://wraith.botpack.net/wiki/Download
Git: git://github.com/wraith/wraith.git

See git for a list of Contributors: git shortlog -sen master

Support:
* How To Contribute: http://github.com/wraith/wraith/blob/master/CONTRIBUTING.md
* Getting Started: http://github.com/wraith/wraith/wiki/GettingStarted
* FAQ: http://wraith.botpack.net/wiki/FrequentlyAskedQuestions
* Documentation Index: http://wraith.botpack.net/wiki/Documentation
* Issues can be reported at: https://github.com/wraith/wraith/issues
* #wraith @ EFnet

Please support wraith by signing up for a shell at http://www.xzibition.com (coupon 'wraith' for 30% off)

The botpack ghost inspired the early versions of wraith and a few cmds.
  einride
  ievil

The following botpacks gave inspiration, ideas, and some code:
  awptic by lordoptic
  optikz by ryguy and lordoptic
  celdrop by excelsior
  genocide by Crazi, Dor, psychoid, and Ace24
  tfbot by warknite and loslinux

