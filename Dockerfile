FROM ubuntu:18.04

RUN apt-get update -qq && apt-get install -yq build-essential \
    cmake git wget libxml2-dev libxslt1-dev libjpeg-dev expat \
    libpng-dev libtiff-dev

# we want to use the norwegian language ontop of the english default
RUN apt-get install -yq libtesseract-dev tesseract-ocr-nor libleptonica-dev

# build gdcm and the executable
RUN cd /root && git clone https://github.com/mmiv-center/RewritePixel.git && cd RewritePixel && \
    wget https://github.com/malaterre/GDCM/archive/v3.0.4.tar.gz && \
    tar xzvf v3.0.4.tar.gz && mkdir gdcm-build && cd gdcm-build && cmake -DGDCM_BUILD_SHARED_LIBS=ON ../GDCM-3.0.4 && \
    make

#RUN cd /root/RewritePixel && cmake . && make

#ENTRYPOINT [ "/root/RewritePixel/rewritepixel" ]
