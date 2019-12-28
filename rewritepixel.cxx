/*=========================================================================

  Program: Remove rectangular regions in pixel data using gdcm

  Copyright (c) 2020 Hauke Bartsch

  For debugging use:
    cmake -DCMAKE_BUILD_TYPE=Release ..
  to build gdcm.

  https://github.com/tesseract-ocr/tesseract/wiki/APIExample

  =========================================================================*/
#include "SHA-256.hpp"
#include "dateprocessing.h"
#include "gdcmAnonymizer.h"
#include "gdcmAttribute.h"
#include "gdcmDefs.h"
#include "gdcmDirectory.h"
#include "gdcmGlobal.h"
#include "gdcmImageReader.h"
#include "gdcmReader.h"
#include "gdcmStringFilter.h"
#include "gdcmSystem.h"
#include "gdcmWriter.h"
#include "json.hpp"
#include "optionparser.h"
#include <gdcmUIDGenerator.h>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

#include <dirent.h>
#include <errno.h>
#include <exception>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <thread>

struct threadparams {
  const char **filenames;
  size_t nfiles;
  char *scalarpointer;
  std::string outputdir;
  std::string patientid;
  std::string projectname;
  std::string sitename;
  std::string siteid;
  int dateincrement;
  bool byseries;
  int thread; // number of the thread
  // each thread will store here the study instance uid (original and mapped)
  std::map<std::string, std::string> byThreadStudyInstanceUID;
};

