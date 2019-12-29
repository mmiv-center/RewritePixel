FROM ubuntu:18.04

RUN apt-get update -qq && apt-get install -yq build-essential \
    cmake git wget libxml2-dev libxslt1-dev libjpeg-dev expat \
    libpng12-dev libtiff4-dev

# build gdcm and the executable
RUN cd /root && git clone https://github.com/mmiv-center/RewritePixel.git && cd RewritePixel && \
    wget https://github.com/malaterre/GDCM/archive/v3.0.4.tar.gz && \
    tar xzvf v3.0.4.tar.gz && mkdir gdcm-build && cd gdcm-build && cmake -DGDCM_BUILD_SHARED_LIBS=ON ../GDCM-3.0.4 && \
    make && cd .. && cmake . && make

ENTRYPOINT [ "/root/RewritePixel/rewritepixel" ]