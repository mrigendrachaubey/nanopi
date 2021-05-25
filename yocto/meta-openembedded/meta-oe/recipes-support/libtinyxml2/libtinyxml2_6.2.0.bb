SUMMARY = "TinyXML-2 is a simple, small, efficient, C++ XML parser that can be easily integrating into other programs"
HOMEPAGE = "http://www.grinninglizard.com/tinyxml2/"
SECTION = "libs"
LICENSE = "Zlib"
LIC_FILES_CHKSUM = "file://tinyxml2.cpp;endline=22;md5=c19221dbd8a66ad3090462af4c5de5e7"

SRC_URI = "git://github.com/leethomason/tinyxml2.git"

SRCREV = "c1424ee4e1c1ed8129d9147ccc920ab92dd9ef86"

S = "${WORKDIR}/git"

inherit cmake

BBCLASSEXTEND = "native"
