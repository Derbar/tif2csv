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
 
#define ABGR2RGBA(r,g,b,a) ((r<<24) | (g<<16) | (b<<8) | a)

#define Get16bitR(abgr) ((abgr) & 0xffff)
#define Get16bitG(abgr) (((abgr) >> 16) & 0xffff)
#define Get16bitB(abgr) (((abgr) >> 32) & 0xffff)
#define Get16bitA(abgr) (((abgr) >> 48) & 0xffff)

std::string INFILE;
std::string OUTFILE;

void dgc_die (std::string msg) 
{
  std::cerr << msg << std::endl;
  exit(0);
}

void showMetadata(TIFF *tif, GTIF *gtif)
{
  /*Show tiff file infomation.*/
  TIFFPrintDirectory(tif,stdout,TIFFPRINT_GEOKEYDIRECTORY);

  uint16 nsamples;
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
  std::cout << "nsamples=" << nsamples << std::endl;
  
  uint16 nbits;
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE , &nbits);
  std::cout << "nbits=" << nbits << std::endl;
  
  char *citation;
  tagtype_t type;
  int size;
  int cit_length = GTIFKeyInfo(gtif,GTCitationGeoKey,&size,&type);
  if (cit_length > 0)
  {
          citation = (char *)malloc(size*cit_length);
          GTIFKeyGet(gtif, GTCitationGeoKey, citation, 0, cit_length);
          printf("Citation:%s\n",citation);
  }

  int config;
  uint32 imagelength;
	uint32 row;
	uint32 tileWidth, tileLength;
	uint32* bc;

	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);
	TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
	TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &bc);
	
	TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
	TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
	std::cout << "config: " << config << std::endl;
	std::cout << "bc: " << bc << std::endl;
	std::cout << "stripsize : " << bc[0] << std::endl;
	std::cout << "Strip number: " << TIFFNumberOfStrips(tif) << std::endl;
  std::cout << "tile size: " << TIFFTileSize(tif) << std::endl;
  std::cout << "scan line size: " << TIFFScanlineSize(tif) << std::endl;
  std::cout << "tileWidth: " << tileWidth << std::endl;
  std::cout << "tileLength: " << tileLength << std::endl;
  std::cout << "Tiles number: " << TIFFNumberOfTiles(tif) << std::endl;
}

int tif2csv()
{
  //std::string strHeader = "x,y,color\n";
  std::string strHeader = "x,y,r,g,b,a\n";
  std::string temp = "";
  FILE *		ofile; /* outfile(csv) */
  TIFF *tif=(TIFF*)0;  /* TIFF-level descriptor */
  GTIF *gtif=(GTIF*)0; /* GeoKey-level descriptor */
  
  double left, right, bottom, top;
  double resolution = 1.0;
  
  /* Open TIFF descriptor to read GeoTIFF tags */
  tif=XTIFFOpen(INFILE.c_str(),"r");
  if (!tif) dgc_die("Unable open to file " + INFILE );
  gtif = GTIFNew( tif );


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
  
  printf("pos after %.10f %.10f -> %.10f %.10f \n", left, top, right, bottom);
  
  
  /* Write csv File. */ 
  ofile = fopen(OUTFILE.c_str(), "w");
  uint16 nbits;
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE , &nbits);
  if (nbits == 16)
  {
      fwrite( strHeader.c_str(), sizeof(char), strHeader.length(), ofile);
      uint32 tileWidth, tileLength;
      TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
      TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
      
      tdata_t buf = _TIFFmalloc(TIFFTileSize(tif));
      for (int tileY = 0; tileY < height; tileY += tileLength)
      {
          printf("tileY: %d\n", tileY);
          for (int tileX = 0; tileX < width; tileX += tileWidth)
          {
              TIFFReadTile(tif, buf, tileX, tileY, 0, 4);
              uint64_t* buf_t = (uint64_t*)buf;
              for (int i=0; i< tileWidth*tileLength; i++)
              {
                  int x = tileX + i%tileWidth;
                  int y = tileY + i/tileWidth ;
                  if ( x >= width) continue;
                  if ( y >= height) continue;
                  char buff[100];
                  sprintf(buff, "%d,%d,%d,%d,%d,%d\n", x, y, Get16bitR(buf_t[i]), Get16bitG(buf_t[i]), Get16bitB(buf_t[i]), Get16bitA(buf_t[i]));
                  temp = buff;
                  fwrite( temp.c_str(), sizeof(char), temp.length(), ofile);        
                  //printf("i=%d: %d %d %d %d\n", i, Get16bitR(buf_t[i]),Get16bitG(buf_t[i]),Get16bitB(buf_t[i]),Get16bitA(buf_t[i]) );
              }
          }
      }
      
      /*
      int x = 768;
      int y = 768;
      TIFFReadTile(tif, buf, x, y, 0,4);
      std::cout << "buf: " << buf << std::endl;
      std::cout << sizeof(buf) << std::endl;
      
      uint64_t* buf_t = (uint64_t*)buf;
      std::cout << buf_t[0] << std::endl;
      for (int i=0; i< 10 ; i++ )
      {
          printf("i=%d: %lx\n", i, buf_t[i]);
          printf("i=%d: %d %d %d %d\n", i, Get16bitR(buf_t[i]),Get16bitG(buf_t[i]),Get16bitB(buf_t[i]),Get16bitA(buf_t[i]) );
          printf("i=%d: %lx %lx %lx %lx\n", i, Get16bitR(buf_t[i]),Get16bitG(buf_t[i]),Get16bitB(buf_t[i]),Get16bitA(buf_t[i]) );
      }*/
      
      _TIFFfree(buf);
  }
  else if (nbits == 8)
  {
      //https://www.asmail.be/msg0054846698.html   
      const size_t npx = width*height;
      uint32_t *raster = new uint32_t[npx];
      int image = TIFFReadRGBAImage(tif,width,height,raster,0);
      
      //fwrite( strHeader.c_str(), sizeof(char), strHeader.length(), ofile);
      for(int y=0;y<height;y++) //height
      {
        for(int x=0;x<width;x++) //width
        {
            //printf("%.6lX ",raster[y*width+x]);
            char buff[50];
            uint32_t value = ABGR2RGBA(TIFFGetR(raster[y*width+x]),TIFFGetG(raster[y*width+x]),TIFFGetB(raster[y*width+x]),TIFFGetA(raster[y*width+x]));
            //sprintf(buff, "%d, %d, %ld\n", x,y,raster[y*width+x]);
            sprintf(buff, "%d,%d,%d,%d,%d,%d\n", x, y, TIFFGetR(raster[y*width+x]), TIFFGetG(raster[y*width+x]), TIFFGetB(raster[y*width+x]), TIFFGetA(raster[y*width+x]));
            temp = buff;
            fwrite( temp.c_str(), sizeof(char), temp.length(), ofile);        
        }        
        //printf("\n");
      }
      delete []raster;
  }
  else 
  {
      printf("Not supported for %d bits.\n",nbits);
  }

  /* Close */  
  fclose(ofile);
  GTIFFree(gtif);
  XTIFFClose(tif);
  
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
