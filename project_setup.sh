#!/bin/sh

path_to_esp=
libraries_folder=
arduino_core_version=

help() {
  echo "Usage:"
  echo " ./project_setup.sh -l <absolute path to Arduino IDE libraries folder> -e <absolute path to folder with ESP8266 hardware platform for Arduino>"
  echo
  echo "Example:"
  echo " ./project_setup.sh -l /home/johndoe/Arduino/libraries -e /home/johndoe/Arduino/hardware/esp8266"
  echo
  echo " ./project_setup.sh -h --- this help"

  exit 1
}

while getopts 'l:e:v:h' opt
do
  case "$opt" in
    l)
      libraries_folder=$OPTARG
      ;;
    e)
      path_to_esp=$OPTARG
      ;;
    h)
      help
      ;;
    \?)
      echo "Invalid option '-$OPTARG'"
      help
      ;;
    :)
      echo "Option '-$OPTARG' requires an argument"
      help
      ;;
  esac
done

if [ -z "$path_to_esp" ] || [ -z "$libraries_folder" ] ; then
  help
fi

sed "s#%ABSOLUTE_PATH_TO_ARDUINO_IDE_LIBRARIES_FOLDER%#$libraries_folder#g; s#%ABSOLUTE_PATH_TO_ESP8266_ARDUINO_PLATFROM_FOLDER%#$path_to_esp#g" ./SamsungTvController/.project.template > ./SamsungTvController/.project
cp ./SamsungTvController/.settings/org.eclipse.cdt.core.prefs.template ./SamsungTvController/.settings/org.eclipse.cdt.core.prefs
sed "s#%ABSOLUTE_PATH_TO_ARDUINO_IDE_LIBRARIES_FOLDER%#$libraries_folder#g; s#%ABSOLUTE_PATH_TO_ESP8266_ARDUINO_PLATFROM_FOLDER%#$path_to_esp#g" ./DaitsuController/.project.template > ./DaitsuController/.project
cp ./DaitsuController/.settings/org.eclipse.cdt.core.prefs.template ./DaitsuController/.settings/org.eclipse.cdt.core.prefs
