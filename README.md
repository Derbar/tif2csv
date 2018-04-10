# tif2csv
* environment: CentOS7 
* install requirement: <code>yum install gcc-c++ libtiff libtiff-devel libgeotiff libgeotiff-devel</code>
* build cmd:  <code>g++ -o tif2csv tif2csv.cpp -ltiff -lgeotiff -I/usr/include/libgeotiff</code>
* usage: <code>./tif2csv -i [tif file name] -o [csv file name]</code>