void *ReadFilesThread(void *voidparams) {
  threadparams *params = static_cast<threadparams *>(voidparams);

  const size_t nfiles = params->nfiles;
  for (unsigned int file = 0; file < nfiles; ++file) {
    const char *filename = params->filenames[file];
    // std::cerr << filename << std::endl;

    // gdcm::ImageReader reader;
    gdcm::Reader reader;
    reader.SetFileName(filename);
    try {
      if (!reader.Read()) {
        std::cerr << "Failed to read: \"" << filename << "\" in thread " << params->thread
                  << std::endl;
      }
    } catch (...) {
      std::cerr << "Failed to read: \"" << filename << "\" in thread " << params->thread
                << std::endl;
      continue;
    }

    // process sequences as well
    // lets check if we can change the sequence that contains the ReferencedSOPInstanceUID inside
    // the 0008,1115 sequence

    gdcm::DataSet &dss = reader.GetFile().GetDataSet();
    // look for any sequences and process them
    gdcm::DataSet::Iterator it = dss.Begin();
    for (; it != dss.End();) {
      const gdcm::DataElement &de = *it;
      gdcm::Tag tt = de.GetTag();
      gdcm::SmartPointer<gdcm::SequenceOfItems> seq = de.GetValueAsSQ();
      if (seq && seq->GetNumberOfItems()) {
        // fprintf(stdout, "Found sequence in: %04x, %04x\n", tt.GetGroup(), tt.GetElement());
        //	anonymizeSequence( params, &dss, &tt);
      }
      ++it;
    }

    /*    gdcm::Tag tsqur(0x0008,0x1115);
    if (dss.FindDataElement(tsqur)) { // this work on the highest level (Tag is found in dss), but
    what if we are already in a sequence?
      //const gdcm::DataElement squr = dss.GetDataElement( tsqur );
      anonymizeSequence( params, &dss, &tsqur);
      } */

    // const gdcm::Image &image = reader.GetImage();
    // if we have the image here we can anonymize now and write again
    gdcm::Anonymizer anon;
    gdcm::File &fileToAnon = reader.GetFile();
    anon.SetFile(fileToAnon);

    gdcm::MediaStorage ms;
    ms.SetFromFile(fileToAnon);
    if (!gdcm::Defs::GetIODNameFromMediaStorage(ms)) {
      std::cerr << "The Media Storage Type of your file is not supported: " << ms << std::endl;
      std::cerr << "Please report" << std::endl;
      continue;
    }
    gdcm::DataSet &ds = fileToAnon.GetDataSet();

    gdcm::StringFilter sf;
    sf.SetFile(fileToAnon);

    // use the following tags
    // https://wiki.cancerimagingarchive.net/display/Public/De-identification+Knowledge+Base

    /*    Tag    Name    Action */

    std::string filenamestring = "";
    std::string seriesdirname = ""; // only used if byseries is true
                                    // gdcm::Trace::SetDebug( true );
                                    // gdcm::Trace::SetWarning( true );
                                    // gdcm::Trace::SetError( true);

    // lets add the private group entries
    // gdcm::AddTag(gdcm::Tag(0x65010010), gdcm::VR::LO, "MY NEW DATASET",
    // reader.GetFile().GetDataSet());

    // maxval = 0xff;
    // if (DEPTH == 16)
    //  maxval = 0xffff;
    int HEIGHT = sf.ToInt(gdcm::Tag(0x0028, 0x0010)); // acquisition matrix
    int WIDTH = sf.ToInt(gdcm::Tag(0x0028, 0x0011));  // acquisition matrix
    gdcm::Image im = new gdcm::Image();
    im = reader.GetImage(); // is this color or grayscale????
    ushort[] theData = new ushort[WIDTH * HEIGHT];
    im.GetArray(theData);

    PIX *pixs = pixCreate(WIDTH, HEIGHT, 32); // rgba colors
    for (i = 0; i < HEIGHT; i++) {
      for (j = 0; j < WIDTH; j++) {
        l_int32 val;
        l_int32 red = 0;
        l_int32 green = 0;
        l_int32 blue = 0;
        // if im is grayscale or color

        composeRGBPixel(red, green, blue, &val);
        //    val = maxval * j / WIDTH;

        pixSetPixel(pixs, j, i, val);
      }
    }

    //  Pix *image = pixRead("/usr/src/tesseract/testing/phototest.tif");
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    api->Init(NULL, "eng+nor");
    api->SetImage(pixs);
    api->Recognize(0);
    tesseract::ResultIterator *ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
    if (ri != 0) {
      do {
        const char *word = ri->GetUTF8Text(level);
        float conf = ri->Confidence(level);
        int x1, y1, x2, y2;
        ri->BoundingBox(level, &x1, &y1, &x2, &y2);
        printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n", word, conf, x1, y1, x2,
               y2);
        delete[] word;
      } while (ri->Next(level));
    }

    // ok save the file again
    std::string imageInstanceUID = filenamestring;
    if (imageInstanceUID == "") {
      fprintf(stderr, "Error: cannot read image instance uid from %s\n", filename);
      gdcm::UIDGenerator gen;
      imageInstanceUID = gen.Generate();
    }
    std::string fn = params->outputdir + "/" + filenamestring + ".dcm";
    if (params->byseries) {
      // use the series instance uid as a directory name
      std::string dn = params->outputdir + "/" + seriesdirname;
      struct stat buffer;
      if (!(stat(dn.c_str(), &buffer) == 0)) {
        // DIR *dir = opendir(dn.c_str());
        // if ( ENOENT == errno)	{
        mkdir(dn.c_str(), 0700);
      } // else {
        // closedir(dir);
      //}
      fn = params->outputdir + "/" + seriesdirname + "/" + filenamestring + ".dcm";
    }

    fprintf(stdout, "[%d] write to file: %s\n", params->thread, fn.c_str());
    std::string outfilename(fn);

    // save the file again to the output
    gdcm::Writer writer;
    writer.SetFile(fileToAnon);
    writer.SetFileName(outfilename.c_str());
    try {
      if (!writer.Write()) {
        fprintf(stderr, "Error [#file: %d, thread: %d] writing file \"%s\" to \"%s\".\n", file,
                params->thread, filename, outfilename.c_str());
      }
    } catch (const std::exception &ex) {
      std::cout << "Caught exception \"" << ex.what() << "\"\n";
    }
  }
  return voidparams;
}

void ShowFilenames(const threadparams &params) {
  std::cout << "start" << std::endl;
  for (unsigned int i = 0; i < params.nfiles; ++i) {
    const char *filename = params.filenames[i];
    std::cout << filename << std::endl;
  }
  std::cout << "end" << std::endl;
}

