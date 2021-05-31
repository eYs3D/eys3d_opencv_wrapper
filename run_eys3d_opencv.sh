#!/bin/sh

ulimit -c unlimited
ulimit unlimited
export LD_LIBRARY_PATH=./../eSPDI:$LD_LIBRARY_PATH 
export LD_LIBRARY_PATH=./../libeYs3D_linux/lib/:$LD_LIBRARY_PATH 





echo "Project list : "
echo "=================="
echo "1. OpenCV(x86-64)-system default path"
echo "2. OpenCV(x86-64)-3.4.2 support GTK"
echo "3. OpenCV(x86-64)-3.4.2 support QT&OpencGL"
echo "=================="
echo

read -p "Please select project: " project
cd bin
case $project in
        [1]* )
        echo "1. OpenCV(x86-64)-system default path"
           ./eys3d_opencv_default
        break;;
        [2]* )
        echo "2. OpenCV(x86-64)-3.4.2 support GTK"
          export LD_LIBRARY_PATH=./../opencv_libs/opencv3.4.2_gtk/lib:$LD_LIBRARY_PATH 
          ./eys3d_opencv_gtk
        break;;
        [3]* )
        echo "3. OpenCV(x86-64)-3.4.2 support QT&OpencGL"
          export LD_LIBRARY_PATH=./../opencv_libs/opencv3.4.2_QT_GL/lib:$LD_LIBRARY_PATH 
          ./eys3d_opencv_opengl
        break;;
        * )
        echo "Please select project no.";;
esac
