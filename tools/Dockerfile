FROM opensuse/tumbleweed:latest

ARG ARCHITECTURE=
ARG PROJECT_NAME=

# packages
RUN zypper --non-interactive refresh &&\
    zypper --non-interactive install\
    opi glibc-locale\
    make makedepend gcc15 gcc15-c++\
    wget\
    tar\
    bzip2\
    python315\
    bash-completion\
    zsh\
    git\
    libncurses5 libncurses6\
    libreadline6 libreadline8\
    libgtk-2_0-0 libgthread-2_0-0 libSM6 libGLU1\
    dialog\
    srecord\
    minicom minicom-lang\
    which\
    libtool\
    pkgconf gawk\
    jimtcl jimtcl-devel\
    libusb-1_0-0 libusb-1_0-devel\
    libjaylink libjaylink-devel libjaylink0\
    xorg-x11-fonts xorg-x11-fonts-core xorg-x11-fonts-legacy\
    wine wine-gecko wine-mono\
    ccache distcc rsync libxml2-tools\
    clang clang-tools\
    lua55 lua55-devel lua55-luarocks lua55-luafilesystem\
    dosfstools e2fsprogs
RUN zypper clean -a

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

# i686-unknown-linux-musl-gcc (POSIX simulator, x86_64 hosts only)
WORKDIR /home/developer
RUN if [ "${ARCHITECTURE}" = "x86_64" ]; then \
        wget https://github.com/cross-tools/musl-cross/releases/download/20261001/i686-unknown-linux-musl.tar.xz && \
        echo "d433bfbe9e5f7daea61e76fbe5ba28050750ec853cc69a2311fe3479ec3f7f34  i686-unknown-linux-musl.tar.xz" | sha256sum -c && \
        tar -Jxf i686-unknown-linux-musl.tar.xz -C /home/developer/bin/ && \
        rm i686-unknown-linux-musl.tar.xz && \
        echo 'export PATH=$PATH:/home/developer/bin/i686-unknown-linux-musl/bin' >> /home/developer/.bashrc; \
    fi
ENV PATH="$PATH:/home/developer/bin/i686-unknown-linux-musl/bin"

# oh-my-zsh
WORKDIR /home/developer
RUN ZSH=/home/developer/.oh-my-zsh sh -c "$(wget -O- https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
RUN echo 'export ZSH="/home/developer/.oh-my-zsh"' > .zshrc
RUN echo 'ZSH_DISABLE_COMPFIX="true"' >> .zshrc
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
RUN make CFLAGS="-Wno-error" -j all
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