void ReadFiles(size_t nfiles, const char *filenames[], const char *outputdir, int numthreads,
               std::string storeMappingAsJSON) {
  // \precondition: nfiles > 0
  assert(nfiles > 0);

  // lets change the DICOM dictionary and add some private tags - this is still not sufficient to be
  // able to write the private tags
  gdcm::Global gl;
  if (gl.GetDicts().GetPrivateDict().FindDictEntry(gdcm::Tag(0x0013, 0x0010))) {
    gl.GetDicts().GetPrivateDict().RemoveDictEntry(gdcm::Tag(0x0013, 0x0010));
  }
  gl.GetDicts().GetPrivateDict().AddDictEntry(
      gdcm::Tag(0x0013, 0x0010), gdcm::DictEntry("Private Creator Group CTP-LIKE", "0x0013, 0x0010",
                                                 gdcm::VR::LO, gdcm::VM::VM1));

  if (gl.GetDicts().GetPrivateDict().FindDictEntry(gdcm::Tag(0x0013, 0x1010))) {
    gl.GetDicts().GetPrivateDict().RemoveDictEntry(gdcm::Tag(0x0013, 0x1010));
  }
  gl.GetDicts().GetPrivateDict().AddDictEntry(
      gdcm::Tag(0x0013, 0x1010),
      gdcm::DictEntry("ProjectName", "0x0013, 0x1010", gdcm::VR::LO, gdcm::VM::VM1));

  if (gl.GetDicts().GetPrivateDict().FindDictEntry(gdcm::Tag(0x0013, 0x1013))) {
    gl.GetDicts().GetPrivateDict().RemoveDictEntry(gdcm::Tag(0x0013, 0x1013));
  }
  gl.GetDicts().GetPrivateDict().AddDictEntry(
      gdcm::Tag(0x0013, 0x1013),
      gdcm::DictEntry("SiteID", "0x0013, 0x1013", gdcm::VR::LO, gdcm::VM::VM1));

  if (gl.GetDicts().GetPrivateDict().FindDictEntry(gdcm::Tag(0x0013, 0x1012))) {
    gl.GetDicts().GetPrivateDict().RemoveDictEntry(gdcm::Tag(0x0013, 0x1012));
  }
  gl.GetDicts().GetPrivateDict().AddDictEntry(
      gdcm::Tag(0x0013, 0x1012),
      gdcm::DictEntry("SiteName", "0x0013, 0x1012", gdcm::VR::LO, gdcm::VM::VM1));

  if (nfiles <= numthreads) {
    numthreads = 1; // fallback if we don't have enough files to process
  }

  const unsigned int nthreads = numthreads; // how many do we want to use?
  threadparams params[nthreads];

  pthread_t *pthread = new pthread_t[nthreads];

  // There is nfiles, and nThreads
  assert(nfiles > nthreads);
  const size_t partition = nfiles / nthreads;
  for (unsigned int thread = 0; thread < nthreads; ++thread) {
    params[thread].filenames = filenames + thread * partition;
    params[thread].outputdir = outputdir;
    params[thread].nfiles = partition;
    params[thread].thread = thread;
    if (thread == nthreads - 1) {
      // There is slightly more files to process in this thread:
      params[thread].nfiles += nfiles % nthreads;
    }
    assert(thread * partition < nfiles);
    int res = pthread_create(&pthread[thread], NULL, ReadFilesThread, &params[thread]);
    if (res) {
      std::cerr << "Unable to start a new thread, pthread returned: " << res << std::endl;
      assert(0);
    }
  }
  // DEBUG
  size_t total = 0;
  for (unsigned int thread = 0; thread < nthreads; ++thread) {
    total += params[thread].nfiles;
  }
  assert(total == nfiles);
  // END DEBUG

  for (unsigned int thread = 0; thread < nthreads; thread++) {
    pthread_join(pthread[thread], NULL);
  }

  // we can access the per thread storage of study instance uid mappings now
  if (storeMappingAsJSON.length() > 0) {
    std::map<std::string, std::string> uidmappings;
    for (unsigned int thread = 0; thread < nthreads; thread++) {
      for (std::map<std::string, std::string>::iterator it =
               params[thread].byThreadStudyInstanceUID.begin();
           it != params[thread].byThreadStudyInstanceUID.end(); ++it) {
        uidmappings.insert(std::pair<std::string, std::string>(it->first, it->second));
      }
    }
    nlohmann::json ar;
    for (std::map<std::string, std::string>::iterator it = uidmappings.begin();
         it != uidmappings.end(); ++it) {
      ar[it->first] = it->second;
    }

    // if the file exists already, don't store again (maybe its from another thread?)
    std::ofstream jsonfile(storeMappingAsJSON);
    jsonfile << ar;
    jsonfile.flush();
    jsonfile.close();
  }

  delete[] pthread;
}

