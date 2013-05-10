/**
  @file
  Header and templated code for reading and writing data, primarily in the
  ArcGrid ASCII format.

  Richard Barnes (rbarnes@umn.edu), 2012
*/
#ifndef _data_io_included
#define _data_io_included

#include "interface.h"
#include <fstream>
#include <string>
//#include <fcntl.h> //Used for posix_fallocate

int load_ascii_data(const char filename[], float_2d &elevations);
int load_ascii_data(const char filename[], char_2d &data);
int write_arrows(const char filename[], const char_2d &flowdirs);

#define OUTPUT_DEM  1
#define OUTPUT_OMG  2

//load_ascii_data
/**
  @brief  Writes an ArcGrid ASCII file or OmniGlyph file
  @author Richard Barnes (rbarnes@umn.edu)

  @param[in]  &filename     Name of ArcGrid ASCII file to write
  @param[in]  &elevations   DEM object to write
  @param[in]  precision     Floating-point precision of output

  @returns 0 upon success
*/
template <class T>
int output_ascii_data(
  const std::string filename,
  const array2d<T> &output_grid,
  int precision=8
){
  std::ofstream fout;
  std::string outputsep=" ";
  int output_type=OUTPUT_DEM;
  Timer write_time;
  ProgressBar progress;

  write_time.start();

  diagnostic_arg("Opening ASCII output file \"%s\"...",filename.c_str());
  fout.open(filename.c_str());
  if(!fout.is_open()){
    diagnostic("failed!\n");
    exit(-1);  //TODO: Need to make this safer! Don't just close after all that work!
  }
  diagnostic("succeeded.\n");

/*  diagnostic_arg("Reserving %ldMB of disk space...", output_grid.estimated_output_size()/1024/1024);
  if(posix_fallocate(fileno(fout),0,output_grid.estimated_output_size())){
    diagnostic("failed!\n");
    return -1;
  }
  diagnostic("succeeded.\n");
*/

  //OmniGlyph output
  if(filename.substr(filename.length()-4)==".omg"){
    outputsep="|";
    output_type=OUTPUT_OMG;
    diagnostic("Writing OmniGlyph file header...");
    fout<<"Contents: Pixel array"<<std::endl;
    fout<<std::endl;
    fout<<"Width:    "<<output_grid.width()<<std::endl;
    fout<<"Height:   "<<output_grid.height()<<std::endl;
    fout<<std::endl;
    fout<<"Spectral bands:   1"<<std::endl;
    fout<<"Bits per band:   32"<<std::endl;
    fout<<"Range of values:   "<<output_grid.min()<<","<<output_grid.max()<<std::endl;
    fout<<"Actual range:   "<<output_grid.no_data<<","<<output_grid.max()<<std::endl;  //TODO: Assumes no_data is a small negative value
    fout<<"Gamma exponent:   0."<<std::endl;
    fout<<"Resolution:   100 pixels per inch"<<std::endl;
    fout<<std::endl;
    fout<<"|"<<std::endl;
  } else {
    diagnostic("Writing ArcGrid ASCII file header...");
    fout<<"ncols\t\t"<<output_grid.width()<<std::endl;
    fout<<"nrows\t\t"<<output_grid.height()<<std::endl;
    fout<<"xllcorner\t"<<std::fixed<<std::setprecision(precision)<<output_grid.xllcorner<<std::endl;
    fout<<"yllcorner\t"<<std::fixed<<std::setprecision(precision)<<output_grid.yllcorner<<std::endl;
    fout<<"cellsize\t"<<std::fixed<<std::setprecision(precision)<<output_grid.cellsize<<std::endl;
    fout<<"NODATA_value\t"<<std::fixed<<std::setprecision(precision);
    if(sizeof(T)==1)  //TODO: Crude way of detecting chars and bools
      fout<<(int)output_grid.no_data<<std::endl;
    else
      fout<<output_grid.no_data<<std::endl;
  }
  diagnostic("succeeded.\n");

  diagnostic("%%Writing ArcGrid ASCII file data...\n");
  progress.start( output_grid.width()*output_grid.height() );
  fout.precision(precision);
  fout.setf(std::ios::fixed);
  for(int y=0;y<output_grid.height();y++){
    progress.update( y*output_grid.width() );
    if(output_type==OUTPUT_OMG)
      fout<<"|";
    for(int x=0;x<output_grid.width();x++)
      if(sizeof(T)==1)  //TODO: Crude way of detecting chars and bools
        fout<<(int)output_grid(x,y)<<outputsep;
      else
        fout<<output_grid(x,y)<<outputsep;
    fout<<std::endl;
  }
  diagnostic_arg(SUCCEEDED_IN,progress.stop());

//  diagnostic("Writing file data...");
//  fout<<std::setprecision(precision)<<output_grid;
//  diagnostic("succeeded.\n");

  fout.close();

  write_time.stop();
  diagnostic_arg("Write time was: %lf\n", write_time.accumulated());

  return 0;
}








