#include <xtiffio.h>
#include <geotiff.h>
#include <geotiffio.h>
#include <tiff.h>
#include <tiffio.h>

#include <iostream>
#include <string>
#include "time.h"
#include <sstream>
#include <stdlib.h>
#include <stdint.h>
 
#define      MIN_RES_MULT         1

std::string INFILE;
std::string OUTFILE;

void dgc_die (std::string msg) 
{
  std::cerr << msg << std::endl;
  exit(0);
}

int tif2csv()
{
  std::string strHeader = "x,y,color\n";
  std::string temp = "";
  FILE *		ofile; /* outfile(csv) */
  TIFF *tif=(TIFF*)0;  /* TIFF-level descriptor */
  GTIF *gtif=(GTIF*)0; /* GeoKey-level descriptor */
  
  int cit_length;
  geocode_t model;    /* all key-codes are of this type */
  char *citation;
  int size;
  tagtype_t type;
  double left, right, bottom, top;
  double resolution = 1.0;
  
  /* Open TIFF descriptor to read GeoTIFF tags */
  tif=XTIFFOpen(INFILE.c_str(),"r");
  if (!tif) dgc_die("Unable open to file " + INFILE );
  gtif = GTIFNew( tif );
  
  
  /*Show tiff file infomation.*/
  TIFFPrintDirectory(tif,stdout,TIFFPRINT_GEOKEYDIRECTORY);


  /* Get width, height */
   uint32_t width = 0;
   if( !TIFFGetField(tif,TIFFTAG_IMAGEWIDTH, &width) ) 
       dgc_die("couldn't read width");
   std::cout << "w=" << width << std::endl;
    
   uint32_t height=0;
   if( !TIFFGetField(tif,TIFFTAG_IMAGELENGTH, &height) )
       dgc_die("couldn't read height");
   std::cout << "h=" << height << std::endl;
  
  
  /* Get left, top, right, bottom */
  //http://robots.stanford.edu/teichman/repos/track_classification/src/program/util/breakup_image.cpp
  left = 0; bottom = height;
  if(!GTIFImageToPCS(gtif, &left, &bottom))
    dgc_die("Error: could not transform coordinate to PCS");
  
  right = width; top = 0;
  if(!GTIFImageToPCS(gtif, &right, &top))
    dgc_die("Error: could not transform coordinate to PCS");
  
  printf("pos after %.10f %.10f -> %.10f %.10f  res %f\n", left, top, right, bottom,resolution * MIN_RES_MULT);
  
  
  /* Read RGBA value. */
  //https://www.asmail.be/msg0054846698.html   
  const size_t npx = width*height;
  uint32_t *raster = new uint32_t[npx];
  int image = TIFFReadRGBAImage(tif,width,height,raster,0);
  
  
  /* Write csv File. */
  ofile = fopen(OUTFILE.c_str(), "w");
  //fwrite( strHeader.c_str(), sizeof(char), strHeader.length(), ofile);
  int cnt = 0;
  for(int y=0;y<height;y++) //height
  {
    for(int x=0;x<width;x++) //width
    {
        //printf("%.6lX ",raster[y*width+x]);
        //printf("%d %d %d\n", TIFFGetR(raster[y*width+x]),TIFFGetG(raster[y*width+x]),TIFFGetB(raster[y*width+x]));
        if (raster[y*width+x] != 0xFF000000) cnt++;      
        
        char buff[50];
        sprintf(buff, "%d, %d, %ld\n", x,y,raster[y*width+x]);
        temp = buff;
        fwrite( temp.c_str(), sizeof(char), temp.length(), ofile);        
    }        
    //printf("\n");
  }
  printf("cnt: %d\n",cnt);


  /* Close */
  delete []raster;
  GTIFFree(gtif);
  XTIFFClose(tif);
  
  fclose(ofile);
  
  return 0;
}

int main(int argc, char **argv)
{
	clock_t 	before;
	double 		result;
	
	before = clock();

	if (argc < 5) 
	{
		printf("[ERROR] An insufficient number of arguments\n");
		return 0;
	}
	else if (argc > 5)
	{
		printf("[ERROR] Too many arguments \n");
		return 0;
	}

	std::ostringstream os;
	for (int i=0; i<argc; i++)
	{
		os << argv[i] << " ";
	}

	std::istringstream is(os.str());

	while( !is.eof() )
	{
		std::string tag;
		is >> tag;

		if (tag == "-o")
			is >> OUTFILE;
		if (tag == "-i")
			is >> INFILE;
		if (is.fail())
			break;
	}

	tif2csv();

	result = (double)(clock() - before) / CLOCKS_PER_SEC;
	printf("time: %5.2f sec \n", result); 

	return 0;
}