struct Arg : public option::Arg {
  static option::ArgStatus Required(const option::Option &option, bool) {
    return option.arg == 0 ? option::ARG_ILLEGAL : option::ARG_OK;
  }
  static option::ArgStatus Empty(const option::Option &option, bool) {
    return (option.arg == 0 || option.arg[0] == 0) ? option::ARG_OK : option::ARG_IGNORE;
  }
};

enum optionIndex { UNKNOWN, HELP, INPUT, OUTPUT, NUMTHREADS };
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
     "USAGE: rewritepixel [options]\n\n"
     "Options:"},
    {HELP, 0, "", "help", Arg::None,
     "  --help  \tRewrite DICOM images to remove text. Read DICOM image series and write "
     "out an anonymized version of the image data."},
    {INPUT, 0, "i", "input", Arg::Required, "  --input, -i  \tInput directory."},
    {OUTPUT, 0, "o", "output", Arg::Required, "  --output, -o  \tOutput directory."},
    {NUMTHREADS, 0, "t", "numthreads", Arg::Required,
     "  --numthreads, -t  \tHow many threads should be used (default 4)."},
    {UNKNOWN, 0, "", "", Arg::None,
     "\nExamples:\n"
     "  rewritepixel --input directory --output directory\n"
     "  rewritepixel --help\n"},
    {0, 0, 0, 0, 0, 0}};

int main(int argc, char *argv[]) {

  argc -= (argc > 0);
  argv += (argc > 0); // skip program name argv[0] if present

  option::Stats stats(usage, argc, argv);
  std::vector<option::Option> options(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

  if (parse.error())
    return 1;

  if (options[HELP] || argc == 0) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  for (option::Option *opt = options[UNKNOWN]; opt; opt = opt->next())
    std::cout << "Unknown option: " << std::string(opt->name, opt->namelen) << "\n";

  std::string input;
  std::string output;
  int numthreads = 4;
  for (int i = 0; i < parse.optionsCount(); ++i) {
    option::Option &opt = buffer[i];
    switch (opt.index()) {
    case HELP:
      // not possible, because handled further above and exits the program
    case INPUT:
      if (opt.arg) {
        fprintf(stdout, "--input '%s'\n", opt.arg);
        input = opt.arg;
      } else {
        fprintf(stdout, "--input needs a directory specified\n");
        exit(-1);
      }
      break;
    case OUTPUT:
      if (opt.arg) {
        fprintf(stdout, "--output '%s'\n", opt.arg);
        output = opt.arg;
      } else {
        fprintf(stdout, "--output needs a directory specified\n");
        exit(-1);
      }
      break;
    case NUMTHREADS:
      if (opt.arg) {
        fprintf(stdout, "--numthreads %d\n", atoi(opt.arg));
        numthreads = atoi(opt.arg);
      } else {
        fprintf(stdout, "--numthreads needs an integer specified\n");
        exit(-1);
      }
      break;
    case UNKNOWN:
      // not possible because Arg::Unknown returns ARG_ILLEGAL
      // which aborts the parse with an error
      break;
    }
  }

  // Check if user pass in a single directory
  if (gdcm::System::FileIsDirectory(input.c_str())) {
    gdcm::Directory d;
    d.Load(argv[1]);
    gdcm::Directory::FilenamesType l = d.GetFilenames();
    const size_t nfiles = l.size();
    const char **filenames = new const char *[nfiles];
    for (unsigned int i = 0; i < nfiles; ++i) {
      filenames[i] = l[i].c_str();
    }
    if (storeMappingAsJSON.length() > 0) {
      storeMappingAsJSON = output + std::string("/") + storeMappingAsJSON;
    }

    // do all the left-over once
    ReadFiles(nfiles, filenames, output.c_str(), numthreads, storeMappingAsJSON);
    delete[] filenames;
  }

  return 0;
}