/**
  @brief  Writes a floating-point grid file
  @author Richard Barnes (rbarnes@umn.edu)

  @param[in]  &basename     Name, without extension, of output file
  @param[in]  &output_grid  DEM object to write

  @todo Does not check byte order (big-endian, little-endian)
  @todo Does not output only IEEE-754 32-bit floating-point,
        which is required by ArcGIS

  @returns 0 upon success
*/
template <class T>
int write_floating_data(
  const std::string basename,
  const array2d<T> &output_grid
){
  Timer write_time;
  ProgressBar progress;
  std::string fn_header(basename), fn_data(basename);

  fn_header+=".hdr";
  fn_data+=".flt";

  write_time.start();


  {
    diagnostic_arg("Opening floating-point header file \"%s\" for writing...",fn_header.c_str());
    std::ofstream fout;
    fout.open(fn_header.c_str());
    if(!fout.is_open()){
      diagnostic("failed!\n");
      exit(-1);  //TODO: Need to make this safer! Don't just close after all that work!
    }
    diagnostic("succeeded.\n");

    diagnostic("Writing floating-point header file...");
    fout<<"ncols\t\t"<<output_grid.width()<<std::endl;
    fout<<"nrows\t\t"<<output_grid.height()<<std::endl;
    fout<<"xllcorner\t"<<std::fixed<<std::setprecision(10)<<output_grid.xllcorner<<std::endl;
    fout<<"yllcorner\t"<<std::fixed<<std::setprecision(10)<<output_grid.yllcorner<<std::endl;
    fout<<"cellsize\t"<<std::fixed<<std::setprecision(10)<<output_grid.cellsize<<std::endl;
    fout<<"NODATA_value\t"<<std::fixed<<std::setprecision(10)<<output_grid.no_data<<std::endl;
    fout<<"BYTEORDER\tLSBFIRST"<<std::endl; //TODO
    fout.close();
    diagnostic("succeeded.\n");
  }


  diagnostic_arg("Opening floating-point data file \"%s\" for writing...",fn_data.c_str());

  {
    std::ofstream fout(fn_data.c_str(), std::ios::binary | std::ios::out);
    if(!fout.is_open()){
      diagnostic("failed!\n");
      exit(-1);  //TODO: Need to make this safer! Don't just close after all that work!
    }
    diagnostic("succeeded.\n");

    diagnostic("%%Writing floating-point data file...\n");
    progress.start( output_grid.width()*output_grid.height() );
    for(int y=0;y<output_grid.height();++y){
      progress.update( y*output_grid.width() );
      for(int x=0;x<output_grid.width();++x)
        fout.write(reinterpret_cast<const char*>(&output_grid(x,y)), std::streamsize(sizeof(T)));
    }
    fout.close();
    write_time.stop();
    diagnostic_arg(SUCCEEDED_IN,progress.stop());
  }

  diagnostic_arg("Write time was: %lf\n", write_time.accumulated());

  return 0;
}






/**
  @brief  Reads in a floating-point grid file
  @author Richard Barnes (rbarnes@umn.edu)

  @param[in]  &basename     Name, without extension, of input file
  @param[in]  &grid         DEM object in which to store data

  @todo Does not check byte order (big-endian, little-endian)
  @todo Does not input only IEEE-754 32-bit floating-point,
        which is required by ArcGIS

  @returns 0 upon success
*/
template <class T>
int read_floating_data(
  const std::string basename,
  array2d<T> &grid
){
  Timer io_time;
  ProgressBar progress;
  std::string fn_header(basename), fn_data(basename);

  fn_header+=".hdr";
  fn_data+=".flt";

  int columns, rows;
  char byteorder;

  io_time.start();


  {
    FILE *fin;
    diagnostic_arg("Opening floating-point header file \"%s\" for reading...",fn_header.c_str());
    fin=fopen(fn_header.c_str(),"r");
    if(fin==NULL){
      diagnostic("failed!\n");
      exit(-1);
    }
    diagnostic("succeeded.\n");


    diagnostic("Reading DEM header...");
    if(fscanf(fin,"ncols %d nrows %d xllcorner %lf yllcorner %lf cellsize %lf NODATA_value %f BYTEORDER %c",&columns, &rows, &grid.xllcorner, &grid.yllcorner, &grid.cellsize, &grid.no_data, &byteorder)!=7){
      diagnostic("failed!\n");
      exit(-1);
    }
    diagnostic("succeeded.\n");
    fclose(fin);
  }

  diagnostic_arg("The loaded DEM will require approximately %ldMB of RAM.\n",columns*rows*((long)sizeof(float))/1024/1024);

  diagnostic("Resizing grid...");  //TODO: Consider abstracting this block
  grid.resize(columns,rows);
  diagnostic("succeeded.\n");



  diagnostic_arg("Opening floating-point data file \"%s\" for reading...",fn_data.c_str());

  {
    std::ifstream fin(fn_data.c_str(), std::ios::binary | std::ios::in);
    if(!fin.is_open()){
      diagnostic("failed!\n");
      exit(-1);  //TODO: Need to make this safer! Don't just close after all that work!
    }
    diagnostic("succeeded.\n");


    diagnostic("%%Reading data...\n");
    progress.start(columns*rows);
    grid.data_cells=0;
    for(int y=0;y<rows;++y){
      progress.update(y*columns); //Todo: Check to see if ftell fails here?
      for(int x=0;x<columns;++x){
        fin.read(reinterpret_cast<char*>(&grid(x,y)), std::streamsize(sizeof(T)));
        if(grid(x,y)!=grid.no_data)
          grid.data_cells++;
      }
    }
    io_time.stop();
    diagnostic_arg(SUCCEEDED_IN,progress.stop());

  }

  diagnostic_arg("Write time was: %lf\n", io_time.accumulated());

  return 0;
}

#endif
