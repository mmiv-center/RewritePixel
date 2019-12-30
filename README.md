# Sanitize DICOM image data with text annotations

This project uses the tesseract >4.0 OCR engine to identify text that is burned into DICOM image data. For each text fragment (usually a word) a square black frame is written into the DICOM pixel information. The resulting DICOM file should be inspected - hopefully it is free of participant identifying information.

### Build

We are using cmake to create a make file for the compilation. The program depends on a number of libraries - best to look at the Dockerfile to get an idea on how to compile this program.

```
cmake -DCMAKE_BUILD_TYPE=Debug .
```

Using docker:
```
> docker build -t rewritepixel -f Dockerfile .
...
> docker run -it --rm rewritepixel 
USAGE: rewritepixel [options]

Options:
  --help            Rewrite DICOM images to remove text. Read DICOM image series
                    and write out an anonymized version of the image data.
  --input, -i       Input directory.
  --output, -o      Output directory.
  --confidence, -c  Confidence threshold (0..100).
  --numthreads, -t  How many threads should be used (default 4).

Examples:
  rewritepixel --input directory --output directory
  rewritepixel --help
```