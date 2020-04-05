FROM ubuntu:18.04

RUN apt-get update -qq && apt-get install -yq build-essential \
    cmake git wget libxml2-dev libxslt1-dev libjpeg-dev expat \
    libpng-dev libtiff-dev software-properties-common 

RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt install -yq gcc-8 g++-8 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8

# we want to use the norwegian language ontop of the english default
RUN apt-get install -yq libtesseract-dev tesseract-ocr-nor libleptonica-dev

# build gdcm and the executable
RUN cd /root && git clone https://github.com/mmiv-center/RewritePixel.git && cd RewritePixel && \
    wget https://github.com/malaterre/GDCM/archive/v3.0.4.tar.gz && \
    tar xzvf v3.0.4.tar.gz && mkdir gdcm-build && cd gdcm-build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DGDCM_BUILD_SHARED_LIBS=ON ../GDCM-3.0.4 && \
    make

RUN cd /root/RewritePixel &&  cmake -DCMAKE_BUILD_TYPE=Release . && make


ENV ND_ENTRYPOINT="/startup.sh"

RUN echo '#!/usr/bin/env bash' >> $ND_ENTRYPOINT \
    && echo 'set +x' >> $ND_ENTRYPOINT \
    && echo 'umask 0000' >> $ND_ENTRYPOINT \
    && echo '/root/RewritePixel/rewritepixel $*' >> $ND_ENTRYPOINT \
    && chmod 777 $ND_ENTRYPOINT

ENTRYPOINT [ "/startup.sh" ]
