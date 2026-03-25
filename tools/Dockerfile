FROM opensuse/leap:15.6

ARG ARCHITECTURE=
ARG PROJECT_NAME=

# packages
RUN zypper --non-interactive install\
    opi glibc-locale\
    make makedepend gcc\
    wget\
    tar\
    bzip2\
    python3\
    bash-completion\
    zsh\
    git\
    libncurses5 libncurses6 ncurses-devel\
    libreadline6 libreadline7\
    libgtk-2_0-0\
    libgthread-2_0-0\
    libSM6\
    libGLU1\
    srecord\
    minicom minicom-lang\
    which\
    libtool\
    pkgconf\
    jimtcl jimtcl-devel\
    libusb-1_0-0 libusb-1_0-devel\
    libjaylink libjaylink-devel libjaylink0\
    gawk lua53 lua53-devel lua53-luarocks\
    xorg-x11-fonts xorg-x11-fonts-converted xorg-x11-fonts-core xorg-x11-fonts-legacy\
    wine wine-32bit wine-gecko wine-mono

# post-install operations
RUN ln -s $(which srec_cat) /bin/srec_cat

# environment variables
ENV HOME=/home/developer
ENV LANG="C.UTF-8"
ENV LC_ALL="C.UTF-8"
ENV DISPLAY=:0
ENV PATH="/home/developer/bin/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH"
ENV PATH="/home/developer/bin:$PATH"

# basic files and directories
WORKDIR /home/developer
RUN echo 'export PATH=$PATH:/home/developer/bin' > /home/developer/.bashrc
RUN useradd --no-log-init --create-home --shell /bin/zsh developer
RUN mkdir -p /home/developer/bin
RUN mkdir -p /home/developer/${PROJECT_NAME}

# arm-none-eabi-gcc
WORKDIR /home/developer
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN tar -jxf gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2 -C /home/developer/bin/
RUN rm gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN echo 'export PATH=$PATH:/home/developer/bin/gcc-arm-none-eabi-10.3-2021.10/bin' >> /home/developer/.bashrc

# oh-my-zsh
WORKDIR /home/developer
RUN ZSH=/home/developer/.oh-my-zsh sh -c "$(wget -O- https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
RUN echo 'export ZSH="/home/developer/.oh-my-zsh"' > .zshrc
RUN echo 'ZSH_THEME="fwalch"' >> .zshrc
RUN echo 'plugins=(git)' >> .zshrc
RUN echo 'source $ZSH/oh-my-zsh.sh' >> .zshrc
RUN echo 'source $HOME/.bashrc' >> .zshrc
CMD ["zsh", "-l"]

# openocd compilation
WORKDIR /home/developer/bin
RUN git clone https://git.code.sf.net/p/openocd/code openocd-code
WORKDIR /home/developer/bin/openocd-code
RUN chmod +x ./bootstrap
RUN ./bootstrap
RUN ./configure --enable-ftdi --enable-jlink --enable-stlink --enable-picoprobe
RUN make -j all
RUN mkdir -p /home/developer/bin/openOCD
RUN cp /home/developer/bin/openocd-code/src/openocd /home/developer/bin/openOCD/openocd
RUN cp -R /home/developer/bin/openocd-code/tcl/* /home/developer/bin/openOCD/.
WORKDIR /home/developer/bin
RUN rm -rf /home/developer/bin/openocd-code
RUN echo '#!/usr/bin/env bash' > /home/developer/bin/openocd
RUN echo 'cd $(dirname $0)' >> /home/developer/bin/openocd
RUN echo 'cd ./openOCD' >> /home/developer/bin/openocd
RUN echo './openocd "$@"' >> /home/developer/bin/openocd
RUN chmod +x /home/developer/bin/openocd

# set user id and group
RUN chown -R developer:users /home/developer
RUN echo 'plugdev:x:1001:developer' >> /etc/group
